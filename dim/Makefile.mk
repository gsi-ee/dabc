DIM_VERSION = dim_v18r0

DIM_ZIP = $(DIM_VERSION).zip

ifneq ($(wildcard $(DABCSYS)/dim/$(DIM_ZIP)),)

DIMDIR := $(DABCSYS)/dim/$(DIM_VERSION)

DIM_LIB = $(DABCDLLPATH)/libdim.so

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

$(DIMDIR):
	@echo "Extract dim zip file"
	cd $(DABCSYS)/dim; unzip $(DIM_ZIP)  

$(DIM_LIB): $(DIMDIR)
	@echo "JAVA_HOME=$(JAVA_HOME)"
	cd $(DIMDIR); export DIMDIR=$(DIMDIR); export OS=$(DIM_OS); export ODIR=$(DIM_ODIR); export JDK_INCLUDE=$(DIM_JDK_INCLUDE); make -j1 $(DIM_BUILD_ARGS)
ifneq ($(DIM_JDK_INCLUDE),)
	cp -f $(DIMDIR)/$(DIM_ODIR)/libdim.so $(DIM_LIB)
endif
	cp -f $(DIMDIR)/$(DIM_ODIR)/libjdim.so $(DABCDLLPATH)  
	cp -f $(DIMDIR)/$(DIM_ODIR)/dns $(DABCBINPATH)/dimDns  
	@echo "Dim library build"
#	cd $(DIMDIR)/jdim/classes; jar cf dim.jar dim/*.class
#	mv -f $(DIMDIR)/jdim/classes/dim.jar $(DABCDLLPATH)

libs:: $(DIM_LIB)

clean::
	cd $(DABCSYS)/dim; rm -rf $(DIM_VERSION)

else

libs::
	echo "Cannot build DIM - dim is not specified"

endif

