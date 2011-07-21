all: example

example: example.f simplext.tcl ../../../build/harmony.a
	f77 -c example.f
	f77 -o example example.o ../../../build/harmony.a -lstdc++

clean:
	rm -f core a.out *.o example
 
