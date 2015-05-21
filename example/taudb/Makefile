TO_BASE=../..

MPICC=mpicc

override CFLAGS+=-I$(TO_BASE)/src

TARGETS=example example_mpi

install:
	@echo "Examples must be executed from source directory (for now)."

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
