local_sources := $(shell find $(subdirectory) -maxdepth 1 -name '*.cpp')
local_objects := $(addprefix $(OBJDIR)/,$(subst src/,,$(local_sources:.cpp=.o)))
local_program := $(OBJDIR)/$(subst src,,$(subdirectory))flutterrust

sources  += $(local_sources)
programs += $(local_program)

$(local_program) : local_ldflags = $(addprefix -L,$(ld_dirs)) $(all_ldflags)
$(local_program) : local_ldlibs  = $(all_ldlibs) \
   $$($(ICUCONFIG) --ldflags-libsonly) \
   $$($(WXCONFIG) --libs) \
   -lboost_regex \
   $(patsubst lib%.a,-l%,$(notdir $(libraries)))

# Enable the second expansion of prerequisites.
.SECONDEXPANSION:

$(local_program): $(local_objects) $$(libraries) | $$(dir $$@)
	$(CXX) $(local_ldflags) $^ $(local_ldlibs) -o $@

# vim: tw=90 ts=8 sts=-1 sw=3 noet
