## special makefile, cannot be copied out of DABC

DABC_BASEDIR     = base
DABC_BASEDIRI    = $(DABC_BASEDIR)/dabc
DABC_BASEDIRS    = $(DABC_BASEDIR)/src
DABC_BASEDIRTEST = $(DABC_BASEDIR)/test
DABC_BASEDIRRUN  = $(DABC_BASEDIR)/run

DABCBASE_EXE      = $(DABCBINPATH)/dabc_run

BASE_H            = $(wildcard $(DABC_BASEDIRI)/*.$(HedSuf))
BASE_S            = $(wildcard $(DABC_BASEDIRS)/*.$(SrcSuf))
BASE_O            = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BASE_S))
BASE_D            = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BASE_S))

BASERUN_S         = $(wildcard $(DABC_BASEDIRRUN)/*.$(SrcSuf))
BASERUN_O         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BASERUN_S))
BASERUN_D         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BASERUN_S))

BASERUN_LIBS      = $(LIBS_CORESET)
ifdef DABCSCTRL_LIB
BASERUN_LIBS     += -lDabcSctrl
endif

######### used in main Makefile

ALLHDRS          +=  $(patsubst $(DABC_BASEDIR)/%.h, $(DABCINCPATH)/%.h, $(BASE_H))
ALLDEPENDENC     += $(BASE_D) $(BASERUN_D)
APPLICATIONS_DIRS += $(DABC_BASEDIRTEST)

libs:: $(DABCBASE_LIB) $(DABCBASE_SLIB)

exes:: $(DABCBASE_EXE)

##### local rules #####

$(DABCINCPATH)/%.h: $(DABC_BASEDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCBASE_LIB):   $(BASE_O)
	@$(MakeLib) $(DABCBASE_LIBNAME) "$(BASE_O)" $(DABCDLLPATH) "-lpthread -ldl"

$(DABCBASE_SLIB): $(BASE_O)
	$(AR) $(ARFLAGS) $(DABCBASE_SLIB) $(BASE_O)

$(DABCBASE_EXE):  $(BASERUN_O) 
	$(LD) $(LDFLAGS) $(BASERUN_O) $(BASERUN_LIBS) $(OutPutOpt) $(DABCBASE_EXE)

ifdef DABCSCTRL_LIB
$(BASERUN_O): DEFINITIONS += __USE_STANDALONE__
endif	
