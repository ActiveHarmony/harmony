OPTIMIZE = -O2
GCC_LINUX = g++ -g -DLinux -DLINUX -Dlinux
LIBS_LINUX =    -lm -ldl  -lnsl #-L/hivehomes/tiwari/ann_1.1.1/lib -lANN
INCLUDE_LINUX = #-I/hivehomes/tiwari/ann_1.1.1/include/

GCC_SOLARIS = g++ -g -DSOLARIS
LIBS_SOLARIS = -L/usr/imports/lib -lsocket -lnsl -ltcl -lm -ldl -lX11
INCLUDE_SOLARIS = -I/usr/imports/include -I/hivehomes/tiwari/ann_1.1.1/include/

GCC=$(GCC_LINUX)
LIBS=$(LIBS_LINUX)
INCLUDE=$(INCLUDE_LINUX)

#GCC=$(GCC_SOLARIS)
#LIBS=$(LIBS_SOLARIS)
#INCLUDE=$(INCLUDE_SOLARIS)

LIBOBJS= code-client.o hsockutil.o hutil.o hmesgs.o Tokenizer.o
AR= ar
ARCREATE= cr

all: code-server swig_lib code-client

code-server: code-server.o cutil.o csockutil.o cmesgs.o Tokenizer.o
	$(GCC) -o ../bin/code-server code-server.o cutil.o csockutil.o cmesgs.o Tokenizer.o $(LIBS) $(INCLUDE)

code-server.o: code-server.c code-server.h cutil.h csockutil.h cmesgs.h generator_manager.c
	$(GCC) -c code-server.c $(INCLUDE)

cutil.o: cutil.c cutil.h
	$(GCC) -c cutil.c $(INCLUDE)

csockutil.o: csockutil.c csockutil.h cutil.h cmesgs.h
	$(GCC) -c csockutil.c $(INCLUDE)

cmesgs.o: cmesgs.c cmesgs.h cutil.h
	$(GCC) -c cmesgs.c $(INCLUDE)

StringTokenizer.o: Tokenizer.cpp Tokenizer.h
	$(GCC) -c Tokenizer.cpp

code-client: code-client.c code-client.h cutil.o csockutil.o cmesgs.o Tokenizer.o
	$(GCC) -o code-client code-client.c $(INCLUDE) csockutil.o cutil.o cmesgs.o -DTEST

swig_lib: code-client.c code_client.i code-client.h cutil.o csockutil.o cmesgs.o Tokenizer.o
	g++ -c code-client.c $(INCLUDE)
	swig -tcl8 -c++ code_client.i
	g++ -c -fpic code-client.c code_client_wrap.cxx -isystem /usr/include
	g++ -shared code-client.o code_client_wrap.o csockutil.o cutil.o cmesgs.o -o code-server.so
clean:
	rm -f core a.out lex.yy.c y.tab.* *.o parser libcreater y.output ../bin/libcreater *~ hserver libharmony.a ../bin/hserver ../bin/hconfig.tcl


