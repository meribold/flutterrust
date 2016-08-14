# Top-level makefile; includes all Module.mk files it can find in any of the
# subdirectories.  Partly based on the non-recursive make from "Managing Projects With GNU
# Make".  Written for GNU Make and not expected to be portable.

# Parameters controlling implicit rules (as listed in section 10.3 of the GNU Make Manual)
# or that the user can override with command options use upper case letters.  Those
# serving internal purposes in this makefile use lower case letters.  This practice is
# recommended in chapter 6 of the GNU Make Manual.

# "Every Makefile should contain this line..." - section 7.2.1 of the GNU Coding Standards
SHELL := /bin/sh

# Disable all built-in rules.  See http://stackoverflow.com/q/4122831 and
# http://gnu.org/software/make/manual/html_node/Catalogue-of-Rules.html.
MAKEFLAGS += --no-builtin-rules

# Clear the suffix list; no suffix rules in this makefile.  See section 7.2.1 of the GNU
# Coding Standards.
.SUFFICES:

CXX       ?= g++
WXCONFIG  ?= wx-config
ICUCONFIG ?= icu-config
CPPFLAGS  += -Wall -Wextra -pedantic
CXXFLAGS  += -std=c++14
LDFLAGS   +=
LDLIBS    +=
ARFLAGS   += cs

##########################################################################################

ifeq ($(CXX), clang++)
   CPPFLAGS += -Wno-missing-braces
endif

# Default is release build so users can do a normal make.
DEBUG ?= 0
ifeq ($(DEBUG), 0)
   CPPFLAGS := -DNDEBUG $(CPPFLAGS)
   ifeq ($(CXX), g++)
      CXXFLAGS := -O3 -flto $(CXXFLAGS)
      LDFLAGS := -O3 -flto -fuse-linker-plugin $(LDFLAGS)
   else ifeq ($(CXX), clang++)
      # There are strange linker errors when I use any level of optimization with Clang...
      CXXFLAGS := $(CXXFLAGS)
      LDFLAGS := $(LDFLAGS)
   endif
   OBJDIR := build/release
else
   CPPFLAGS := -DDEBUG $(CPPFLAGS)
   ifeq ($(CXX), g++)
      CXXFLAGS := -g -Og $(CXXFLAGS)
      LDFLAGS := -g -Og $(LDFLAGS)
   else
      CXXFLAGS := -g $(CXXFLAGS)
      LDFLAGS := -g $(LDFLAGS)
   endif
   OBJDIR := build/debug
endif
# http://stackoverflow.com/q/1079832
# http://stackoverflow.com/q/792217

# Taken from "Managing Projects With GNU Make".
subdirectory = $(patsubst %/Module.mk,%, \
   $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))

# See section 7.2.3 'Variables for Specifying Commands' of the GNU Coding Standards.  The
# `sed` invocations are used to [suppress warnings][1] for code in wxWidgets headers.
# [1]: http://stackoverflow.com/q/1867065
all_cppflags := $(shell $(WXCONFIG) --cppflags | sed 's/-I/-isystem/g') $(CPPFLAGS)
all_cxxflags := $(shell $(WXCONFIG) --cxxflags | sed 's/-I/-isystem/g') $(CXXFLAGS) -c
# Remove options from `all_cxxflags` that are already in `all_cppflags`.
all_cxxflags := $(filter-out $(all_cppflags),$(all_cxxflags))
all_ldflags  := $(LDFLAGS)
all_ldlibs   := $(LDLIBS)
all_arflags  := $(ARFLAGS)

# Explicitly initialize as simple variables as recursive ones are the default.
sources   :=
libraries :=
programs  :=

# Set the default target.
all:

include $(shell find -name 'Module.mk')

prereq_files := $(addprefix $(OBJDIR)/,$(subst src/,,$(sources:.cpp=.d)))
objects      := $(addprefix $(OBJDIR)/,$(subst src/,,$(sources:.cpp=.o)))

# All whitespace-separated words in the working directory and its subdirectories that do
# match any of the pattern words $(prereq_files).  File names shall not contain the "%"
# character.
existent_prereqs := \
   $(filter $(prereq_files),$(shell find -regex '.*\.d$$' -printf '%P\n'))

# Was any goal (other than `clean`) specified on the command line?  None counts as `all`.
ifneq ($(filter-out clean,$(or $(MAKECMDGOALS),all)),)
   # Include existent makefiles of prerequisites.  After reading in all those files none
   # of them will have to be updated.  Non-existent prerequisite files will be build along
   # with their respective object files.
   include $(existent_prereqs)
endif

.PHONY: all

all: $(programs) $(libraries)

.PHONY: clean

clean:
	$(RM) $(prereq_files) $(objects) $(libraries) $(programs)

# Calling `sort` removes duplicates.
$(sort $(dir $(objects))):
	mkdir -p $@

all: $(OBJDIR)/icons $(OBJDIR)/CreatureTable.txt

$(OBJDIR)/icons: | $(OBJDIR)
	ln -s ../../icons $@

$(OBJDIR)/CreatureTable.txt: | $(OBJDIR)/task1
	ln -s ../../task1/CreatureTable.txt $@

# If the target is not an existent file, then Make will imagine it to have been updated
# whenever this rule is run -- and the rule should only be run when the target is not an
# existent file.
# This way it is ensured that an object file corresponding to any of the targets of this
# rule (a makefile of prereq_files) will be updated, if the target does not exist.  That
# file will be created along with the object file and included during the next invocation
# of make.
$(prereq_files):

.SECONDEXPANSION:

# A call $(subst foo,bar,text) replaces each occurrence of 'foo' by 'bar' and does not
# substitute 'foo' for 'bar' as I tend to misunderstand it recurringly.
$(objects): $$(subst .o,.d,$$@) | $$(dir $$@)
	$(CXX) -MMD $(all_cppflags) $(all_cxxflags) $(subst $(OBJDIR)/,,$(dir $@))src/$(notdir $(@:.o=.cpp)) -o $@

# vim: tw=90 ts=8 sts=-1 sw=3 noet
