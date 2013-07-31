all: tester

check: tester
	./tester
travis: tester
	./tester --simple

clean:
	rm -f *.o tester

tester: main.o
	gcc main.o -o tester

main.o: main.c
	gcc -c main.c

