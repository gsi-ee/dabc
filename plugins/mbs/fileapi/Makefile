TEST_S          = test.cxx
TEST_O          = test.o
TEST_EXE        = test

MBSFAPI_DICT    = G__LmdFile
MBSFAPI_DH      = $(MBSFAPI_DICT).h
MBSFAPI_DS      = $(MBSFAPI_DICT).cxx
MBSFAPI_DO      = $(MBSFAPI_DICT).o
MBSFAPI_LINKDEF = LinkDef.h

MBSFAPI_CS      = $(wildcard *.c)
MBSFAPI_S       = $(filter-out $(TEST_S) $(MBSFAPI_DS), $(wildcard *.cxx))
MBSFAPI_H       = $(MBSFAPI_S:.cxx=.h)
MBSFAPI_O       = $(MBSFAPI_CS:.c=.o) $(MBSFAPI_S:.cxx=.o)

MBSFAPI_LIB     = libLmdFile.so
MBSFAPI_RMAP    = libLmdFile.rootmap

CXX = g++
CC = gcc
LD = g++

LDFLAGS       =
WARNINGS      = -Wall
OPTIONS       = -fPIC

ifeq ($(shell uname -m),x86_64)
OPTIONS      += -m64
endif

# uncomment first line for debug symbols
#OPTIONS      += -g 
OPTIONS      += -O

DEFINITIONS   = Linux FILEONLY _LARGEFILE64_SOURCE

INCLUDES      =


ifdef ROOTSYS
MBSFAPI_O += $(MBSFAPI_DO)
INCLUDES  += $(ROOTSYS)/include
LDFLAGS   += $(shell root-config --libs)
endif


#### Combine compilers flags in one variable: ###
CXXFLAGS      =  $(WARNINGS) $(OPTIONS) $(DEFINITIONS:%=-D%) $(INCLUDES:%=-I%)

CCFLAGS        = $(WARNINGS) $(OPTIONS) $(DEFINITIONS:%=-D%) $(INCLUDES:%=-I%)


all: lib exe map

lib: $(MBSFAPI_LIB)

exe: $(TEST_EXE)

ifdef ROOTSYS
map: $(MBSFAPI_RMAP)
else
map:
	echo "No root map"
endif    

clean:
	rm -f $(TEST_O) $(TEST_EXE) $(MBSFAPI_O) $(MBSFAPI_LIB) $(MBSFAPI_RMAP) $(MBSFAPI_DICT).*

$(MBSFAPI_LIB): $(MBSFAPI_O)
	$(LD) -shared -O $(MBSFAPI_O) -lrt -o $(MBSFAPI_LIB)

$(MBSFAPI_DS): $(MBSFAPI_H)  $(MBSFAPI_LINKDEF)
	echo "Generating root dictionary $(MBSFAPI_DS) ..."
	$(ROOTSYS)/bin/rootcint -f $@ -c -p $(DEFINITIONS:%=-D%) $(INCLUDES:%=-I%) $(MBSFAPI_H) $(MBSFAPI_LINKDEF)

$(MBSFAPI_RMAP): $(MBSFAPI_LIB)
	echo "Generating root map $@ ..."
	$(ROOTSYS)/bin/rlibmap -o $@ -l ./$(MBSFAPI_LIB) -c $(MBSFAPI_LINKDEF)

$(TEST_EXE): $(TEST_O)
	$(LD) -O $(TEST_O) $(LDFLAGS) -L. -lLmdFile -o $(TEST_EXE)

%.o: %.cxx
	$(CXX) -c $< $(CXXFLAGS) -o $@ 

%.o: %.c
	$(CC) -c $< $(CCFLAGS) -o $@
