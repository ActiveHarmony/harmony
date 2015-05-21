TO_BASE=../..

MPICC=mpicc

override CPPFLAGS+=-I$(TO_BASE)/src
override LDLIBS+=-ldl

TARGETS=gemm

install:
	@echo "Examples must be executed from source directory (for now)."

gemm: gemm.c $(TO_BASE)/src/libharmony.a
	@if [ -x "`which $(MPICC) 2>&1`" ]; then                              \
	  echo "$(MPICC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@";\
	  $(MPICC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@;       \
	else                                                                  \
	  echo "Skipping build of $@: $(MPICC) not found.";                   \
	fi

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
