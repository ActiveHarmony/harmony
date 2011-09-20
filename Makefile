all_subsystems = build code-server/build code-server/standalone example/parallel/code_generation

all clean:
	+@for subsystem in $(all_subsystems); do \
	    $(MAKE) -C $$subsystem $@;           \
	done
