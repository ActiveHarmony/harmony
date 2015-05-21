TO_BASE=../..

EXTRA_CLEANUP=*.xml
MPICC=mpicc

override CFLAGS+=-I$(TO_BASE)/src

NO_INST_TGTS=example example_mpi

example: example.c $(TO_BASE)/src/libharmony.a
	@if [ -x "`which $(CC) 2>&1`" ]; then                              \
	  echo "$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@";\
	  $(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@;       \
	else                                                               \
	  echo "Skipping build of $@: $(CC) not found.";                   \
	fi

example_mpi: example_mpi.c $(TO_BASE)/src/libharmony.a
	@if [ -x "`which $(MPICC) 2>&1`" ]; then                              \
	  echo "$(MPICC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@";\
	  $(MPICC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@;       \
	else                                                                  \
	  echo "Skipping build of $@: $(MPICC) not found.";                   \
	fi

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
