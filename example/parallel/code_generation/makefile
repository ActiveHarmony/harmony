TO_ROOT=../../..

all: pro_example

pro_example: mpi_stub.c ../../../build/libharmony.a
	$(MAKE) -C $(TO_ROOT)/build libharmony.a
	mpic++ -g -O3 -o pro_example -Wall mpi_stub.c -L../../../build -lharmony -lm -ldl -lnsl -ltcl -I../../../build

clean:
	rm -f core a.out *.o pro_example
