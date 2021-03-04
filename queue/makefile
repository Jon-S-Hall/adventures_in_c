CC=gcc -std=c99 -Werror -Wall  -g

all: q_test_battery q_input_test

q_test_battery: queue q_test_battery.c
	$(CC) -o q_battery q_test_battery.c queue.o

q_input_test: queue queue_input_test.c
	$(CC) -o q_input queue_input_test.c queue.o

queue: queue.c
	$(CC) -c -o queue.o queue.c


clean:
	rm queue.o q_battery q_input
