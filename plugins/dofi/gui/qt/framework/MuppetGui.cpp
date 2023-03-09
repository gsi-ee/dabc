#include "MuppetGui.h"

#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <stdlib.h>
#include <errno.h>

#include <QString>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QDateTime>
#include <QTimer>
#include <QMessageBox>
#include <QMdiSubWindow>
#include <QSignalMapper>
#include <QKeyEvent>

#include <sstream>


// this we need to implement for output of mbspex library, but also useful to format output without it:
MuppetGui* MuppetGui::fInstance = 0;

#include <stdarg.h>





void printm (char *fmt, ...)
{
  char c_str[512];
  va_list args;
  va_start(args, fmt);
  vsprintf (c_str, fmt, args);
//printf ("%s", c_str);
  MuppetGui::fInstance->AppendTextWindow (c_str);
  MuppetGui::fInstance->FlushTextWindow();
  va_end(args);
}





// *********************************************************

/*
 *  Constructs a MuppetGui which is a child of 'parent', with the
 *  name 'name'.'
 */
MuppetGui::MuppetGui (QWidget* parent) :
    QMainWindow (parent), fSettings(0), fWinMapper(0), fDebug (false), fSaveConfig(false), fSlotGuard(false), fFullScreen(false), fMdiTabViewMode(true),
      fConfigFile(NULL)
{

 Q_INIT_RESOURCE(muppeticons);


  setupUi (this);
#if QT_VERSION >= QT_VERSION_CHECK(4,6,0)
  fEnv = QProcessEnvironment::systemEnvironment ();    // get PATH to gosipcmd from parent process
#endif

  TextOutput->setUndoRedoEnabled(false); // JAM2020 - avoid increasing memory cosumption by text output (verbose mode!)
  fImplementationName="MUPPET";
  fVersionString="Welcome to MUPPET GUI!\n\t v0.50 of 3-March-2023 by JAM (j.adamczewski@gsi.de)";


  fNumberBase=10;
  fPort=54321;
  fHost="ra-9";

  RefreshColouredLabel(Connectlabel,"Init",muppet_yellow_background);

  TextOutput->setCenterOnScroll (false);
  ClearOutputBtn_clicked ();



  QObject::connect (TestButton, SIGNAL (clicked ()), this, SLOT (TestBtn_clicked ()));



  QObject::connect(actionConfigure, SIGNAL(triggered()), this, SLOT(ConfigBtn_clicked ()));
  QObject::connect(actionSave, SIGNAL(triggered()), this, SLOT(SaveConfigBtn_clicked ()));

  QObject::connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));
  QObject::connect(actionAbout, SIGNAL(triggered()), this, SLOT(about()));

//  QObject::connect(actionCascade, SIGNAL(triggered()), mdiArea, SLOT(cascadeSubWindows()));
//  QObject::connect(actionTile, SIGNAL(triggered()), mdiArea, SLOT(tileSubWindows()));


  QObject::connect(actionSaveSettings, SIGNAL(triggered()), this, SLOT(WriteSettings()));

  connect(menuWindows, SIGNAL(aboutToShow()), this, SLOT(windowsMenuAboutToShow()));
  // need to create menu item with F11
  windowsMenuAboutToShow();


  QAction* refreshaction=ControlToolBar->addAction( QIcon( ":/icons/refresh.png" ), "Show current hardware status (Refresh GUI) - F5",
                        this, SLOT(ShowBtn_clicked ()));
  refreshaction->setShortcut(Qt::Key_F5);

  QAction* applyaction=ControlToolBar->addAction(  QIcon( ":/icons/left.png" ), "Apply GUI setup to hardware - F12",
                          this, SLOT(ApplyBtn_clicked ()));
  applyaction->setShortcut(Qt::Key_F12);

  ControlToolBar->addSeparator();
  ControlToolBar->addAction(  QIcon( ":/icons/fileopen.png" ), "Load configuration from file into GUI",
                           this, SLOT(ConfigBtn_clicked ()));

  ControlToolBar->addAction(  QIcon( ":/icons/filesave.png" ), "Save current hardware configuration into setup file",
                             this, SLOT(SaveConfigBtn_clicked ()));

  ControlToolBar->addSeparator();
  ControlToolBar->addAction(  QIcon( ":/icons/analysiswin.png" ), "Dump main register contents of selected device to output window",
                              this, SLOT(DumpBtn_clicked ()));

  ControlToolBar->addAction(  QIcon( ":/icons/clear.png" ), "Clear output text window",
                               this, SLOT(ClearOutputBtn_clicked ()));


