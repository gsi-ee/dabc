## normally should be like this for every plugin
## one can extract complete plugin subdirectory and compile it independent from rest

include $(DABCSYS)/config/Makefile.config

#DOOPTIMIZATION = false

ifdef DABCMAINMAKE
DOGMADIR = plugins/dogma
DLDPRINT_EXE      = $(DABCBINPATH)/dldprint
else
DOGMADIR = .
INCLUDES += $(DOGMADIR)
DLDPRINT_EXE      = dldprint
endif


DOGMADIRI         = $(DOGMADIR)/dogma
DOGMADIRS         = $(DOGMADIR)/src

# now defined in the Makefile.config
#DABCDOGMA_LIBNAME = $(LIB_PREFIX)DabcDogma
#DABCDOGMA_LIB     = $(TGTDLLPATH)/$(DABCDOGMA_LIBNAME).$(DllSuf)

DOGMA_H           = $(wildcard $(DOGMADIRI)/*.$(HedSuf))
DOGMA_S           = $(wildcard $(DOGMADIRS)/*.$(SrcSuf))
DOGMA_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DOGMA_S))
DOGMA_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DOGMA_S))

DLDPRINT_EXES   = $(DOGMADIR)/dldprint.$(SrcSuf)
DLDPRINT_EXEO   = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DLDPRINT_EXES))
DLDPRINT_EXED   = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DLDPRINT_EXES))

# used in the main Makefile

ALLHDRS           += $(patsubst $(DOGMADIR)/%.h, $(DABCINCPATH)/%.h, $(DOGMA_H))
ALLDEPENDENC      += $(DOGMA_D) $(DLDPRINT_EXED)

libs:: $(DABCDOGMA_LIB)

exes:: $(DLDPRINT_EXE)

clean::
	@$(RM) $(DLDPRINT_EXE)


##### local rules #####

$(DABCINCPATH)/%.h: $(DOGMADIR)/%.h
	@echo "Header: $@"
	@cp -f $< $@

$(DABCDOGMA_LIB):  $(DOGMA_O) $(DABCBASE_LIB) $(DABCMBS_LIB)
	@$(MakeLib) $(DABCDOGMA_LIBNAME) "$(DOGMA_O)" $(TGTDLLPATH) "-lDabcBase -lDabcMbs"

$(DLDPRINT_EXE) : $(DLDPRINT_EXEO) $(DABCDOGMA_LIB)
	$(LD) $(LDFLAGSPRE) -O $(DLDPRINT_EXEO) $(LIBS_CORESET) -lDabcMbs -lDabcDogma -o $(DLDPRINT_EXE)

########### extra rules #############

include $(DABCSYS)/config/Makefile.rules
