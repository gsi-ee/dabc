
QTVERSION = 3

#ifneq ($(shell which qmake 2>/dev/null),)
#else
ifneq ($(shell qmake-qt5 --version 2>&1 | grep "Qt version 5."),)
QTVERSION = 5
QMAKE = qmake-qt5
else
ifneq ($(shell qmake -qt5 --version 2>&1 | grep "Qt version 5."),)
QTVERSION = 5
QMAKE = qmake -qt5
else
ifneq ($(shell qmake --version 2>&1 | grep "Qt version 4."),)
QTVERSION = 4
QMAKE = qmake
else
ifneq ($(shell which qmake-qt4 2>/dev/null),)
QTVERSION = 4
QMAKE = qmake-qt4
endif
endif
endif
endif
#endif
#endif
#endif





INC = $(DABCSYS)/include

DABC_BINPATH=$(DABCSYS)/bin/
DABC_LIBPATH=$(DABCSYS)/lib/
DABC_INCPATH=$(DABCSYS)/include
DABC_INC = -I$(DABC_INCPATH)
#DABC_LIBA = $(DABC_LIBPATH)/libmbspex.a




CFLAGS  = $(POSIX) -DLinux -Dunix -DV40 -DGSI_MBS -D$(GSI_CPU_ENDIAN) -D$(GSI_CPU_PLATFORM)
