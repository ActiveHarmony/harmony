CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Werror -g
CPPFLAGS=-D_ISOC99_SOURCE -D_XOPEN_SOURCE=500

.PHONY: all install clean distclean

all: synth

install: synth
	@echo "Examples must be executed from source directory (for now)."

synth: synth.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -f *.o synth gmon.out tuna*.log *.tuna.dat

distclean: clean
	rm -f *~
