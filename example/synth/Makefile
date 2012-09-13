CFLAGS=-Wall -g -I$(TO_BASE)/build
LDFLAGS=-L$(TO_BASE)/build -lharmony

all: synth

synth: synth.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f *.o synth gmon.out tuna*.log *.tuna.dat
