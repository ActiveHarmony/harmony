all: example

example: simplextest.o ../../../build/libharmony.a
	$(MAKE) -C $(TO_ROOT)/build libharmony.a
	g++ -g -Wall simplextest.o $(LDFLAGS) -lharmony -lm -lnsl -o $@

simplextest.o: simplextest.c
	gcc -g -Wall $(CFLAGS) -c $<

clean:
	rm -f core a.out *.o example
