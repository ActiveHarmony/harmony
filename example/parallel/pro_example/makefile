all: pro_example 

pro_example: mpi_stub.c offline.tcl ../../../build/harmony.a
	mpic++ -w -g -pg -o pro_example -Wall mpi_stub.c ../../../build/harmony.a -L../lib -lm -ldl -lnsl -I../../../build

clean:
	rm -f core a.out *.o pro_example
