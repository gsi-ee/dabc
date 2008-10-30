 #include "ui_First.h"
 #include "myUi_First.h"
 #include <QApplication>
#include <QMainWindow>
#include <QDirModel>
int main(int argc, char *argv[])
 {
     QApplication app(argc, argv);
     QMainWindow *window = new QMainWindow;
     //QDirModel *dirm = new QDirModel;
     myUi_First myui(window);
     // tree ---------------
     window->show();
     return app.exec();
 }
