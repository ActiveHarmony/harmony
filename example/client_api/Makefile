TO_BASE=../..

CFLAGS=-Wall -g -I$(TO_BASE)/build
FFLAGS=-Wall -g -fno-underscoring
CHPLC=chpl
CHPLFLAGS=-M$(TO_BASE)/build -I$(TO_BASE)/build
LDFLAGS=-L$(TO_BASE)/build -lharmony

TARGETS=example_c #example_f example_chpl

all: $(TARGETS)

example_c: example.c $(TO_BASE)/build/libharmony.a
	@if [ -x "`which $(CC) 2>&1`" ]; then				\
		echo "$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@";		\
		$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@;			\
	else								\
		echo "Skipping build of $@: $(CC) not found.";		\
	fi

example_f: example.f $(TO_BASE)/build/libharmony.a
	@if [ -x "`which $(FC) 2>&1`" ]; then				\
		echo "$(FC) $(FFLAGS) $< $(LDFLAGS) -o $@";		\
		$(FC) $(FFLAGS) $< $(LDFLAGS) -o $@;			\
	else								\
		echo "Skipping build of $@: $(FC) not found.";		\
	fi

example_chpl: example.chpl $(TO_BASE)/build/libharmony.a
	@if [ -x "`which $(CHPLC) 2>&1`" ]; then			\
		echo "$(CHPLC) $(CHPLFLAGS) $< hclient.h $(LDFLAGS) -o $@"; \
		$(CHPLC) $(CHPLFLAGS) $< hclient.h $(LDFLAGS) -o $@;	\
	else								\
		echo "Skipping build of $@: chpl not found.";		\
	fi

$(TO_BASE)/build/libharmony.a:
	$(MAKE) -C $(TO_BASE)/build libharmony.a

clean:
	rm -f core $(TARGETS)
