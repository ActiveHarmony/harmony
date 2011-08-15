OPTIMIZE =  

GCC_LINUX = g++ -DLinux -DLINUX -Dlinux -Df2cFortran -w -DUSE_NON_CONST -g -pg 
LIBS_LINUX =   -lm -ldl  -lnsl -ltcl8.4 -ltk8.4
INCLUDE_LINUX =

GCC=$(GCC_LINUX)
LIBS=$(LIBS_LINUX)
INCLUDE=$(INCLUDE_LINUX)

LIBOBJS= hclient.o hsockutil.o hutil.o hmesgs.o hclientc.o
AR= ar
ARCREATE= cr

all: hserver hserver_no_tk harmony.a parse swig_round_lib

hserver: hglobal_config.o hserver.o hutil.o hsockutil.o hmesgs.o 
	$(GCC) -o hserver hglobal_config.o hserver.o hutil.o hsockutil.o hmesgs.o $(LIBS) $(INCLUDE)
	cp hserver ../bin

hserver_no_tk:  hglobal_config.o hserver_no_tk.o hutil.o hsockutil.o hmesgs.o
	$(GCC) -o hserver_no_tk hglobal_config.o hserver_no_tk.o hutil.o hsockutil.o hmesgs.o $(LIBS) $(INCLUDE)
	cp hserver_no_tk ../bin

hserver_no_tk.o: hserver.c hserver.h hutil.h hdb.h hsockutil.h hmesgs.h
	$(GCC) -c hserver.c -o hserver_no_tk.o $(INCLUDE)

hglobal_config.o: hglobal_config.c hglobal_config.h
	$(GCC) -c hglobal_config.c $(INCLUDE)

hserver.o: hserver.c hserver.h hutil.h hdb.h hsockutil.h hmesgs.h
	$(GCC) -c -DUSE_TK hserver.c $(INCLUDE) 

hutil.o: hutil.c hutil.h
	$(GCC) -c hutil.c $(INCLUDE)

hsockutil.o: hsockutil.c hsockutil.h hutil.h hmesgs.h
	$(GCC) -c hsockutil.c $(INCLUDE)

hmesgs.o: hmesgs.c hmesgs.h hutil.h
	$(GCC) -c hmesgs.c $(INCLUDE) 

harmony.a: $(LIBOBJS)
	$(AR) $(ARCREATE) harmony.a $(LIBOBJS)

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

parse: y.tab.o lex.yy.c stack.o libcreater.c
	gcc -g  y.tab.o lex.yy.c stack.o libcreater.c -DYYDEBUG $(OPTIMIZE) -o libcreater -w
	cp libcreater ../bin

clean:
	rm -f core a.out lex.yy.c y.tab.* *.o parser libcreater y.output ../bin/libcreater *~ hserver harmony.a ../bin/hserver ../bin/hconfig.tcl ../bin/hserver_no_tk hserver_no_tk

swig_round_lib:
	cd ../round_scp; gmake
