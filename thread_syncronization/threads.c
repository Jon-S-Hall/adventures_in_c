//by: Jonathan Shuang Hall
//email: jshall@bu.edu
//buid: U21798292

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <setjmp.h>
#include "ec440threads.h"
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include<errno.h>

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
#define MAX_SEM 456
#define READY 0
#define RUNNING 1

int threadCount; //counts number of current threads (besides main thread)
int threadnum; //index of thread that we are currently using
int semNum;
jmp_buf storebuf;
int init; //var that lets us now if threads has started. initialized to 0 


struct TCB {
	long unsigned int threadID;
	jmp_buf registers;
	int status;
	int *joinThreads; //thread IDs of threads that are waiting to join this thread.
	void* exit_status;
	long unsigned int *stackBottom;
};

struct SEM {
	int value;
	int* semQueue; //FIFO queue
	int active;
	int queueStart; //points to first threadID in queue.
	int queueEnd; //points to last threadID in queue. 
};

void scheduler(void);
void pthread_exit_wrapper();
void lock();
void unlock();
struct TCB TCB_arr[MAX_THREADS]; //we have at most 128 threads.
struct SEM SEM_arr[MAX_SEM];

static void handler(int signum)
{
	lock();
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
	lock();
	jmp_buf buf; //buffer to store state of main thread
	threadCount = 0;
	threadnum = 0;
	setjmp(buf);
	struct TCB mainThread = { .threadID = 0, .registers = {*buf}, .status = 1, .exit_status = NULL}; //status of 0 means ready. 1 means running. Main threadID is 1111 for simplicity.
	TCB_arr[0] = mainThread;	
	//setting up 50ms timer 
	struct itimerval it_val;
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
	unlock();
	return;
}


//----------------------rdi--------------------rsi-------------------------rdx-----------------------rcx
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void*), void *arg)
{	
	lock();
	//so I have to first check if there's any other threads running.
	if(init != 1)
	{
		unlock();
		//here we have to initialize our library.
		init = 1;
		thread_start_func();
	}else if(threadCount == MAX_THREADS) //if we have 128 threads, exit.
	{
		unlock();
		return -1; //thread limit reached.
	}
	//continue as usual.
	unsigned long *stack = malloc(STACK_SIZE*sizeof(unsigned long));
	if (stack == 0) { printf("malloc could not allocate stack.\n"); exit(1); }
	jmp_buf new_buf;
	setjmp(new_buf); //have to setjmp to get current mask! otherwise we have undefined behavior.
	new_buf[0].__jmpbuf[JB_RSP] = ptr_mangle((long unsigned int) &(stack[STACK_SIZE])); 
	new_buf[0].__jmpbuf[JB_PC] = ptr_mangle((long unsigned int) start_thunk); //jump to start_thunk so that we can pass in arg.
	new_buf[0].__jmpbuf[JB_R12] = (long unsigned int) start_routine; //syntax probably incorrect.
	new_buf[0].__jmpbuf[JB_R13] = (long unsigned int) arg;
	

	stack[STACK_SIZE] = (unsigned long) pthread_exit_wrapper; 
	struct TCB newThread = { .threadID = ++threadCount , .registers = {*new_buf}, .exit_status = NULL, .status = 0, .stackBottom = stack }; //need to put curly brackets around {*new_buf} because .registers itself is an aggregated data type.
	newThread.joinThreads = (int*)calloc(128,sizeof(int));
	for (int i = 0; i < 128; i++) { newThread.joinThreads[i] = -1; } //initialize joinThreads to -1 ({empty)
	lock();
	TCB_arr[threadCount] = newThread;
	unlock();
	*thread = newThread.threadID;
	return 0;

}

void scheduler()
{
	jmp_buf nextbuf;
	storebuf[0].__jmpbuf[JB_R12] = ptr_demangle(storebuf[0].__jmpbuf[JB_PC]); //put PC into reg12 for start_thunk
	struct TCB storeThread = { .threadID = TCB_arr[threadnum].threadID, .registers = {*storebuf}, .joinThreads = TCB_arr[threadnum].joinThreads, .exit_status = TCB_arr[threadnum].exit_status, .status = (TCB_arr[threadnum].status - 1), .stackBottom = TCB_arr[threadnum].stackBottom };
	TCB_arr[threadnum] = storeThread;
	threadnum++;
	threadnum = threadnum % (threadCount+1); //get next thread in round robin fashion.
	while (TCB_arr[threadnum].status != READY) //loop through until we arrive at a thread ready to be resumed.
	{ threadnum++; threadnum = threadnum % (threadCount+1);}
	TCB_arr[threadnum].status = RUNNING; //status 1 is running
	//now we've found our thread. continue.
	memcpy(nextbuf, TCB_arr[threadnum].registers, sizeof(nextbuf));
	nextbuf[0].__jmpbuf[JB_PC] = ptr_mangle((long unsigned int)start_thunk);
	unlock();
	longjmp(nextbuf, 1);

}

//wrapper method to allow return value from thread. rax (return value) is moved into the res register.
void pthread_exit_wrapper()
{
	unsigned long int res;
	asm("movq %%rax, %0\n":"=r"(res));
	pthread_exit((void*)res); //fine 250 = fa. res value is address of int 250
}

