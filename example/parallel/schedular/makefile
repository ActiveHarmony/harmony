all: schedular

# where is the harmony installed in your system
HARMONY=${HOME}/tutorial-2010/harmony/
schedular: run_sch.c  ${HARMONY}/build/libharmony.a
	g++ -o schedular run_sch.c -L${HARMONY}/build -lharmony -lm -ldl -lnsl -ltcl -I${HARMONY}/build

clean:
	rm -f core a.out *.o schedular
