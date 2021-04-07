test: functions.o driver.o
	gcc -o test functions.o driver.o
	@echo "Execute using ./test input.txt output.txt"
	
driver.o: driver.c
	gcc -c driver.c

functions.o: functions.c
	gcc -c functions.c

clean:
	rm -f *.o test
