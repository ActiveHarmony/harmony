OPTIMIZE = -O2
GCC_LINUX = g++ -g -DLinux -DLINUX -Dlinux -w
LIBS_LINUX =    -lm -ldl  -lnsl -L/hivehomes/tiwari/ann_1.1.1/lib -lANN
INCLUDE_LINUX = -I/hivehomes/tiwari/ann_1.1.1/include/

GCC=$(GCC_LINUX)
LIBS=$(LIBS_LINUX)
INCLUDE=$(INCLUDE_LINUX)

LIBOBJS= pclient.o psockutil.o putil.o pmesgs.o StringTokenizer.o
AR= ar
ARCREATE= cr

all: pserver swig_lib pclient

pserver: pserver.o putil.o psockutil.o pmesgs.o StringTokenizer.o ANN_init.h
	$(GCC) -o pserver pserver.o putil.o psockutil.o pmesgs.o StringTokenizer.o $(LIBS) $(INCLUDE)

pserver.o: pserver.c pserver.h putil.h psockutil.h pmesgs.h ANN_init.h
	$(GCC) -c pserver.c $(INCLUDE)

putil.o: putil.c putil.h
	$(GCC) -c putil.c $(INCLUDE)

psockutil.o: psockutil.c psockutil.h putil.h pmesgs.h
	$(GCC) -c psockutil.c $(INCLUDE)

pmesgs.o: pmesgs.c pmesgs.h putil.h
	$(GCC) -c pmesgs.c $(INCLUDE)

StringTokenizer.o: StringTokenizer.cpp StringTokenizer.h
	$(GCC) -c StringTokenizer.cpp

pclient: pclient.c pclient.h putil.o psockutil.o pmesgs.o StringTokenizer.o
	$(GCC) -o pclient pclient.c $(INCLUDE) psockutil.o putil.o pmesgs.o -DTEST

swig_lib: pclient.c pclient.i pclient.h putil.o psockutil.o pmesgs.o StringTokenizer.o
	$(GCC) -c pclient.c $(INCLUDE)
	swig -tcl8 -c++ pclient.i
	$(GCC) -c -fpic pclient.c pclient_wrap.cxx -isystem /usr/include
	$(GCC) -shared pclient.o pclient_wrap.o psockutil.o putil.o pmesgs.o -o nearest_neighbor.so

clean:
	rm -f core a.out core.* pserver pclient *.o *~

clean_core:
	rm -f core.*


