/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/
void Configurator::init()
{
    //setIcon(QPixmap::fromMimeSource("dabcicon.png"));
    fbSeparateBuilders=true;
    fiCurrentReadout=-1;
    fShell=0;
    fbWasSaved=true;
    restoreSettings("dabc_default");

}


void Configurator::destroy()
{

}



void Configurator::fileNew()
{
      bool ok;
    QString text = QInputDialog::getText(
            "New Configurator setup", "Type new setup name:", QLineEdit::Normal,
            QString::null, &ok, this );
    if ( ok && !text.isEmpty() ) {
     fxProjectName=text;
     clearForm();
  }
  }


void Configurator::fileOpen()
{
    QFileDialog fd( this, "Open Configurator setup file", TRUE );
    fd.setCaption("Select Configurator settings file:");
   fd.setMode( QFileDialog::ExistingFile);
   fd.setShowHiddenFiles(true);
   fd.setName( "Open Configurator file");
   fd.setFilters(QString("Configurator files (*rc)"));
   QString wdir=QDir::currentDirPath()+"/.dabc";
   fd.setDir(QDir(wdir));
   if ( fd.exec() != QDialog::Accepted ) return;
   QString text=fd.selectedFile();
   text=text.section( '/', -1 ); // strip full path
   text=text.left(text.length()-2); // remove rc at the end
   restoreSettings(text);

}


void Configurator::fileSave()
{
     std::cout <<"****************Saving project "<<fxProjectName << std::endl;
    QSettings sett;
    sett.insertSearchPath( QSettings::Unix, QDir::currentDirPath()+ "/.dabc");
    sett.writeEntry( fxProjectName+"/SeparateBuilders", fbSeparateBuilders ? 1: 0);
    sett.writeEntry(fxProjectName+"/ControllerNode", ControllerLineEdit->text().stripWhiteSpace());
    QString builders="";
     for(unsigned int ix=0;ix<BuilderListBox->count();++ix){
                builders.append(BuilderListBox->text(ix));
                builders.append(";");
            }
       sett.writeEntry(fxProjectName+"/BuilderNodes", builders);
     QString readers="";
      for(unsigned int ix=0;ix<ReadoutListBox->count();++ix){
          readers.append(ReadoutListBox->text(ix));
          readers.append(":");
          QStringList mlist=fxMbsList[ix];
          QString mbsnodes=mlist.join(":");
          readers.append(mbsnodes);
          readers.append(";");
      }
     sett.writeEntry(fxProjectName+"/ReaderNodes", readers);
     fbWasSaved=true;
}


void Configurator::fileSaveAs()
{
     bool ok;
    QString text = QInputDialog::getText(
            "Saveconfigurator project", "Type new setup name:", QLineEdit::Normal,
            QString::null, &ok, this );
    if ( ok	 && !text.isEmpty() ) {
        fxProjectName=text;
        updateCaption();
        fileSave();
    }
}



void Configurator::fileExit()
{
     close();
}




void Configurator::helpAbout()
{
      QString mestring("DABC Configurator for MBS v0.9");
   mestring.append("\n Configurator GUI for Data Acquisition Backbone Core");
   mestring.append("\n J. Adamczewski, Experiment Electronics (EE) at GSI, July 2008");
   QMessageBox AboutConfigurator("DABC Configurator", mestring,
                         QMessageBox::NoIcon,QMessageBox::Ok,QMessageBox::NoButton,QMessageBox::NoButton ,this);
     //AboutConfigurator.setIconPixmap(QPixmap::fromMimeSource( "dabcicon.png"));
   AboutConfigurator.exec();

}