//  SFPToolBar->addAction( QIcon( ":/icons/connect.png" ), "Initialize chain of slaves at SFP as set in spinbox",
 //                        this, SLOT(InitChainBtn_clicked ()));

  SFPToolBar->addAction( QIcon( ":/icons/control.png" ), "Initialize/Reset logic on selected slave device",
                          this, SLOT(ResetSlaveBtn_clicked ()));

//  SFPToolBar->addAction( QIcon( ":/icons/killanal.png" ), "Reset SFP engine on PEXOR/KINPEX board",
//                            this, SLOT(ResetBoardBtn_clicked ()));


QObject::connect(DebugBox, SIGNAL(stateChanged(int)), this, SLOT(DebugBox_changed(int)));
QObject::connect(HexBox, SIGNAL(stateChanged(int)), this, SLOT(HexBox_changed(int)));


// JAM2017: some more signals for the autoapply feature:




  fInstance = this;

  // NOTE: this MUST be done in subclass constructor, otherwise factory method for setup structure will fail!
  //    (not tested, but to be expected so...)
  //GetSFPChainSetup(); // ensure that any slave has a status structure before we begin clicking...

   //show();
}

MuppetGui::~MuppetGui ()
{

if(fSettings) delete fSettings;

}

void MuppetGui::ShowBtn_clicked ()
{

//std::cout << "MuppetGui::ShowBtn_clicked()"<< std::endl;
  EvaluateSlave ();
//  GetSFPChainSetup();
//
//  if(!AssertNoBroadcast(false)) return;
//  if(!AssertChainConfigured()) return;
  MUPPET_LOCK_SLOT
  GetRegisters ();
  RefreshView ();
  MUPPET_UNLOCK_SLOT

}

void MuppetGui::ApplyBtn_clicked ()
{
//std::cout << "MuppetGui::ApplyBtn_clicked()"<< std::endl;
  EvaluateSlave ();
  if (!checkBox_AA->isChecked ())
  {
    QString message = QString("Really apply GUI Settings to Command server at HOST %1 Port %2?").arg(fHost).arg(fPort);
    if (QMessageBox::question (this, fImplementationName, message, QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes) != QMessageBox::Yes)
    {
      return;
    }
  }
  MUPPET_BROADCAST_ACTION(ApplyGUISettings());
}














void MuppetGui::ResetSlaveBtn_clicked ()
{
  char buffer[1024];
  EvaluateSlave ();
  snprintf (buffer, 1024, "Really reset %s device at Host %s, Port %d ?", fImplementationName.toLatin1 ().constData () ,fHost.toLatin1 ().constData (), fPort);
  if (QMessageBox::question (this, fImplementationName, QString (buffer), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)
      != QMessageBox::Yes)
  {
    //std::cout <<"QMessageBox does not return yes! "<< std::endl;
    return;
  }
  MUPPET_BROADCAST_ACTION(ResetSlave());
}

void MuppetGui::ResetSlave ()
{

 printm("ResetSlave() for Host %s, Port %d - not yet implemented, please design a subclass of MuppetGui!\n",fHost.toLatin1 ().constData (), fPort);
}

void MuppetGui::EvaluateView()
{
 printm("EvaluateView() for Host %s, Port %d  - not yet implemented, please design a subclass of MuppetGui!\n",fHost.toLatin1 ().constData (), fPort);
}

void MuppetGui::SetRegisters ()
{

  printm("SetRegisters() for Host %s, Port %d  - not yet implemented, please design a subclass of MuppetGui!\n",fHost.toLatin1 ().constData (), fPort);
}

