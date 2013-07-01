include $(DABCSYS)/config/Makefile.config

ifdef DABC_ISROOT


ifdef DABCMAINMAKE
DABCMBSROOTDIR = plugins/mbs-root
else
DABCMBSROOTDIR = .
endif

DABCMBSROOTDIRI         = $(DABCMBSROOTDIR)/mbs_root
DABCMBSROOTDIRS         = $(DABCMBSROOTDIR)/src

DABCMBSROOT_LINKDEF     = $(DABCMBSROOTDIRI)/mbs_rootLinkDef.h
DABCMBSROOT_LIBNAME     = $(LIB_PREFIX)DabcMbsRoot
DABCMBSROOT_LIB         = $(TGTDLLPATH)/$(DABCMBSROOT_LIBNAME).$(DllSuf)

## must be similar for every module

#DABCMBSROOT_H        = $(wildcard $(DABCMBSROOTDIRI)/*.$(HedSuf))
#DABCMBSROOT_S        = $(wildcard $(DABCMBSROOTDIRS)/*.$(SrcSuf))
DABCMBSROOT_H         = $(filter-out $(DABCMBSROOT_DH) $(DABCMBSROOT_LINKDEF), $(wildcard $(DABCMBSROOTDIRI)/*.$(HedSuf)))
DABCMBSROOT_S         = $(filter-out $(DABCMBSROOT_DS), $(wildcard $(DABCMBSROOTDIRS)/*.$(SrcSuf)))


DABCMBSROOT_O         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCMBSROOT_S))
DABCMBSROOT_D         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCMBSROOT_S))


DABCMBSROOT_DICT      = $(BLD_DIR)/$(DABCMBSROOTDIR)/G__MBSROOT
DABCMBSROOT_DH        = $(DABCMBSROOT_DICT).$(HedSuf)
DABCMBSROOT_DS        = $(DABCMBSROOT_DICT).$(SrcSuf)
DABCMBSROOT_DO        = $(DABCMBSROOT_DICT).$(ObjSuf)
DABCMBSROOT_DD        = $(DABCMBSROOT_DICT).$(DepSuf)

# used in the main Makefile

#ALLHDRS          += $(patsubst $(DABCMBSROOTDIR)/%.h, $(DABCINCPATH)/%.h, $(DABCMBSROOT_H))
ALLDEPENDENC     += $(DABCMBSROOT_D) 

libs:: $(DABCMBSROOT_LIB)

##### local rules #####

#$(DABCINCPATH)/%.h: $(DABCMBSROOTDIR)/%.h
#	@echo "Header: $@" 
#	@cp -f $< $@

$(DABCMBSROOT_LIB): LDFLAGS += $(DABC_ROOTLIBS)

$(DABCMBSROOT_LIB):   $(DABCMBSROOT_O) $(DABCMBSROOT_DO)
	@$(MakeLib) $(DABCMBSROOT_LIBNAME) "$(DABCMBSROOT_O) $(DABCMBSROOT_DO)" $(TGTDLLPATH)

$(DABCMBSROOT_DS): $(DABCMBSROOT_H)  $(DABCMBSROOT_LINKDEF)
	@$(ROOTCINTDABC)  $(DABCMBSROOT_H) $(DABCMBSROOT_LINKDEF)


########### extra roles #############
$(DABCMBSROOT_O) $(DABCMBSROOT_D): INCLUDES += $(DABC_ROOTINCDIR) $(DABCMBSROOTDIR)

$(DABCMBSROOT_DO) $(DABCMBSROOT_DD): INCLUDES += $(DABC_ROOTINCDIR) $(DABCMBSROOTDIR) .

endif

include $(DABCSYS)/config/Makefile.rules

