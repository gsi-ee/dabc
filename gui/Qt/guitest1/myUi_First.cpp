//
// C++ Implementation: myUi_First
//
// Description:
//
//
// Author: goofy <goofy@depc225>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "myUi_First.h"
#include <QStandardItemModel>
#include <iostream>

myUi_First::myUi_First ( QMainWindow* top )
{
QStandardItem *item;
	ui.setupUi ( top );
	QStandardItemModel *model = new QStandardItemModel;
	QStandardItem *parentItem = model->invisibleRootItem();
	for ( int i = 0; i < 3; ++i )
	{
		for ( int n = 0; n < 3; ++n )
		{
			item = new QStandardItem ( QString ( "item %0 %1" ).arg ( i ) .arg(n));
			parentItem->appendRow ( item );
		} // ------------------------
		parentItem = item;
	} // ------------------------
	ui.treeView->setModel ( model );
	connect ( ui.pushButton_2, SIGNAL ( released() ), this, SLOT ( slotButton2() ) );
	connect ( ui.actionAction1, SIGNAL ( triggered() ), this, SLOT ( slotAction1() ) );
	connect ( ui.treeView, SIGNAL ( clicked(const QModelIndex &) ), this, SLOT ( slotView(const QModelIndex &)));
}
void myUi_First::slotButton2()
{
	ui.radioButton->toggle();
	std::cout << "pressed" << std::endl;
}
void myUi_First::slotAction1()
{
	ui.radioButton->toggle();
	std::cout << "action1" << std::endl;
}
void myUi_First::slotView(const QModelIndex &mi)
{
	const QAbstractItemModel *model = mi.model();
	std::cout << "view r:" << mi.row() << " c:" << mi.column() << "n:" << model->objectName().toStdString()
			<< std::endl;
}