void Configurator::helpUsageAction_activated()
{
    std::cout << std::endl;
    std::cout <<"******** DABC configurator for bnet with Multi Branch System readout" << std::endl;
    std::cout <<"- Define name of bnet controller node in text field" << std::endl;
    std::cout <<"- Choose if you want to have symmetric builder net or separate eventbuilders" << std::endl;
    std::cout <<"- Add new DABC nodes using the 'Add new DABC' buttons (those with the dabc logo) for Worker, Readout, or Eventbuilder nodes,resp." <<  std::endl;
    std::cout <<"- Add new Mbs readout connections for each DABC event builder/worker node using the 'Add new Mbs' button (the one with the MBS symbol)" << std:: endl;
    std::cout <<"- Generate DABC setup file with Menu: 'File/GenerateDABC Setup'. This file is to be used by dabc gui 'dabc'" <<  std::endl;
    std::cout <<"- Load/Save configurator projects in Qt settings wtih Menu: 'File/Open and Save'" <<  std::endl;

}



void Configurator::newReadoutSlot()
{
    bool ok;
    QString text = QInputDialog::getText(
            "Add Readout Node", "Type node name:", QLineEdit::Normal,
            QString::null, &ok, this );
    if ( ok && !text.isEmpty() ) {
        // user entered something and pressed OK
        //std::cout <<"got new readout slot "<<text << std::endl;
        ReadoutListBox->insertItem(text,-1);
        QStringList mbslist;
        fxMbsList.push_back(mbslist);
        fbWasSaved=false;


    }
}


void Configurator::removeReadoutSlot()
{
    fiCurrentReadout= ReadoutListBox->currentItem();
    //std::cout <<"removeReadoutSlot() sees current item "<<fiCurrentReadout << std::endl;
    if (fiCurrentReadout < 0) return;
      //std::cout <<"removeReadoutSlot() erasing.."<<std::endl;
    fxMbsList.erase(fxMbsList.begin()+fiCurrentReadout);
    ReadoutListBox->removeItem(fiCurrentReadout); // will update current index by signal after removing the old one
  fbWasSaved=false;
}


void Configurator::editReadoutSlot()
{
  fiCurrentReadout= ReadoutListBox->currentItem();
  if (fiCurrentReadout < 0) return;
     bool ok;
    QString text = QInputDialog::getText(
            "Edit Readout Node", "Change node name:", QLineEdit::Normal,
            ReadoutListBox->currentText(), &ok, this );
    if ( ok && !text.isEmpty() ) {
        ReadoutListBox->changeItem(text,fiCurrentReadout);
        fbWasSaved=false;
    }
}






void Configurator::newBuilderSlot()
{
    bool ok;
    QString text = QInputDialog::getText(
            "Add Event builder Node", "Type node name:", QLineEdit::Normal,
            QString::null, &ok, this );
    if ( ok && !text.isEmpty() ) {
        // user entered something and pressed OK
        //std::cout <<"got new readout slot "<<text << std::endl;
        BuilderListBox->insertItem(text,-1);
        fbWasSaved=false;
    }


}


void Configurator::removeBuilderSlot()
{
    BuilderListBox->removeItem(BuilderListBox->currentItem());
    fbWasSaved=false;
}

void Configurator::editBuilderSlot()
{
 int currentbuilder=BuilderListBox->currentItem();
  if (currentbuilder < 0) return;
     bool ok;
    QString text = QInputDialog::getText(
            "Edit Eventbuilder Node", "Change node name:", QLineEdit::Normal,
            BuilderListBox->currentText(), &ok, this );
    if ( ok && !text.isEmpty() ) {
        BuilderListBox->changeItem(text,currentbuilder);
        fbWasSaved=false;
    }
}


void Configurator::newMbsSlot()
{
    if(fiCurrentReadout<0) return;
    bool ok;
    QString text = QInputDialog::getText(
            "Add MBS Node", "Type node name:", QLineEdit::Normal,
            QString::null, &ok, this );
    if ( ok && !text.isEmpty() ) {
        // user entered something and pressed OK
        //std::cout <<"got new Mbs slot "<<text << std::endl;
        QStringList& mbsnodes=fxMbsList[fiCurrentReadout];
        mbsnodes.push_back(text);
        updateMbsList();
        fbWasSaved=false;

    }
}


void Configurator::removeMbsSlot()
{
    if(fiCurrentReadout<0) return;
    int currentmbs=MBSListBox->currentItem();
    if(currentmbs<0) return;
    MBSListBox->removeItem(currentmbs);
    QStringList& mbsnodes=fxMbsList[fiCurrentReadout];
    mbsnodes.remove(mbsnodes[currentmbs]);
    fbWasSaved=false;
}


