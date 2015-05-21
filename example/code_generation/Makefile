TO_BASE=../..

MPICC=mpicc

override CPPFLAGS+=-I$(TO_BASE)/src
override LDLIBS+=-ldl

NO_INST_TGTS=gemm

gemm: gemm.c $(TO_BASE)/src/libharmony.a
	@if [ -x "`which $(MPICC) 2>&1`" ]; then                              \
	  echo "$(MPICC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@";\
	  $(MPICC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@;       \
	else                                                                  \
	  echo "Skipping build of $@: $(MPICC) not found.";                   \
	fi

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
