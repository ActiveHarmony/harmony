OPTIMIZE = -O2
GCC_LINUX = g++ -g -DLinux -DLINUX -Dlinux
LIBS_LINUX =    -lm -ldl  -lnsl -L/hivehomes/tiwari/ann_1.1.1/lib -lANN
INCLUDE_LINUX = -I/hivehomes/tiwari/ann_1.1.1/include/

GCC_SOLARIS = g++ -g -DSOLARIS -DUSE_TK
LIBS_SOLARIS = -L/usr/imports/lib -lsocket -lnsl -ltk8.0 -ltcl -lm -ldl -lX11
INCLUDE_SOLARIS = -I/usr/imports/include -I/hivehomes/tiwari/ann_1.1.1/include/


all: swig_lib


swig_lib: scp_candidate.c round.i scp_candidate.h
	g++ -fpic -c scp_candidate.c $(INCLUDE) -w
	swig -tcl8 -c++ round.i
	g++ -c -fpic round_wrap.cxx -isystem /usr/include
	g++ -shared round_wrap.o scp_candidate.o -o round.so
	cp round.so ../tcl/
clean:
	rm -f core a.out core.* *.o *~ round.so

clean_core:
	rm -f core.* *~ round.so


