local_directory := src

local_program := flutterrust

local_sources := $(shell find $(local_directory) -maxdepth 1 -name '*.cpp')
local_prereqs := $(local_sources:.cpp=.d)
local_objects := $(local_sources:.cpp=.o)
local_library := $(addprefix $(local_directory)/,$(local_library))

sources      += $(local_sources)
prereq_files += $(local_prereqs)
objects      += $(local_objects)
libraries    += $(local_library)
programs     += $(local_program)

$(local_program) : all_ldflags  = $(addprefix -L,$(ld_dirs)) $(LDFLAGS)
$(local_program) : all_ldlibs   = $(LDLIBS) -lboost_regex \
	$(shell icu-config --ldflags-libsonly) \
	$(patsubst lib%.a,-l%,$(notdir $(libraries)))

$(local_program): $(objects) $(libraries)
	$(CXX) $(all_ldflags) $(objects) $(all_ldlibs) -o $(local_program)

