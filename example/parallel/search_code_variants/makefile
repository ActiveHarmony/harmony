all: search_code_variants

search_code_variants: search_code_variants.o ../../../build/harmony.a
	g++ -g -Wall search_code_variants.o ../../../build/harmony.a -lm -lnsl -ltcl -I../../../build -o search_code_variants

search_code_variants.o: search_code_variants.c
	g++ -g -Wall -c search_code_variants.c -I../../../build

clean:
	rm -f core a.out *.o search_code_variants