//main thread will never execute pthread_exit. retval is returned from pthread_exit_wrapper.
void pthread_exit(void* retval) {
	lock();
	TCB_arr[threadnum].status = 0; //exit state of a register.
	TCB_arr[threadnum].exit_status = retval; //we're storing a void that is the pointer to something.
	//get all joined threads ready
	for (int i = 0; i < 128; i++)
	{
		if (TCB_arr[threadnum].joinThreads[i] != -1)
		{
			TCB_arr[TCB_arr[threadnum].joinThreads[i]].status = READY;
		}
		else {
			break;
		}
	}
	free(TCB_arr[threadnum].stackBottom);
	free(TCB_arr[threadnum].joinThreads);
	unlock();
	handler(0);
	__builtin_unreachable();
}

pthread_t pthread_self(void)
{
	return (pthread_t) TCB_arr[threadnum].threadID;
}



int pthread_join(pthread_t thread, void** value_ptr)
{	lock();
	if (TCB_arr[(int)thread].status == -1)//if already exited, simply return value.
	{
		if (value_ptr != NULL) {
			*value_ptr = TCB_arr[thread].exit_status;
		}
		//free(TCB_arr[thread].stackBottom);
		unlock();
		return 0;
	}
	else {
		TCB_arr[threadnum].status = -1; //set state as -1 so that scheduler makes it -2 (blocked).
		for (int i = 0; i < 128; i++)
		{
			if (TCB_arr[thread].joinThreads[i] == -1) {
				TCB_arr[thread].joinThreads[i] = threadnum;
				unlock();
				break; //break at the first one.
			}
		}
		handler(0);//leave. When we come back, we should be ready to update value_ptr.
	}

	lock();
	if (TCB_arr[(int)thread].status == -1)
	{
		if (value_ptr != NULL) {
			*value_ptr = TCB_arr[thread].exit_status;
		}
		//printf("Value of retval in join: %d\n", TCB_arr[thread].exit_status);
		//free(TCB_arr[thread].stackBottom);
		unlock();
		return 0;
	}else {
		unlock();
		return -1; //error, something went wrong
	}
}



void lock()
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	if(sigprocmask(SIG_BLOCK, &set, NULL)!=0)
	{printf("failed to block signal\n");exit(1);}
}

void unlock()
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	if(sigprocmask(SIG_UNBLOCK, &set, NULL)!=0)
	{printf("failed to unblock signal\n"); exit(1);}
}

//semaphore support

//returns 0 upon success, -1 on failure. pshared is 0.
int sem_init(sem_t* sem, int pshared, unsigned value) {
	lock();
	if (pshared != 0 || value < 0) {
		unlock();
		return -1; //pshared should always be 0
	}
	int* waitingThreads;
	waitingThreads = (int*)malloc(MAX_THREADS * sizeof(int)); //we can have at most, all threads waiting at the semaphore.
	struct SEM sem_init = { .value = value, .active = 1, .semQueue = waitingThreads, .queueStart = 0, .queueEnd = 0 };
	SEM_arr[semNum] = sem_init;
	sem->__align = (long int)semNum;
	semNum = (semNum + 1) % MAX_SEM;
	unlock();
	return 0;
}

int sem_wait(sem_t* sem) {
	lock();
	int curSem = (int)sem->__align;

	if (!SEM_arr[curSem].active) { 
		unlock();
		return -1; //can't wait on a not active thread.
	} 

	while (SEM_arr[curSem].value <= 0) {
		TCB_arr[threadnum].status = -1; //set status as blocked. scheduler will make value -2.
		SEM_arr[curSem].semQueue[SEM_arr[curSem].queueEnd] = threadnum;
		SEM_arr[curSem].queueEnd = (SEM_arr[curSem].queueEnd + 1) % MAX_SEM; //maybe I need to put this back into the sem obj?
		unlock();
		handler(0);
		lock();
	}
	SEM_arr[curSem].value -= 1; //always update waitSem value when refreshing.
	if (SEM_arr[curSem].value < 0) { unlock();  return -1; }//semaphore value should never be negative.
	unlock();
	return 0;
}

int sem_post(sem_t* sem) {
	lock();
	int curSem = (int)sem->__align;
	if (!SEM_arr[curSem].active) { unlock(); return -1; }
	SEM_arr[curSem].value += 1;

	if (SEM_arr[curSem].queueStart == SEM_arr[curSem].queueEnd) { 
	
	} //this means nothing is in the queue.
	else {
		TCB_arr[SEM_arr[curSem].semQueue[SEM_arr[curSem].queueStart]].status = READY;
		SEM_arr[curSem].queueStart = (SEM_arr[curSem].queueStart + 1) % MAX_SEM;
	}
	unlock();
	return 0;
}

int sem_destroy(sem_t* sem) {
	lock();
	int curSem = (int)sem->__align;
	if (!SEM_arr[curSem].active) { unlock(); strerror(errno); return -1; }
	SEM_arr[curSem].active = 0; //semaphore is inactive.
	free(SEM_arr[curSem].semQueue);
	unlock();
	return 0;
}