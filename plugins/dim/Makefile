include $(DABCSYS)/config/Makefile.config

DIM_VERSION = dim_v19r20

DIM_ZIP = $(DIM_VERSION).zip

ifdef DABCMAINMAKE
DABCDIMDIR = $(CURDIR)/plugins/dim
DIMCTLDIR = plugins/dim
else
DABCDIMDIR = $(CURDIR)
DIMCTLDIR = .
endif

ifneq ($(wildcard $(DABCDIMDIR)/$(DIM_ZIP)),)

################### building of DIM itself ##################


DIMDIR = $(DABCDIMDIR)/$(DIM_VERSION)

DIM_LIB = $(TGTDLLPATH)/libdim.so

CREATE_DIRS += $(TGTBINPATH)

DIM_JDK_INCLUDE = $(JDK_INCLUDE)

ifeq ($(DIM_JDK_INCLUDE),)
ifneq ($(JAVA_HOME),)
DIM_JDK_INCLUDE = $(JAVA_HOME)/include
endif
endif

DIM_BUILD_ARGS=

ifdef DIM_JDK_INCLUDE
ifneq ($(wildcard $(DIM_JDK_INCLUDE)),)
DIM_BUILD_ARGS = JDIM=yes
JDK_INCLUDE:=$(DIM_JDK_INCLUDE)
else 
DIM_JDK_INCLUDE=
endif
endif

ifeq ($(wildcard /usr/include/Mrm/MrmAppl.h)$(wildcard /usr/local/include/Mrm/MrmAppl.h),)
DIM_BUILD_ARGS+= GUI=no
endif

DIM_ODIR = linux
DIM_OS = Linux

$(DIMDIR): $(DABCDIMDIR)/$(DIM_ZIP)
	@echo "Extract dim zip file"
	cd $(DABCDIMDIR); rm -rf $(DIM_VERSION); unzip $(DIM_ZIP)

$(DIM_LIB): $(DIMDIR) $(DABCDIMDIR)/Makefile
	@echo "JAVA_HOME=$(JAVA_HOME)"
	cd $(DIMDIR); export DIMDIR=$(DIMDIR); export OS=$(DIM_OS); export ODIR=$(DIM_ODIR); export JDK_INCLUDE=$(DIM_JDK_INCLUDE); make -j1 $(DIM_BUILD_ARGS)
	cp -f $(DIMDIR)/$(DIM_ODIR)/libdim.so $(DIM_LIB)
ifneq ($(DIM_JDK_INCLUDE),)
	cd $(DIMDIR)/jdim; make
	cp -f $(DIMDIR)/$(DIM_ODIR)/libjdim.so $(TGTDLLPATH)
	cp -f $(DIMDIR)/jdim/classes/dim.jar $(TGTDLLPATH)
endif
	cp -f $(DIMDIR)/$(DIM_ODIR)/dns $(TGTBINPATH)/dimDns
	if [ -f $(DIMDIR)/$(DIM_ODIR)/did ] ; then cp -f $(DIMDIR)/$(DIM_ODIR)/did $(TGTBINPATH)/dimDid; fi
	@echo "Dim library build"

libs:: $(DIM_LIB)

clean::
	cd $(DABCDIMDIR); rm -rf $(DIM_VERSION)


################## building of DABC plugin ###################

DIMCTLDIRI         = $(DIMCTLDIR)/dimc
DIMCTLDIRS         = $(DIMCTLDIR)/src

DIMCTL_LIBNAME     = $(LIB_PREFIX)DabcDim
DIMCTL_LIB         = $(TGTDLLPATH)/$(DIMCTL_LIBNAME).$(DllSuf)

DIMCTL_EXCLUDE      = 

DIMCTL_H           = $(filter-out $(DIMCTL_EXCLUDE), $(wildcard $(DIMCTLDIRI)/*.$(HedSuf)))
DIMCTL_S           = $(filter-out $(DIMCTL_EXCLUDE), $(wildcard $(DIMCTLDIRS)/*.$(SrcSuf)))
DIMCTL_O           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(DIMCTL_S))
DIMCTL_D           = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(DIMCTL_S))

ALLHDRS           += $(patsubst $(DIMCTLDIR)/%.h, $(DABCINCPATH)/%.h, $(DIMCTL_H))
ALLDEPENDENC      += $(DIMCTL_D)

libs:: $(DIMCTL_LIB) 


$(DABCINCPATH)/%.h: $(DIMCTLDIR)/%.h
	@echo "Header: $@" 
	@cp -f $< $@

$(DIMCTL_O) : $(DIM_LIB)

$(DIMCTL_LIB):  $(DIMCTL_O) 
	@$(MakeLib) $(DIMCTL_LIBNAME) "$(DIMCTL_O)" $(TGTDLLPATH) "-L$(DABCSYS)/lib -ldim"

$(DIMCTL_O) $(DIMCTL_D): INCLUDES += $(DIMCTLDIR)/$(DIM_VERSION)/dim


########################## building of JAVA gui ##########################

ifneq ($(DIM_JDK_INCLUDE),)
ifneq ($(shell which javac 2>/dev/null),)

DIMGUI_JAR = $(TGTDLLPATH)/xgui.jar

DIMGUI_SRCS = $(wildcard $(DIMCTLDIR)/gui/src/*)

$(DIMGUI_JAR): $(DIM_LIB) $(DIMCTL_LIB) $(DIMGUI_SRCS) $(DIMCTLDIR)/gui/Makefile
	@echo "Compiling java gui..."
	cd $(DIMCTLDIR)/gui; make DIMCLASSES=$(DIMDIR)/jdim/classes/dim.jar
	cp -f $(DIMCTLDIR)/gui/jar/xgui.jar $(TGTDLLPATH)
#	cd $(DIMCTLDIR)/gui/application; export DIMDIR=$(DIMDIR); make

exes:: $(DIMGUI_JAR)

docs::
	@echo "Compiling java gui docs..."
	cd $(DIMCTLDIR)/gui; export DIMDIR=$(DIMDIR); make doc 

endif
endif


clean::
	@echo "Cleaning java gui..."
	@cd $(DIMCTLDIR)/gui; $(MAKE) clean

else

libs::
	echo "Cannot build DIM - dim is not specified"

endif

include $(DABCSYS)/config/Makefile.rules