void MuppetGui::GetRegisters ()
{
  printm("GetRegisters() for Host %s, Port %d - not yet implemented, please design a subclass of MuppetGui!\n",fHost.toLatin1 ().constData (), fPort);
}



void MuppetGui::ApplyGUISettings()
{
     //std::cout << "MuppetGui::ApplyGUISettings()"<< std::endl;
    // default behaviour, may be overwritten
    EvaluateView(); // from gui to memory
    SetRegisters(); // from memory to device
}








void MuppetGui::TestBtn_clicked ()
{
//std::cout << "MuppetGui::TestBtn_clicked "<< std::endl;

// here a dummy request to server in debug mode
// grep response times and put them to log window
  char buffer[1024];
EvaluateSlave ();
AppendTextWindow ("--- Test Connection:");
//MUPPET_BROADCAST_ACTION(TestSlave());
 snprintf (buffer, 1024, "rdoficom %s:%d -d -r -x  0x%x %d ", fHost.toLatin1 ().constData (), fPort, 0, 1);
 QString com (buffer);
 QString result = ExecuteMuppetCmd (com);
 // here grep lines with time information
QStringList lines=result.split('\n');
QString connect="nop", execute="nop";
//std::cout <<"debug the list: " << std::endl;
for (auto i = lines.begin(), end = lines.end(); i != end; ++i)
{
    //std::cout <<(*i).toLatin1 ().constData ()<< std::endl ;
    if(i->contains("Connect")) connect=*i;
    if(i->contains("Command")) execute=*i;
}
// AppendTextWindow (result);
 AppendTextWindow (connect);
 AppendTextWindow (execute);
 if(connect.contains("nop"))
   RefreshColouredLabel(Connectlabel,"No connection!",muppet_red_background);
 else
   RefreshColouredLabel(Connectlabel,connect,muppet_green_background);

}

void MuppetGui::DumpBtn_clicked ()
{
//std::cout << "MuppetGui::DumpBtn_clicked"<< std::endl;
// dump register contents from gosipcmd into TextOutput (QPlainText)
EvaluateSlave ();
AppendTextWindow ("--- Register Dump ---:");
MUPPET_BROADCAST_ACTION(DumpSlave());
RefreshStatus();
}


void MuppetGui::DumpSlave ()
{
  // most generic form, just uses dump function of setup structure
  // for detailed data fifo dump etc. please overwrite this method!
  GetRegisters();
  printm("Dump setup of %s",fHost);
  theSetup_GET_FOR_SLAVE(MuppetSetup);
  theSetup->Dump();

}


void MuppetGui::ClearOutputBtn_clicked ()
{
//std::cout << "MuppetGui::ClearOutputBtn_clicked()"<< std::endl;
TextOutput->clear ();
TextOutput->setPlainText (fVersionString);

}

void MuppetGui::ConfigBtn_clicked ()
{
//std::cout << "MuppetGui::ConfigBtn_clicked" << std::endl;
  ApplyFileConfig();
}


void MuppetGui::ApplyFileConfig(int gosipwait)
{
  // most generic form
  // TODO: JAM 3-3-2023: note that configuration file must be on filesystem at command server
  // if this differs we can not use file dialog here
  // however, should be no problem if we use mbs cluster with common /daq/usr mount on raspi and client PC
  QFileDialog fd (this, "Select MUPPET configuration file on server", ".", "doficmd file (*.dof)");
  fd.setFileMode (QFileDialog::ExistingFile);
  if (fd.exec () != QDialog::Accepted)
    return;
  QStringList flst = fd.selectedFiles ();
  if (flst.isEmpty ())
    return;

  QString fileName = flst[0];
  if (!fileName.endsWith (".dof"))
    fileName.append (".dof");
  char buffer[1024];
  snprintf (buffer, 1024, "rdoficom %s:%d -x -c %s ", fHost.toLatin1 ().constData (),fPort,  fileName.toLatin1 ().constData ());
  QString com (buffer);
  QString result = ExecuteMuppetCmd (com);
  AppendTextWindow (result);



}



