
/*
(1) TerrierID: terrier028
================================================================================
(2) Target Characteristic To Test:
--------------------------------------------------------------------------------
Test A is meant to check for read write create and destroy errors 
Test B is meant to test for clone functionality 
my_thread test with a data type that isnt a Char * 
Test D is meant to test for multipule page functionality 
Test E is meant to test for invalid memory access attempts

================================================================================
(3) Justification for WHY Target Charactersitic is Valuable to Test
--------------------------------------------------------------------------------
A- because it checks to see if it conforms to the error condions on the homework sheet 
B- because it checks to see if clone works which must be in a seporate thread
my_thread may be unnessisary but it is cool and terrier089 gave me a good idea for the _Line_ part of my assert 
D- because it is important to see that multiple page creation works and allows for CoW check 
E- because it is important to see that the sig handeler correctly closes the thread
================================================================================
(4) Sketch HOW You Would Implement the Testcase
--------------------------------------------------------------------------------
Bassically each void method is a meant to test a different thing, i found that 
A and B are the most important but the others helped me with peace of mind. 
I would recomend uncommenting include "tls.h" if you use that kind of format. 
it is also useful to have a method which prints out some number of your tls array. 
by default it runs a-d but you can uncomment the others if you have an easy way of getting your tls index element

 (250 Words max)
================================================================================
(5) All tests must build as:
// if tls.h uncommented 
gcc -Werror -Wall -g -c -o <tlsfileName>.o <tlsfileName>.c
gcc -Werror -Wall -g -O0  <ExecutableName>.c <tlsfileName>.o -lpthread -o <ExecutableName> 

or if tls.h commented and functions dropped in for currently commented function primitives;

gcc -Wall -Werror -o main -lpthread <filename.c>
*/
#include<pthread.h>
#include<string.h>
#include"tls.h"
#include<unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include<stdlib.h>
#include<stdio.h>

//^^^ please change these 


int AssCount = 1;
pthread_t An = (pthread_t)-1;
void Assert(int bool,int AssertID,int line);
void shouldBeTrue();

sem_t mutex,WaitForB,WaitForE,WaitForF; 
char * fourty2 = "Far in the uncharted backwaters of the unfashionable end of the Western Spiral arm of the galaxy lies a small unregarded yellow sun.";
char * fourty3 = "Call me Ishmael. ";

