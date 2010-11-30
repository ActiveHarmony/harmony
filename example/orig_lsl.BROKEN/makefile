DEFINED_TAG = -DLINUX -I../../nb
LINK_TAG= -lhrtime

all:	libll libary try tryll tryary

libnb.o : 
	cd ../../nb; make

TwoDTable.c : example.lsl
	../../bin/libcreater example.lsl

TwoDTable.o : TwoDTable.c
	gcc -g -Wall -c TwoDTable.c -I../../nb

try.o : try.c
	gcc -g -Wall -c try.c -D_LSL_ $(DEFINED_TAG)

try : libnb.o TwoDTable.o try.o
	gcc -g -o try -Wall ../../nb/libnb.o TwoDTable.o try.o  -ldl $(LINK_TAG)


tryll.o : try.c
	gcc -g -Wall -c try.c -D_LIBLL_ $(DEFINED_TAG)

tryll: tryll.o libll.o
	gcc -g -o tryll -Wall libll.o try.o -lm $(LINK_TAG)

tryary.o: try.c
	gcc -g -Wall -c try.c -D_LIBARY_ $(DEFINED_TAG)

tryary: tryary.o libary.o
	gcc -g -o tryary -Wall libary.o try.o -lm $(LINK_TAG)

clean:
	rm -f core a.out try tryll tryary *.o *.so  TwoDTable.*

libll: libll.c
	gcc -fPIC -Wall -g -c libll.c $(DEFINED_TAG)
	gcc -g -shared -o libll.so libll.o -lc -lm
	gcc -Wall -g -c libll.c $(DEFINED_TAG)

libary: libary.c
	gcc -fPIC -Wall -g -c libary.c $(DEFINED_TAG)
	gcc -g -shared -o libary.so libary.o -lc -lm
	gcc -Wall -g -c libary.c $(DEFINED_TAG)


