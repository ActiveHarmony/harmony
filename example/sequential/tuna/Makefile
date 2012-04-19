TO_BASE=../../..

CFLAGS=-Wall -g -I$(TO_BASE)/build
LDFLAGS=-L$(TO_BASE)/build -lharmony

all: tuna synth

tuna: tuna.c
	$(MAKE) -C $(TO_BASE)/build libharmony.a
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

synth: synth.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f *.o tuna synth
