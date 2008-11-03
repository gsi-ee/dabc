ifdef ROOTSYS

include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
DABCROOTDIR = plugins/root
else
DABCROOTDIR = .
INCLUDES += $(DABCROOTDIR)
endif

DABCROOTDIRI         = $(DABCROOTDIR)/dabc_root
DABCROOTDIRS         = $(DABCROOTDIR)/src

DABCROOT_LIBNAME  = $(LIB_PREFIX)DabcRoot
DABCROOT_LIB      = $(TGTDLLPATH)/$(DABCROOT_LIBNAME).$(DllSuf)

## must be similar for every module

DABCROOT_H        = $(wildcard $(DABCROOTDIRI)/*.$(HedSuf))
DABCROOT_S        = $(wildcard $(DABCROOTDIRS)/*.$(SrcSuf))
DABCROOT_O        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABCROOT_S))
DABCROOT_D        = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABCROOT_S))

# used in the main Makefile

ALLHDRS          += $(patsubst $(DABCROOTDIR)/%.h, $(DABCINCPATH)/%.h, $(DABCROOT_H))
ALLDEPENDENC     += $(DABCROOT_D) 
CREATE_DIRS      += $(TGTDLLPATH)
LIBS_PLUGINS     += $(DABCROOT_LIB)

libs:: $(DABCROOT_LIB)

##### local rules #####

$(DABCINCPATH)/%.h: $(DABCROOTDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCROOT_LIB):   $(DABCROOT_O)
	@$(MakeLib) $(DABCROOT_LIBNAME) "$(DABCROOT_O)" $(TGTDLLPATH) "$(shell $(ROOTSYS)/bin/root-config --libs)"


########### extra roles #############
$(DABCROOT_O) $(DABCROOT_D): INCLUDES += $(ROOTSYS)/include

include $(DABCSYS)/config/Makefile.rules

endif
