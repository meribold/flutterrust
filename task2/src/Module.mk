local_program := $(subdirectory)/task2
local_sources := $(shell find $(subdirectory) -maxdepth 1 -name '*.cpp')

sources       += $(local_sources)
programs      += $(local_program)

$(local_program) : all_ldflags  = $(addprefix -L,$(ld_dirs)) $(LDFLAGS)
$(local_program) : all_ldlibs   = $(LDLIBS) \
   $(patsubst lib%.a,-l%,$(notdir $(libraries)))

$(local_program): $(local_sources:.cpp=.o) $(libraries)
	$(CXX) $(all_ldflags) $^ $(all_ldlibs) -o $@

# vim: tw=80 sw=3 noet