void MuppetGui::DebugBox_changed (int on)
{
 MUPPET_LOCK_SLOT
//std::cout << "DebugBox_changed to "<< on << std::endl;
fDebug = on;
MUPPET_UNLOCK_SLOT
}

void MuppetGui::HexBox_changed(int on)
{
  MUPPET_LOCK_SLOT
  fNumberBase= (on ? 16 :10);
  //std::cout << "HexBox_changed set base to "<< fNumberBase << std::endl;
  RefreshView ();
  MUPPET_UNLOCK_SLOT
}




void MuppetGui::RefreshStatus ()
{
  QString text;
  QString statustext;
   statustext.append ("HOST ");
   statustext.append (fHost);
   statustext.append (" Port ");
   statustext.append (text.setNum (fPort));
   statustext.append (" - Last refresh:");
   statustext.append (QDateTime::currentDateTime ().toString (Qt::TextDate));
   statusBar()->showMessage(statustext);
}



void MuppetGui::RefreshView ()
{
// display setup structure to gui:

// for base class, we only update status messages etc.
RefreshStatus();
}



void MuppetGui::EvaluateSlave ()
{
  //if(fBroadcasting) return;
  fHost = HostnameLineEdit->text ();
  fPort = PortSpinBox->value ();
}



void MuppetGui::SaveConfigBtn_clicked ()
{
  //std::cout << "MuppetGui::SaveConfigBtn_clicked()"<< std::endl;
  SaveConfig();
}




void MuppetGui::SaveConfig()
{
  // default: write a gosipcmd script file
  // this one may be reimplemented in subclass if other file formats shall be supported.
  static char buffer[1024];
  QString dofi_filter ("rdoficommand file (*.dof)");
  //QString dmp_filter ("data dump file (*.dmp)");
  QStringList filters;
  filters << dofi_filter;// << dmp_filter;

  QFileDialog fd (this, "Write DOFI@MUPPET configuration file");

  fd.setNameFilters (filters);
  fd.setFileMode (QFileDialog::AnyFile);
  fd.setAcceptMode (QFileDialog::AcceptSave);
  if (fd.exec () != QDialog::Accepted)
    return;
  QStringList flst = fd.selectedFiles ();
  if (flst.isEmpty ())
    return;
  QString fileName = flst[0];

  // complete suffix if user did not
   if (fd.selectedNameFilter () == dofi_filter)
  {
    if (!fileName.endsWith (".dof"))
      fileName.append (".dof");
  }
  else
  {
    std::cout << "MuppetGui::SaveConfigBtn_clicked( - NEVER COME HERE!!!!)" << std::endl;
  }

  // open file
  if (OpenConfigFile (fileName) != 0)
    return;

  if (fileName.endsWith (".dof"))
  {
    WriteConfigFile (QString ("# Format *.dof\n"));
    WriteConfigFile (QString ("# usage: rdifucin -x -c file.dof \n"));
    WriteConfigFile (QString ("#                                         \n"));
    WriteConfigFile (QString ("# address value (setbit|clearbit)\n"));


    MUPPET_BROADCAST_ACTION(SaveRegisters()); // refresh actual setup from hardware and write it to open file
  }
  else
  {
    std::cout << "MuppetGui::SaveConfigBtn_clicked( -  unknown file type, NEVER COME HERE!!!!)" << std::endl;
  }

  // close file
  CloseConfigFile ();
  snprintf (buffer, 1024, "Saved current slave configuration to file '%s' .\n", fileName.toLatin1 ().constData ());
  AppendTextWindow (buffer);
}





int MuppetGui::OpenConfigFile (const QString& fname)
{
  fConfigFile = fopen (fname.toLatin1 ().constData (), "w");
  if (fConfigFile == NULL)
  {
    char buffer[1024];
    snprintf (buffer, 1024, " Error opening Configuration File '%s': %s\n", fname.toLatin1 ().constData (),
        strerror (errno));
    AppendTextWindow (buffer);
    return -1;
  }
  QString timestring = QDateTime::currentDateTime ().toString ("ddd dd.MM.yyyy hh:mm:ss");
  WriteConfigFile (QString ("# Muppet configuration file saved on ") + timestring + QString ("\n"));
  return 0;
}

