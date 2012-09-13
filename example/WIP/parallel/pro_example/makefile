all: pro_example 

pro_example: mpi_stub.c offline.tcl ../../../build/libharmony.a
	mpic++ -w -g -pg -o pro_example -Wall mpi_stub.c -L../../../build -lharmony -lm -ldl -lnsl -ltcl -I../../../build

clean:
	rm -f core a.out *.o pro_example
