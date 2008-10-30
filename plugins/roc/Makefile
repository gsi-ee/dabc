ifndef DABCSYSCORE
$(error DABCSYSCORE is not specified, project cannot be compiled)
endif


include $(DABCSYSCORE)/config/Makefile.config

## normally should be like this for every module, but can be specific

ROCDIR          = .
ROCDIRI         = $(ROCDIR)/roc
ROCDIRS         = $(ROCDIR)/src

ROCKNUTDIR      = $(ROCDIR)/sw-local
ROCCTRLDIR      = $(ROCKNUTDIR)/Control
#ROCSIMDIR       = $(ROCKNUTDIR)/Simulator

ROCPLUGINDIR    = $(ROCDIR)/dataserver

ROC_NOTLIBF     =


KNUT_LIB        = $(ROCDIR)/libKnut.so

ROC_DEFINITIONS =
ROC_INCLUDES    = $(ROCCTRLDIR) $(ROCDIR)
#ROC_EXTRALIBS   = -L. -lKnut
ROC_EXTRALIBS   =

ifdef USEROOT
ROC_DEFINITIONS  += DABC_ROOT
ROC_INCLUDES     += $(ROOT_INCLUDE)
ROC_EXTRALIBS    += $(ROOT_LIBS)
endif

ROC_LIBNAME     = $(LIB_PREFIX)DabcKnut
ROC_LIB         = $(ROCDIR)/$(ROC_LIBNAME).$(DllSuf)

## must be similar for every module

