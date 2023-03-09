#include "DofiGui.h"

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
#include <QMdiSubWindow>


//#include <kplotobject.h>
//#include <kplotwidget.h>
//#include <kplotaxis.h>


#include <sstream>





// *********************************************************

/*
 *  Constructs a DofiGui which is a child of 'parent', with the
 *  name 'name'.'
 */
DofiGui::DofiGui (QWidget* parent) :
    MuppetGui (parent)
{
  fImplementationName="DOFI";
  fVersionString="Welcome to Digital signals Over FIbre (DOFI) GUI!\n\t v0.50 of 03-March-2023 by JAM (j.adamczewski@gsi.de)";
  setWindowTitle(QString("%1 GUI").arg(fImplementationName));


  fSettings=new QSettings("GSI", fImplementationName);

  Qt::WindowFlags wflags= Qt::CustomizeWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowTitleHint;

  fDofiControlWidget=new DofiControlWidget(this);
  fDofiScalerWidget = new DofiScalerWidget(this);

  if(fDofiControlWidget)
       {
	  	 fDofiControlWidget->setWindowTitle("Main Control");
         fDofiControlWidget->show();
         QMdiSubWindow* subtrig=mdiArea->addSubWindow(fDofiControlWidget, wflags);
         subtrig->setAttribute(Qt::WA_DeleteOnClose, false);
       }

  if(fDofiScalerWidget)
        {
          fDofiScalerWidget->setWindowTitle("Scalers");

          // here initialize the tables:
          fDofiScalerWidget->InputScalersTableWidget->setRowCount(DOFI_NUM_CHANNELS);
          fDofiScalerWidget->OutputScalersTableWidget->setRowCount(DOFI_NUM_CHANNELS);
          for(int i=0; i<DOFI_NUM_CHANNELS;++i)
          {
            QTableWidgetItem* itemin= new QTableWidgetItem("init");
            fDofiScalerWidget->InputScalersTableWidget->setItem(i,0,itemin);

            QTableWidgetItem* itemout= new QTableWidgetItem("init");
            fDofiScalerWidget->OutputScalersTableWidget->setItem(i,0,itemout);

          }



          fDofiScalerWidget->show();
          QMdiSubWindow* subtrig=mdiArea->addSubWindow(fDofiScalerWidget, wflags);
          subtrig->setAttribute(Qt::WA_DeleteOnClose, false);
        }

  ClearOutputBtn_clicked ();


//  QObject::connect (fDofiWidget->OffsetButton, SIGNAL (clicked ()), this, SLOT (OffsetBtn_clicked ()));
//
//
//  QObject::connect (fDofiWidget->TriggerButton, SIGNAL (clicked ()), this, SLOT (TriggerBtn_clicked ()));
//
//  QObject::connect (fDofiWidget->QFWResetButton, SIGNAL (clicked ()), this, SLOT (QFWResetBtn_clicked ()));





//  QObject::connect(fDofiWidget->DACModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(DACMode_changed(int)));

// JAM2017: some more signals for the autoapply feature:

//QObject::connect(fDofiWidget->ModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(QFW_changed()));
//QObject::connect (fDofiWidget->MasterTriggerBox, SIGNAL(stateChanged(int)), this, SLOT (QFW_changed()));
//QObject::connect (fDofiWidget->InternalTriggerBox, SIGNAL(stateChanged(int)), this, SLOT (QFW_changed()));
//QObject::connect (fDofiWidget->FesaModeBox, SIGNAL(stateChanged(int)), this, SLOT (QFW_changed()));
//
//QObject::connect (fDofiWidget->TSLoop1lineEdit, SIGNAL(returnPressed()),this,SLOT (QFW_changed()));
//QObject::connect (fDofiWidget->TSLoop2lineEdit, SIGNAL(returnPressed()),this,SLOT (QFW_changed()));
//QObject::connect (fDofiWidget->TSLoop3lineEdit, SIGNAL(returnPressed()),this,SLOT (QFW_changed()));
//QObject::connect (fDofiWidget->TS1TimelineEdit, SIGNAL(returnPressed()),this,SLOT (QFW_changed()));
//QObject::connect (fDofiWidget->TS2TimelineEdit, SIGNAL(returnPressed()),this,SLOT (QFW_changed()));
//QObject::connect (fDofiWidget->TS3TimelineEdit, SIGNAL(returnPressed()),this,SLOT (QFW_changed()));
//
//QObject::connect (fDofiWidget->DACStartValueLineEdit, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DACOffsetLineEdit, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DACDeltaOffsetLineEdit, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DACCalibTimeLineEdit, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//
//QObject::connect (fDofiWidget->DAClineEdit_1, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_2, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_3, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_4, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_5, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_6, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_7, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_8, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_9, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_10, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_11, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_12, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_13, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_14, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_15, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_16, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_17, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_18, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_19, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_20, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_21, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_22, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_23, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_24, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_25, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_26, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_27, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_28, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_29, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_30, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_31, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//QObject::connect (fDofiWidget->DAClineEdit_32, SIGNAL(returnPressed()),this,SLOT (DAC_changed()));
//
//QObject::connect (fDofiWidget->FanDial, SIGNAL(valueChanged(int)),this,SLOT (Fan_changed()));
//
//
//QObject::connect (fDofiViewpanelWidget->SampleButton, SIGNAL (clicked ()), this, SLOT (ShowSample ()));
//
//QObject::connect (fDofiCSAWidget->CSA_inswitch_tocsa_radioButton, SIGNAL (toggled (bool)), this, SLOT (CSA_changed()));

// JAM 29-7-21: we do not need to connect second radio button, since they are grouped exclusively. avoid second call of slots
//QObject::connect (fDofiCSAWidget->CSA_inswitch_bypass_radioButton, SIGNAL (toggled (bool)), this, SLOT (CSA_changed()));

//QObject::connect (fDofiCSAWidget->CSA_autorange_auto_radioButton, SIGNAL (toggled (bool)), this, SLOT (CSA_changed()));
//
////QObject::connect (fDofiCSAWidget->CSA_autorange_manual_radioButton, SIGNAL (toggled (bool)), this, SLOT (CSA_changed()));
//
//QObject::connect (fDofiCSAWidget->CSA_outswitch_fromcsa_radioButton, SIGNAL (toggled (bool)), this, SLOT (CSA_changed()));
//
////QObject::connect (fDofiCSAWidget->CSA_outswitch_bypass_radioButton, SIGNAL (toggled (bool)), this, SLOT (CSA_changed()));
//
//QObject::connect (fDofiCSAWidget->CSA_feedback_spinBox, SIGNAL (valueChanged(int)), this, SLOT (CSA_spinbox_changed(int)));
//
//
//QObject::connect (fDofiCSAWidget->CSA_feedbackLineEdit, SIGNAL (returnPressed()), this, SLOT (CSA_lineEdit_changed()));

ReadSettings();


fSetup=CreateSetup();
show();
}

