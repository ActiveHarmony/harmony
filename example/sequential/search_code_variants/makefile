all: search_code_variants

search_code_variants: search_code_variants.c ../../../build/libharmony.a
	g++ -w -g -o search_code_variants -Wall search_code_variants.c -L../../../build -lharmony -lm -lnsl -I../../../build

clean:
	rm -f core a.out *.o search_code_variants
