TEMPLATE	= lib
LANGUAGE	= C++

greaterThan(QT_MAJOR_VERSION, 4) {
  QT += widgets
}

CONFIG += debug qt warn_off thread
CONFIG += staticlib

RESOURCES += ../muppeticons.qrc


#SOURCES += main.cpp MuppetGui.cpp
SOURCES += MuppetGui.cpp 

HEADERS += MuppetGui.h 

#FORMS = MuppetGui.ui
FORMS = MuppetMainwindow.ui