DofiGui::~DofiGui ()
{
}




void DofiGui::ApplyGUISettings()
{
  //std::cout << "DofiGui::ApplyGUISettings()"<< std::endl;
  // depending on activated view, we either set qfw parameters or change DAC programming

//// TODO JAM2019 - for subwindow mode, always apply everything
//  //if(fDofiWidget->QFW_DAC_tabWidget->currentIndex()==0)
//{
//  ApplyQFWSettings();
//
//}
////else if (fDofiWidget->QFW_DAC_tabWidget->currentIndex()==1)
//{
//  ApplyDACSettings();
//}
////else if (fDofiWidget->QFW_DAC_tabWidget->currentIndex()==2)
//{
//  ApplyFanSettings();
//}
//
//{
//  ApplyCSASettings();
//}


}


//void DofiGui::ApplyFanSettings()
//{
//  EvaluateFans(); // from gui to memory
//  SetFans ();
//}
//
//
//void DofiGui::ApplyQFWSettings()
//{
//  EvaluateView(); // from gui to memory
//  SetRegisters ();
//}
//
//void DofiGui::ApplyDACSettings()
//{
//  EvaluateDAC();
//  ApplyDAC();
//}
//
//void DofiGui::QFW_changed ()
//{
//  MUPPET_LOCK_SLOT
//  MUPPET_AUTOAPPLY(ApplyQFWSettings());
//  MUPPET_UNLOCK_SLOT
//}
//
//void DofiGui::DAC_changed ()
//{
//  MUPPET_LOCK_SLOT
//  MUPPET_AUTOAPPLY(ApplyDACSettings());
//  MUPPET_UNLOCK_SLOT
//}
//
//void DofiGui::Fan_changed ()
//{
//  MUPPET_LOCK_SLOT
//  MUPPET_AUTOAPPLY(ApplyFanSettings());
//
//  // for autoapply refresh fan readout immediately:
//  if(AssertNoBroadcast(false))
//  {
//    GetSensors();
//    RefreshSensors();
//  }
//  MUPPET_UNLOCK_SLOT
//}
//
//
//void DofiGui::CSA_changed ()
//{
//  //std::cout << "DofiGui::CSA_changed()"<< std::endl;
//  MUPPET_LOCK_SLOT
//  MUPPET_AUTOAPPLY(ApplyCSASettings());
//  MUPPET_UNLOCK_SLOT
//}
//
//void DofiGui::CSA_spinbox_changed (int value)
//{
//  //std::cout << "DofiGui::CSA_spinbox_changed() to "<< value<< std::endl;
//  QString pre;
//  QString text;
//  fNumberBase==16? pre="0x" : pre="";
//  uint8_t feedback =(value & 0xF);
//  fDofiCSAWidget->CSA_feedbackLineEdit->setText (pre+text.setNum (feedback, fNumberBase));
//  CSA_changed();
//}
//
//
//void DofiGui::CSA_lineEdit_changed()
//{
//  //std::cout << "DofiGui::CSA_lineEdit_changed()"<< std::endl;
//  uint8_t feedback = fDofiCSAWidget->CSA_feedbackLineEdit->text().toInt(0, fNumberBase) & 0xF;
//  fDofiCSAWidget->CSA_feedback_spinBox->setValue(feedback);
//  CSA_changed();
//}
//
//
//void DofiGui::ApplyCSASettings()
//{
//  EvaluateCSA();
//  ApplyCSA();
//}
//
//
//void DofiGui::EvaluateCSA()
//{
//  theSetup_GET_FOR_SLAVE(DofiSetup);
//
//  // first synchronize spinBox with hex text field:
////  int value=fDofiCSAWidget->CSA_feedbackLineEdit->text().toInt(0, fNumberBase);
////  fDofiCSAWidget->CSA_feedback_spinBox->setValue(value);
//  uint8_t feedback = fDofiCSAWidget->CSA_feedbackLineEdit->text().toInt(0, fNumberBase) & 0xF;
////(fDofiCSAWidget->CSA_feedback_spinBox->value() & 0xF);
//  bool autorangemanual= fDofiCSAWidget->CSA_autorange_manual_radioButton->isChecked();
//  bool inswitchbypass= fDofiCSAWidget->CSA_inswitch_bypass_radioButton->isChecked();
//  bool outswitchbypass= fDofiCSAWidget->CSA_outswitch_bypass_radioButton->isChecked();
//  //std::cout << "DofiGui::EvaluateCSA sees autorangemanual:"<<autorangemanual<<", inbypass:"<<inswitchbypass << ", outbypass:"<< outswitchbypass<<", feedback:"<< (int)feedback << std::endl;
//  theSetup->SetCSASettings(autorangemanual, inswitchbypass, outswitchbypass, feedback);
//  //std::cout << "DofiGui::EvaluateCSA() sets register value 0x"<< std::hex << theSetup->GetCSAControl()<< std::dec<< std::endl;
//}
//
//
//void DofiGui::ApplyCSA()
//{
//  theSetup_GET_FOR_SLAVE(DofiSetup);
//  int onvalue=theSetup->GetCSAControl() | 0x100; // JAM 29-7-21: need bit 8 to enable
//  WriteMuppet (fSFP, fSlave, POLAND_REG_CSA_CTRL, onvalue);
//  WriteMuppet (fSFP, fSlave, POLAND_REG_CSA_CTRL, theSetup->GetCSAControl()); // bit 8 is zero here anyway
//}



