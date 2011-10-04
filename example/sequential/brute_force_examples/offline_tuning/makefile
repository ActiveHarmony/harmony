all: offline_tuning

offline_tuning: offline_tuning.c offline.tcl ../../../../build/harmony.a
	g++ -w -o offline_tuning -Wall offline_tuning.c ../../../../build/harmony.a -lm -ldl -lnsl -ltcl -I../../../../build

clean:
	rm -f core a.out *.o offline_tuning
