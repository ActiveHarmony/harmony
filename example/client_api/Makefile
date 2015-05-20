TO_BASE=../..

CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Werror -g
FC=gfortran
FFLAGS=-Wall -g
CHPLC=chpl

override CPPFLAGS+=-I$(TO_BASE)/src
override FFLAGS+=-fno-underscoring
override CHPLFLAGS+=-M$(TO_BASE)/src -I$(TO_BASE)/src
override LDFLAGS+=-L$(TO_BASE)/src
override LDLIBS+=-lharmony -lm

TARGETS=example_c #example_f example_chpl

install:
	@echo "Examples must be executed from source directory (for now)."

example_c: example.c libharmony
	@if [ -x "`which $(CC) 2>&1`" ]; then                              \
	  echo "$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@";\
	  $(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@;       \
	else                                                               \
	  echo "Skipping build of $@: $(CC) not found.";                   \
	fi

example_f: example.f libharmony
	@if [ -x "`which $(FC) 2>&1`" ]; then                  \
	  echo "$(FC) $(FFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@";\
	  $(FC) $(FFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@;       \
	else                                                   \
	  echo "Skipping build of $@: $(FC) not found.";       \
	fi

example_chpl: example.chpl libharmony
	@if [ -x "`which $(CHPLC) 2>&1`" ]; then                        \
	  echo "$(CHPLC) $(CHPLFLAGS) $< hclient.h $(LDFLAGS) $(LDLIBS) -o $@";\
	  $(CHPLC) $(CHPLFLAGS) $< hclient.h $(LDFLAGS) $(LDLIBS) -o $@;\
	else                                                            \
	  echo "Skipping build of $@: chpl not found.";                 \
	fi

libharmony:
	$(MAKE) -C $(TO_BASE)/src libharmony.a

# Active Harmony makefiles should always include this file last.
include $(TO_BASE)/Makefile.common
