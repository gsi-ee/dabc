ifdef DABC_WITH_BNET

include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
BNETMBS_DIR = plugins/bnet-mbs
else
BNETMBS_DIR = .
INCLUDES += .
endif

BNETMBS_LIBNAME     = $(LIB_PREFIX)BnetMbs
BNETMBS_LIB         = $(TGTDLLPATH)/$(BNETMBS_LIBNAME).$(DllSuf)

BNETMBS_H           = $(wildcard $(BNETMBS_DIR)/bnet/*.$(HedSuf))
BNETMBS_S           = $(wildcard $(BNETMBS_DIR)/src/*.$(SrcSuf))
BNETMBS_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BNETMBS_S))
BNETMBS_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BNETMBS_S))

ALLHDRS             += $(patsubst $(BNETMBS_DIR)/%.h, $(DABCINCPATH)/%.h, $(BNETMBS_H))
ALLDEPENDENC        += $(BNETMBS_D)

libs:: $(BNETMBS_LIB) 

$(DABCINCPATH)/%.h: $(BNETMBS_DIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(BNETMBS_LIB):   $(BNETMBS_O)
	@$(MakeLib) $(BNETMBS_LIBNAME) "$(BNETMBS_O)" $(TGTDLLPATH)

include $(DABCSYS)/config/Makefile.rules

endif