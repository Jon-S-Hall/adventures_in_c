#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>

#define TRUE 1
#define STD_INPUT 0
#define STD_OUTPUT 1

void type_prompt(int argc, char **argv){
	if(argc > 1 && strcmp(argv[1], "-n")==0)
	{
	
	}else{
		printf("my_shell$");
	}
}

//function to put spacing between executing characters if needed
//only got placing first space to work.
char* buffer_parser(char *buffer)
{
	int bufferSize = 1024;
	for(int i = 0; buffer[i] != '0'; i++)
	{
		if(buffer[i] == '>' || buffer[i] == '|' || buffer[i] == '<')
		{
			if(buffer[i+1] < 1024 && buffer[i+1] != ' ')
			{
				char beforeBuffer[1024];
				char afterBuffer[1025];
				strncpy(beforeBuffer, buffer, i+1);
				strcpy(afterBuffer, &buffer[i+1]);
				strcat(beforeBuffer, " ");
				strcat(beforeBuffer, afterBuffer);
				strncpy(buffer, beforeBuffer, 1024);
				
			}
		}
		
		if(buffer[i+1] == '>' || buffer[i+1] == '|' || buffer[i+1] == '<')
		{
			if(buffer[i] != ' ')
			{
				char beforeBuffer[1024];
				char afterBuffer[1025];
				strncpy(beforeBuffer, buffer, i+1);
				strcpy(afterBuffer, &buffer[i+1]);
				strcat(beforeBuffer, " ");
				strcat(beforeBuffer, afterBuffer);
				strncpy(buffer, beforeBuffer, 1024);
				
			}
		}
		
	}
	return buffer;
}

//***parameters is a pointer to a pointer of char pointer array.
void read_command(char *buffer, char ****parameters, char **redirects, int* numRedirects)
{
	fgets(buffer, 1024, stdin);
	buffer = strtok(buffer, "\n"); //remove newline that was causing issues with finding executable 
	buffer = buffer_parser(buffer);
	char *token = strtok(buffer, " ");
	char *originalArg = (char*)malloc(strlen(token)+1);
	strcpy(originalArg, token);
	int counter = 0;
	int RDCounter = 0;
	int seperationCounter = 0; //measures the number of strings seperated by redirections
	*parameters = (char***)malloc(2*sizeof(char**));  //allocating the pointer to each separate desired execution (seperated by I/O redirection)
	(*parameters)[seperationCounter] = (char**)malloc(sizeof(char*)); //allocating the char arrays for each token
	(*parameters)[seperationCounter][counter] = (char*)malloc(12*sizeof(char)); //allocating actual char bits in each token. (12)
	*redirects = (char*)malloc(sizeof(char)); //malloc for our array of redirection symbols.
	while(token != NULL)
	{		
		//if we have a redirection, we can seperate the input. Also seperate parameters.
		if(strcmp(token, "|")==0 || strcmp(token, ">")==0 || strcmp(token, "<")==0){
			(*redirects)[RDCounter] = *token;
			*redirects = realloc(*redirects, (RDCounter+2)*sizeof(char));
			(*parameters)[seperationCounter+1] = (char**)malloc(sizeof(char*)); //increase size of pointer to have new commands after.
			(*parameters)[seperationCounter+1][0] = (char*)malloc(12*sizeof(char)); //allocate space for next token
			(*parameters)[seperationCounter][counter] = (char*)0;
			RDCounter += 1;
			seperationCounter += 1;
			counter = 0;
			//now we have to make space for redirection
			
		}else{
			strcpy((*parameters)[seperationCounter][counter], token);
			counter += 1;
			(*parameters)[seperationCounter] = realloc((*parameters)[seperationCounter], (counter+1)*sizeof(char*)); //increase number of char pointers by 1.
			(*parameters)[seperationCounter][counter] = (char*)malloc(12*sizeof(char)); //allocate space for next token
		}
		token = strtok(NULL, " ");//get new token
		

		

	}
	(*redirects)[RDCounter]= '\0'; //set last value to 0
	(*parameters)[seperationCounter][counter] = (char*)0; //terminate final char pointer as 0 (in last argument)
	if(counter == 0){
		(*parameters)[seperationCounter] = NULL; //set last command group to null
	}else
	{
		(*parameters)[seperationCounter+1] = (char**)malloc(sizeof(char*)); //increase size of pointer to have new commands after.
		(*parameters)[seperationCounter+1] = NULL;
	}
	strcpy((*parameters)[0][0], originalArg); //hotfix, for some reason when I have 3 or more redirections and the last one is ">", it makes the original argument null.
	*numRedirects = RDCounter;
	
}

/* Function to free dynamically allocated variables */
void free_Malloc(char ****parameters)
{
	int counter = 0;
	
	while((**parameters)[counter] != (char*)0)
	{
		free((**parameters)[counter]);
		counter+=1;
	}
	free(*parameters);
}

//code that actually parses and executes code. doesn't need to return anything, everything is finished in here until next

