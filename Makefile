all: tester

check: tester
	./tester

clean:
	rm -f *.o tester

tester: main.o
	gcc main.o -o tester

main.o: main.c
	gcc -c main.c

