## special makefile, cannot be copied out of DABC

DABC_BASEDIR     = base
DABC_BASEDIRI    = $(DABC_BASEDIR)/dabc
DABC_BASEDIRS    = $(DABC_BASEDIR)/src
DABC_BASEDIRTEST = $(DABC_BASEDIR)/test
DABC_BASEDIRRUN  = $(DABC_BASEDIR)/run

DABC_BASEEXE      = $(DABCBINPATH)/dabc_run
DABC_XMLEXE       = $(DABCBINPATH)/dabc_xml
DABC_BASESH       = $(DABCBINPATH)/run.sh

BASE_H            = $(wildcard $(DABC_BASEDIRI)/*.$(HedSuf))
BASE_S            = $(wildcard $(DABC_BASEDIRS)/*.$(SrcSuf))
BASE_O            = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BASE_S))
BASE_D            = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BASE_S))

BASERUN_S         = $(wildcard $(DABC_BASEDIRRUN)/dabc_run.$(SrcSuf))
BASERUN_O         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BASERUN_S))
BASERUN_D         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BASERUN_S))

DABC_XMLEXES      = $(wildcard $(DABC_BASEDIRRUN)/dabc_xml.$(SrcSuf))
DABC_XMLEXEO      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DABC_XMLEXES))
DABC_XMLEXED      = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DABC_XMLEXES))

BASERUN_SH        = $(DABC_BASEDIRRUN)/run.sh

######### used in main Makefile

ALLHDRS          +=  $(patsubst $(DABC_BASEDIR)/%.h, $(DABCINCPATH)/%.h, $(BASE_H))
ALLDEPENDENC     += $(BASE_D) $(BASERUN_D) $(DABC_XMLEXED)
APPLICATIONS_DIRS += $(DABC_BASEDIRTEST)

libs:: $(DABCBASE_LIB)

exes:: $(DABC_BASEEXE) $(DABC_XMLEXE) $(DABC_BASESH)

##### local rules #####

$(DABCINCPATH)/%.h: $(DABC_BASEDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCBASE_LIB):   $(BASE_O)
	@$(MakeLib) $(DABCBASE_LIBNAME) "$(BASE_O)" $(DABCDLLPATH) "-lpthread -ldl"

$(DABCBASE_SLIB): $(BASE_O)
	$(AR) $(ARFLAGS) $(DABCBASE_SLIB) $(BASE_O)

$(DABC_BASEEXE):  $(BASERUN_O) 
	$(LD) $(LDFLAGS) $(BASERUN_O) $(LIBS_CORESET) $(OutPutOpt) $(DABC_BASEEXE)

$(DABC_XMLEXE) : $(DABC_XMLEXEO) $(BASE_O) 
	$(LD) $(LDFLAGS) $(DABC_XMLEXEO) $(BASE_O) -pthread -ldl $(OutPutOpt) $(DABC_XMLEXE)

$(DABC_BASESH): $(BASERUN_SH)
	@cp -f $< $@
