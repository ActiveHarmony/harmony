all: example

example: example.o ../../../build/libharmony.a
	f77 -g -fno-underscoring -o example example.o -L../../../build -lharmony -lstdc++

example.o: example.f
	f77 -g -fno-underscoring -c example.f

clean:
	rm -f core a.out *.o example
 
