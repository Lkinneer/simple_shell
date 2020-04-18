#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#define MAXLINE 80
#define HISTORY_NUM 10
/* The maximum length command */
typedef struct line{
	char message[MAXLINE] ;
	int line_number;
}line;
void free_args(char* args[]){
	for(int i = 0; i<MAXLINE/2 + 1; i++){
		if (args[i]!=NULL){
			free(args[i]);
		}
	}
}
int contains(char* source, char character){
	while(*source != '\0'){
		if(*source == character){
			return 1;
		}
		source++;
	}
	return 0;
}
//trims any whitespace off the front and back of
//string with a max size of 80 chars
void trim(char* source, char* destination){
	int actual_character_encountered = 0;
	char temp[MAXLINE];
	char * temp_p = temp;
	char * ptr;
	for(int i = 0; i<MAXLINE; i++)
	{
				temp[i] = '\0';
	}

	while(*source != '\0'){
		if(!isspace(*source)){
			actual_character_encountered = 1;

			//add temp to the destination string
			ptr = temp;
			while(*ptr!='\0'){
				 *destination = *ptr;
				 destination++;
				 ptr++;
			}
			//reset temp array
			for(int i = 0; i<MAXLINE; i++){
				temp[i] = '\0';
			}
			temp_p = temp;

			//add the new actual character from 
			*destination = *source;
			destination++;
		}else{
			if(actual_character_encountered){
				//store spaces in temp array
				*temp_p = *source;
				temp_p++;
			}
		}
		source++;
	}
	*destination = '\0';
}
//adds an item to the history array
void add_to_history(char message[],int number,line history[]){
	//push everything in history up one
	for(int i =HISTORY_NUM-1; i>0; i--){
		strcpy (history[i].message , history[i-1].message);
		history[i].line_number = history[i-1].line_number;
	}
	//add new line to history
	strcpy(history[0].message, message);
	history[0].line_number = number;
}
//prints the histroy array
void print_history(line history[]){
	for(int i = 0; i <HISTORY_NUM; i++){
		if(history[i].message[0] != 0){
			printf("%d %s",history[i].line_number, history[i].message);
		}
	}
}
//parses a string of arguments into many strings of arguments
void parse_to_args(char message[],char *args[]){
	//create a temporary array so the orginal is not destroyed
	char message_copy[MAXLINE];
	strcpy(message_copy,message);
	//remove any newline from the arguments
	for (int i = 0; i<MAXLINE; i++){
		if(message_copy[i]=='\n' || message_copy[i]=='&'){
			message_copy[i] = ' ';
		}
	}
	//seperate the argments into the args array
	//alocate memory for each string on the heap
	char* temp;
	temp = strtok(message_copy," ");
	args[0] = malloc(MAXLINE*sizeof(char));
	strcpy(args[0],temp);
	for (int i = 1; i<MAXLINE/2 + 1; i++){
		if((temp = strtok(NULL," "))!=NULL){
			args[i] = malloc(MAXLINE*sizeof(char));
			//segmentation faut when this line of code runs
			strcpy(args[i],temp);		
		}
		else{
			args[i]=NULL;
		}

	}
	
	
}
//forks and executes a process whos name and arguments are in args
void execute(char* args[] , int wait){
	int forkreturn;
	forkreturn = fork();
	if(forkreturn < 0){
		perror("failed to fork");
		exit(0);
	}
	if(forkreturn==0){
		//printf("A fork has happened");
		
		//call execvp
		//printf("first arg to execvp is %s",args[0]);
		execvp(args[0],args);
		exit(1);
	}
	if(wait){
		int child_return_value;
		waitpid(forkreturn, &child_return_value, 0);
	}
}
//parses a line into a distinct set of commands
//takes in a line to parse and an array of an array of characters to store the commands in
//returns the number of commands stored
//need to use free args on the returned strings
void parse_command(char message[],char *args[],char* del){
	//create a temporary array so the orginal is not destroyed
	char message_copy[MAXLINE];
	strcpy(message_copy,message);
	//seperate the argments into the args array
	//alocate memory for each string on the heap
	char* temp;
	temp = strtok(message_copy,del);
	args[0] = malloc(MAXLINE*sizeof(char));
	strcpy(args[0],temp);
	for (int i = 1; i<MAXLINE/2 + 1; i++){
		if((temp = strtok(NULL,del))!=NULL){
			args[i] = malloc(MAXLINE*sizeof(char));
			//segmentation faut when this line of code runs
			strcpy(args[i],temp);		
		}
		else{
			args[i]=NULL;
		}

	}
	
	
}
//takes in a line from the shell
//parses it into idividual commands
//can hadle piping on process into another
void parse_line(char newline[]){
	char *args[MAXLINE/2 + 1];	
	char *commands[MAXLINE/2 + 1];
	parse_command(newline,commands,";");
	char **commands_ptr = commands;
	for (int i = 0; i <MAXLINE/2 + 1; i++ ){
		if (commands[i]!=NULL){
			//commands[i] now points to a string containing the next command to be executed
			if(contains(commands[i],'|')){
				
				char* pipe_commands[MAXLINE/2+1];
				parse_command(commands[i],pipe_commands,"|");
				//printf(The first command is );
				//do stuff to execute things with pipes
				int sum_pipes[2];
				 if(pipe(sum_pipes) == -1) {
          			perror("Pipe failed");
          			exit(1);
        		}
        		
        		if(fork()==0){
        			parse_to_args(pipe_commands[0],args);
        			close(STDOUT_FILENO);  
            		dup(sum_pipes[1]);         
            		close(sum_pipes[0]);       
            		close(sum_pipes[1]);
            		execvp(args[0],args);
            		exit(1);
        		}


        		if(fork()==0){
        			parse_to_args(pipe_commands[1],args);
        			close(STDIN_FILENO);   
            		dup(sum_pipes[0]);         
            		close(sum_pipes[1]);       
            		close(sum_pipes[0]);
            		execvp(args[0],args);
            		exit(1);
        		}
        		free_args(pipe_commands);
        		close(sum_pipes[0]);
        		close(sum_pipes[1]);
        		wait(0);
        		wait(0);
        		
				
			}else{
			parse_to_args(commands[i],args);
				if(contains(commands[i],'&')){
					execute(args , 0);	
				}
				else{
					execute(args , 1);		
				}
			free_args(args);
		}

		}
	}
	free_args(commands);

	
}
//sets up piping between two commands