int MuppetGui::CloseConfigFile ()
{
  int rev = 0;
  if (fConfigFile == NULL)
    return 0;
  if (fclose (fConfigFile) != 0)
  {
    char buffer[1024];
    snprintf (buffer, 1024, " Error closing Configuration File! (%s)\n", strerror (errno));
    AppendTextWindow (buffer);
    rev = -1;
  }
  fConfigFile = NULL;    // must not use handle again even if close fails
  return rev;
}

int MuppetGui::WriteConfigFile (const QString& text)
{
  if (fConfigFile == NULL)
    return -1;
  if (fprintf (fConfigFile, text.toLatin1 ().constData ()) < 0)
    return -2;
  return 0;
}




void MuppetGui::SaveRegisters ()
{
  // default implementation. might be overwritten by frontend subclass -
  // this may be called in explicit broadcast mode, so it is independent of the view on gui
  GetRegisters(); // refresh actual setup from hardware
  fSaveConfig = true;    // switch to file output mode
  SetRegisters();    // register settings are written to file
  fSaveConfig = false;
}




int MuppetGui::ReadMuppet (QString& host, int port, int address)
{
int value = -1;

 char buffer[1024];
 snprintf (buffer, 1024, "rdoficom %s:%d -r  0x%x  ", host.toLatin1 ().constData (), port,  address);
 QString com (buffer);
 QString result = ExecuteMuppetCmd (com);

if (result != "ERROR")
{
  DebugTextWindow (result);
  value = result.toInt (0, 0);
}
else
{
  value = -1;
}
return value;
}

int MuppetGui::WriteMuppet (QString& host, int port, int address, int value)
{
  int rev = 0;
  if (fSaveConfig)
      return SaveMuppet (host, port, address, value);
 char buffer[1024];
 snprintf (buffer, 1024, "rdoficom %s:%d  -w  0x%x %d ", host.toLatin1 ().constData (), port, address, value);
 QString com (buffer);
 QString result = ExecuteMuppetCmd (com);
if (result == "ERROR")
  rev = -1;
return rev;
}

int MuppetGui::SaveMuppet (QString& host, int port, int address, int value)
{
//std::cout << "# SaveMuppet" << std::endl;
  static char buffer[1024] = { };
  snprintf (buffer, 1024, "%s %d %x %x \n", host.toLatin1 ().constData (), port, address, value);
  QString line (buffer);
  return (WriteConfigFile (line));
}



int MuppetGui::MultiReadMuppet (QString &host, int port, int baseaddress, int times, unsigned long long *destbuffer)
{
  int rev = -1;

  char buffer[1024];
  snprintf (buffer, 1024, "rdoficom %s:%d  -r  0x%x %d ", host.toLatin1 ().constData (), port, baseaddress, times);
  QString com (buffer);
  QString result = ExecuteMuppetCmd (com);

  if (result != "ERROR")
  {
    DebugTextWindow (result);

    // split values from response string lines
    // fill into destination array
    QStringList lines = result.split ('\n');
    lines.removeLast();        // we just skip trailing empty line from rdoficom here
    int dix = 0;
    for (auto i = lines.begin (), end = lines.end (); i != end; ++i)
    {
      if (dix >= times)
      {
        std::cout << "NEVER COME HERE:MultiReadMuppet more response values" << dix << " than requested " << times
            << std::endl;
        break;
      }
      long long value=i->toLongLong (); // output of rdoficom uses signed long long
      //destbuffer[dix++] = i->toULongLong ();
      destbuffer[dix++] = (unsigned long long) value; // we cast it afterwords.
    }
    if(dix!=times)
    {
      std::cout << "NEVER COME HERE:MultiReadMuppet unmatching response values" << dix << " with requested " << times
               << std::endl;
      return -2;
    }
      rev = 0;
  }
  return rev;
}






