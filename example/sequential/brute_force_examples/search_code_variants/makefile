all: search_code_variants

search_code_variants: search_code_variants.c parameter_space.tcl ../../../../build/harmony.a
	g++ -w -o search_code_variants -Wall search_code_variants.c ../../../../build/harmony.a -lm -ldl -lnsl -ltcl -ltk -I../../../../build

clean:
	rm -f core a.out *.o search_code_variants
