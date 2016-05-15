local_sources := $(shell find $(subdirectory) -maxdepth 1 -name '*.cpp')

sources += $(local_sources)

# vim: tw=90 ts=8 sts=-1 sw=3 noet
