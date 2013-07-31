all: lexer.a lexer.so

check: tester
	./tester
travis: tester
	./tester --simple
clean:
	rm -f *.o *.a *.so tester
tester: main.c
	gcc -DTESTS main.c -o tester
lexer.a: main.o
	ar rcs lexer.a main.o
lexer.so: main.o
	gcc -shared -Wl,-soname,lexer -o lexer.so main.o
main.o: main.c
	gcc -fPIC -c main.c
