###############################################################
#
# Change History:
#
# $Log: common.mk,v $
# Revision 1.5  2008/04/07 16:09:05  adamczew
# *** empty log message ***
#
# Revision 1.1  2006/10/10 14:46:49  marcus
# Initial commit of the new pciDriver for kernel 2.6
#
# Revision 1.1  2006/08/20 13:21:54  marcus
# Initial commit
#
###############################################################

# Compiler and default flags
CXX ?= g++
CXXFLAGS ?= -O2
CC ?= gcc
CFLAGS ?= -O2


# Defaults for directories
ROOTDIR ?= $(shell pwd)

INCDIR ?= $(ROOTDIR)/include
BINDIR ?= $(ROOTDIR)/bin
LIBDIR ?= $(ROOTDIR)/lib
OBJDIR ?= $(ROOTDIR)/obj
DEPENDDIR ?= $(ROOTDIR)/depend

CXXFLAGS += $(addprefix -I ,$(INCDIR))
CFLAGS += $(addprefix -I ,$(INCDIR))

# Source files in this directory
SRC = $(wildcard *.cpp)
SRCC = $(wildcard *.c)

# Corresponding object files 
OBJ = $(addprefix $(OBJDIR)/,$(SRC:.cpp=.o))
OBJ += $(addprefix $(OBJDIR)/,$(SRCC:.c=.o))

# Corresponding dependency files
DEPEND = $(addprefix $(DEPENDDIR)/,$(SRC:.cpp=.d)) 
DEPEND += $(addprefix $(DEPENDDIR)/,$(SRCC:.c=.d)) 

# This makes Verbose easier. Just prefix $(Q) to any command
ifdef VERBOSE
	Q ?= 
else
	Q ?= @
endif

###############################################################
# Target definitions

# Create the directories if the do not exist
dirs:
	$(Q)mkdir -p $(BINDIR)
	$(Q)mkdir -p $(OBJDIR)
	$(Q)mkdir -p $(LIBDIR)
	$(Q)mkdir -p $(DEPENDDIR)

# Target for automatic dependency generation
depend: $(DEPEND) $(DEPENDC);

# This rule generates a dependency makefile for each source
$(DEPENDDIR)/%.d: %.cpp
	@echo -e "DEPEND \t$<"
	$(Q)$(CXX) $(addprefix -I ,$(INCDIR)) -MM -MF $@ \
		-MT $(OBJDIR)/$(<:.cpp=.o) -MT $@ $< 

$(DEPENDDIR)/%.d: %.c
	@echo -e "DEPEND \t$<"
	$(Q)$(CC) $(addprefix -I ,$(INCDIR)) -MM -MF $@ \
		-MT $(OBJDIR)/$(<:.c=.o) -MT $@ $< 

# This includes the automatically 
# generated dependency files
-include $(DEPEND)

# This rule is to compile each source. Dependencies are
# merged by make
$(OBJDIR)/%.o: %.cpp
	@echo -e "CC \t$<"
	$(Q)@$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.c
	@echo -e "CC \t$<"
	$(Q)@$(CC) $(CFLAGS) -c -o $@ $<

# Do nothing with headers
%.h: ;
        	
