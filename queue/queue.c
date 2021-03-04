/*
Jonathan Hall
jshall@bu.edu

Queue Implementation for Navy Research Labs Full Time Application
*/


#include "queue.h"

#define INIT_VAL 1234567890


/* Return the length of our queue. */
int q_len(queue *curQ)
{
	//Make sure queue is initialized. If not, initialize it.
	if(curQ->init != INIT_VAL)
	{
		if(!q_init(curQ))
		{
			perror("error initializing queue. Exiting.\n");
			exit(-1);
		}
	}
	return curQ->len;
}


/* Print the length of our queue */
void print_len(queue *curQ)
{
	if(curQ->init != INIT_VAL)	
	{
		if(!q_init(curQ))
		{
			perror("error initializing queue. Exiting.\n");
			exit(-1);
		}
	}
	printf("Number of elements in queue: %d\n", q_len(curQ));
}


/* Push 'element' to the back of given queue. */
int enqueue(queue* curQ, char* element)
{
	//base case testing. make sure queue is initialized.
	if(curQ->init != INIT_VAL)
	{
		if(!q_init(curQ))
		{
			return -1;
		}
	}
	
	node *newNode = malloc(sizeof(node));
	newNode->data = malloc(strlen(element)+1); //allocate an extra byte for null terminator.
	memcpy(newNode->data, element, strlen(element));
	newNode->data[strlen(element)] = '\0';
	node *lastNode = curQ->tail->next;
	lastNode->next = newNode;
	newNode->next = curQ->tail;
	curQ->tail->next = newNode; //newNode is our new last node
	curQ->len++;
	return 0;
}


/* Remove element at the front of our queue, and store value into element*. If element** is null, print removed element. */
int dequeue(queue* curQ, char** element)
{
	//base case testing. make sure queue is initialized.
	if(curQ->init != INIT_VAL)
	{
		if(!q_init(curQ))
		{
			perror("error initializing queue. Exiting.\n");
			exit(-1);
		}
	}
	if(curQ->len == 0)
	{
		return -1;
	}
	
	if(element == NULL)
	{
		printf("Popped: %s\n", (char*)curQ->head->next->data);
	}else{
		strcpy(*element, curQ->head->next->data);
	}
	
	

	node* del_node = curQ->head->next;
	curQ->head->next = curQ->head->next->next;//head now references the next element.
	free(del_node->data);
	free(del_node);
	
	if((--(curQ->len))==0)
	{
		curQ->tail->next = curQ->head;
	}
	//best if it is a link list since it's way easer to remove a value
	return 0;
}


/* Print the element at the front of our queue (next to be popped)*/
void q_peek(queue *curQ)
{
	//base case testing. make sure queue is initialized.
	if(curQ->init != INIT_VAL)
	{
		if(!q_init(curQ))
		{
			perror("error initializing queue. Exiting.\n");
			exit(-1);
		}
	}
	
	if(curQ->len == 0)
	{
		printf("Queue is empty.\n");
	}else
	{
		printf("Front of queue: %s\n", curQ->head->next->data);
	}
}


/* Initialize our queue. This library is written in such a way that the user need not call this function. 
   A queue is determined to be initialized based on the value of queue->init, which is undefined on queue declaration.
   Thus, there is a very small likelihood (< 1/10 Billion) that init's value is INIT_VAL prior to function call.
*/
int q_init(queue *curQ)
{
	curQ->init = INIT_VAL;
	node *newHead = malloc(sizeof(node));	
	node *newTail = malloc(sizeof(node));
	newHead->next = newTail;
	newTail->next = newHead;
	curQ->head = newHead;
	curQ->tail = newTail;
	curQ->len = 0;
	return 1; //success
}

/* Destroys our queue, freeing all allocated memory.
*/
int  q_destroy(queue *curQ)
{
	//base case testing. make sure queue is initialized.
	if(curQ->init != INIT_VAL)
	{
		return -1; //Queue doesn't exist or is already deleted.
	}
	
	curQ->tail->next = (node*) NULL;
	
	node* head = curQ->head;
	curQ->head = curQ->head->next;
	free(head);
	
	while(curQ->head->next != (node*)NULL)
	{
		node* prev_node = curQ->head;
		curQ->head = curQ->head->next;
		free(prev_node->data);
		free(prev_node);
	}
	
	free(curQ->head);
	curQ->init = 0;
	
	return 0;
}
