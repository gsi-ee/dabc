## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
BNETDIR = plugins/bnet
else
BNETDIR = .
INCLUDES += $(BNETDIR)
endif

BNETDIRI          = $(BNETDIR)/bnet
BNETDIRS          = $(BNETDIR)/src

DABCBNET_LIBNAME  = $(LIB_PREFIX)DabcBnet
DABCBNET_LIB      = $(TGTDLLPATH)/$(DABCBNET_LIBNAME).$(DllSuf)

## must be similar for every module

BNET_H           = $(wildcard $(BNETDIRI)/*.$(HedSuf))
BNET_S           = $(wildcard $(BNETDIRS)/*.$(SrcSuf))
BNET_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BNET_S))
BNET_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BNET_S))

# used in the main Makefile

ALLHDRS          += $(patsubst $(BNETDIR)/%.h, $(DABCINCPATH)/%.h, $(BNET_H))
ALLDEPENDENC     += $(BNET_D)

libs:: $(DABCBNET_LIB)


##### local rules #####

$(DABCINCPATH)/%.h: $(BNETDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCBNET_LIB):   $(BNET_O)
	@$(MakeLib) $(DABCBNET_LIBNAME) "$(BNET_O)" $(TGTDLLPATH)

include $(DABCSYS)/config/Makefile.rules