void * testA(void *helo){   
    unsigned int Create_size = 140; 
    printf("\n\t\tin A\n"); 
    unsigned int lengthW = 132; 
    unsigned int lengthR = 75; 
    unsigned int offset = 0;
    char * readbuff =(char *)malloc(Create_size);
    // /readbuff = "silly puppies";
    
    printf("\t\tA write string len  = %d\n",(int)strlen(fourty2));
    printf("\t\tA read string len  = %d\n",(int)strlen(readbuff));

    printf("\t\tchecking for incorrect run %d\n",__LINE__);
    //assertion 3 
    Assert(tls_read(offset, lengthR, readbuff)==-1,AssCount,__LINE__);// test0;

    Assert(tls_create(0)==-1,AssCount,__LINE__);// checks that it fails on a 0 
    Assert(tls_write(0, lengthW, fourty2)==-1,AssCount,__LINE__);// checks to see that you cant write on something that isnt itialize
    Assert(tls_read(offset, lengthR, readbuff)==-1,AssCount,__LINE__);// checks to see that you cant READ on something that isnt itialize
    Assert(tls_destroy()==-1,AssCount,__LINE__);// checks to see that you cant destroy on something that isnt itialize
    //assertion 7
    printf("\t\tchecking for correct run %d\n",__LINE__);
    Assert(tls_create(Create_size)==0,AssCount,__LINE__);// checks that it succeeds on a valid size 
    printf("\t\tchecking for incorrect run %d\n",__LINE__);
    Assert(tls_create(Create_size)==-1,AssCount,__LINE__);// checks that it fails if one is already there 
    Assert(lengthW <= (int)strlen(fourty2),AssCount,__LINE__);// check for undifined condion 
    //test 1
    Assert(tls_write(offset, Create_size+1, fourty2)==-1,AssCount,__LINE__);// checks that it errors when offset + length > size of LSA)
  

    Assert(tls_read(offset, Create_size+1, readbuff)==-1,AssCount,__LINE__);
    printf("\t\tchecking for correct run %d\n",__LINE__);
    Assert(tls_write(offset, lengthW, fourty2)==0,AssCount,__LINE__);// checks that succeeds on a good call 
    
    
    printf("\n\t\tin A\n");
    printf("\t\t%s\n\t\tvs\n",readbuff);
    //Assert(lengthR <= (int)strlen(readbuff),AssCount);// check for undifined condion 
    Assert(tls_read(offset, lengthW, readbuff)==0,AssCount,__LINE__);// checks that succeeds on a good call 
    printf("\t\t%s\n",readbuff);
    
    //cloning stuff
    An = pthread_self();
    
    sem_post(&mutex); // allows b to run
    sem_wait(&WaitForB);// stops A from running 
    printf("\t\tin A\n");

    Assert(tls_write(offset, lengthW, fourty3)==0,AssCount,__LINE__);
    sem_post(&mutex);// let b run 
    sem_wait(&WaitForB);// stop a from running 
    printf("\t\tin A:");
    Assert(tls_read(offset, lengthW, readbuff)==0,AssCount,__LINE__);// checks that succeeds on a good call 
    printf("%s\n",readbuff);
    // checks that it errors when offset + length > size of LSA)
    printf("\t\tDONE A\n");
    sem_post(&mutex);
    //test 11
    Assert(tls_destroy()==0,AssCount,__LINE__);// checks for a succussfull return of destroy. 
    free(readbuff);
    return NULL;
}
void * testB(void *helo){
    sem_wait(&mutex); // stops b from running 
    printf("\t\tin B\n");
    unsigned int lengthR = 75; 
    unsigned int offset = 0;

    char k[lengthR];
    //char j[lengthR];
    Assert((int)An!=-1,AssCount,__LINE__);
    Assert(tls_clone(An) == 0,AssCount,__LINE__);
    //printTLS(20);- a method in my cod that prints my tls data structure
    Assert(tls_read(offset, lengthR, k)==0,AssCount,__LINE__);
    
    printf("\t\tin clone B: %s\n",k);
    sem_post(&WaitForB);// allow A to run 
    sem_wait(&mutex);// stop B from running 
    
    printf("\t\tin B\n");
    Assert(tls_read(offset, lengthR, k)==0,AssCount,__LINE__);
    //printTLS(20);
    printf("\t\tin clone B after change: %s\n",k);
    //Assert(strcmp(j,k)==0,AssCount,__LINE__);// comment this out after making sure their the same string
    sem_post(&WaitForB);// allow A to run
    sem_wait(&mutex);// stop B from running
     //test 11
    Assert(tls_destroy()==0,AssCount,__LINE__);
    printf("\t\tDONE B\n");
    return NULL;
}
void * testD(void *helo){
    printf("\t\tin D\n");
    //test 2
    int numberOfApages = 4;
    int getPagesize = getpagesize();
    char longData[numberOfApages*getPagesize];
    char longDatareadBuff[numberOfApages*getPagesize];
    int i;
    for(i =0; i<numberOfApages;i++){
        int j; 
        for (j = 0;j<getPagesize-1;j++){
            longData[(i*getPagesize)+j] = 'G'+i;
        }
        longData[(i*getPagesize)+j] = '\n';
    }
    printf("\t\tlong string is \n%s",longData);
    Assert(tls_create(numberOfApages*getPagesize)==0,AssCount,__LINE__);
    Assert(tls_write(0, numberOfApages*getPagesize, longData)==0,AssCount,__LINE__);
    Assert(tls_read(0, numberOfApages*getPagesize, longDatareadBuff)==0,AssCount,__LINE__);
    printf("\t\tlong string from buff is \n%s",longDatareadBuff);

    printf("\t\t%d\n",strcmp(longDatareadBuff,longData));
    //Assert(strcmp(longDatareadBuff,longData)==0,AssCount,__LINE__);
    Assert(tls_destroy()==0,AssCount,__LINE__);
    printf("\t\tDONE D\n");
    return NULL;
}
void*
my_thread(void *arg)// from terrier089
{
    printf("\t\tStarting MyThread\n");
    int written_int = 10;
    int read_int = 0;
    Assert(tls_clone(An) == -1,AssCount,__LINE__);//test 10 
    Assert(tls_create(sizeof(int)) == 0,AssCount,__LINE__);
    Assert(tls_write(0, sizeof(int), (char*)&written_int) == 0,AssCount,__LINE__);
    Assert(tls_read(0, sizeof(int), (char*)&read_int) == 0,AssCount,__LINE__);
    Assert(read_int == written_int,AssCount,__LINE__);
    // printf("\t\tin mythread w in = %d R out = %d\n",written_int,read_int);
    Assert(tls_destroy() == 0,AssCount,__LINE__);
    printf("\t\tDONE MYTHREAD\n");

    return NULL;
}
pthread_t Eid;
void * testE(void *helo){
    printf("\t\tin E\n");
    unsigned int Create_size = 140; 
    printf("\n\t\tin A\n"); 
    unsigned int lengthW = 132; 
    unsigned int offset = 0;
    char * readbuff =(char *)malloc(Create_size);
    Assert(tls_create(Create_size)==0,AssCount,__LINE__);
    Assert(tls_write(offset, lengthW, fourty2)==0,AssCount,__LINE__);
    Eid = pthread_self();
    sem_post(&WaitForE);// allows f to go 
    sem_wait(&WaitForF);// forces E to wait for F 
    free(readbuff);
    printf("\t\tDONE E\n");
    return NULL;
}
/*
void * testF(void *helo){
    printf("\t\tin F\n");
    sem_wait(&WaitForE);// Allows f to go when e is done initializeing  
    sem_post(&WaitForF);// Allows E to go when f is exited 
    Pages * illeagal = TLSIndex[findTLSIndex(Eid)].pages[0]; //however you would access your tls array 
    char * k;
    k = (char *)illeagal->address;
    printf("\t\tshould have exited before here shouldnt be able to say the buff is %s\n",k);
    unsigned long int c = (unsigned long int)helo;
    c++;
    printf("\t\tDONE F\n");
    return (void *)c;
}*/
int main(int argc, char **argv) {
    printf("Hi, I left the print statements in this draft, idk if that is annoying,\nplease feel to take them out.\n IMPORTANT:I also didnt do much formatting of my code or checking my assertions against other classmates. \n I really apreacated the work put out by terrier089! this is a cool concept for the homeworks\ndont use these as gosple tests.\n all print statements are tabbed to make it easiear to distingish between prints in your methods and ones from the main. \n");
    shouldBeTrue();
    sem_init(&mutex, 0, 1);
    sem_init(&WaitForB, 0, 1);
    sem_wait(&mutex);// flag down 
    sem_wait(&WaitForB); // flag down

    sem_init(&WaitForE, 0, 1);
    sem_init(&WaitForF, 0, 1);
    sem_wait(&WaitForE);// flag down 
    sem_wait(&WaitForF); // flag down

    pthread_t A, B;
    pthread_create(&A,NULL,testA,NULL);
    pthread_create(&B,NULL,testB,NULL);
    pthread_join(A, NULL);
    pthread_join(B, NULL);

    pthread_t C;
    pthread_create(&C,NULL,my_thread,NULL);
    pthread_join(C, NULL);

    pthread_t D;
    pthread_create(&D,NULL,testD,NULL);
    pthread_join(D, NULL);

    // still working on this for myself, i dont know if it works 
    /*
    pthread_t E,F;
    pthread_create(&E,NULL,testE,NULL);
    pthread_create(&F,NULL,testF,(void*)5);
    pthread_join(E, NULL);
    void * outvar;
    pthread_join(F, &outvar);
    Assert((unsigned long int)outvar!=6,AssCount,__LINE__);
*/
    sem_destroy(&mutex);
    sem_destroy(&WaitForB);
    sem_destroy(&WaitForF);
    sem_destroy(&WaitForE);
    //printTLS(5);
	return 0;
}

void Assert(int bool, int AssertID, int line){
    if(bool){
        AssCount ++;
        return; 
    }
    else{
        printf("\t\tThis assertion(%d)was wrong on line-%d\n",AssertID,line);// line taken from terrier089
        //printTLS(20);
        exit(AssCount);
    }
}
void shouldBeTrue(){
    Assert(1,AssCount,__LINE__);
    Assert(An == -1,AssCount,__LINE__);
}