//ex ls < fileA.txt > fileB.txt ,    ls | wc,     ls | cat | wc    ls < textIn.txt | wc > textOut.txt , cat < textIn.txt | wc
//assuming parameteres contains, ls fileA.txt fileB.txt, ls wc -a, ls cat wc
//assuming redirects cotains "<>" "|" "||"
void execute_command(char ***parameters, char *redirects, int numRedirects)
{
	//int redirectCounter = 0;
	int commandCounter = 0;
	char path[37] = "/bin/";
	int status;
	char *env_args[] = {(char*)0};
	//int pipefd[2];
	int stdin_copy = dup(0);
	int stdout_copy = dup(1);
	char pipeBuffer[1024];
	int counter = 0;
	
	//pipe(pipefd); //pipefid[0] will be read, pipefd[1] will be write.
	
	memset(pipeBuffer, 0, 1024); //clear buffer
	while(parameters[commandCounter] != NULL)
	{
		int pipefd[2];
		int pipefd_second[2];
		pipe(pipefd);//pipefid[0] will be read, pipefd[1] will be write.
		pipe(pipefd_second);
		char tmp[37];
		char **curCommand = parameters[commandCounter]; //always a command.
		char* returnedBuffer;
		int retBufferLength;
		int readFD;
		int writeFD;
		bool needToRead = false; // if true, parent reads from pipeFD to buffer.
		bool needToWrite = false; //if true, parent writes from buffer to child
		strcpy(tmp, path);
		strcat(tmp, curCommand[0]);
		strcpy(curCommand[0], tmp);
		
		if(redirects[commandCounter] != '\0')
		{	
			if(redirects[commandCounter] == '<')
			{
				//switch pipes stdin to be the file. close stdin, replace with file. 
				readFD = open(parameters[commandCounter+1][0], O_RDONLY);
				close(STD_INPUT); //prepare for new standard input
				dup2(readFD, STD_INPUT);  //replaces STD INPUT FD file with readFD.
				close(readFD);
				if( (commandCounter+1) < (numRedirects) && redirects[commandCounter + 1] == '|')
				{
					//set stdout FD to be the input to the pipe
					needToRead = true;
					
				}else if( (commandCounter+1) < (numRedirects) && redirects[commandCounter + 1] == '>')
				{
					//switch pipes stdout to be the file
					writeFD = open(parameters[commandCounter+2][0], O_WRONLY);
					close(STD_OUTPUT);
					dup2(writeFD, STD_OUTPUT);
					close(writeFD);
				}
				
				commandCounter += 1;
			}
			else if(redirects[commandCounter] == '|') 
			{
				//set stdout FD to be the entrance to pipe.
				needToRead = true;
				
				if( commandCounter > 0 && (redirects[commandCounter - 1] == '|' || redirects[commandCounter - 1] == '<'))
				{
					// input from pipe
					needToWrite = true;
				}

			}	
			else if(redirects[commandCounter] == '>')
			{
				writeFD = open(parameters[commandCounter+1][0], O_WRONLY | O_CREAT);
				dup2(writeFD, STD_OUTPUT);
				//redirect output to the file.
				if( commandCounter > 0 && redirects[commandCounter - 1] =='|')
				{
					//input from pipe
					needToWrite = true;
				}
				
				commandCounter += 1;
				
			}
			
		}else if( commandCounter > 0 && redirects[commandCounter - 1] == '|')
		{
			//set stdin FD to be read from pipe			
			needToWrite = true;
			
		}
		
		
		
		//forking process.
		
		int pid = fork();
		if(pid !=0){
			if(needToWrite)  //ex ls | wc (after pipe)
			{
				write(pipefd[1], returnedBuffer, retBufferLength); //6
				free(returnedBuffer);
				close(pipefd[1]);
				if(needToRead)
				{
					memset(pipeBuffer, 0, 1024);
					read(pipefd_second[0], pipeBuffer, 1024);
					retBufferLength = strlen(pipeBuffer);
					returnedBuffer = malloc(retBufferLength);
					strncpy(returnedBuffer, pipeBuffer, retBufferLength);
					close(pipefd_second[0]);
				}else{
					close(pipefd[0]);
				}
			}else if(needToRead){  //ex ls | wc (before pipe)
				memset(pipeBuffer, 0, 1024);
				read(pipefd[0], pipeBuffer, 1024);
				close(pipefd[0]);
				retBufferLength = strlen(pipeBuffer);
				returnedBuffer = malloc(retBufferLength);
				strncpy(returnedBuffer, pipeBuffer, retBufferLength);
			}
			
			waitpid(-1, &status, 0);
		}else{
			if(needToWrite) //here the child is actually reading
			{
				close(STD_INPUT);
				dup2(pipefd[0], STD_INPUT);
				close(pipefd[0]);
				close(pipefd[1]); //maybe write side is open in the child... YES IT WAS.
				if(needToRead) //child then needs to write
				{
					close(STD_OUTPUT);
					dup2(pipefd_second[1], STD_OUTPUT);
					close(pipefd_second[1]);
					close(pipefd_second[0]); //don't ever need to read this side in child.	
				}
			}else if(needToRead) //here child is actually writing
			{
				dup2(pipefd[1], STD_OUTPUT);
				close(pipefd[1]);
				close(pipefd[0]);
			}
			execve(curCommand[0], curCommand, env_args);
			printf("ERROR: %s: command not found\n", curCommand[0]);
			exit(1);

		}
		

		commandCounter+=1; //potentially just break.
		close(STD_INPUT);
		close(STD_OUTPUT);
		close(pipefd[0]);
		close(pipefd_second[0]);
		close(pipefd[1]);
		close(pipefd_second[1]);
		//reset standard input and output in case of modification
		dup2(stdin_copy, STD_INPUT);
		dup2(stdout_copy, STD_OUTPUT);
	}
	free_Malloc(&parameters);
}

int main(int argc, char **argv) {
	char *env_args[] = {(char*)0};
	char ***parameters;
	char *redirects;
	int numRedirects = 0;
	//int execError;
	char buffer[1024];
	
	
	while(TRUE){
		type_prompt(argc, argv);
		read_command(buffer,&parameters, &redirects, &numRedirects);
		//execute everything that was entered into the buffer
		execute_command(parameters, redirects, numRedirects);		
	}


}



