###############################################################
#
# Change History:
#
# $Log: Makefile,v $
# Revision 1.4  2008/04/07 16:09:05  adamczew
# *** empty log message ***
#
# Revision 1.1  2007-02-12 18:09:13  marcus
# Initial commit.
#
###############################################################

# Makefile for the mprace library utilities

include ../../common.mk

INCDIR += ../../include
LDINC += $(addprefix -L ,$(LIBDIR))
LDFLAGS += -lpcidriver -lmprace

BINARIES = setclock fpgaconfig adcread

###############################################################
# Target definitions

.PHONY: all dirs depend clean

all: dirs depend $(BINARIES)

# Relate all exec names to it exec in the bin dir
$(BINARIES) : % : $(BINDIR)/% ;

# Target for each exec from the object file
$(BINDIR)/%: $(OBJDIR)/%.o
	@echo -e "LD \t$@"
	$(Q)$(CXX) $(LDINC) $(LDFLAGS) $(CXXFLAGS) -o $@ $<

#$(BINDIR)/testPciDriver: $(OBJDIR)/testPciDriver.o
#	@echo -e "LD \t$@"
#	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -o $@ $<
	    	
clean:
	@echo -e "CLEAN \t$(shell pwd)"
	-$(Q)rm -f $(addprefix $(BINDIR)/,$(BINARIES))
	-$(Q)rm -f $(OBJ)
	-$(Q)rm -f $(DEPEND)
