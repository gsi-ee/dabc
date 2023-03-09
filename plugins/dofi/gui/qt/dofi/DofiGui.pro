TEMPLATE	= app
LANGUAGE	= C++

DEPENDPATH += . ../framework
INCLUDEPATH += ../framework
LIBS += -L../framework -lMuppetGui


greaterThan(QT_MAJOR_VERSION, 4) {
  QT += widgets
}


equals(QT_MAJOR_VERSION, 4) {
 	 LIBS += -lkdeui
}
equals(QT_MAJOR_VERSION, 5) {
  QT += widgets
  QT += network
  INCLUDEPATH += /usr/include/KF5/KPlotting/
  LIBS += -lKF5Plotting
}


CONFIG += debug qt warn_off thread



SOURCES += main.cpp DofiGui.cpp DofiControlWidget.cpp DofiScalerWidget.cpp 

HEADERS += DofiGui.h DofiControlWidget.h DofiScalerWidget.h DofiSetup.h 

FORMS = DofiControlWidget.ui DofiScalerWidget.ui 