int main(void)
{
char current_line[MAXLINE];
line history[HISTORY_NUM];
for(int i = 0; i<HISTORY_NUM; i++){
	history[i].line_number = 0;
	for(int j =0; j<MAXLINE; j++){
		history[i].message[j] = 0;	
	}
}
char *args[MAXLINE/2 + 1];
int shouldrun = 1;
int current_line_number =1;


while (shouldrun) {
	printf("osh>");
	
	//get the line that was just eneted into stdin.
	fgets(current_line, sizeof(current_line), stdin);

	//change directory
	if(current_line[0] == 'c' && current_line[1] == 'd' && current_line[2] == ' '){
		char file_path[MAXLINE];
		trim(&current_line[2],file_path);
		chdir(file_path);
		add_to_history(current_line,current_line_number,history);
		current_line_number++;
		printf("chaged directory too %s\n",file_path);
	}
	//print history
	else if(strcmp(current_line,"history\n")==0){
		print_history(history);

	//use a command from previous history
	}else if(current_line[0]=='!'){
		if(current_line[1]=='!'){
			strcpy(current_line,history[0].message);
			parse_line(current_line);
		}
		else{
			if(current_line[1]!='\0' && current_line[1]!='\n'){
				int line_found_in_history = 0;
				int newline_number = atoi(&current_line[1]);	
				for(int i=0; i<HISTORY_NUM; i++){
					if(history[i].line_number == newline_number){
						strcpy(current_line,history[i].message);
						parse_line(current_line);
						line_found_in_history = 1;
					}
				}
				if(!line_found_in_history){
					printf("command number %d not found in histoy\n",newline_number);
				}
			}else{
				printf("either type \'!!\' or \'!n\' where n is an integer\n");
			}
			//code here to conver rest of argument to int
			//and set current line equal to the right string
		}
	}
	//normal execution
	else{
		add_to_history(current_line,current_line_number,history);
		current_line_number++;
		parse_line(current_line);
	}
	fflush(stdout);


/**
After reading user input, the steps are:
(1) fork a child process using fork()
(2) the child process will invoke execvp()
(3) if command included &, parent will invoke wait()
*/
}
return 0;
}