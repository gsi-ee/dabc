## special makefile, cannot be copied out of DABC

ifdef DABCMAINMAKE
BASEDIR = base
else
BASEDIR = .
endif

BASEDIRI         = $(BASEDIR)/dabc
BASEDIRS         = $(BASEDIR)/src
BASETESTDIR      = $(BASEDIR)/test
BASERUNDIR       = $(BASEDIR)/run

BASE_NOTLIBF     =

DABCBASE_LIBNAME  = $(LIB_PREFIX)DabcBase
DABCBASE_LIB      = $(DABCDLLPATH)/$(DABCBASE_LIBNAME).$(DllSuf)
DABCBASE_EXE      = $(DABCBINPATH)/dabc_run

BASE_H            = $(wildcard $(BASEDIRI)/*.$(HedSuf))
BASE_S            = $(wildcard $(BASEDIRS)/*.$(SrcSuf))
BASE_O            = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BASE_S))
BASE_D            = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BASE_S))

BASERUN_S         = $(wildcard $(BASERUNDIR)/*.$(SrcSuf))
BASERUN_O         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BASERUN_S))
BASERUN_D         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BASERUN_S))

######### used in main Makefile

ALLHDRS          +=  $(patsubst $(BASEDIR)/%.h, $(DABCINCPATH)/%.h, $(BASE_H))
ALLDEPENDENC     += $(BASE_D) $(BASERUN_D)
APPLICATIONS_DIRS += $(BASETESTDIR)

libs:: $(DABCBASE_LIB)

exes:: $(DABCBASE_EXE)

##### local rules #####

$(DABCINCPATH)/%.h: $(BASEDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCBASE_LIB):   $(BASE_O)
	@$(MakeLib) $(DABCBASE_LIBNAME) "$(BASE_O)" $(DABCDLLPATH) "-lpthread -ldl"

$(DABCBASE_EXE):  $(BASERUN_O) $(DABCBASE_LIB)
	$(LD) $(LDFLAGS) $(BASERUN_O) $(LIBS_CORESET) $(OutPutOpt) $(DABCBASE_EXE)
