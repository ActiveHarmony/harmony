all: schedular

# where is the harmony installed in your system
HARMONY=${HOME}/tutorial-2010/harmony/
schedular: run_sch.c  ${HARMONY}/build/harmony.a
	g++ -o schedular run_sch.c ${HARMONY}/build/harmony.a -lm -ldl -lnsl -ltcl -ltk -I${HARMONY}/build

clean:
	rm -f core a.out *.o schedular
