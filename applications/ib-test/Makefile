ifndef USEOFEDVERBS
ifneq ($(wildcard $(DABCSYS)/lib/libDabcVerbs.*),)
USEOFEDVERBS    = true
endif
endif


ifdef AMDAPPSDKROOT
USEGPU = true
GPU_LIBPATH = $(AMDAPPSDKROOT)/lib/x86_64
GPU_INCPATH = $(AMDAPPSDKROOT)/include
GPU_LIB = -lOpenCL
endif


include $(DABCSYS)/config/Makefile.config

ifdef DABCMAINMAKE
IBTESTDIR = applications/ib-test/
else
IBTESTDIR = 
endif

IBTEST_LIBNAME   = $(LIB_PREFIX)DabcIbTest
IBTEST_LIB       = $(IBTESTDIR)$(IBTEST_LIBNAME).$(DllSuf)

IBTEST_S         = $(wildcard $(IBTESTDIR)*.$(SrcSuf))
IBTEST_O         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(ObjSuf), $(IBTEST_S))
IBTEST_D         = $(patsubst %.$(SrcSuf), $(BLD_DIR)/%.$(DepSuf), $(IBTEST_S))

TRACERT_DIR      = $(IBTESTDIR)tracert
TRACERT_S        = $(wildcard $(TRACERT_DIR)/*.c $(TRACERT_DIR)/*.h)
TRACERT_EXE      = $(IBTESTDIR)tracert/ibtracertm 

ALLDEPENDENC += $(IBTEST_D)


libs::  $(IBTEST_LIB)

ifeq ($(USEOFEDVERBS), true)

IBTEST_EXTRAOPT += -lDabcVerbs
IBTEST_LIBDEP = $(DABCVERBS_LIB)

ifneq ($(wildcard /usr/include/infiniband/umad.h),)
ifneq ($(wildcard /usr/include/infiniband/complib/cl_nodenamemap.h),)

libs:: $(TRACERT_EXE)


$(TRACERT_EXE) : $(TRACERT_S)
	cd $(TRACERT_DIR); $(MAKE)
ifdef DABCMAINMAKE
	cp -f $(TRACERT_EXE) $(DABCBINPATH)
endif

clean::
	cd $(TRACERT_DIR); $(MAKE) clean

endif
endif

endif


clean::
	@$(RM) $(IBTEST_LIB) $(IBTESTDIR)*.log

ifeq ($(USEOFEDVERBS),true)
$(IBTEST_O) : DEFINITIONS += WITH_VERBS
endif

ifeq ($(USEGPU),true)
IBTEST_EXTRAOPT += $(call lddiropt,$(GPU_LIBPATH)) $(GPU_LIB)
$(IBTEST_O) : DEFINITIONS += WITH_GPU
$(IBTEST_O) : INCLUDES += $(GPU_INCPATH)
endif

$(IBTEST_LIB):  $(IBTEST_O) $(IBTEST_LIBDEP)
	@$(MakeLib) $(IBTEST_LIBNAME) "$(IBTEST_O)" $(IBTESTDIR) "$(IBTEST_EXTRAOPT)"


include $(DABCSYS)/config/Makefile.rules
