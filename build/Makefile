OPTIMIZE =  

GCC_LINUX = g++ -DLinux -DLINUX -Dlinux -Df2cFortran -w -DUSE_NON_CONST -g -pg 
LIBS_LINUX = -lm -ldl -lnsl -ltcl
INCLUDE_LINUX =

GCC=$(GCC_LINUX)
LIBS=$(LIBS_LINUX)
INCLUDE=$(INCLUDE_LINUX)

LIBOBJS= hclient.o hsockutil.o hutil.o hmesgs.o hclientc.o
AR= ar
ARCREATE= cr

all: hserver harmony.a libcreater

hserver: hserver.o hutil.o hsockutil.o hmesgs.o hglobal_config.o httpsvr.o
	$(GCC) -o hserver hserver.o hutil.o hsockutil.o hmesgs.o hglobal_config.o httpsvr.o $(LIBS) $(INCLUDE)
	cp hserver ../bin

harmony.a: $(LIBOBJS)
	$(AR) $(ARCREATE) harmony.a $(LIBOBJS)

hglobal_config.o: hglobal_config.c hglobal_config.h
	$(GCC) -c hglobal_config.c $(INCLUDE)

hserver.o: hserver.c hserver.h hutil.h hdb.h hsockutil.h hmesgs.h
	$(GCC) -c hserver.c -o hserver.o $(INCLUDE)

hutil.o: hutil.c hutil.h
	$(GCC) -c hutil.c $(INCLUDE)

hsockutil.o: hsockutil.c hsockutil.h hutil.h hmesgs.h
	$(GCC) -c hsockutil.c $(INCLUDE)

hmesgs.o: hmesgs.c hmesgs.h hutil.h
	$(GCC) -c hmesgs.c $(INCLUDE) 

httpsvr.o: httpsvr.c httpsvr.h hserver.h hsockutil.h
	$(GCC) -c httpsvr.c $(INCLUDE)

hclient.o: hclient.c hclient.h hmesgs.h hsockutil.h hutil.h
	$(GCC) -c hclient.c $(INCLUDE) 

hclientf.o: hclientf.c hclient.h hmesgs.h hutil.h hsockutil.h
	$(GCC) -c hclientf.c $(INCLUDE)

hclientc.o: hclientc.c hclient.h hmesgs.h hutil.h hsockutil.h
	$(GCC) -c hclientc.c $(INCLUDE)

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
	rm -f core a.out *.o y.output harmony.a hserver ../bin/hserver libcreater ../bin/libcreater ../bin/hconfig.tcl

distclean:
	rm -f *~
