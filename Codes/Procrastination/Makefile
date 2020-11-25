CC = gcc
CFLAG = -g -Wall

all: test

test: functions.o driver.o
	$(CC) $(CFLAGS) -o test functions.o driver.o
	
driver.o: driver.c
	$(CC) $(CFLAGS) -c driver.c

functions.o: functions.c
	$(CC) $(CFLAGS) -c functions.c

clean:
	rm -f *.o test
