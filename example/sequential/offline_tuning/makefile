all: offline_tuning

offline_tuning: offline_tuning.c ../../../build/harmony.a
	g++ -w -g -o offline_tuning -Wall offline_tuning.c ../../../build/harmony.a -lm -lnsl -I../../../build

clean:
	rm -f core a.out *.o offline_tuning
