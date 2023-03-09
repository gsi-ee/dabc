#ifndef DOFICONTROLWIDGET_H
#define DOFICONTROLWIDGET_H

#include "ui_DofiControlWidget.h"




class DofiControlWidget: public QWidget, public Ui::DofiControlWidget
{
  Q_OBJECT

public:
  DofiControlWidget (QWidget* parent = 0);
  virtual ~DofiControlWidget ();





protected:



};

#endif
