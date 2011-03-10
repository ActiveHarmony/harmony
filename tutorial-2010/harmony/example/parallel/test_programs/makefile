
all: testprogram

CC=gcc
CFLAGS=-g -o testprogram

HARMONY= -I/hivehomes/rahulp/tutorial-2010/harmony/build/

testprogram: binaryarraysearch.o binarrsearch.o
	$(CC) $(CFLAGS) binaryarraysearch.o binarrsearch.o

binaryarraysearch.o: binaryarraysearch.c
	$(CC) $(CFLAGS) binaryarraysearch.c $(HARMONY)

binarrsearch.o: binarrsearch.tcl
	$(CC) $(CFLAGS) binarrsearch.tcl $(HARMONY)

clean:
	rm -rf *o testprogram