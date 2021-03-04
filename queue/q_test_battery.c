/*
Jonathan Hall
jshall@bu.edu

Queue Implementation for Navy Research Labs Full Time Applicant

This file is a test bench for the attached queue library. A variety of tests are run to make sure that the library functions correctly. 
*/
#include <stdio.h>
#include "queue.h"
#include <assert.h> //used only in battery for testing.


void simple_battery()
{
	printf("....Starting simple tests....\n\n");
	queue newQ;
	int Q1_len = q_len(&newQ);
	char* t1 = "I'm first!";
	char* t2 = "I'm second!";
	char* r1 = malloc(40*sizeof(char));
	enqueue(&newQ, t1);
	enqueue(&newQ, t2);
	Q1_len = q_len(&newQ);
	printf("Length of queue after 2 insertions: %d\n", Q1_len);
	
	dequeue(&newQ, &r1);
	printf("Removing an element: %s\n", r1);
	
	assert(q_len(&newQ)==1);
	dequeue(&newQ, &r1);
	printf("Removing an element: %s\n", r1);
	assert(q_len(&newQ)==0);
	
	//making sure that nothing breaks if we try to dequeue an empty queue.
	assert(dequeue(&newQ, NULL)==-1); 
	
	char* t3 = "I'm third!";
	enqueue(&newQ, t3);
	enqueue(&newQ, t3);
	dequeue(&newQ, NULL);
	dequeue(&newQ, NULL);
	dequeue(&newQ, NULL);
	dequeue(&newQ, NULL);
	
	enqueue(&newQ, t2);
	q_destroy(&newQ);
	q_destroy(&newQ);
	enqueue(&newQ, t1);
	enqueue(&newQ, t2);
	enqueue(&newQ, t3);
	q_destroy(&newQ);
	
	printf("\n....simple test battery passed....\n\n");
	free(r1);

}

/*Test bench requested by NRL*/
void NRL_testbench()
{
	printf("....Starting NRL test bench....\n\n");
	queue NRL_Q;
	char* td1 = "1111-1111";
	char* td2 = "When you invoke GCC, it normally does preprocessing, compilation";
	char* td3 = "assembly and linking. The overall options allow";
	char* td4 = "the gcc program accepts options and file name";
	char* td5 = "Many options have long names starting with -f or with -W---for";
	char* td6 = "language, you can use that option with all supported langu";
	char* td7 = "not to run the linker. The";
	char* td8 = "k";
	char* td9 = "9999";
	char* td10 = "0";
	//i
	print_len(&NRL_Q);
	
	//ii
	enqueue(&NRL_Q, td1);
	
	//iii
	print_len(&NRL_Q);
	
	//iv.
	dequeue(&NRL_Q, NULL);
	
	//v.
	printf("Status of dequeue when queue length is %d: %d\n", q_len(&NRL_Q), dequeue(&NRL_Q, NULL));
	
	//vi.
	enqueue(&NRL_Q, td1);
	enqueue(&NRL_Q, td2);
	enqueue(&NRL_Q, td3);
	enqueue(&NRL_Q, td4);
	enqueue(&NRL_Q, td5);
	enqueue(&NRL_Q, td6);
	enqueue(&NRL_Q, td7);
	enqueue(&NRL_Q, td8);
	enqueue(&NRL_Q, td9);
	enqueue(&NRL_Q, td10);
	
	//vii
	print_len(&NRL_Q);
	
	//viii
	dequeue(&NRL_Q, NULL);
	
	//x
	print_len(&NRL_Q);
	
	//xi
	dequeue(&NRL_Q, NULL);
	print_len(&NRL_Q);
	dequeue(&NRL_Q, NULL);
	print_len(&NRL_Q);
	dequeue(&NRL_Q, NULL);
	print_len(&NRL_Q);
	dequeue(&NRL_Q, NULL);
	print_len(&NRL_Q);
	dequeue(&NRL_Q, NULL);
	print_len(&NRL_Q);
	dequeue(&NRL_Q, NULL);
	print_len(&NRL_Q);
	dequeue(&NRL_Q, NULL);
	print_len(&NRL_Q);
	dequeue(&NRL_Q, NULL);
	print_len(&NRL_Q);
	dequeue(&NRL_Q, NULL);
	print_len(&NRL_Q);
	
	printf("\n....Finished NRL testbench....\n\n");
	
	q_destroy(&NRL_Q);
	
}


int main()
{
	simple_battery();
	NRL_testbench();
	
	return 0;
}
