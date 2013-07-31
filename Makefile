all: lexer.a lexer.so

check: lexer_test
	./lexer_test
travis: lexer_test
	./lexer_test --simple
clean:
	rm -f *.o *.a *.so tester
lexer_test: lexer.c
	gcc -DTESTS lexer.c -o lexer_test
lexer.a: lexer.o
	ar rcs lexer.a lexer.o
lexer.so: lexer.o
	gcc -shared -Wl,-soname,lexer -o lexer.so lexer.o
lexer.o: lexer.c
	gcc -fPIC -c lexer.c