void DofiGui::ResetSlave ()
{
// WriteMuppet (fSFP, fSlave, POLAND_REG_RESET, 0);
// WriteMuppet (fSFP, fSlave, POLAND_REG_RESET, 1);
  // TODO
 printm("Did reset DOFI at %s:%d\n",fHost.toLatin1 ().constData (), fPort);
}




//void DofiGui::OffsetBtn_clicked ()
//{
//char buffer[1024];
//EvaluateSlave ();
//snprintf (buffer, 1024, "Really scan offset for SFP chain %d, Slave %d ?", fSFP, fSlave);
//if (QMessageBox::question (this, fImplementationName, QString (buffer), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)
//    != QMessageBox::Yes)
//{
//  //std::cout <<"QMessageBox does not return yes! "<< std::endl;
//  return;
//}
//
//MUPPET_BROADCAST_ACTION(ScanOffsets());
//}






//void DofiGui::ScanOffsets ()
//{
//  char buffer[1024];
//  WriteMuppet (fSFP, fSlave, POLAND_REG_DO_OFFSET, 1);
//  printm("--- SFP %d Dofi %d : Doing offset measurement... ",fSFP, fSlave);
//  QApplication::setOverrideCursor( Qt::WaitCursor );
//
//  WriteMuppet (fSFP, fSlave, POLAND_REG_DO_OFFSET, 0);
//  sleep(2);
//  AppendTextWindow ("    ... done. Dumping offset values:");
//
//  snprintf (buffer, 1024, "gosipcmd -d -r -x -- 0x%x 0x%x 0x%x 0x%x", fSFP, fSlave, POLAND_REG_OFFSET_BASE, 32);
//  QString com (buffer);
//  QString result = ExecuteMuppetCmd (com);
//  AppendTextWindow (result);
//  QApplication::restoreOverrideCursor();
//
//}
//
//
//
//void DofiGui::TriggerBtn_clicked ()
//{
//  //std::cout << "DofiGui::TriggerBtn_clicked"<< std::endl;
//  char buffer[1024];
//  fTriggerOn ?   snprintf (buffer, 1024, "Really disable Frontend Trigger acceptance?") : snprintf (buffer, 1024, "Really enable Frontend Trigger acceptance?");
//
//  if (QMessageBox::question (this, fImplementationName, QString (buffer), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)
//      != QMessageBox::Yes)
//  {
//    //std::cout <<"QMessageBox does not return yes! "<< std::endl;
//    return;
//  }
//
//  fTriggerOn ? fTriggerOn=false: fTriggerOn=true;
//
//
//  int value = (fTriggerOn ? 1 : 0);
//  QString state= (fTriggerOn ? "ON" : "OFF");
//  WriteMuppet (-1, -1, POLAND_REG_TRIG_ON, value); // send broadcast of register to all slaves.
//  QString msg="--- Set all devices trigger acceptance to ";
//  AppendTextWindow (msg+state);
//  RefreshTrigger();
//
//
//}
//
//void DofiGui::RefreshTrigger()
//{
//   // todo: modify label
//  QString labelprefix="<html><head/><body><p>Trigger";
//  QString labelstate = fTriggerOn ? " <span style=\" font-weight:600; color:#00ff00;\">ON </span></p></body></html>" :
//      " <span style=\" font-weight:600; color:#ff0000;\">OFF</span></p></body></html>" ;
//  fDofiWidget->TriggerLabel->setText(labelprefix+labelstate);
//
//}
//
//
//void DofiGui::QFWResetBtn_clicked ()
//{
//char buffer[1024];
//EvaluateSlave ();
//snprintf (buffer, 1024, "Really Reset QFWs for SFP chain %d, Slave %d ?", fSFP, fSlave);
//if (QMessageBox::question (this, fImplementationName, QString (buffer), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)
//    != QMessageBox::Yes)
//{
//  //std::cout <<"QMessageBox does not return yes! "<< std::endl;
//  return;
//}
//fDoResetQFW=true;
//QFW_changed ();
//fDoResetQFW=false;
//
//}


