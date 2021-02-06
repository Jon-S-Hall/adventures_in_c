CC=gcc -Wall -g

all: testCase01 testCase02 testCase03 TC04 TC05 TC06


TC06: threads TC05.c
	$(CC) -o TC06 TC06.c threads.o
TC05: threads TC05.c
	$(CC) -o TC05 TC05.c threads.o
TC04: threads TC04.c
	$(CC) -o TC04 TC04.c threads.o
testCase03: threads testCase03.c
	$(CC) -o testCase03 testCase03.c threads.o

testCase02: threads 03_test_thread.c
	$(CC) -o testCase02 03_test_thread.c threads.o

testCase01: threads testCase01.c
	$(CC) -o testCase01 testCase01.c threads.o

threads: threads.c
	$(CC) -c -o threads.o threads.c

clean:
	rm testCase01 testCase02 testCase03 TC04 TC05 TC06 threads.o
