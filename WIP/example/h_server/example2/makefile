all: example

example: simplextest.c simplext.tcl ../../../../../build/libharmony.a
	g++ -g -o example -Wall simplextest.c -L../../../../../build -lharmony -lm -ldl -lnsl -ltcl -I../../../../../build

clean:
	rm -f core a.out *.o example
