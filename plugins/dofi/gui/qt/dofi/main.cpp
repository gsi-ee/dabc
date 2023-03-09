#include <QApplication>
#include <QMainWindow>


#include "DofiGui.h"

int main(int argc, char *argv[])
{

  QApplication::setStyle("plastique");
  QApplication app(argc, argv);
  DofiGui main(0);
  main.show();
  int ret = app.exec();
return ret;
}
