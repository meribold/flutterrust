local_directory := task2/src

local_program := task2

local_sources := $(shell find $(local_directory) -maxdepth 1 -name '*.cpp')
local_prereqs := $(local_sources:.cpp=.d)
local_objects := $(local_sources:.cpp=.o)
local_program := $(addprefix $(local_directory)/,$(local_program))

sources      += $(local_sources)
prereq_files += $(local_prereqs)
objects      += $(local_objects)
programs     += $(local_program)

$(local_program) : all_ldflags  = $(addprefix -L,$(ld_dirs)) $(LDFLAGS)
$(local_program) : all_ldlibs   = $(LDLIBS) \
	$(patsubst lib%.a,-l%,$(notdir $(libraries)))

$(local_program): $(local_objects) $(libraries)
	$(CXX) $(all_ldflags) $^ $(all_ldlibs) -o $@

# vim: tw=100 sw=3 noet
