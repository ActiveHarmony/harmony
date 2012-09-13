all_subsystems = build code-server/standalone example/client_api example/code_generation example/synth

all clean:
	+@for subsystem in $(all_subsystems); do \
	    $(MAKE) -C $$subsystem $@;           \
	done