QString MuppetGui::ExecuteMuppetCmd (QString& com, int timeout)
{
// interface to shell gosipcmd
// TODO optionally some remote call via ssh for Go4 gui?
QString result;
QProcess proc;
DebugTextWindow (com);
#if QT_VERSION >= QT_VERSION_CHECK(4,6,0)
proc.setProcessEnvironment (fEnv);
#endif
proc.setReadChannel (QProcess::StandardOutput);
QApplication::setOverrideCursor( Qt::WaitCursor );

proc.start (com);
// if(proc.waitForReadyRead (1000)) // will give termination warnings after leaving this function
if (proc.waitForFinished (timeout))    // after process is finished we can still read stdio buffer
{
  // read back stdout of proc here
  result = proc.readAll ();
}
else
{

 std::stringstream buf;
    buf << "! Warning: ExecuteMuppetCmd not finished after " << timeout / 1000 << " s timeout !!!" << std::endl;
    std::cout << " MuppetGui: " << buf.str ().c_str ();
    AppendTextWindow (buf.str ().c_str ());
    result = "ERROR";
}
QApplication::restoreOverrideCursor();
return result;
}

void MuppetGui::AppendTextWindow (const QString& text)
{
TextOutput->appendPlainText (text);
TextOutput->update ();
}



void MuppetGui::FlushTextWindow ()
{
  TextOutput->repaint ();
}





void MuppetGui::windowsMenuAboutToShow()
{
    //std::cout<< "windowsMenuAboutToShow..." << std::endl;
    menuWindows->clear();

    bool on = ! mdiArea->subWindowList().isEmpty();


    menuWindows->addAction("Cascade", mdiArea, SLOT(cascadeSubWindows()), Qt::Key_F7)->setEnabled(on);

    menuWindows->addAction("Tile", mdiArea, SLOT(tileSubWindows()), Qt::Key_F8)->setEnabled(on);
    menuWindows->addAction("Minimize all", this, SLOT(MinAllWindows()),Qt::Key_F9)->setEnabled(on);
    menuWindows->addAction((fMdiTabViewMode ? "Tabbed subwindows" : "Separate subwindows"), this, SLOT(ToggleSubwindowModeSlot()), Qt::Key_F6);


    menuWindows->addAction((fFullScreen ? "Normal window" : "Full screen"), this, SLOT(ToggleFullScreenSlot()), Qt::Key_F11);


    menuWindows->addSeparator();


    delete fWinMapper;
    fWinMapper = new QSignalMapper(this);
    connect(fWinMapper, SIGNAL(mapped(int)), this, SLOT(windowsMenuActivated(int)));

    QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
    for (int i=0; i< windows.count(); i++ ) {
       QAction* act = new QAction(windows.at(i)->widget()->windowTitle(), fWinMapper);
       act->setCheckable(true);
       act->setChecked(mdiArea->activeSubWindow() == windows.at(i));

       menuWindows->addAction(act);

       connect(act, SIGNAL(triggered()), fWinMapper, SLOT(map()) );
       fWinMapper->setMapping(act, i);
    }
}

void MuppetGui::windowsMenuActivated( int id )
{
 // std::cout<< "windowsMenuActivated..." << std::endl;
   QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
   if ((id>=0) && (id<windows.count())) {
      windows.at(id)->widget()->showNormal();
      windows.at(id)->widget()->setFocus();
   }
}

void MuppetGui::MinAllWindows()
{
   QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
   for ( int i = 0; i < windows.count(); i++ )
       windows.at(i)->widget()->showMinimized();
}



void MuppetGui::ToggleFullScreenSlot()
{
   if (fFullScreen) showNormal();
               else showFullScreen();
   fFullScreen = !fFullScreen;
}

void MuppetGui::ToggleSubwindowModeSlot()
{
   if (fMdiTabViewMode){
     mdiArea->setViewMode(QMdiArea::TabbedView);
   }
   else{
     mdiArea->setViewMode(QMdiArea::SubWindowView);
   }
   fMdiTabViewMode = !fMdiTabViewMode;
}