void DofiGui::DumpSlave ()
{
  char buffer[1024];

  GetRegisters();
  printm("dump registers of  %s:%d\n",fHost.toLatin1 ().constData (), fPort);
  theSetup_GET_FOR_SLAVE(DofiSetup);
  theSetup->Dump();

  AppendTextWindow ("Input Scalers:\n");
  snprintf (buffer, 1024, "rdoficom %s:%d %s -r %s  0x%x %d ", fHost.toLatin1 ().constData (), fPort, (fDebug ? "-d":" "), (fNumberBase==16 ? "-x": " "), DOFI_INPUTSCALER_BASE, DOFI_NUM_CHANNELS);
  QString com (buffer);
  QString result = ExecuteMuppetCmd (com);
  AppendTextWindow (result);

  AppendTextWindow ("Output Scalers:\n");
  snprintf (buffer, 1024, "rdoficom %s:%d %s -r %s  0x%x %d ", fHost.toLatin1 ().constData (), fPort, (fDebug ? "-d":" "), (fNumberBase==16 ? "-x": " "), DOFI_OUTPUTSCALER_BASE, DOFI_NUM_CHANNELS);
  com=buffer;
  result = ExecuteMuppetCmd (com);
  AppendTextWindow (result);

  RefreshStatus();
}









