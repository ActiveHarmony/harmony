TO_BASE=../..

CC=gcc
MPICC=mpicc
CFLAGS=-std=c99 -pedantic -Wall -Werror -g
CPPFLAGS=-D_ISOC99_SOURCE -D_XOPEN_SOURCE=500

override CFLAGS+=-I$(TO_BASE)/build
override LDFLAGS+=-L$(TO_BASE)/build -lharmony

TARGETS=example example_mpi #example_f example_chpl

.PHONY: all install clean distclean

all: $(TARGETS)

install: $(TARGETS)
	@echo "Examples must be executed from source directory (for now)."

example: example.c $(TO_BASE)/build/libharmony.a
	@if [ -x "`which $(CC) 2>&1`" ]; then                           \
		echo "$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@"; \
		$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@;        \
	else                                                            \
		echo "Skipping build of $@: $(CC) not found.";          \
	fi

example_mpi: example_mpi.c $(TO_BASE)/build/libharmony.a
	@if [ -x "`which $(MPICC) 2>&1`" ]; then                           \
		echo "$(MPICC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@"; \
		$(MPICC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@;        \
	else                                                               \
		echo "Skipping build of $@: $(MPICC) not found.";          \
	fi

$(TO_BASE)/build/libharmony.a:
	$(MAKE) -C $(TO_BASE)/build libharmony.a

clean:
	rm -f core $(TARGETS) *.xml

distclean: clean
