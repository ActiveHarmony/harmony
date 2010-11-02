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

LIBOBJS= code_generator.o StringTokenizer.o

all: code_generator

StringTokenizer.o:
	$(GCC) -c StringTokenizer.cpp

Tokenizer.o:
	$(GCC) -c Tokenizer.cpp

code_generator: code_generator.c StringTokenizer.o Tokenizer.o
	$(GCC) -o code_generator code_generator.c StringTokenizer.o Tokenizer.o

clean:
	rm -f code_generator core.* *.o


