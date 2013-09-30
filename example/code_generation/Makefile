TO_BASE=../..

MPICC=mpicc
CFLAGS=-std=c99 -pedantic -Wall -Werror -g
CPPFLAGS=-D_ISOC99_SOURCE -D_XOPEN_SOURCE=500

override CPPFLAGS+=-I$(TO_BASE)/src
override LDFLAGS+=-L$(TO_BASE)/src
override LDLIBS+=-lharmony -ldl

.PHONY: all install libharmony clean distclean

all: gemm

install: gemm
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

clean:
	rm -f core gemm

distclean: clean
	rm -f *~
