TO_BASE=../..

CC=gcc
MPICC=mpicc
CFLAGS=-std=c99 -pedantic -Wall -Werror -g
CPPFLAGS=-D_ISOC99_SOURCE -D_XOPEN_SOURCE=500

override CFLAGS+=-I$(TO_BASE)/src
override LDFLAGS+=-L$(TO_BASE)/src
override LDLIBS+=-lharmony

TARGETS=example example_mpi

.PHONY: all install libharmony clean distclean

all: $(TARGETS)

install: $(TARGETS)
	@echo "Examples must be executed from source directory (for now)."

example: example.c libharmony
	@if [ -x "`which $(CC) 2>&1`" ]; then                              \
	  echo "$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@";\
	  $(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@;       \
	else                                                               \
	  echo "Skipping build of $@: $(CC) not found.";                   \
	fi

example_mpi: example_mpi.c libharmony
	@if [ -x "`which $(MPICC) 2>&1`" ]; then                              \
	  echo "$(MPICC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@";\
	  $(MPICC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@;       \
	else                                                                  \
	  echo "Skipping build of $@: $(MPICC) not found.";                   \
	fi

libharmony:
	$(MAKE) -C $(TO_BASE)/src libharmony.a

clean:
	rm -f core $(TARGETS) *.xml

distclean: clean
	rm -f *~
