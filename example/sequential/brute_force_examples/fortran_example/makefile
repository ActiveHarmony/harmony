all: example

example: example.f simplext.tcl ../../../../build/harmony.a
	f77 -g -fno-underscoring -c example.f
	f77 -g -fno-underscoring -o example example.o ../../../../build/harmony.a -lstdc++

clean:
	rm -f core a.out *.o example
 
