TO_BASE=../..

CC=gcc
CFLAGS=-Wall -g
FC=gfortran
FFLAGS=-Wall -g
CHPLC=chpl

override CPPFLAGS+=-I$(TO_BASE)/build
override FFLAGS+=-fno-underscoring
override CHPLFLAGS+=-M$(TO_BASE)/build -I$(TO_BASE)/build
override LDFLAGS+=-L$(TO_BASE)/build -lharmony

all: example_c example_f example_chpl

example_c: example.c $(TO_BASE)/build/libharmony.a
	@if [ -x "`which $(CC) 2>&1`" ]; then                    \
		echo "$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@";      \
		$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) -o $@; \
	else                                                     \
		echo "Skipping build of $@: $(CC) not found.";   \
	fi

example_f: example.f $(TO_BASE)/build/libharmony.a
	@if [ -x "`which $(FC) 2>&1`" ]; then                  \
		echo "$(FC) $(FFLAGS) $< $(LDFLAGS) -o $@";    \
		$(FC) $(FFLAGS) $< $(LDFLAGS) -o $@;           \
	else                                                   \
		echo "Skipping build of $@: $(FC) not found."; \
	fi

example_chpl: example.chpl $(TO_BASE)/build/libharmony.a
	@if [ -x "`which $(CHPLC) 2>&1`" ]; then                            \
		echo "$(CHPLC) $(CHPLFLAGS) $< hclient.h $(LDFLAGS) -o $@"; \
		$(CHPLC) $(CHPLFLAGS) $< hclient.h $(LDFLAGS) -o $@;        \
	else                                                                \
		echo "Skipping build of $@: chpl not found.";               \
	fi

$(TO_BASE)/build/libharmony.a:
	$(MAKE) -C $(TO_BASE)/build libharmony.a

clean:
	rm -f core example_c example_f example_chpl
