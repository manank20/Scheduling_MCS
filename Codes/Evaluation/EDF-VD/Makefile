CC = gcc
CFLAG = -g -Wall

test: auxiliary_functions.o check_functions.o queue.o allocation_functions.o scheduler_functions.o driver.o
	$(CC) $(CFLAG) -o test auxiliary_functions.o check_functions.o queue.o allocation_functions.o scheduler_functions.o driver.o -lm
	./test

driver.o: driver.c
	$(CC) $(CFLAG) -c driver.c

scheduler_functions.o: scheduler_functions.c
	$(CC) $(CFLAG) -c scheduler_functions.c

allocation_functions.o: allocation_functions.c
	$(CC) $(CFLAG) -c allocation_functions.c

queue.o: queue.c
	$(CC) $(CFLAG) -c queue.c

check_functions.o: check_functions.c
	$(CC) $(CFLAG) -c check_functions.c

auxiliary_functions.o: auxiliary_functions.c
	$(CC) $(CFLAG) -c auxiliary_functions.c

clean:
	rm -f *.o test
