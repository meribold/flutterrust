# Top-level makefile; includes all Module.mk files it can find in any of the
# subdirectories. Partly based on the nonrecursive make from "Managing Projects
# With GNU Make". Written for GNU Make and not expected to be portable.

# Parameters controlling inplicit rules (as listed in section 10.3 of the GNU
# Make Manual) or that the user can override with command options use upper case
# letters. Those serving internal purposes in this makefile use lower case
# letters. This practice is recommended in chapter 6 of the GNU Make Manual.

# "Every Makefile should contain this line..."
SHELL := /bin/sh # - section 7.2.1 of the GNU Coding Standards

# Clear the suffix list; no suffix rules in this makefile. See section 7.2.1 of
# the GNU Coding Standards.
.SUFFICES:

CXX      ?= g++
CPPFLAGS += -Wall -Wextra -pedantic -g -O
CXXFLAGS += -std=c++11 -Wold-style-cast
LDFLAGS  += -g -O
LDLIBS   +=
ARFLAGS  += cs

################################################################################

# Taken from "Managing Projects With GNU Make".
subdirectory = $(patsubst %/Module.mk,%, \
   $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))

# See section 7.2.3 'Varialbes for Specifying Commands' of the GNU Coding
# Standards.
all_cppflags := $(CPPFLAGS)
all_cxxflags := $(CXXFLAGS) -c
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

prereq_files := $(sources:.cpp=.d)
objects      := $(sources:.cpp=.o)

# All whitespace-separated words in the working directory and its subdirectories
# that do match any of the pattern words $(prereq_files). file names shall not
# contain the "%" character.
existant_prereqs := \
   $(filter $(prereq_files),$(shell find -regex '.*\.d$$' -printf '%P\n'))

# Was any goal (other than `clean`) specified on the command line? None counts
# as `all`.
ifneq ($(filter-out clean,$(or $(MAKECMDGOALS),all)),)
   # Include existant makefiles of prerequisites. After reading in all those
   # files none of them will have to be updated. Non-existant prerequisite files
   # will be build along with their respective object files.
   include $(existant_prereqs)
endif

.PHONY: all

all: $(programs) $(libraries)

.PHONY: clean

clean:
	$(RM) $(prereq_files) $(objects) $(libraries) $(programs)

# If the target is not an existant file, then Make will imagine it to have been
# updated whenever this rule is run - and the rule should only be run when the
# target is not an existant file.
# This way it is ensured that an object file corresponding to any of the targets
# of this rule (a makefile of prereq_files) will be updated, if the target does
# not exist. That file will be created along with the object file and included
# during the next invocation of make.
$(prereq_files):

.SECONDEXPANSION:

# A call $(subst foo,bar,text) replaces each occurence of 'foo' by 'bar' and
# does not substitute 'foo' for 'bar' as I tend to misunderstand it recurringly.
$(objects): $$(subst .o,.d,$$@)
	$(CXX) -MMD $(all_cppflags) $(all_cxxflags) $(subst .o,.cpp,$@) -o $@

# vim: tw=80 sw=3 noet
