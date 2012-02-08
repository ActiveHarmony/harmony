all: example

example: simplextest.o ../../../../build/harmony.a
	g++ -g -Wall simplextest.o ../../../../build/harmony.a -lm -ldl -lnsl -ltcl -I../../../build -o example

simplextest.o: simplextest.c
	g++ -g -Wall -c simplextest.c -I../../../../build

clean:
	rm -f core a.out *.o example
