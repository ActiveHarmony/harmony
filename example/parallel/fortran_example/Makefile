#WRKDIR=/hivehomes/tiwari/harmony_pro_release/example/fortran_example
WRKDIR=$(PWD)
DIR=$(WRKDIR)

SHELL=/bin/bash

EXE=$(DIR)/l3d.x
HARMONY=/hivehomes/tiwari/harmony_pro_release
TIMER=/hivehomes/tiwari/timer
FC=mpif90 -w -O3 -g -pg
FCOPT= -O3
FCFLAGS=$(FCOPT) #-Mpreprocess
LDFLAGS=$(FCOPT) -lstdc++

OBJS=mykind.o solve3d.o dump.o l3d.o cscmpi.o $(HARMONY)/build/harmony.a $(TIMER)/rtclock.o

PES=1 8 27

MAXITER=10000
EPS=0.005

EXAMPLE=96 $(MAXITER) $(EPS)

.SUFFIXES :
.SUFFIXES : .o .f90

$(EXE): $(DIR) $(OBJS)
	$(FC) $(LDFLAGS) $(OBJS) -o $@
	@touch $@


run:
	for n in $(PES) ; do \
	  (cd $(DIR)/ ; aprun -n$$n $(EXE) $(EXAMPLE)) ; \
	done

prof:	$(EXEPROF)
	@make EXE=$(EXEPROF) run

$(DIR):
	mkdir -p $(DIR)/

.f90.o:
	$(FC) $(FCFLAGS) -c $<

cscmpi.o:	cscmpi.f90
dump.o: 	dump.f90 mykind.o cscmpi.o
mykind.o: 	mykind.f90
l3d.o: 		l3d.f90 mykind.o solve3d.o dump.o cscmpi.o
solve3d.o: 	solve3d.f90 mykind.o cscmpi.o

all:
	@make
	@make run

clean:
	\rm -f *.o *.mod *.x* *~ $(DIR)/*.x $(DIR)/*.x.pat

