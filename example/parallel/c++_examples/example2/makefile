all: example

example: simplextest.c simplext.tcl ../../../../build/harmony.a
	g++ -g -o example -Wall simplextest.c ../../../../build/harmony.a -lm -ldl -lnsl -I../../../../build -w

clean:
	rm -f core a.out *.o example