void DofiGui::RefreshView ()
{
// display setup structure to gui:
QString text;
QString pre;
fNumberBase==16? pre="0x" : pre="";
theSetup_GET_FOR_SLAVE(DofiSetup);


for(int inc=0; inc<DOFI_NUM_CHANNELS; ++inc)
{
  QTableWidgetItem* item=fDofiScalerWidget->InputScalersTableWidget->item(inc,0);
  qulonglong scaler=theSetup->fInputScalers[inc];
  text=pre+QString("%1").arg(scaler,0,fNumberBase);
  item->setText(text);
}

for(int out=0; out<DOFI_NUM_CHANNELS; ++out)
{
  QTableWidgetItem* item=fDofiScalerWidget->OutputScalersTableWidget->item(out,0);
  qulonglong scaler=theSetup->fOutputScalers[out];
  text=pre+QString("%1").arg(scaler,0,fNumberBase);
  item->setText(text);
}



//RefreshMode();
//
//
//fDofiWidget->TSLoop1lineEdit->setText (pre+text.setNum (theSetup->fSteps[0], fNumberBase));
//fDofiWidget->TSLoop2lineEdit->setText (pre+text.setNum (theSetup->fSteps[1], fNumberBase));
//fDofiWidget->TSLoop3lineEdit->setText (pre+text.setNum (theSetup->fSteps[2], fNumberBase));
//fDofiWidget->TS1TimelineEdit->setText (text.setNum (theSetup->GetStepTime(0)));
//fDofiWidget->TS2TimelineEdit->setText (text.setNum (theSetup->GetStepTime(1)));
//fDofiWidget->TS3TimelineEdit->setText (text.setNum (theSetup->GetStepTime(2)));
//fDofiWidget->MasterTriggerBox->setChecked (theSetup->IsTriggerMaster ());
//fDofiWidget->FesaModeBox->setChecked (theSetup->IsFesaMode ());
//fDofiWidget->InternalTriggerBox->setChecked (theSetup->IsInternalTrigger ());
//
//
//fDofiWidget->EventCounterNumber->setMode((fNumberBase==16) ? QLCDNumber::Hex :  QLCDNumber::Dec);
//fDofiWidget->ErrorCounter1->setMode((fNumberBase==16) ? QLCDNumber::Hex :  QLCDNumber::Dec);
//fDofiWidget->ErrorCounter2->setMode((fNumberBase==16) ? QLCDNumber::Hex :  QLCDNumber::Dec);
//fDofiWidget->ErrorCounter3->setMode((fNumberBase==16) ? QLCDNumber::Hex :  QLCDNumber::Dec);
//fDofiWidget->ErrorCounter4->setMode((fNumberBase==16) ? QLCDNumber::Hex :  QLCDNumber::Dec);
//fDofiWidget->ErrorCounter5->setMode((fNumberBase==16) ? QLCDNumber::Hex :  QLCDNumber::Dec);
//fDofiWidget->ErrorCounter6->setMode((fNumberBase==16) ? QLCDNumber::Hex :  QLCDNumber::Dec);
//fDofiWidget->ErrorCounter7->setMode((fNumberBase==16) ? QLCDNumber::Hex :  QLCDNumber::Dec);
//fDofiWidget->ErrorCounter8->setMode((fNumberBase==16) ? QLCDNumber::Hex :  QLCDNumber::Dec);
//
//
//fDofiWidget->EventCounterNumber->display ((int) theSetup->fEventCounter);
//fDofiWidget->ErrorCounter1->display ((int) theSetup->fErrorCounter[0]);
//fDofiWidget->ErrorCounter2->display ((int) theSetup->fErrorCounter[1]);
//fDofiWidget->ErrorCounter3->display ((int) theSetup->fErrorCounter[2]);
//fDofiWidget->ErrorCounter4->display ((int) theSetup->fErrorCounter[3]);
//fDofiWidget->ErrorCounter5->display ((int) theSetup->fErrorCounter[4]);
//fDofiWidget->ErrorCounter6->display ((int) theSetup->fErrorCounter[5]);
//fDofiWidget->ErrorCounter7->display ((int) theSetup->fErrorCounter[6]);
//fDofiWidget->ErrorCounter8->display ((int) theSetup->fErrorCounter[7]);
//
//RefreshDACMode();
//RefreshDAC(); // probably this is already triggered by signal
//RefreshTrigger(); // show real trigger register as read back from actual device
//
//RefreshSensors();
//fDofiViewpanelWidget->RefreshEventCounter(); // hex/dec toggle here
//
//RefreshCSA();
//
//RefreshChains();
RefreshStatus();
}

