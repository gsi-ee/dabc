include $(DABCSYS)/config/Makefile.config

ifdef DABC_ISROOT


ifdef DABCMAINMAKE
DABCROOTDIR = plugins/root
else
DABCROOTDIR = .
endif

DABCROOTDIRI      = $(DABCROOTDIR)/dabc_root
DABCROOTDIRS      = $(DABCROOTDIR)/src

DABCROOT_LINKDEF  = $(DABCROOTDIR)/LinkDef.h

DABCROOT_LIBNAME  = $(LIB_PREFIX)DabcRoot
DABCROOT_LIB      = $(TGTDLLPATH)/$(DABCROOT_LIBNAME).$(DllSuf)
DABCROOT_MAP      = $(TGTDLLPATH)/$(DABCROOT_LIBNAME).rootmap 

## must be similar for every module

DABCROOT_H        = $(wildcard $(DABCROOTDIRI)/*.$(HedSuf))
DABCROOT_S        = $(wildcard $(DABCROOTDIRS)/*.$(SrcSuf))
DABCROOT_O        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCROOT_S))
DABCROOT_D        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCROOT_S))

DABCROOT_DICT      = $(BLD_DIR)/$(DABCROOTDIR)/G__DABCROOT
DABCROOT_DH        = $(DABCROOT_DICT).$(HedSuf)
DABCROOT_DS        = $(DABCROOT_DICT).$(SrcSuf)
DABCROOT_DO        = $(DABCROOT_DICT).$(ObjSuf) 
DABCROOT_DD        = $(DABCROOT_DICT).$(DepSuf) 



# used in the main Makefile

#ALLHDRS          += $(patsubst $(DABCROOTDIR)/%.h, $(DABCINCPATH)/%.h, $(DABCROOT_H))
ALLDEPENDENC     += $(DABCROOT_D) $(DABCROOT_DD) 

libs:: $(DABCROOT_LIB) $(DABCROOT_MAP)

##### local rules #####

#$(DABCINCPATH)/%.h: $(DABCROOTDIR)/%.h
#	@echo "Header: $@" 
#	@cp -f $< $@

$(DABCROOT_LIB): LDFLAGS += $(DABC_ROOTLIBS)

$(DABCROOT_LIB):   $(DABCROOT_O) $(DABCROOT_DO)
	@$(MakeLib) $(DABCROOT_LIBNAME) "$(DABCROOT_O) $(DABCROOT_DO)" $(TGTDLLPATH)

$(DABCROOT_DS): $(DABCROOT_H)  $(DABCROOT_LINKDEF)
	$(ROOTCINTDABC)  $(DABCROOT_H) $(DABCROOT_LINKDEF)

$(DABCROOT_MAP) : $(DABCROOT_LIB) $(DABCROOT_LINKDEF) 
	$(DABC_ROOTBIN)/rlibmap -r $(DABCROOT_MAP) -l $(DABCROOT_LIB) -d $(TGTDLLPATH)/libDabcHttp.$(DllSuf) $(TGTDLLPATH)/libDabcBase.$(DllSuf) -c $(DABCROOT_LINKDEF)

########### extra roles #############
$(DABCROOT_O) $(DABCROOT_D): INCLUDES += $(DABC_ROOTINCDIR) $(DABCROOTDIR)

$(DABCROOT_DO) $(DABCROOT_DD): INCLUDES += $(DABC_ROOTINCDIR) $(DABCROOTDIR) .

endif

include $(DABCSYS)/config/Makefile.rules

