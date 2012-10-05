OPTIMIZE =  

CC=gcc
CXX=g++
AR= ar
ARCREATE= rc

XML_INCLUDE=-I/usr/include/libxml2
INCLUDE=${XML_INCLUDE}


CFLAGS=-Wall -g -pg $(INCLUDE)
CXXFLAGS=$(CFLAGS)

XML_LIB = -L/opt/local/lib -lxml2
LDFLAGS=-lm -ldl -lnsl -ltcl -lxml2

OBJS=hclient.o hsockutil.o hutil.o hmesgs.o hclientf.o record.o

all: hserver libharmony.a

hserver: hserver.o hutil.o hsockutil.o hmesgs.o hglobal_config.o httpsvr.o record.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
	cp hserver ../bin

libharmony.a: $(OBJS)
	$(AR) $(ARCREATE) $@ $^

hserver.o: hserver.cxx hserver.h hutil.h hdb.h hsockutil.h hmesgs.h
	$(CXX) -c $(CFLAGS) $< -o $@

httpsvr.o: httpsvr.cxx httpsvr.h hserver.h hsockutil.h
	$(CXX) -c $(CFLAGS) $< -o $@

hglobal_config.o: hglobal_config.c hglobal_config.h
	$(CC) -c $(CFLAGS) $< -o $@

hutil.o: hutil.c hutil.h
	$(CC) -c $(CFLAGS) $< -o $@

hsockutil.o: hsockutil.c hsockutil.h hutil.h hmesgs.h
	$(CC) -c $(CFLAGS) $< -o $@

hmesgs.o: hmesgs.c hmesgs.h hutil.h
	$(CC) -c $(CFLAGS) $< -o $@

hclient.o: hclient.c hclient.h hmesgs.h hsockutil.h hutil.h
	$(CC) -c $(CFLAGS) $< -o $@

hclientf.o: hclientf.c hclient.h hmesgs.h hutil.h hsockutil.h
	$(CC) -c $(CFLAGS) $< -o $@

record.o: record.c record.h
	$(CC) -c $(CFLAGS) $< -o $@

y.tab.c: grammar.y
	yacc -v -d grammar.y

lex.yy.c: scan.l
	lex scan.l

y.tab.o: y.tab.c
	gcc -g -Wall -c $(OPTIMIZE) y.tab.c

stack.o : stack.c stack.h
	gcc -g -Wall -c $(OPTIMIZE) stack.c

libcreater: y.tab.o lex.yy.c stack.o libcreater.c
	gcc -g  y.tab.o lex.yy.c stack.o libcreater.c -DYYDEBUG $(OPTIMIZE) -o libcreater -w
	cp libcreater ../bin

clean:
	rm -f core a.out *.o y.output libharmony.a hserver ../bin/hserver libcreater ../bin/libcreater ../bin/hconfig.tcl

distclean:
	rm -f *~
