## special makefile, cannot be copied out of DABC

ifdef DABCMAINMAKE
BASEDIR = base
else
BASEDIR = .
endif

BASEDIRI         = $(BASEDIR)/dabc
BASEDIRS         = $(BASEDIR)/src
BASETESTDIR      = $(BASEDIR)/test

BASE_NOTLIBF     =

DABCBASE_LIBNAME  = $(LIB_PREFIX)DabcBase
DABCBASE_LIB      = $(DABCDLLPATH)/$(DABCBASE_LIBNAME).$(DllSuf)

BASE_H           = $(wildcard $(BASEDIRI)/*.$(HedSuf))
BASE_S           = $(wildcard $(BASEDIRS)/*.$(SrcSuf))
BASE_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(BASE_S))
BASE_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(BASE_S))

ALLHDRS         +=  $(patsubst $(BASEDIR)/%.h, $(DABCINCPATH)/%.h, $(BASE_H))
ALLDEPENDENC    += $(BASE_D)
TEST_DIRS       += $(BASETESTDIR)

##### local rules #####

$(DABCINCPATH)/%.h: $(BASEDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DABCBASE_LIB):   $(BASE_O)
	@$(MakeLib) $(DABCBASE_LIBNAME) "$(BASE_O)" $(DABCDLLPATH)

libs:: $(DABCBASE_LIB)

exes::

