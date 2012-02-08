all: example

example: simplextest.o ../../../build/harmony.a
	g++ -g -Wall simplextest.o ../../../build/harmony.a -lm -lnsl -ltcl -w -lstdc++ -o example

simplextest.o: simplextest.c
	gcc -g -Wall -c simplextest.c -I../../../build

clean:
	rm -f core a.out *.o example
