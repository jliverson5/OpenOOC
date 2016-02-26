# Modules in the project (you could 'find' these, but stating them explicitly
# allows for subdirectories like 'tmp' or 'doc' without upsetting the build
# process.
MODULES := apps src


# List of all non-source files/directories that are part of the distribution
AUXFILES := AUTHORS ChangeLog COPYING Makefile NEWS README.md
 

# Default target
default: all


#-------------------------------------------------------------------------------
# DEFAULT PROGRAMS
#-------------------------------------------------------------------------------
#{{{1
# Program variables
AR   := ar
RM   := rm -f
CC   := cc
LD   := cc
ECHO := echo
#}}}1


#-------------------------------------------------------------------------------
# DEFAULT FLAGS
#-------------------------------------------------------------------------------
#{{{1
ARFLAGS  := crsP
WARNING  := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
            -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
            -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
            -Wuninitialized -Wconversion -Wstrict-prototypes
OPTIMIZE := -O0 -g
CFLAGS   := $(OPTIMIZE) $(WARNING)
LDFLAGS  :=
#}}}1


#-------------------------------------------------------------------------------
# COMPILATION INFO
#-------------------------------------------------------------------------------
#{{{1
# Root directory where Makefile is located
ROOTDIR := $(dir $(lastword $(MAKEFILE_LIST)))
# Date of compilation
DATE    := $(shell date)
# Git commit of compilation
COMMIT  := $(shell cd $(ROOTDIR) && git rev-parse --short HEAD)
#}}}1


#-------------------------------------------------------------------------------
# TEMPLATES
#-------------------------------------------------------------------------------
#{{{1
DEPFILES :=


# Including a module's build.mk
define MK_template
include $(ROOTDIR)$(1)/build.mk
endef

 
# Setting a module's build rules for object files in <module>/obj.
define RULES_template
obj/$(1)/%.o: $(ROOTDIR)$(1)/%.c $(ROOTDIR)Makefile $(ROOTDIR)$(1)/build.mk
	@$(ECHO) "  CC       $(1)/$$(@F)"
	@(test -d obj/$(1) || mkdir -p obj/$(1)) && \
    (test -d dep/$(1) || mkdir -p dep/$(1)) && \
    $$(CC) $$(CFLAGS) -I include -MMD -MP -MF dep/$(1)/`basename $$(@F) .o`.d \
      -DDATE="$(DATE)" -DCOMMIT="$(COMMIT)" -c $$< -o $$@
endef

 
# Setting a module's build rules for executable targets.
# (Depending on its sources' object files and any libraries.)
# Also adds a module's dependency files to the global list.
define PROGRAM_template
DEPFILES += $(patsubst %,dep/$(2)/%.d,$(basename $($(1)_SOURCES)))

bin/$(1): $(patsubst %,obj/$(2)/%.o,$(basename $($(1)_SOURCES))) \
          $(foreach library,$($(1)_LDADD),lib/$(library)) \
          $(foreach library,$($(2)_LDADD),lib/$(library))
	@$(ECHO) "  LD       $$@"
	@(test -d bin || mkdir -p bin) && \
    $$(LD) $$($(1)_LDFLAGS) $$($(2)_LDFLAGS) $$(LDFLAGS) $$^ -o $$@
endef

 
# Setting a module's build rules for archive targets.
# (Depending on its sources' object files.)
define LIBRARY_template
DEPFILES += $(patsubst %,dep/$(2)/%.d,$(basename $($(1)_SOURCES)))

lib/$(1): $(patsubst %,obj/$(2)/%.o,$(basename $($(1)_SOURCES)))
	@$(ECHO) "  AR       $$@"
	@(test -d lib || mkdir -p lib) && $$(AR) $$(ARFLAGS) $$@ $$?
endef

 
# Linking a module's global includes into the global include directory
# (where they will be available as <module>/filename.h).
define INCLUDE_template
$$(shell (test -d include || mkdir -p include))
$$(shell (test -h include/$(1) || ln -s ../$(ROOTDIR)$(1)/include include/$(1)))
endef
#}}}1

 
#-------------------------------------------------------------------------------
# INSTANTIATE TEMPLATES
#-------------------------------------------------------------------------------
#{{{1
# Now, instantiating the templates for each module.
$(foreach mod, $(MODULES),$(eval $(call MK_template,$(mod))))
$(foreach mod, $(MODULES),$(eval $(call RULES_template,$(mod))))
$(foreach mod, $(MODULES),$(eval $(foreach bin,$($(mod)_PROGRAMS),\
  $(call PROGRAM_template,$(bin),$(mod)))))
$(foreach mod, $(MODULES),$(eval $(foreach lib,$($(mod)_LIBRARIES),\
  $(call LIBRARY_template,$(lib),$(mod)))))
$(foreach mod, $(MODULES),$(eval $(call INCLUDE_template,$(mod))))

 
# Include the dependency files (generated by GCC's -MMD option)
-include $(sort $(DEPFILES))
#}}}1


#-------------------------------------------------------------------------------
# SPECIAL TARGETS
#-------------------------------------------------------------------------------
#{{{1
# Make all modules programs and libraries
all: \
  $(foreach mod, $(MODULES), $(if $(strip $($(mod)_LIBRARIES)),\
    lib/$($(mod)_LIBRARIES))) \
  $(foreach mod, $(MODULES), $(if $(strip $($(mod)_PROGRAMS)),\
    bin/$($(mod)_PROGRAMS)))
 

# Remove object (*.o) and dependency (*.d) files
clean:
	$(RM) $(foreach mod,$(MODULES),obj/$(mod)/*.o) \
    $(foreach mod,$(MODULES),dep/$(mod)/*.d)


# Make clean and remove executables and libraries 
realclean: clean
	$(RM) bin/* include/* lib/*


# Aggregation of all files
SRCFILES := $(foreach mod, $(MODULES), $(foreach lib, $($(mod)_LIBRARIES), \
  $(addprefix $(mod)/,$($(lib)_SOURCES))))
HDRFILES := $(foreach mod, $(MODULES), $(foreach lib, $($(mod)_LIBRARIES), \
  $(addprefix $(mod)/,$($(lib)_HEADERS))))
SRCFILES += $(foreach mod, $(MODULES), $(foreach bin, $($(mod)_PROGRAMS), \
  $(addprefix $(mod)/,$($(bin)_SOURCES))))
HDRFILES += $(foreach mod, $(MODULES), $(foreach bin, $($(mod)_PROGRAMS), \
  $(addprefix $(mod)/,$($(bin)_HEADERS))))
BLDFILES := $(foreach mod, $(MODULES), $(mod)/build.mk)
ALLFILES := $(addprefix $(ROOTDIR), $(SRCFILES) $(HDRFILES) $(AUXFILES) $(BLDFILES))


# Print out any TODO or FIXME notations
todolist:
	-@for file in $(ALLFILES:$(ROOTDIR)Makefile=); do \
    fgrep -H -e TODO -e FIXME $$file; \
  done; \
  true
#}}}1


# vim: set foldmethod=marker:
