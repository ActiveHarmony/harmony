all: example

DEBUG_FLAG=-g
OPT_FLAG=-O3

example: simplextest.c simplext.tcl ../../../build/harmony.a
	gcc $(OPT_FLAG) -o example -Wall simplextest.c ../../../build/harmony.a -lm -ldl -lnsl -I../../../build -w -lstdc++

clean:
	rm -f core a.out *.o example