void MuppetGui::about()
{

   QMessageBox AboutWindow(fImplementationName, fVersionString, QMessageBox::NoIcon,QMessageBox::Ok,QMessageBox::NoButton,QMessageBox::NoButton ,this);
   AboutWindow.setIconPixmap(QPixmap( ":/icons/mbslogorun.gif"));
   AboutWindow.setTextFormat(Qt::RichText);
   AboutWindow.exec();
}


// stolen from Go4 to store/restore the subwindow sizes:
void MuppetGui::storePanelGeometry(QWidget* w, const QString& kind)
{
  if(fSettings)
    {
     fSettings->setValue(QString("/") + kind + QString("/Width"), w->width() );
     fSettings->setValue(QString("/") + kind + QString("/Height"), w->height() );
     fSettings->setValue(QString("/") + kind + QString("/X"), w->pos().x());
     fSettings->setValue(QString("/") + kind + QString("/Y"), w->pos().y());
    // std::cout<<"storePanelSize for "<<kind.toStdString().c_str() <<" saves width:"<<w->width()<<", height:"<< w->height();
     //std::cout<<" x:"<< w->pos().x()<<", y:"<<w->pos().y()<< std::endl;
    }
}

QSize MuppetGui::lastPanelSize(const QString& kind, int dfltwidth, int dfltheight)
{
    if(fSettings)
    {
      QSize rect(fSettings->value(QString("/") + kind + QString("/Width"), dfltwidth).toInt(),
              fSettings->value(QString("/") + kind + QString("/Height"), dfltheight).toInt());


     //std::cout<<"lastPanelSize for "<<kind.toStdString().c_str()<<" gets width:"<<rect.width()<<", height:"<< rect.height() << std::endl;

     return rect;
    }
    else
      return QSize( dfltwidth, dfltheight);
}

QPoint MuppetGui::lastPanelPos(const QString& kind)
{
    if(fSettings)
    {
      QPoint pt(fSettings->value(QString("/") + kind + QString("/X")).toInt(),
              fSettings->value(QString("/") + kind + QString("/Y")).toInt());


    // std::cout<<"lastPanelPosition for "<<kind.toStdString().c_str()<<" gets x:"<<pt.x()<<", y:"<< pt.y() << std::endl;

     return pt;
    }
    else
      return QPoint( 0, 0);
}



void MuppetGui::ReadSettings()
{
  //std::cout<< "ReadSettings..." << std::endl;
  if(fSettings)
    {
      // have to lock the slots of the hexbox to avoid that we trigger a refresh view before everything is ready!
    MUPPET_LOCK_SLOT
      bool autoapp=fSettings->value("/ModeControl/isAutoApply", false).toBool();
      checkBox_AA->setChecked(autoapp);

      fNumberBase=fSettings->value("/ModeControl/numberBase", 10).toInt();
      HexBox->setChecked(fNumberBase>10 ? true : false);
      fDebug= fSettings->value("/ModeControl/isVerbose", false).toBool();
      DebugBox->setChecked(fDebug);

      fMdiTabViewMode=!(fSettings->value("/Mdi/isTabbedMode",false).toBool());
      ToggleSubwindowModeSlot(); // set the desired mode and invert the flag again for correct menu display!



      restoreState(fSettings->value("/MainWindow/State").toByteArray());
      restoreGeometry(fSettings->value("/MainWindow/Geometry").toByteArray());

// JAM2019: the following will not work to store window sizes of mdi subwindows
// the settings file do not get byte array from subwindows, but only "false" property
// => not intended to use qsettings for mdi subwindows directly
//      mdiArea->restoreGeometry(fSettings->value("/MdiArea/Geometry").toByteArray());
//      QList<QMdiSubWindow*> subwinlist= mdiArea->subWindowList();
//         for (int i = 0; i < subwinlist.size(); ++i)
//         {
//           QMdiSubWindow* win= subwinlist.at(i);
//           win->restoreGeometry(fSettings->value( QString ("/MdiArea/SubWindow%1").arg (i)).toByteArray());
//           std::cout <<"ReadSettings restored subwindow "<< i << std::endl;
//         }
//////////// end comment on failed try,

      QList<QMdiSubWindow*> subwinlist= mdiArea->subWindowList();
      for (int i = 0; i < subwinlist.size(); ++i)
      {
        QMdiSubWindow* win= subwinlist.at(i);
        win->resize(lastPanelSize(QString ("SubWindow%1").arg (i)));
        win->move(lastPanelPos(QString ("SubWindow%1").arg (i)));
        //std::cout <<"ReadSettings restored subwindow panel geometry "<< i << std::endl;
      }
      //std::cout<< "ReadSettings gets numberbase:"<<fNumberBase<<", fDebug:"<<fDebug<<", autoapply:"<<autoapp << std::endl;
    MUPPET_UNLOCK_SLOT
    }
}

