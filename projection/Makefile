OPTIMIZE = -O2
GCC_LINUX = g++ -g -DLinux -DLINUX -Dlinux -w
LIBS_LINUX =    -lm -ldl  -lnsl -L/hivehomes/tiwari/ann_1.1.1/lib -lANN
INCLUDE_LINUX = -I/hivehomes/tiwari/ann_1.1.1/include/

GCC=$(GCC_LINUX)
LIBS=$(LIBS_LINUX)
INCLUDE=$(INCLUDE_LINUX)

LIBOBJS= pclient.o hsockutil.o hutil.o hmesgs.o StringTokenizer.o
AR= ar
ARCREATE= cr

all: pserver swig_lib pclient

pserver: pserver.o hutil.o hsockutil.o hmesgs.o StringTokenizer.o ANN_init.h
	$(GCC) -o pserver pserver.o hutil.o hsockutil.o hmesgs.o StringTokenizer.o $(LIBS) $(INCLUDE)

pserver.o: pserver.c hserver.h hutil.h hsockutil.h hmesgs.h ANN_init.h
	$(GCC) -c pserver.c $(INCLUDE)

hutil.o: hutil.c hutil.h
	$(GCC) -c hutil.c $(INCLUDE)

hsockutil.o: hsockutil.c hsockutil.h hutil.h hmesgs.h
	$(GCC) -c hsockutil.c $(INCLUDE)

hmesgs.o: hmesgs.c hmesgs.h hutil.h
	$(GCC) -c hmesgs.c $(INCLUDE)

StringTokenizer.o: StringTokenizer.cpp StringTokenizer.h
	$(GCC) -c StringTokenizer.cpp

pclient: pclient.c pclient.h hutil.o hsockutil.o hmesgs.o StringTokenizer.o
	$(GCC) -o pclient pclient.c $(INCLUDE) hsockutil.o hutil.o hmesgs.o -DTEST

swig_lib: pclient.c pclient.i pclient.h hutil.o hsockutil.o hmesgs.o StringTokenizer.o
	$(GCC) -c pclient.c $(INCLUDE)
	swig -tcl8 -c++ pclient.i
	$(GCC) -c -fpic pclient.c pclient_wrap.cxx -isystem /usr/include
	$(GCC) -shared pclient.o pclient_wrap.o hsockutil.o hutil.o hmesgs.o -o nearest_neighbor.so

clean:
	rm -f core a.out core.* pserver pclient *.o *~

clean_core:
	rm -f core.*


