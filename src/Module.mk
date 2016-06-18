local_sources := $(shell find $(subdirectory) -maxdepth 1 -name '*.cpp')
local_program := $(subdirectory)/flutterrust

programs      += $(local_program)
sources       += $(local_sources)

$(local_program) : local_ldflags  = $(addprefix -L,$(ld_dirs)) $(all_ldflags)
$(local_program) : local_ldlibs   = $(all_ldlibs) \
   $(patsubst lib%.a,-l%,$(notdir $(libraries)))

# Enable the second expansion of prerequisites (only).
.SECONDEXPANSION:

$(local_program): $(local_sources:.cpp=.o) $$(libraries)
	$(CXX) $(local_ldflags) $^ $(local_ldlibs) -o $@

# vim: tw=90 ts=8 sts=-1 sw=3 noet
