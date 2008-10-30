//
// C++ Interface: myUi_First
//
// Description:
//
//
// Author: goofy <goofy@depc225>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef MYUI_FIRST_H
#define MYUI_FIRST_H

#include <ui_First.h>
#include <QModelIndex>

/**
	@author goofy <goofy@depc225>
*/
class myUi_First : public QObject
{
		Q_OBJECT
	public:
		myUi_First ( QMainWindow* top=0 );
	private:
		Ui::First ui;
	private slots:
		void slotButton2();
		void slotAction1();
		void slotView ( const QModelIndex& mi );
};

#endif
