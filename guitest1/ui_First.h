/********************************************************************************
** Form generated from reading ui file 'First.ui'
**
** Created: Mon Mar 12 16:38:47 2007
**      by: Qt User Interface Compiler version 4.2.2
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_FIRST_H
#define UI_FIRST_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCalendarWidget>
#include <QtGui/QCheckBox>
#include <QtGui/QDateEdit>
#include <QtGui/QDateTimeEdit>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QListWidget>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QStatusBar>
#include <QtGui/QTabWidget>
#include <QtGui/QTimeEdit>
#include <QtGui/QToolBar>
#include <QtGui/QToolBox>
#include <QtGui/QTreeView>
#include <QtGui/QWidget>

class Ui_First
{
public:
    QAction *actionAction1;
    QAction *actionOpen;
    QAction *actionClose;
    QAction *actionAction2;
    QAction *actionFitter;
    QAction *actionAction3;
    QWidget *centralwidget;
    QRadioButton *radioButton;
    QPushButton *pushButton_3;
    QPushButton *pushButton_2;
    QCheckBox *checkBox;
    QPushButton *exitButton;
    QPushButton *pushButton_4;
    QTabWidget *tabWidget;
    QWidget *tab;
    QTreeView *treeView;
    QWidget *tab_3;
    QListWidget *listWidget;
    QWidget *tab_2;
    QCalendarWidget *calendarWidget;
    QToolBox *toolBox;
    QWidget *page;
    QTimeEdit *timeEdit;
    QWidget *page_2;
    QDateEdit *dateEdit;
    QWidget *page_3;
    QDateTimeEdit *dateTimeEdit;
    QPushButton *PushButton_1;
    QDialogButtonBox *buttonBox;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuOptions;
    QStatusBar *statusbar;
    QToolBar *toolBar;

    void setupUi(QMainWindow *First)
    {
    First->setObjectName(QString::fromUtf8("First"));
    actionAction1 = new QAction(First);
    actionAction1->setObjectName(QString::fromUtf8("actionAction1"));
    actionAction1->setIcon(QIcon(QString::fromUtf8(":/images/images/color.png")));
    actionOpen = new QAction(First);
    actionOpen->setObjectName(QString::fromUtf8("actionOpen"));
    actionOpen->setIcon(QIcon(QString::fromUtf8(":/images/images/open.png")));
    actionClose = new QAction(First);
    actionClose->setObjectName(QString::fromUtf8("actionClose"));
    actionClose->setIcon(QIcon(QString::fromUtf8(":/images/images/close.png")));
    actionAction2 = new QAction(First);
    actionAction2->setObjectName(QString::fromUtf8("actionAction2"));
    actionAction2->setIcon(QIcon(QString::fromUtf8(":/images/images/dynlist.png")));
    actionFitter = new QAction(First);
    actionFitter->setObjectName(QString::fromUtf8("actionFitter"));
    actionFitter->setIcon(QIcon(QString::fromUtf8(":/images/images/fitter.png")));
    actionAction3 = new QAction(First);
    actionAction3->setObjectName(QString::fromUtf8("actionAction3"));
    centralwidget = new QWidget(First);
    centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
    radioButton = new QRadioButton(centralwidget);
    radioButton->setObjectName(QString::fromUtf8("radioButton"));
    radioButton->setGeometry(QRect(0, 90, 102, 25));
    pushButton_3 = new QPushButton(centralwidget);
    pushButton_3->setObjectName(QString::fromUtf8("pushButton_3"));
    pushButton_3->setGeometry(QRect(0, 40, 102, 21));
    pushButton_2 = new QPushButton(centralwidget);
    pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));
    pushButton_2->setGeometry(QRect(0, 20, 102, 21));
    checkBox = new QCheckBox(centralwidget);
    checkBox->setObjectName(QString::fromUtf8("checkBox"));
    checkBox->setGeometry(QRect(0, 110, 102, 24));
    exitButton = new QPushButton(centralwidget);
    exitButton->setObjectName(QString::fromUtf8("exitButton"));
    exitButton->setGeometry(QRect(10, 340, 84, 29));
    pushButton_4 = new QPushButton(centralwidget);
    pushButton_4->setObjectName(QString::fromUtf8("pushButton_4"));
    pushButton_4->setGeometry(QRect(0, 60, 102, 21));
    tabWidget = new QTabWidget(centralwidget);
    tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
    tabWidget->setGeometry(QRect(100, 0, 441, 371));
    tab = new QWidget();
    tab->setObjectName(QString::fromUtf8("tab"));
    treeView = new QTreeView(tab);
    treeView->setObjectName(QString::fromUtf8("treeView"));
    treeView->setGeometry(QRect(10, 10, 421, 321));
    tabWidget->addTab(tab, QApplication::translate("First", "Tree", 0, QApplication::UnicodeUTF8));
    tab_3 = new QWidget();
    tab_3->setObjectName(QString::fromUtf8("tab_3"));
    listWidget = new QListWidget(tab_3);
    listWidget->setObjectName(QString::fromUtf8("listWidget"));
    listWidget->setGeometry(QRect(40, 0, 120, 94));
    tabWidget->addTab(tab_3, QApplication::translate("First", "Table", 0, QApplication::UnicodeUTF8));
    tab_2 = new QWidget();
    tab_2->setObjectName(QString::fromUtf8("tab_2"));
    calendarWidget = new QCalendarWidget(tab_2);
    calendarWidget->setObjectName(QString::fromUtf8("calendarWidget"));
    calendarWidget->setGeometry(QRect(30, 10, 208, 175));
    tabWidget->addTab(tab_2, QApplication::translate("First", "Calendar", 0, QApplication::UnicodeUTF8));
    toolBox = new QToolBox(centralwidget);
    toolBox->setObjectName(QString::fromUtf8("toolBox"));
    toolBox->setGeometry(QRect(540, 30, 201, 189));
    page = new QWidget();
    page->setObjectName(QString::fromUtf8("page"));
    page->setGeometry(QRect(0, 0, 96, 26));
    timeEdit = new QTimeEdit(page);
    timeEdit->setObjectName(QString::fromUtf8("timeEdit"));
    timeEdit->setGeometry(QRect(10, 10, 118, 27));
    toolBox->addItem(page, QApplication::translate("First", "Time", 0, QApplication::UnicodeUTF8));
    page_2 = new QWidget();
    page_2->setObjectName(QString::fromUtf8("page_2"));
    page_2->setGeometry(QRect(0, 0, 201, 90));
    dateEdit = new QDateEdit(page_2);
    dateEdit->setObjectName(QString::fromUtf8("dateEdit"));
    dateEdit->setGeometry(QRect(20, 10, 110, 27));
    toolBox->addItem(page_2, QApplication::translate("First", "Date", 0, QApplication::UnicodeUTF8));
    page_3 = new QWidget();
    page_3->setObjectName(QString::fromUtf8("page_3"));
    page_3->setGeometry(QRect(0, 0, 96, 26));
    dateTimeEdit = new QDateTimeEdit(page_3);
    dateTimeEdit->setObjectName(QString::fromUtf8("dateTimeEdit"));
    dateTimeEdit->setGeometry(QRect(0, 20, 194, 27));
    toolBox->addItem(page_3, QApplication::translate("First", "Time/Date", 0, QApplication::UnicodeUTF8));
    PushButton_1 = new QPushButton(centralwidget);
    PushButton_1->setObjectName(QString::fromUtf8("PushButton_1"));
    PushButton_1->setGeometry(QRect(0, 0, 102, 21));
    PushButton_1->setFlat(false);
    buttonBox = new QDialogButtonBox(centralwidget);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setGeometry(QRect(0, 140, 81, 29));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok);
    First->setCentralWidget(centralwidget);
    menubar = new QMenuBar(First);
    menubar->setObjectName(QString::fromUtf8("menubar"));
    menubar->setGeometry(QRect(0, 0, 614, 31));
    menuFile = new QMenu(menubar);
    menuFile->setObjectName(QString::fromUtf8("menuFile"));
    menuOptions = new QMenu(menubar);
    menuOptions->setObjectName(QString::fromUtf8("menuOptions"));
    First->setMenuBar(menubar);
    statusbar = new QStatusBar(First);
    statusbar->setObjectName(QString::fromUtf8("statusbar"));
    First->setStatusBar(statusbar);
    toolBar = new QToolBar(First);
    toolBar->setObjectName(QString::fromUtf8("toolBar"));
    toolBar->setOrientation(Qt::Horizontal);
    First->addToolBar(static_cast<Qt::ToolBarArea>(4), toolBar);

    menubar->addAction(menuFile->menuAction());
    menubar->addAction(menuOptions->menuAction());
    menuFile->addAction(actionOpen);
    menuFile->addAction(actionClose);
    menuOptions->addAction(actionAction1);
    menuOptions->addAction(actionAction2);
    menuOptions->addAction(actionFitter);
    toolBar->addAction(actionAction1);
    toolBar->addSeparator();
    toolBar->addAction(actionAction2);
    toolBar->addSeparator();
    toolBar->addAction(actionOpen);
    toolBar->addSeparator();
    toolBar->addAction(actionClose);
    toolBar->addSeparator();
    toolBar->addAction(actionFitter);

    retranslateUi(First);

    QSize size(614, 668);
    size = size.expandedTo(First->minimumSizeHint());
    First->resize(size);

    QObject::connect(PushButton_1, SIGNAL(released()), radioButton, SLOT(toggle()));
    QObject::connect(exitButton, SIGNAL(released()), First, SLOT(close()));

    tabWidget->setCurrentIndex(0);
    toolBox->setCurrentIndex(1);


    QMetaObject::connectSlotsByName(First);
    } // setupUi

    void retranslateUi(QMainWindow *First)
    {
    First->setWindowTitle(QApplication::translate("First", "First", 0, QApplication::UnicodeUTF8));
    actionAction1->setText(QApplication::translate("First", "Action1", 0, QApplication::UnicodeUTF8));
    actionOpen->setText(QApplication::translate("First", "Open...", 0, QApplication::UnicodeUTF8));
    actionClose->setText(QApplication::translate("First", "Close...", 0, QApplication::UnicodeUTF8));
    actionAction2->setText(QApplication::translate("First", "Action2", 0, QApplication::UnicodeUTF8));
    actionFitter->setText(QApplication::translate("First", "Fitter", 0, QApplication::UnicodeUTF8));
    actionAction3->setText(QApplication::translate("First", "Action3", 0, QApplication::UnicodeUTF8));
    radioButton->setText(QApplication::translate("First", "RadioButton", 0, QApplication::UnicodeUTF8));
    pushButton_3->setText(QApplication::translate("First", "PushButton3", 0, QApplication::UnicodeUTF8));
    pushButton_2->setText(QApplication::translate("First", "PushButton2", 0, QApplication::UnicodeUTF8));
    checkBox->setText(QApplication::translate("First", "CheckBox", 0, QApplication::UnicodeUTF8));
    exitButton->setText(QApplication::translate("First", "Exit", 0, QApplication::UnicodeUTF8));
    pushButton_4->setText(QApplication::translate("First", "PushButton4", 0, QApplication::UnicodeUTF8));
    tabWidget->setTabText(tabWidget->indexOf(tab), QApplication::translate("First", "Tree", 0, QApplication::UnicodeUTF8));
    tabWidget->setTabText(tabWidget->indexOf(tab_3), QApplication::translate("First", "Table", 0, QApplication::UnicodeUTF8));
    tabWidget->setTabText(tabWidget->indexOf(tab_2), QApplication::translate("First", "Calendar", 0, QApplication::UnicodeUTF8));
    toolBox->setItemText(toolBox->indexOf(page), QApplication::translate("First", "Time", 0, QApplication::UnicodeUTF8));
    toolBox->setItemText(toolBox->indexOf(page_2), QApplication::translate("First", "Date", 0, QApplication::UnicodeUTF8));
    toolBox->setItemText(toolBox->indexOf(page_3), QApplication::translate("First", "Time/Date", 0, QApplication::UnicodeUTF8));
    PushButton_1->setText(QApplication::translate("First", "PushButton1", 0, QApplication::UnicodeUTF8));
    menuFile->setTitle(QApplication::translate("First", "File", 0, QApplication::UnicodeUTF8));
    menuOptions->setTitle(QApplication::translate("First", "Options", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class First: public Ui_First {};
} // namespace Ui

#endif // UI_FIRST_H
