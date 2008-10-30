###############################################################
#
# Change History:
#
# $Log: Makefile,v $
# Revision 1.4  2008/04/07 16:09:04  adamczew
# *** empty log message ***
#
# Revision 1.4  2008-01-11 10:31:56  marcus
# Added testABBlong.
#
# Revision 1.3  2007-10-31 15:46:11  marcus
# Added IG and KERNEL_PIECES tests.
#
# Revision 1.2  2007-05-29 07:50:43  marcus
# Backup commit.
#
# Revision 1.1  2007/02/12 18:09:13  marcus
# Initial commit.
#
###############################################################

# Makefile for the mprace library test programs

include ../../common.mk

INCDIR += ../../include
LDINC += $(addprefix -L ,$(LIBDIR))
LDFLAGS += -lpcidriver -lmprace

BINARIES = testABB testABBlong testig testSGDMA
#testABBconfig

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

clean:
	@echo -e "CLEAN \t$(shell pwd)"
	-$(Q)rm -f $(addprefix $(BINDIR)/,$(BINARIES))
	-$(Q)rm -f $(OBJ)
	-$(Q)rm -f $(DEPEND)
