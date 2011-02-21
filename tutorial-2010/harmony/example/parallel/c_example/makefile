all: example

example: simplextest.c simplext.tcl ../../../build/harmony.a
	gcc -g -o example -Wall simplextest.c ../../../build/harmony.a -lm -ldl -lnsl -ltcl -ltk -I../../../build -w -lstdc++

clean:
	rm -f core a.out *.o example
