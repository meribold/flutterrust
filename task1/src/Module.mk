local_sources := $(shell find $(subdirectory) -maxdepth 1 -name '*.cpp')
local_objects := $(addprefix $(OBJDIR)/,$(subst src/,,$(local_sources:.cpp=.o)))
local_program := $(OBJDIR)/$(subst src,,$(subdirectory))task1

sources  += $(local_sources)
programs += $(local_program)

$(local_program) : local_ldflags = $(addprefix -L,$(ld_dirs)) $(all_ldflags)
$(local_program) : local_ldlibs  = $(all_ldlibs) \
   $$($(ICUCONFIG) --ldflags-libsonly) \
   -lboost_regex \
   $(patsubst lib%.a,-l%,$(notdir $(libraries)))

.SECONDEXPANSION:

# Beware: while variables appearing in the target and prerequisite parts of a rule are
# immediately evaluated, inside recipes, expanding "occurs after make has finished reading
# all the makefiles and the target is determined to be out of date"
# (http://www.gnu.org/software/make/manual/make.html#Variables-in-Recipes).  This means we
# can't use the `local_` variables in the recipe.
$(local_program): $(local_objects) $$(libraries) | $$(dir $$@)
	$(CXX) $(local_ldflags) $^ $(local_ldlibs) -o $@

# vim: tw=90 ts=8 sts=-1 sw=3 noet
