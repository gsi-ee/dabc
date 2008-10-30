BNETDIR          = bnet
BNETDIRI         = $(BNETDIR)/inc
BNETDIRS         = $(BNETDIR)/src
BNETINCPATH      = include/bnet
BNETBINPATH      = $(BNETDIR)/bin
BNETTESTDIR      = $(BNETDIR)/test
BNETMBSDIR       = $(BNETDIR)/mbs

BNET_NOTLIBF     =

DABCBNET_LIBNAME  = $(LIB_PREFIX)DabcBnet
DABCBNET_LIB      = $(BNETBINPATH)/$(DABCBNET_LIBNAME).$(DllSuf)

DABCBNETCL_LIBNAME  = $(LIB_PREFIX)BnetCluster
DABCBNETCL_LIB      = $(BNETBINPATH)/$(DABCBNETCL_LIBNAME).$(DllSuf)

DABCBNET_EXE        = $(BNETBINPATH)/bnet_exe

## must be similar for every module

BNETCL_S         = $(BNETDIRS)/ClusterPluginInit.cxx
BNETCL_O         = $(BNETCL_S:.$(SrcSuf)=.$(ObjSuf))
BNETCL_DEP       = $(BNETCL_O:.$(ObjSuf)=.$(DepSuf))

BNETEXE_S         = $(BNETDIRS)/bnet_exe.cxx
BNETEXE_O         = $(BNETEXE_S:.$(SrcSuf)=.$(ObjSuf))
BNETEXE_DEP       = $(BNETEXE_O:.$(ObjSuf)=.$(DepSuf))

BNET_H           = $(wildcard $(BNETDIRI)/*.$(HedSuf))
BNET_S           = $(filter-out $(BNETCL_S) $(BNETEXE_S), $(wildcard $(BNETDIRS)/*.$(SrcSuf)))
BNET_O           = $(BNET_S:.$(SrcSuf)=.$(ObjSuf))
BNET_DEP         = $(BNET_O:.$(ObjSuf)=.$(DepSuf))

# used in the main Makefile

ALLHDRS +=  $(patsubst $(BNETDIRI)/%.h, $(BNETINCPATH)/%.h, $(BNET_H))

BUILDDABCLIBS += $(DABCBNET_LIB)

BUILDDABCEXES += $(DABCBNET_EXE)

ALLDEPENDENC       += $(BNET_DEP) $(BNETCL_DEP) $(BNETEXE_DEP)

DISTRFILES         += $(BNET_S) $(BNET_H) $(BNETCL_S) $(BNETEXE_S)
DISTRFILES         += $(BNETBINPATH)/*.sh $(BNETBINPATH)/*.txt 
DISTRFILES         += $(BNETTESTDIR)/*.h $(BNETTESTDIR)/*.cxx $(BNETTESTDIR)/Makefile
DISTRFILES         += $(BNETMBSDIR)/*.h $(BNETMBSDIR)/*.cxx $(BNETMBSDIR)/*.sh $(BNETMBSDIR)/Makefile

##### local rules #####

build-init::
	@(if [ ! -f $(BNETINCPATH) ] ; then mkdir -p $(BNETINCPATH); fi)

$(BNETINCPATH)/%.h: $(BNETDIRI)/%.h
	@cp -f $< $@

$(DABCBNET_LIB):   $(BNET_O)
	@$(MakeLib) $(DABCBNET_LIBNAME) "$(BNET_O)" $(BNETBINPATH)

$(DABCBNETCL_LIB) : $(BNETCL_O) $(DABCBNET_LIB)
	@$(MakeLib) $(DABCBNETCL_LIBNAME) "$(BNETCL_O)" $(BNETBINPATH) "-L$(BNETBINPATH) -lDabcBnet"

$(DABCBNET_EXE) : $(BNETEXE_O)
	$(LD) $(LDFLAGS) $(BNETEXE_O) -ldl $(LIBS_FULLSET) -L$(BNETBINPATH) -lDabcBnet $(OutPutOpt) $(DABCBNET_EXE)

all-bnet:  $(BUILDDABCLIBS) $(DABCBNET_LIB) $(DABCBNETCL_LIB) $(DABCBNET_EXE) bnet-test

clean-bnet: clean-bnet-test
	@$(CleanLib) $(DABCBNET_LIBNAME) $(BNETBINPATH)
	@$(CleanLib) $(DABCBNETCL_LIBNAME) $(BNETBINPATH)
	@rm -f $(DABCBNET_EXE) $(BNET_O) $(BNETCL_O) $(BNETEXE_O) $(BNET_DEP) $(BNETCL_DEP) $(BNETEXE_DEP)

all:: all-bnet

clean:: clean-bnet

bnet-test:
	@export DABCSYSCORE=$(CURDIR); cd $(BNETTESTDIR); make all
	@export DABCSYSCORE=$(CURDIR); cd $(BNETMBSDIR); make all

clean-bnet-test:
	@export DABCSYSCORE=$(CURDIR); cd $(BNETTESTDIR); make clean
	@export DABCSYSCORE=$(CURDIR); cd $(BNETMBSDIR); make clean
