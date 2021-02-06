#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>


static void* parallelFunction(void* arg)
{
	printf("I'm going to sleep now for a second.\n");
	printf("Now I woke up!\n");
}


int main(void)
{
	
	pthread_t tid;
	pthread_create(&tid, NULL, parallelFunction, NULL);
	printf("waiting now... \n");
	sleep(2);
	printf("Print last \n");
	return 0;
}
