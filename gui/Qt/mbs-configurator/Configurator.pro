TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release

SOURCES	+= dabcConfigurator.cpp

FORMS	= configurator.ui

IMAGEFILE = images.cpp
  
unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}


 