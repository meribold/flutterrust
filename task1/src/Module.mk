local_program := $(subdirectory)/task1

sources  += $(addsuffix .cpp,$(local_program))
programs += $(local_program)

$(local_program) : all_ldflags = $(addprefix -L,$(ld_dirs)) $(LDFLAGS)
$(local_program) : all_ldlibs  = $(LDLIBS) -lboost_regex \
   $(shell icu-config --ldflags-libsonly) \
   $(patsubst lib%.a,-l%,$(notdir $(libraries)))

# Enable the second expansion of prerequisites (only).
.SECONDEXPANSION:

# Beware: while variables appearing in the target and prerequisite parts of a rule are
# immediately evaluated, inside recipes, expanding "occurs after make has finished reading
# all the makefiles and the target is determined to be out of date"
# (http://www.gnu.org/software/make/manual/make.html#Variables-in-Recipes).  This means we
# can't use the `local_` variables in the recipe.
$(local_program): $(addsuffix .o,$(local_program)) $$(libraries)
	$(CXX) $(all_ldflags) $^ $(all_ldlibs) -o $@

# vim: tw=90 ts=8 sts=-1 sw=3 noet
