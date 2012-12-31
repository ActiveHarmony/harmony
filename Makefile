all_subsystems = build code-server/standalone example/client_api example/code_generation example/synth

.PHONY: all install clean distclean

all install clean distclean:
	+@for subsystem in $(all_subsystems); do \
	    $(MAKE) -C $$subsystem $@;           \
	done
