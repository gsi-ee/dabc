###############################################################
#
# Change History:
#
# $Log: Makefile,v $
# Revision 1.4  2008/04/07 16:08:52  adamczew
# *** empty log message ***
#
# Revision 1.3  2007-07-06 19:20:38  marcus
# New Register map for the ABB.
# User Memory Support with Scatter/Gather Lists.
#
# Revision 1.2  2007-07-06 11:22:25  marcus
# Backup commit. Stable version with old register map. Kernel memory only.
#
# Revision 1.1  2007/02/12 18:09:19  marcus
# Initial commit.
#
###############################################################

# Main Makefile for the new pciDriver

FLAGS :=

# Use this in the cluster
FLAGS := -march=nocona -m64

# For opterons
#FLAGS := -march=opteron -m64

# OLD_REGISTERS - Use old Register Mapping. For historical reasons only!
#FLAGS += -DOLD_REGISTERS

# These are switches for the library
# PCIDRIVER_OLD  - Use the old kernel 2.4 PCI Driver
# PCIDRIVER      - Use the new kernel 2.6 PCI Driver
# FLAGS += -DPCIDRIVER_OLD
FLAGS += -DPCIDRIVER

# These are only for the test programs
# <none>

#export VERBOSE := 1

export CXX := g++
export CC := gcc
export CXXFLAGS += -O2 -g $(FLAGS)
export CFLAGS += -O2 -g $(FLAGS)
export LDFLAGS += -lrt

ROOTDIR := $(shell pwd)

export INCDIR := $(ROOTDIR)/include
export SRCDIR := $(ROOTDIR)/src
export BINDIR := $(ROOTDIR)/bin
export LIBDIR := $(ROOTDIR)/lib
export OBJDIR := $(ROOTDIR)/obj
export DEPENDDIR := $(ROOTDIR)/depend

ifdef VERBOSE
	Q ?= 
	MKOPTS := 
else
    Q ?= @
	MKOPTS := --no-print-directory
endif

###############################################################
# Target definitions

.PHONY: all dirs lib tests install uninstall clean doc

all: dirs lib tests

dirs:
	$(Q)mkdir -p $(BINDIR)
	$(Q)mkdir -p $(OBJDIR)
	$(Q)mkdir -p $(LIBDIR)
	$(Q)mkdir -p $(DEPENDDIR)

doc:
	$(Q)doxygen mprace.doxygen
	
lib:
	$(Q)$(MAKE) $(MKOPTS) -C $(SRCDIR)/lib all

tests:
	$(Q)$(MAKE) $(MKOPTS) -C $(SRCDIR)/tests all
	
install:
	$(Q)$(MAKE) $(MKOPTS) -C $(SRCDIR)/lib install

uninstall: 
	$(Q)$(MAKE) $(MKOPTS) -C $(SRCDIR)/lib uninstall
        	
clean:
	@echo "Cleaning..."
	-$(Q)$(MAKE) $(MKOPTS) -C $(SRCDIR)/lib clean
	-$(Q)$(MAKE) $(MKOPTS) -C $(SRCDIR)/tests clean
	