void Configurator::editMBSSlot()
{
 if(fiCurrentReadout<0) return;
  int currentmbs=MBSListBox->currentItem();
    if(currentmbs<0) return;
       bool ok;
    QString text = QInputDialog::getText(
            "Edit MBS Node", "Change node name:", QLineEdit::Normal,
            MBSListBox->currentText(), &ok, this );
    if ( ok && !text.isEmpty() ) {
        // user entered something and pressed OK
        //std::cout <<"got new readout slot "<<text << std::endl;
        MBSListBox->changeItem(text,currentmbs);
        QStringList& mbsnodes=fxMbsList[fiCurrentReadout];
        mbsnodes[currentmbs]=text;
        fbWasSaved=false;
    }

}






void Configurator::setSeparateEventbuidersSlot( bool on)
{
    fbSeparateBuilders=on;
    if(fbSeparateBuilders)
        ReadoutLabel->setText("DABC Readout nodes");
    else
       ReadoutLabel->setText("DABC Worker nodes");
    fbWasSaved=false;
}





void Configurator::setCurrentReadoutSlot(int num)
{
    fiCurrentReadout= num;
    //std::cout <<"setCurrentReadoutSlot to "<<num << std::endl;
    updateMbsList();
}


void Configurator::updateMbsList()
{
    MBSListBox->clear();
    if(fiCurrentReadout<0){
        MBSLabel->setText("MBS nodes (please select Readout first):");
        return;
    }
     //std::cout <<"updateMbsList for "<<fiCurrentReadout << std::endl;
     QStringList mlist=fxMbsList[fiCurrentReadout]; // use copy of stringlist
    MBSListBox->insertStringList(mlist);
    QString labeltext="MBS nodes for "+ReadoutListBox->currentText() + ":";
    MBSLabel->setText(labeltext);
}


void Configurator::generateSetupSlot()
{
    // first write text files with node descriptions:
    if(fbSeparateBuilders){
        //std::cout <<"writing buildernodes.txt" << std::endl;
        QFile file( "buildernodes.txt" );
        if ( file.open( IO_WriteOnly ) ) {
            QTextStream stream( &file );
            for(unsigned int ix=0;ix<BuilderListBox->count();++ix){
                QString current=BuilderListBox->text(ix);
                stream << current << "\n";
            }
            file.close();
        }
    }
        // readout nodes with mbs:
        //std::cout <<"writing readernodes.txt" <<std::endl;
       QFile file2( "readernodes.txt" );
        if ( file2.open( IO_WriteOnly ) ) {
         QTextStream stream( &file2 );
            for(unsigned int ix=0;ix<ReadoutListBox->count();++ix){
                QString readnode=ReadoutListBox->text(ix);
                QStringList mlist=fxMbsList[ix];
                QString mbsnodes=mlist.join(":");
                stream << readnode << ":"<<mbsnodes <<"\n";
            }
            file2.close();

    }


    // then invoke our nice script with correct parameters:
    std::cout <<"****************Generating setup...." << std::endl;
    fShell= new QProcess();
    connect( fShell, SIGNAL(readyReadStdout()), this, SLOT(readFromStdout()));
    connect( fShell, SIGNAL(readyReadStderr()), this, SLOT(readFromStderr()));
    connect( fShell, SIGNAL(processExited()), this, SLOT(exitShell()));

     QString dabcsys=::getenv("DABCSYS"); // note that QProcess can not expand environment variables!
     //std::cout <<"Found dabcsys="<<dabcsys << std::endl;
     QString fullcommand=dabcsys+"/gui/Qt/mbs-configurator/generateSetupMbs.sh";
     //fShell->addArgument("/misc/adamczew/xdaq/devel/dabc/gui/Qt/configurator/generateSetupMbs.sh");
     fShell->addArgument(fullcommand);
     //fShell->addArgument("./generateSetupMbs.sh");
     fShell->addArgument(ControllerLineEdit->text().stripWhiteSpace());
     if(fbSeparateBuilders)
        fShell->addArgument(" 1");
       else
         fShell->addArgument(" 0");
     fShell->setWorkingDirectory(QDir::current());
     if(!fShell->start(0))
        {
          std::cout <<"!!!!!!!! could not start generate Setup process" << std::endl;;
          delete fShell;
          fShell=0;
      }
          //std::cout <<"************* Returned from QProcess::start." << std::endl;
 }


