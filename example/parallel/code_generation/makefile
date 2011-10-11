TO_ROOT=../../..

all: pro_example

pro_example: mpi_stub.c offline.tcl ../../../build/harmony.a
	$(MAKE) -C $(TO_ROOT)/build harmony.a
	mpic++ -w -O3 -o pro_example -Wall mpi_stub.c ../../../build/harmony.a -lm -ldl -lnsl -ltcl -ltk -I../../../build

clean:
	rm -f core a.out *.o pro_example
