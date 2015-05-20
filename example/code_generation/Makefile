TO_BASE=../..

MPICC=mpicc
CFLAGS=-std=c99 -pedantic -Wall -Werror -g

override CPPFLAGS+=-I$(TO_BASE)/src
override LDFLAGS+=-L$(TO_BASE)/src
override LDLIBS+=-lharmony -ldl

TARGETS=gemm

.PHONY: libharmony

install:
	@echo "Examples must be executed from source directory (for now)."

gemm: gemm.c libharmony
	@if [ -x "`which $(MPICC) 2>&1`" ]; then                              \
	  echo "$(MPICC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@";\
	  $(MPICC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@;       \
	else                                                                  \
	  echo "Skipping build of $@: $(MPICC) not found.";                   \
	fi

libharmony:
	$(MAKE) -C $(TO_BASE)/src libharmony.a

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
