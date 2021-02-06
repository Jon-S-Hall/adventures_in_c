#include <pthread.h>
#include <stdio.h>
#include <setjmp.h>
#include "ec440threads.h"
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define  JB_RBX 0
#define  JB_RBP 1
#define  JB_R12 2
#define  JB_R13 3
#define  JB_R14 4
#define  JB_R15 5
#define  JB_RSP 6
#define  JB_PC  7
#define INTERVAL 50 /*number of milliseconds to go off */
#define STACK_SIZE 4096 //4096*8 = 32768
#define MAX_THREADS 128 //maximum number of concurrent threads.

int threadCount; //counts number of current threads (besides main thread)
int threadnum; //index of thread that we are currently using
jmp_buf storebuf;
struct itimerval it_val;

struct TCB {
	long unsigned int threadID;
	jmp_buf registers;
	int status;
	int exit_state;
};

//void restart_timer()
//{
//	if (setitimer(ITIMER_REAL, &it_val, NULL) == -1)
//	{
//		perror("error calling setitimer()\n");	exit(1);
//	}
//}

void scheduler(void);

struct TCB TCB_arr[128]; //we have at most 128 threads.


static void handler(int signum)
{
	//restart_timer();
	if (!setjmp(storebuf))
	{
		//we're heading to scheduler
		scheduler();
	}
	else
	{
		//resume thread
		return;
	}

}
//helper method for first thread creation. recall that main thread must continue the main function. Thus here, add the main thread to the TCB_arr, start the scheduler, and continue.
//What if we're in the middle of thread creation when the timer goes off?
void thread_start_func(void)
{
	jmp_buf buf; //buffer to store state of main thread
	threadCount = 0;
	threadnum = 0;
	setjmp(buf);
	struct TCB mainThread = { .threadID = 1111, .registers = {*buf}, .status = 1, .exit_state = 0 }; //status of 0 means ready. 1 means running. Main threadID is 1111 for simplicity.
	TCB_arr[0] = mainThread;	
	//setting up 50ms timer 
	it_val.it_value.tv_sec = 0;
	it_val.it_value.tv_usec = INTERVAL * 1000;
	it_val.it_interval.tv_usec = INTERVAL * 1000;
	it_val.it_interval.tv_sec = 0;
	it_val.it_interval = it_val.it_value;

	if (setitimer(ITIMER_REAL, &it_val, NULL) == -1)
	{
		perror("error calling setitimer()\n");	exit(1);
	}
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NODEFER; //Allow signals to be caught even when handler has not returned. This is very important since we don't return for a whole cycle.

	if(sigaction(SIGALRM, &sa, NULL) == -1)
	{perror("Unable to catch SIGALARM");exit(1);}
	return;
}

//----------------------rdi--------------------rsi-------------------------rdx-----------------------rcx
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void*), void *arg)
{	
	//restart_timer();
	//so I have to first check if there's any other threads running.
	if(TCB_arr[0].threadID != 1111)
	{
		//here we have to initialize our library.
		thread_start_func();
	}else if(threadCount == MAX_THREADS) //if we have 128 threads, exit.
	{
		printf("Error, thread limit reached. \n");
		exit(1);
	}
	//continue as usual.
	unsigned long *stack = malloc(STACK_SIZE*sizeof(unsigned long));//**change
	if (stack == 0) { printf("malloc could not allocate stack.\n"); exit(1); }

	jmp_buf new_buf;
	setjmp(new_buf); //have to setjmp to get current mask! otherwise we have undefined behavior.
	new_buf[0].__jmpbuf[JB_RSP] = ptr_mangle((long unsigned int) &(stack[STACK_SIZE])); 
	new_buf[0].__jmpbuf[JB_PC] = ptr_mangle((long unsigned int) start_thunk); //jump to start_thunk so that we can pass in arg.
	new_buf[0].__jmpbuf[JB_R12] = (long unsigned int) start_routine; //syntax probably incorrect.
	new_buf[0].__jmpbuf[JB_R13] = (long unsigned int) arg;
	

	stack[STACK_SIZE] = (unsigned long) pthread_exit; 
	struct TCB newThread = { .threadID = ++threadCount , .registers = {*new_buf}, .status = 0, .exit_state = 0 }; //need to put curly brackets around {*new_buf} because .registers itself is an aggregated data type.
	TCB_arr[threadCount] = newThread;
	
	return (int)stack[0];

}

void scheduler()
{
	jmp_buf nextbuf;
	storebuf[0].__jmpbuf[JB_R12] = ptr_demangle(storebuf[0].__jmpbuf[JB_PC]); //put PC into reg12 for start_thunk
	struct TCB storeThread = { .threadID = TCB_arr[threadnum].threadID, .registers = {*storebuf}, .status = (TCB_arr[threadnum].status - 1), .exit_state = 0 }; 
	TCB_arr[threadnum] = storeThread;
	threadnum++;
	threadnum = threadnum % (threadCount+1); //get next thread in round robin fashion.
	while (TCB_arr[threadnum].status != 0) //loop through until we arrive at a thread ready to be resumed.
	{ threadnum++; threadnum = threadnum % (threadCount+1);}
	TCB_arr[threadnum].status = 1;
	//now we've found our thread. continue.
	memcpy(nextbuf, TCB_arr[threadnum].registers, sizeof(nextbuf));
	//*nextbuf = *(TCB_arr[threadnum].registers);
	nextbuf[0].__jmpbuf[JB_PC] = ptr_mangle((long unsigned int)start_thunk);
	longjmp(nextbuf, 1);

}

//main thread will never execute pthread_exit.
void pthread_exit(void* retval) {
	//restart_timer(); //make sure we don't get an alarm while exiting or in scheduler. This will cause big problem
	TCB_arr[threadnum].status = 0; //exit state of a register.
	scheduler();
	__builtin_unreachable();
}

pthread_t pthread_self(void)
{
	return TCB_arr[threadnum].threadID;
}
