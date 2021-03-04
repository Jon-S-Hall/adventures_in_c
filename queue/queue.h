/*
Jonathan Hall
jshall@bu.edu

Queue Implementation for Navy Research Labs Full Time Application
*/

#include <stdio.h>
#include <stdlib.h>
//string is provided for the use of strlen.
#include <string.h>

/*
	Node types are the elements in our queue.
*/
typedef struct Node{
	char* data; 
	struct Node *next; //pointer to next element in queue.
}node;

/*
	Struct for queue type. 
*/
typedef struct Queue {
	node* head; 		//front of queue, where elements are popped
	node* tail; 		//back of queue, where elements are queued
	int init;		//This is a variable to check if our queue is initialized. Uninitialized value is undefined.
	int len;
}queue;


int q_len(queue *curQ);

void print_len(queue *curQ);	

int enqueue(queue* curQ, char* element);

int dequeue(queue* curQ, char** element);

void q_peek(queue* curQ);

int q_init(queue *curQ);

int q_destroy(queue *curQ);