void Configurator::readFromStdout()
{
    if(fShell){
      QByteArray ba = fShell->readStdout();
      QString buf(ba);
       std::cout << buf<<std::endl;
   }
}


void Configurator::readFromStderr()
{
    if(fShell){
         QByteArray ba = fShell->readStderr();
         QString buf(ba);
         std::cout << buf <<std::endl;
   }

}


void Configurator::exitShell()
{
   //std::cout << "got exit shell!!"<<std::endl;
    std::cout <<"****************Setup is ready." << std::endl;
   delete fShell;
   fShell=0;
}


void Configurator::closeEvent( QCloseEvent * ce )
{
//std::cout <<"****************closeEvent( ...." << std::endl;
if(!fbWasSaved)
{
    if(QMessageBox::warning( this, "DABC Configurator", "Save changed setup before exit?",
         QMessageBox::Yes ,
         QMessageBox::No | QMessageBox::Escape) == QMessageBox::Yes ){
        fileSave();
    }
}

if(QMessageBox::question( this, "DABC Configurator", "Really Exit?",
         QMessageBox::Yes ,
         QMessageBox::No | QMessageBox::Escape) != QMessageBox::Yes ) return;


ce->accept();

}


void Configurator::updateCaption()
{
      QString caption="Data Acquisition Backbone Core Configurator - ";
      caption.append(fxProjectName);
      setCaption(caption);
}


void Configurator::clearForm()
{
   ControllerLineEdit->clear();
    fxMbsList.clear();
    MBSListBox->clear();
    ReadoutListBox->clear();
    BuilderListBox->clear();
    fiCurrentReadout=-1;
    updateMbsList();
    updateCaption();
}







void Configurator::restoreSettings( const QString & name )
{
    clearForm();
    fxProjectName=name;
    std::cout <<"****************Open project "<<fxProjectName << std::endl;
    QSettings sett;
    sett.insertSearchPath( QSettings::Unix, QDir::currentDirPath()+ "/.dabc");
    int sepbuilders=sett.readNumEntry(fxProjectName+"/SeparateBuilders", 1);
    if(sepbuilders>0)
        fbSeparateBuilders=true;
    else
        fbSeparateBuilders=false;
    BuilderFrame->setShown(fbSeparateBuilders);
    AsymmCheckBox->setChecked(fbSeparateBuilders);
    QString controllernode=  sett.readEntry(fxProjectName+"/ControllerNode", "master.cluster.domain");
    ControllerLineEdit->setText(controllernode);
    QString builders=  sett.readEntry(fxProjectName+"/BuilderNodes", "please edit node name");
    QStringList builderlist=QStringList::split(";",builders);
    BuilderListBox->insertStringList(builderlist);
     QString readers=  sett.readEntry(fxProjectName+"/ReaderNodes", "please edit node name");
     QStringList readerlist=QStringList::split(";",readers);
      for ( QStringList::Iterator it = readerlist.begin(); it != readerlist.end(); ++it ) {
          QString current=*it;
          QString readernode=current.section( ':', 0, 0 );
          //std::cout <<"++++ got readoutnode"<< readernode << std::endl;
          ReadoutListBox->insertItem(readernode,-1);
          QString mbsnodes=current.section( ':', 1);
          //std::cout <<"++++ got mbsnodes"<< mbsnodes << std::endl;
          QStringList mbslist=QStringList::split(":",mbsnodes);
          fxMbsList.push_back(mbslist);
      } // for iterator
    updateCaption();
    fbWasSaved=true;

}


void Configurator::controllernameChangedSlot()
{
    fbWasSaved=false;
}