void DofiGui::EvaluateView ()
{
EvaluateSlave ();
//EvaluateMode  ();

theSetup_GET_FOR_SLAVE(DofiSetup);


// copy widget values to structure
//theSetup->fSteps[0] = fDofiWidget->TSLoop1lineEdit->text ().toUInt (0, fNumberBase);
//theSetup->fSteps[1] = fDofiWidget->TSLoop2lineEdit->text ().toUInt (0, fNumberBase);
//theSetup->fSteps[2] = fDofiWidget->TSLoop3lineEdit->text ().toUInt (0, fNumberBase);
//
//theSetup->SetStepTime(fDofiWidget->TS1TimelineEdit->text ().toDouble (),0);
//theSetup->SetStepTime(fDofiWidget->TS2TimelineEdit->text ().toDouble (),1);
//theSetup->SetStepTime(fDofiWidget->TS3TimelineEdit->text ().toDouble (),2);
//
//
//theSetup->SetTriggerMaster (fDofiWidget->MasterTriggerBox->isChecked ());
//theSetup->SetFesaMode (fDofiWidget->FesaModeBox->isChecked ());
//theSetup->SetInternalTrigger (fDofiWidget->InternalTriggerBox->isChecked ());

}



void DofiGui::SetRegisters ()
{
  //std::cout << "DofiGui::SetRegisters()"<< std::endl;


// write register values from strucure with gosipcmd
  theSetup_GET_FOR_SLAVE(DofiSetup);
//if (AssertNoBroadcast (false)) // NOTE: after change to broadcast action, this is always true JAM2017
//if(!fBroadcasting) // use macro flag instead!
//{
//  // update trigger modes only in single device
//  WriteMuppet (fSFP, fSlave, POLAND_REG_INTERNAL_TRIGGER, theSetup->fInternalTrigger);
//  WriteMuppet (fSFP, fSlave, POLAND_REG_MASTERMODE, theSetup->fTriggerMode);
//}
//
//if(fDoResetQFW)
//{
//  WriteMuppet (fSFP, fSlave, POLAND_REG_QFW_RESET, 0);
//  WriteMuppet (fSFP, fSlave, POLAND_REG_QFW_RESET, 1);
//  //std::cout << "DofiGui::SetRegisters did reset QFW for ("<<fSFP<<", "<<fSlave<<")"<< std::endl;
//}
//
//
//
//WriteMuppet (fSFP, fSlave, POLAND_REG_QFW_MODE, theSetup->fQFWMode);
//
//// following is required to really activate qfw mode (thanks Sven Loechner for fixing):
//WriteMuppet (fSFP, fSlave, POLAND_REG_QFW_PRG, 1);
//WriteMuppet (fSFP, fSlave, POLAND_REG_QFW_PRG, 0);
//
//
//
//// WriteMuppet(fSFP, fSlave, POLAND_REG_TRIGCOUNT, theSetup->fEventCounter);
//
//for (int i = 0; i < POLAND_TS_NUM; ++i)
//{
//  WriteMuppet (fSFP, fSlave, POLAND_REG_STEPS_BASE + 4 * i, theSetup->fSteps[i]);
//  WriteMuppet (fSFP, fSlave, POLAND_REG_TIME_BASE + 4 * i, theSetup->fTimes[i]);
//}
////    for(int e=0; e<POLAND_ERRCOUNT_NUM;++e)
////     {
////       WriteMuppet(fSFP, fSlave, POLAND_REG_ERRCOUNT_BASE + 4*e, theSetup->fErrorCounter[e]);
////     }
//
//// TODO: error handling with exceptions?

}

