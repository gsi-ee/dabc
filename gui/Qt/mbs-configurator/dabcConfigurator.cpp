#include "configurator.h"
#include "qapplication.h"

int main(int argc, char **argv) 
{
    QApplication myapp( argc, argv, true);
    Configurator* mainwindow = new Configurator(0,"dabc configuration tool");
    myapp.setMainWidget(mainwindow );
    mainwindow ->polish();
    mainwindow ->show();
    QApplication::setDoubleClickInterval(400); //ms, for Qt>=3.3 avoid very fast defaults!
    int res = myapp.exec();
return res;    
}
