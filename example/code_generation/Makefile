TO_BASE=../..

CC=mpicc
CFLAGS=-Wall -g -I$(TO_BASE)/build
LDFLAGS=-L$(TO_BASE)/build -lharmony

all: gemm

gemm: gemm.c $(TO_BASE)/build/libharmony.a
	@if [ -x "`which $(CC) 2>&1`" ]; then				\
		echo "$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@";		\
		$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@;			\
	else								\
		echo "Skipping build of $@: $(CC) not found.";		\
	fi

$(TO_BASE)/build/libharmony.a:
	$(MAKE) -C $(TO_BASE)/build libharmony.a

clean:
	rm -f core gemm
