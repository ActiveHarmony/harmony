all: example

example: simplextest.o ../../../../build/libharmony.a
	g++ -g -Wall simplextest.o -L../../../../build -lharmony -lm -ldl -lnsl -ltcl -I../../../build -o example

simplextest.o: simplextest.c
	g++ -g -Wall -c simplextest.c -I../../../../build

clean:
	rm -f core a.out *.o example
