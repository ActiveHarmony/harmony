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

LIBOBJS= pclient.o hsockutil.o hutil.o hmesgs.o Tokenizer.o
AR= ar
ARCREATE= cr

all: semaphore-test swig_lib pclient

semaphore-test: semaphore-test.o hutil.o hsockutil.o hmesgs.o Tokenizer.o
	$(GCC) -o semaphore-test semaphore-test.o hutil.o hsockutil.o hmesgs.o Tokenizer.o $(LIBS) $(INCLUDE)

semaphore-test.o: semaphore-test.c hserver.h hutil.h hdb.h hsockutil.h hmesgs.h generator_parent.c
	$(GCC) -c semaphore-test.c $(INCLUDE)

hutil.o: hutil.c hutil.h
	$(GCC) -c hutil.c $(INCLUDE)

hsockutil.o: hsockutil.c hsockutil.h hutil.h hmesgs.h
	$(GCC) -c hsockutil.c $(INCLUDE)

hmesgs.o: hmesgs.c hmesgs.h hutil.h
	$(GCC) -c hmesgs.c $(INCLUDE)

StringTokenizer.o: Tokenizer.cpp Tokenizer.h
	$(GCC) -c Tokenizer.cpp

pclient: pclient.c pclient.h hutil.o hsockutil.o hmesgs.o StringTokenizer.o
	$(GCC) -o pclient pclient.c $(INCLUDE) hsockutil.o hutil.o hmesgs.o -DTEST

swig_lib: pclient.c pclient.i pclient.h hutil.o hsockutil.o hmesgs.o Tokenizer.o
	g++ -c pclient.c $(INCLUDE)
	swig -tcl8 -c++ pclient.i
	g++ -c -fpic pclient.c pclient_wrap.cxx -isystem /usr/include
	g++ -shared pclient.o pclient_wrap.o hsockutil.o hutil.o hmesgs.o -o nearest_neighbor.so
clean:
	rm -f core a.out lex.yy.c y.tab.* *.o parser libcreater y.output ../bin/libcreater *~ hserver harmony.a ../bin/hserver ../bin/hconfig.tcl


