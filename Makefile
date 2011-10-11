#all_subsystems = build code-server/build code-server/standalone example/parallel/code_generation ann
# Are we still using stuffs in code-server/build ?
all_subsystems = build code-server/standalone example/parallel/code_generation ann \

all clean:
	+@for subsystem in $(all_subsystems); do \
	    $(MAKE) -C $$subsystem $@;           \
	done
