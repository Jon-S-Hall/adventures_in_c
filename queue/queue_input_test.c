/*
Jonathan Hall
jshall@bu.edu

Queue Implementation for Navy Research Labs Full Time Applicant

This file is a test bench for the attached queue library. Accepts input that enqueues, dequeues, and prints properties of a queue.
*/

#include <stdio.h>
#include <string.h>

#include "queue.h"

#define BUF_SIZE 1024

void type_prompt(int argc, char **argv)
{
	printf("queue_shell$");
}

int cleanse_buffer(char **str)
{
	//is buffer just null
	if(*str == NULL)
	{
		return 0;
	}
  	// Trim leading space
  	while(' ' == **str) (*str)++;
	
  	if(**str == 0)  //Check Spaces
  	{
  		return 0;
  	}
  	
  	return 1;
}

void execute(queue* inp_Q, char* buffer)
{
	memset(buffer, 0, BUF_SIZE);
	fgets(buffer, BUF_SIZE, stdin);
	buffer = strtok(buffer, " ");
	int operation = atoi(buffer);
	
	switch(operation)
	{
		case 1:
			buffer = strtok(NULL, "\n");
			if(cleanse_buffer(&buffer))
			{
				enqueue(inp_Q, buffer);
			}else{
				printf("No entry input. To Enqueue: 1 [value to enqueue]\n");
			}
			break;
		case 2:
			if(dequeue(inp_Q, NULL)==-1)
			{
				printf("Buffer is empty. Nothing removed.\n");
			}
			break;
		case 3:
			q_peek(inp_Q);
			break;
		case 4:
			print_len(inp_Q);
			break;
		case 5:
			printf("Bye! :)\n");
			q_destroy(inp_Q);
			exit(1);
		default: printf("Command not found. Please try again.\n");
	}
}

int main(int argc, char **argv)
{
	char buffer[BUF_SIZE];
	queue inp_Q;
	int contin = 1;
	
	printf("Starting queue input test. Instructions:\n\n");
	printf("1 [enqueue val]: Enqueue [enqueue val]\n");
	printf("2: Dequeue\n");
	printf("3: Print top of queue\n");
	printf("4: Print length of queue\n");
	printf("5: Quit\n\n");
	//script for queue manipulation
	while(contin)
	{
		type_prompt(argc, argv);
		execute(&inp_Q, buffer);
	}
	
}