void DofiGui::GetRegisters ()
{
  std::cout<<"DofiGui::GetRegisters ()" << std::endl;
// read register values from hardware into structure
int status=0;
//if (!AssertNoBroadcast ())
//  return;

theSetup_GET_FOR_SLAVE(DofiSetup);


//MultiReadMuppet (QString &host, int port, int baseaddress, int times, unsigned long long *destbuffer)

status=MultiReadMuppet (fHost, fPort, DOFI_SIGNALCTRL_BASE, DOFI_NUM_CHANNELS,theSetup->fSignalControl);
if(status) printm("GetRegisters error %d when reading input signal registers!",status);

status=MultiReadMuppet (fHost, fPort, DOFI_ORCTRL_BASE, DOFI_NUM_CHANNELS,theSetup->fOutputORBits);
if(status) printm("GetRegisters error %d when reading output OR registers!",status);

status=MultiReadMuppet (fHost, fPort, DOFI_ANDCTRL_BASE, DOFI_NUM_CHANNELS,theSetup->fOutputANDBits);
if(status) printm("GetRegisters error %d when reading output AND registers!",status);

status=MultiReadMuppet (fHost, fPort, DOFI_INPUTSCALER_BASE, DOFI_NUM_CHANNELS,theSetup->fInputScalers);
if(status) printm("GetRegisters error %d when reading input scalers!",status);

status=MultiReadMuppet (fHost, fPort, DOFI_OUTPUTSCALER_BASE, DOFI_NUM_CHANNELS,theSetup->fOutputScalers);
if(status) printm("GetRegisters error %d when reading output scalers!",status);



}





void DofiGui::SaveRegisters ()
{
  // this may be called in explicit broadcast mode, so it is independent of the view on gui
  GetRegisters(); // refresh actual setup from hardware
  fSaveConfig = true;    // switch to file output mode
  //WriteConfigFile (QString ("# QFW Registers: \n"));
  SetRegisters();    // register settings are written to file
  //WriteConfigFile (QString ("# DAC Settings: \n"));
  //ApplyDAC();
  fSaveConfig = false;
}




