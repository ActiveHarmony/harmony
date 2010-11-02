OPTIMIZE = -O2
GCC_LINUX = g++ -g -DLinux -DLINUX -Dlinux
LIBS_LINUX =    -lm -ldl  
INCLUDE_LINUX = 

GCC_SOLARIS = g++ -g -DSOLARIS -DUSE_TK
LIBS_SOLARIS = -L/usr/imports/lib -lsocket -lnsl -ltk8.0 -ltcl -lm -ldl -lX11
INCLUDE_SOLARIS = -I/usr/imports/include -I/hivehomes/tiwari/ann_1.1.1/include/

GCC=$(GCC_LINUX)
LIBS=$(LIBS_LINUX)
INCLUDE=$(INCLUDE_LINUX)

#GCC=$(GCC_SOLARIS)
#LIBS=$(LIBS_SOLARIS)
#INCLUDE=$(INCLUDE_SOLARIS)

LIBOBJS= code_generator.o Tokenizer.o

all: code_generator

Tokenizer.o:
	$(GCC) -c Tokenizer.cpp

code_generator: code_generator.c Tokenizer.o code_generator.h
	$(GCC) -o code_generator code_generator.c Tokenizer.o -O3

clean:
	rm -f code_generator core.*


