CC = gcc
CFLAG = -g -Wall

test: scheduling.o auxiliary_functions.o check_functions.o procrastination.o queue.o allocation_functions.o scheduler_functions.o driver.o
	$(CC) $(CFLAG) -o test scheduling.o auxiliary_functions.o check_functions.o procrastination.o queue.o allocation_functions.o scheduler_functions.o driver.o -lm
	./test

scheduler_functions.o: scheduler_functions.c
	$(CC) $(CFLAG) -c scheduler_functions.c

allocation_functions.o: allocation_functions.c
	$(CC) $(CFLAG) -c allocation_functions.c

driver.o: driver.c
	$(CC) $(CFLAG) -c driver.c

procrastination.o: ../common/procrastination.c
	$(CC) $(CFLAG) -c ../common/procrastination.c

queue.o: ../common/queue.c
	$(CC) $(CFLAG) -c ../common/queue.c

check_functions.o: ../common/check_functions.c
	$(CC) $(CFLAG) -c ../common/check_functions.c

auxiliary_functions.o: ../common/auxiliary_functions.c
	$(CC) $(CFLAG) -c ../common/auxiliary_functions.c

scheduling.o: ../common/scheduling.c
	$(CC) $(CFLAG) -c ../common/scheduling.c

clean:
	rm -f *.o test