void MuppetGui::WriteSettings()
{
  //std::cout<< "WriteSettings..." << std::endl;
  if(fSettings)
  {
    fSettings->setValue("/Mdi/isTabbedMode", fMdiTabViewMode);

    fSettings->setValue("/ModeControl/isVerbose", fDebug);
    fSettings->setValue("/ModeControl/numberBase", fNumberBase);
    fSettings->setValue("/ModeControl/isAutoApply", IsAutoApply());

    fSettings->setValue("/MainWindow/State", saveState());
    fSettings->setValue("/MainWindow/Geometry", saveGeometry());

// scan geometry of subwindows in mdiarea:


// following is not working as intended. subwindowlist is available, but subwindow won't set/restore their geometry
//    fSettings->setValue("/MdiArea/Geometry", mdiArea->saveGeometry());
//    QList<QMdiSubWindow*> subwinlist= mdiArea->subWindowList();
//    for (int i = 0; i < subwinlist.size(); ++i)
//    {
//      QMdiSubWindow* win= subwinlist.at(i);
//      fSettings->setValue( QString ("/MdiArea/SubWindow%1").arg (i), win>saveGeometry());
//      std::cout <<"WriteSettings stored subwindow "<< i << std::endl;
//    }
///////////////////////// end comment failed try


    QList<QMdiSubWindow*> subwinlist= mdiArea->subWindowList();
    for (int i = 0; i < subwinlist.size(); ++i)
    {
      QMdiSubWindow* win= subwinlist.at(i);
      storePanelGeometry(win,QString ("SubWindow%1").arg (i)); // stolen from go4
     // std::cout <<"WriteSettings stored subwindow panel size "<< i << std::endl;
    }



  }
}


void MuppetGui::RefreshColouredLabel(QLabel* label, const QString text, MuppetTextColor_t color)
{
  if(label==0) return;
  QString labeltext=" <html><head/><body><p> <span style=\" font-weight:600;";
  switch (color)
  {
    case muppet_red:
      labeltext.append(" color:#ff0000;\"> ");
      break;
    case muppet_green:
      labeltext.append(" color:#00cc00;\"> ");
      break;
    case muppet_yellow:
      labeltext.append(" color:#cccc00;\"> ");
      break;
    case muppet_blue:
      labeltext.append(" color:#0000cc;\"> ");
      break;
    case muppet_red_background:
      labeltext.append(" background-color:#ff0000;\"> ");
      break;
    case muppet_green_background:
      labeltext.append(" background-color:#00cc00;\"> ");
      break;
    case muppet_yellow_background:
      labeltext.append(" background-color:#cccc00;\"> ");
      break;
    case muppet_blue_background:
      labeltext.append(" background-color:#0000dd;\"> ");
      break;


    case muppet_black:
    default:
      labeltext.append(" background-color:#000000;\"> ");
      break;

  }

  labeltext.append(text);
  labeltext.append(" </span></p></body></html>");
  label->setText(labeltext);

}







void MuppetGui::closeEvent( QCloseEvent* ce)
{

// new for Qt4:
   if(QMessageBox::question( this, fImplementationName, "Really Exit MUPPET control window?",
         QMessageBox::Yes | QMessageBox::No ,
         QMessageBox::Yes) != QMessageBox::Yes ) {
            //std::cout <<"QMessageBox does not return yes! "<< std::endl;
            ce->ignore();
            return;
      }
   ce->accept();
}
