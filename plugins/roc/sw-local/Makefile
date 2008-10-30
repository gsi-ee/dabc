
ifneq ($(findstring $(MAKECMDGOALS), debug),)
RELDIR = Debug
CXXOPT = -g -Wall -fPIC -fmessage-length=0 -D__SC_DEBUG__
else
RELDIR = Release
CXXOPT = -O2 -fPIC
endif

ARCHDIR := $(shell uname -m)

ifeq ($(ARCHDIR),x86_64)
CXXOPT    += -m64
endif

TGTDIR = $(ARCHDIR)/$(RELDIR)

.PHONY: release debug all lib clean simul clean_exe

release: all

debug: all

# All Target
all: $(TGTDIR) clean_exe lib control setup upload simul

$(TGTDIR)/dummy.d:
#	@(if [ ! -f $(ARCHDIR) ] ; then mkdir -p $(ARCHDIR); fi)
#	@(if [ ! -f $(TGTDIR) ] ; then mkdir -p $(TGTDIR); fi)
	@mkdir -p $(TGTDIR)/CppSockets; mkdir -p $(TGTDIR)/Control; mkdir -p $(TGTDIR)/Simulator;
	@touch $(TGTDIR)/dummy.d

# Other Targets


SOCKET_SRCS := $(wildcard CppSockets/*.cpp)
SOCKET_OBJS := $(patsubst %.cpp, $(TGTDIR)/%.o, $(SOCKET_SRCS))
PROJECT_DEP += $(patsubst %.cpp, $(TGTDIR)/%.d, $(SOCKET_SRCS))

SYSCORE_SRCS := Control/SysCoreData.cpp \
                Control/SysCoreSorter.cpp \
                Control/SysCoreBoard.cpp \
                Control/SysCoreControl.cpp
SYSCORE_HDRS := $(SYSCORE_SRCS:.cpp=.h)
SYSCORE_OBJS := $(patsubst %.cpp, $(TGTDIR)/%.o, $(SYSCORE_SRCS))
PROJECT_DEP += $(patsubst %.cpp, $(TGTDIR)/%.d, $(SYSCORE_SRCS))

SIMULATOR_SRCS := $(wildcard Simulator/*.cpp)
SIMULATOR_OBJS := $(patsubst %.cpp, $(TGTDIR)/%.o, $(SIMULATOR_SRCS))
PROJECT_DEP += $(patsubst %.cpp, $(TGTDIR)/%.d, $(SIMULATOR_SRCS))

CONTROL_SRC := Control/SysCoreControlApp.cpp
CONTROL_OBJ := $(patsubst %.cpp, $(TGTDIR)/%.o, $(CONTROL_SRC))
PROJECT_DEP += $(patsubst %.cpp, $(TGTDIR)/%.d, $(CONTROL_SRC))

SETUP_SRC := Control/SysCoreSetupApp.cpp
SETUP_OBJ := $(patsubst %.cpp, $(TGTDIR)/%.o, $(SETUP_SRC))
PROJECT_DEP += $(patsubst %.cpp, $(TGTDIR)/%.d, $(SETUP_SRC))

UPLOAD_SRC := Control/SysCoreUploadApp.cpp
UPLOAD_OBJ := $(patsubst %.cpp, $(TGTDIR)/%.o, $(UPLOAD_SRC))
PROJECT_DEP += $(patsubst %.cpp, $(TGTDIR)/%.d, $(UPLOAD_SRC))


clean_exe:
	@rm -f control setup upload simul libKnut.so libKnut.rootmap

clean: clean_exe
	@rm -rf $(ARCHDIR)
	@echo "Clean done"

ifdef ROOTSYS

LIB_OBJS := $(TGTDIR)/G__SysCore.o

lib: libKnut.so libKnut.rootmap

libKnut.rootmap: libKnut.so
	@echo 'Building: $@'
	@$(ROOTSYS)/bin/rlibmap -o $@ -l ./libKnut.so -c Control/LinkDef.h

$(TGTDIR)/G__SysCore.cpp: Control/LinkDef.h $(SYSCORE_HDRS)
	$(ROOTSYS)/bin/rootcint -f $@ -c -p $(SYSCORE_HDRS) Control/LinkDef.h

$(TGTDIR)/G__SysCore.o: $(TGTDIR)/G__SysCore.cpp
	g++ -c $< $(CXXOPT) -I$(CURDIR) $(shell root-config --cflags) -o $@

else

LIB_OBJS := 

lib: libKnut.so

endif


libKnut.so: $(TGTDIR) $(SOCKET_OBJS) $(SYSCORE_OBJS) $(LIB_OBJS)
	@echo 'Building: $@'
	@g++  -o $@ -shared -O $(SOCKET_OBJS) $(SYSCORE_OBJS) $(LIB_OBJS)

control: $(TGTDIR) $(CONTROL_OBJ) $(SOCKET_OBJS) $(SYSCORE_OBJS)
	@echo 'Building: $@'
	@g++  -o $@ -lpthread $(CONTROL_OBJ) $(SOCKET_OBJS) $(SYSCORE_OBJS)

setup: $(TGTDIR) $(SETUP_OBJ) $(SOCKET_OBJS) $(SYSCORE_OBJS)
	@echo 'Building: $@'
	@g++  -o $@ -lpthread $(SETUP_OBJ) $(SOCKET_OBJS) $(SYSCORE_OBJS)    

upload: $(TGTDIR) $(UPLOAD_OBJ) $(SOCKET_OBJS) $(SYSCORE_OBJS)
	@echo 'Building: $@'
	@g++  -o $@ -lpthread $(UPLOAD_OBJ) $(SOCKET_OBJS) $(SYSCORE_OBJS)

simul: $(TGTDIR) $(SIMULATOR_OBJS) $(SOCKET_OBJS)
	@echo 'Building: $@'
	@g++  -o $@ -lpthread $(SIMULATOR_OBJS) $(SOCKET_OBJS)

# Each subdirectory must supply rules for building sources it contributes
$(TGTDIR)/%.o: %.cpp
	g++ -c $< $(CXXOPT) -o $@

$(TGTDIR)/%.d: %.cpp
	@echo "Build depenency for $< ..."
	@rm -f $@ $@.bak; touch $@
	@makedepend -f$@ -Y -o.o -w 30000 -- $(CXXOPT) -- $< > /dev/null 2>&1
	@sed -i 's|$*.o|$(TGTDIR)/$*.o $(TGTDIR)/$*.d|g' $@
	@rm -f $@.bak

ifeq ($(findstring $(MAKECMDGOALS), clean),)

-include $(PROJECT_DEP)

-include $(TGTDIR)/dummy.d

endif