ROC_H           = $(wildcard $(ROCDIRI)/*.$(HedSuf))
ROC_S           = $(wildcard $(ROCDIRS)/*.$(SrcSuf))
ROC_O           = $(ROC_S:.$(SrcSuf)=.$(ObjSuf))
ROC_DEP         = $(ROC_O:.$(ObjSuf)=.$(DepSuf))


PLUGIN_EXENAME     = $(ROCPLUGINDIR)/rocdataserver
PLUGIN_NOTLIBF     = 
PLUGIN_LIBNAME     = $(LIB_PREFIX)RocDataserver
PLUGIN_LIB         = $(ROCPLUGINDIR)/$(PLUGIN_LIBNAME).$(DllSuf)

PLUGIN_EXEO        = $(PLUGIN_EXENAME).$(ObjSuf)
PLUGIN_EXES        = $(PLUGIN_EXENAME).$(SrcSuf)
PLUGIN_EXE         = $(PLUGIN_EXENAME)$(ExeSuf)   

PLUGIN_H           = $(filter-out $(PLUGIN_EXEH) $(PLUGIN_NOTLIBF), $(wildcard $(ROCPLUGINDIR)/*.$(HedSuf)))
PLUGIN_S           = $(filter-out $(PLUGIN_EXES) $(PLUGIN_NOTLIBF), $(wildcard $(ROCPLUGINDIR)/*.$(SrcSuf)))
PLUGIN_O           = $(PLUGIN_S:.$(SrcSuf)=.$(ObjSuf))

PLUGIN_DEP         =  $(PLUGIN_O:.$(ObjSuf)=.$(DepSuf))
PLUGIN_EDEP        =  $(PLUGIN_EXEO:.$(ObjSuf)=.$(DepSuf))


# used in the main Makefile

ALLDEPENDENC       += $(ROC_DEP) $(PLUGIN_DEP) $(PLUGIN_EDEP)

#DISTRFILES         += $(ROC_S) $(ROC_H) $(ROC_NOTLIBF) 
#DISTRFILES         += $(ROCSOCKETS_H) $(ROCSOCKETS_S)
#DISTRFILES         += $(ROCCTRL_H) $(ROCCTRL_S)
#DISTRFILES         += $(ROCSIM_H) $(ROCSIM_S)
#DISTRFILES         += $(ROCTESTDIR)/*.cxx $(ROCTESTDIR)/Makefile
#DISTRFILES         += $(ROCPLUGINDIR)/*.h $(ROCPLUGINDIR)/*.cxx $(ROCPLUGINDIR)/*.xml $(ROCPLUGINDIR)/Makefile
#DISTRFILES         += $(ROCMONDIR)/*.h $(ROCMONDIR)/*.cxx $(ROCMONDIR)/*.sh $(ROCMONDIR)/Module.mk $(ROCMONDIR)/Makefile




##### local rules #####

all: knutlib $(ROC_LIB) $(PLUGIN_LIB) $(PLUGIN_EXE)

clean:
	@rm -f $(ROC_O) $(ROC_DEP)
	@$(CleanLib) $(ROC_LIBNAME) $(ROCDIR)
	@rm -f $(PLUGIN_O) $(PLUGIN_EXEO) $(PLUGIN_DEP) $(PLUGIN_EDEP)
	@$(CleanLib) $(PLUGIN_LIBNAME) $(ROCPLUGINDIR)
	@rm -f $(ROCPLUGINDIR).$(DllSuf)
	cd $(ROCKNUTDIR); make clean

distclean: clean
	cd $(ROCKNUTDIR); make clean

package: distclean
	cd ..; tar cf roc.tar roc/Makefile roc/roc/*.h roc/src/*.cxx
	cd ..; tar rf roc.tar roc/dataserver/*.cxx roc/dataserver/*.h roc/dataserver/*.xml
	cd ..; tar rf roc.tar roc/go4monitor/*.cxx roc/go4monitor/*.h roc/go4monitor/Makefile roc/go4monitor/*.sh --exclude=roc/go4monitor/G__*.*
	cd ..; tar rf roc.tar roc/go4mon/*.cxx roc/go4mon/*.h roc/go4mon/Makefile roc/go4mon/Module.mk roc/go4mon/*.sh --exclude=roc/go4mon/G__*.*
	cd ..; tar rf roc.tar roc/sw-local --exclude=i686 --exclude=*.o --exclude=*.proj --exclude=.svn
	cd ..; tar rf roc.tar roc/sw-host --exclude=*.bak --exclude=*.proj --exclude=.svn
	cd ..; rm -f roc.tar.gz; gzip roc.tar
	@echo "roc.tar.gz is build"


$(ROC_LIB):  $(ROC_O) $(KNUT_LIB)
	@$(MakeLib) $(ROC_LIBNAME) "$(ROC_O)" $(ROCDIR) "$(ROC_EXTRALIBS)"

knutlib:
	cd $(ROCKNUTDIR); rm -f libKnut.so; export ROOTSYS=; make lib
	cp $(ROCKNUTDIR)/libKnut.so $(KNUT_LIB)
	@echo "Build of knut library done"

$(PLUGIN_LIB): $(PLUGIN_O)
	@$(MakeLib) $(PLUGIN_LIBNAME) "$(PLUGIN_O)" $(ROCPLUGINDIR)

$(PLUGIN_EXE):  $(PLUGIN_EXEO) $(PLUGIN_LIB) $(ROC_LIB)
	@cp -f $(ROC_LIB) $(KNUT_LIB) $(ROCPLUGINDIR)
	$(LD) $(LDFLAGS) $(PLUGIN_EXEO) $(LIBS_CORESET) -lDabcMbs -L$(ROCPLUGINDIR) -lKnut -lDabcKnut -lRocDataserver $(OutPutOpt) $(PLUGIN_EXE)


########### extra rules #############
#$(ROC_O) $(ROC_DEP) : OPTIONS += -g
$(ROC_O) $(ROC_DEP) $(PLUGIN_O) $(PLUGIN_EXEO) : DEFINITIONS += $(ROC_DEFINITIONS:%=-D%)
$(ROC_O) $(ROC_DEP) $(PLUGIN_O) $(PLUGIN_EXEO) : INCLUDES += $(ROC_INCLUDES:%=-I%) 

include $(DABCSYSCORE)/config/Makefile.rules
