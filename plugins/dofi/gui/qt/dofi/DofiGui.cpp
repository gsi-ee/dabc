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



enum DofiScalerColumns_t
{
  dofi_scaler_in=0,
  dofi_scaler_out=1,
  dofi_scaler_inrate=2,
  dofi_scaler_outrate=3
};

enum DofiInputColumns_t
{
  dofi_input_invert=0,
  dofi_input_delay=1,
  dofi_input_len=2
};

// *********************************************************

/*
 *  Constructs a DofiGui which is a child of 'parent', with the
 *  name 'name'.'
 */
DofiGui::DofiGui (QWidget* parent) :
    MuppetGui (parent)
{
  fImplementationName="DOFI";
  fVersionString="Welcome to Digital signals Over FIbre (DOFI) GUI!\n\t v0.60 of 21-March-2023 by JAM (j.adamczewski@gsi.de)";
  setWindowTitle(QString("%1 GUI").arg(fImplementationName));


  fSettings=new QSettings("GSI", fImplementationName);

  Qt::WindowFlags wflags= Qt::CustomizeWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowTitleHint;

  fDofiControlWidget=new DofiControlWidget(this);
  fDofiScalerWidget = new DofiScalerWidget(this);
  fDofiInputsWidget= new DofiInputsWidget(this);

  if(fDofiControlWidget)
       {
	  	 fDofiControlWidget->setWindowTitle("Main Control");

	  	fDofiControlWidget->OutputORTableWidget->setRowCount(DOFI_NUM_CHANNELS);
	  	fDofiControlWidget->OutputORTableWidget->setColumnCount(DOFI_NUM_CHANNELS);
	    fDofiControlWidget->OutputANDTableWidget->setRowCount(DOFI_NUM_CHANNELS);
	    fDofiControlWidget->OutputANDTableWidget->setColumnCount(DOFI_NUM_CHANNELS);


	  	for (int outs = 0; outs < DOFI_NUM_CHANNELS; ++outs)
    {
      fDofiControlWidget->OutputORTableWidget->setHorizontalHeaderItem (outs,
          new QTableWidgetItem (QString ("%1").arg (outs)));
      fDofiControlWidget->OutputANDTableWidget->setHorizontalHeaderItem (outs,
          new QTableWidgetItem (QString ("%1").arg (outs)));
      for (int ins = 0; ins < DOFI_NUM_CHANNELS; ++ins)
      {

        fDofiControlWidget->OutputORTableWidget->setVerticalHeaderItem (ins,
            new QTableWidgetItem (QString ("%1").arg (ins)));
        fDofiControlWidget->OutputANDTableWidget->setVerticalHeaderItem (ins,
            new QTableWidgetItem (QString ("%1").arg (ins)));
        fDofiInOutOR[ins][outs] = new QCheckBox ();
        QObject::connect (fDofiInOutOR[ins][outs], SIGNAL(toggled(bool)),this,SLOT (InOutOR_toggled(bool)));
        fDofiControlWidget->OutputORTableWidget->setCellWidget (ins, outs, fDofiInOutOR[ins][outs]);

        fDofiInOutAND[ins][outs] = new QCheckBox ();
        QObject::connect (fDofiInOutAND[ins][outs], SIGNAL(toggled(bool)),this,SLOT (InOutAND_toggled(bool)));

        fDofiControlWidget->OutputANDTableWidget->setCellWidget (ins, outs, fDofiInOutAND[ins][outs]);

      } // ins
    }    // outs


	  	 QObject::connect (fDofiControlWidget->SelectedANDcheckBox, SIGNAL(toggled(bool)),this,SLOT (InOutAND_selected_toggled(bool)));
	  	 QObject::connect (fDofiControlWidget->SelectedORcheckBox, SIGNAL(toggled(bool)),this,SLOT (InOutOR_selected_toggled(bool)));


	    fDofiControlWidget->update();


         fDofiControlWidget->show();
         QMdiSubWindow* subtrig=mdiArea->addSubWindow(fDofiControlWidget, wflags);
         subtrig->setAttribute(Qt::WA_DeleteOnClose, false);
       }

  if(fDofiScalerWidget)
        {
          fDofiScalerWidget->setWindowTitle("Scalers");

          // here initialize the tables:
          fDofiScalerWidget->ScalersTableWidget->setRowCount(DOFI_NUM_CHANNELS);
          for(int i=0; i<DOFI_NUM_CHANNELS;++i)
          {
            fDofiScalerWidget->ScalersTableWidget->setVerticalHeaderItem(i,new QTableWidgetItem(QString("%1").arg(i)));
            QTableWidgetItem* itemin= new QTableWidgetItem("init");
            itemin->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            fDofiScalerWidget->ScalersTableWidget->setItem(i,dofi_scaler_in, itemin);

            QTableWidgetItem* itemout= new QTableWidgetItem("init");
            itemout->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            fDofiScalerWidget->ScalersTableWidget->setItem(i,dofi_scaler_out, itemout);

            QTableWidgetItem* iteminrate= new QTableWidgetItem("init");
            iteminrate->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            fDofiScalerWidget->ScalersTableWidget->setItem(i,dofi_scaler_inrate, iteminrate);


            QTableWidgetItem* itemoutrate= new QTableWidgetItem("init");
            itemoutrate->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            fDofiScalerWidget->ScalersTableWidget->setItem(i,dofi_scaler_outrate, itemoutrate);

          }

          fScalerTimer = new QTimer (this);
            //fScalerTimer->setInterval (2000);
          QObject::connect (fScalerTimer, SIGNAL (timeout ()), this, SLOT (ScalerTimeout ()));
          QObject::connect (fDofiScalerWidget->ScalerTimerCheckBox, SIGNAL(stateChanged(int)), this, SLOT(ScalerTimer_changed(int)));


          fDofiScalerWidget->show();
          QMdiSubWindow* subtrig=mdiArea->addSubWindow(fDofiScalerWidget, wflags);
          subtrig->setAttribute(Qt::WA_DeleteOnClose, false);
        }


  if (fDofiInputsWidget)
  {
    fDofiInputsWidget->setWindowTitle ("Input signal shaping");
    // here initialize the tables:
    fDofiInputsWidget->InputsTableWidget->setRowCount (DOFI_NUM_CHANNELS);
    for (int i = 0; i < DOFI_NUM_CHANNELS; ++i)
    {

      fDofiInvertState[i]= new QRadioButton();
      fDofiInvertState[i]->setAutoExclusive(false);
      QObject::connect (fDofiInvertState[i], SIGNAL(toggled(bool)),this,SLOT (InvertState_toggled(bool)));
      fDofiInputsWidget->InputsTableWidget->setCellWidget (i, dofi_input_invert, fDofiInvertState[i]);

      fDofiDelayBox[i]= new QSpinBox();
      fDofiDelayBox[i]->setMinimum(0);
      fDofiDelayBox[i]->setMaximum(16777215);
      fDofiDelayBox[i]->setAccelerated(true);
      fDofiDelayBox[i]->setAlignment(Qt::AlignRight);
      fDofiDelayBox[i]->setDisplayIntegerBase(10); // later change this in refresh function?
      fDofiDelayBox[i]->setButtonSymbols(QAbstractSpinBox::PlusMinus);
      QObject::connect (fDofiDelayBox[i], SIGNAL(valueChanged(int)),this,SLOT (Delay_changed(int)));
      fDofiInputsWidget->InputsTableWidget->setCellWidget (i, dofi_input_delay, fDofiDelayBox[i]);


      fDofiLengthBox[i]= new QSpinBox();
      fDofiLengthBox[i]->setMinimum(0);
      fDofiLengthBox[i]->setMaximum(16777215);
      fDofiLengthBox[i]->setAccelerated(true);
      fDofiLengthBox[i]->setAlignment(Qt::AlignRight);
      fDofiLengthBox[i]->setDisplayIntegerBase(10); // later change this in refresh function?
      fDofiLengthBox[i]->setButtonSymbols(QAbstractSpinBox::PlusMinus);
      QObject::connect (fDofiLengthBox[i], SIGNAL(valueChanged(int)),this,SLOT (Length_changed(int)));
      fDofiInputsWidget->InputsTableWidget->setCellWidget (i, dofi_input_len, fDofiLengthBox[i]);



    }
    fDofiInputsWidget->All_delay_spinBox->setAlignment(Qt::AlignRight);
    fDofiInputsWidget->All_length_spinBox->setAlignment(Qt::AlignRight);

    QObject::connect (fDofiInputsWidget->All_Invertradio_Button, SIGNAL(toggled(bool)),this,SLOT (InvertState_selected_toggled(bool)));
    QObject::connect (fDofiInputsWidget->All_delay_spinBox, SIGNAL(valueChanged(int)),this,SLOT (Delay_selected_changed(int)));
    QObject::connect (fDofiInputsWidget->All_length_spinBox, SIGNAL(valueChanged(int)),this,SLOT (Length_selected_changed(int)));



    fDofiInputsWidget->show ();
    QMdiSubWindow *subtrig = mdiArea->addSubWindow (fDofiInputsWidget, wflags);
    subtrig->setAttribute (Qt::WA_DeleteOnClose, false);

  }


  ClearOutputBtn_clicked ();


QObject::connect (fDofiScalerWidget->ResetScalersButton, SIGNAL (clicked ()), this, SLOT (ResetScalersBtn_clicked ()));
QObject::connect (fDofiScalerWidget->StartScalersButton, SIGNAL (clicked ()), this, SLOT (StartScalersBtn_clicked ()));
QObject::connect (fDofiScalerWidget->StopScalersButton, SIGNAL (clicked ()), this, SLOT (StopScalersBtn_clicked ()));



ReadSettings();


fSetup=CreateSetup();
show();
}

DofiGui::~DofiGui ()
{
}







void DofiGui::ResetSlave ()
{
 printm("Did reset DOFI at %s:%d\n",fHost.toLatin1 ().constData (), fPort);
 MuppetSingleCommand(fHost, fPort, "-z");
}


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
RefreshScalers();
RefreshControlBits();
RefreshInputs ();
RefreshStatus();
}


void DofiGui::RefreshScalers ()
{
  QString text;
  QString pre;
  fNumberBase == 16 ? pre = "0x" : pre = "";
  theSetup_GET_FOR_SLAVE(DofiSetup);
  QTableWidgetItem *item = 0;
  qulonglong scaler;
  double rate = 0.0;
  for (int c = 0; c < DOFI_NUM_CHANNELS; ++c)
  {
    item = fDofiScalerWidget->ScalersTableWidget->item (c, dofi_scaler_in);
    scaler = theSetup->fInputScalers[c];
    text = pre + QString ("%1").arg (scaler, 0, fNumberBase);
    item->setText (text);

    item = fDofiScalerWidget->ScalersTableWidget->item (c, dofi_scaler_out);
    scaler = theSetup->fOutputScalers[c];
    text = pre + QString ("%1").arg (scaler, 0, fNumberBase);
    item->setText (text);

    item = fDofiScalerWidget->ScalersTableWidget->item (c, dofi_scaler_inrate);
    rate = theSetup->fInputRate[c];
    text = QString ("%1").arg (rate);
    item->setText (text);

    item = fDofiScalerWidget->ScalersTableWidget->item (c, dofi_scaler_outrate);
    rate = theSetup->fOutputRate[c];
    text = QString ("%1").arg (rate);
    item->setText (text);

  }

}

void DofiGui::RefreshControlBits ()
{
theSetup_GET_FOR_SLAVE(DofiSetup);
for (int outs = 0; outs < DOFI_NUM_CHANNELS; ++outs)
{

  for (int ins = 0; ins < DOFI_NUM_CHANNELS; ++ins)
  {
    bool isor = theSetup->IsOutputOR (outs, ins);
    fDofiInOutOR[ins][outs]->setChecked (isor);
    bool isand = theSetup->IsOutputAND (outs, ins);
    fDofiInOutAND[ins][outs]->setChecked (isand);

  }
}

}

void DofiGui::RefreshInputs ()
{
  theSetup_GET_FOR_SLAVE(DofiSetup);
  QString pre;
  fNumberBase == 16 ? pre = "0x" : pre = "";
  fDofiInputsWidget->All_delay_spinBox->setDisplayIntegerBase (fNumberBase);
  fDofiInputsWidget->All_delay_spinBox->setPrefix (pre);
  fDofiInputsWidget->All_length_spinBox->setDisplayIntegerBase (fNumberBase);
  fDofiInputsWidget->All_length_spinBox->setPrefix (pre);
  for (int ins = 0; ins < DOFI_NUM_CHANNELS; ++ins)
  {
    bool inverted = theSetup->IsInputInvert (ins);
    int len = theSetup->GetInputLength (ins);
    int delay = theSetup->GetInputDelay (ins);

    fDofiInvertState[ins]->setChecked (inverted);
    fDofiLengthBox[ins]->setValue (len);
    fDofiLengthBox[ins]->setDisplayIntegerBase (fNumberBase);
    fDofiLengthBox[ins]->setPrefix (pre);
    fDofiDelayBox[ins]->setValue (delay);
    fDofiDelayBox[ins]->setDisplayIntegerBase (fNumberBase);
    fDofiDelayBox[ins]->setPrefix (pre);
  }

}




void DofiGui::EvaluateView ()
{
  EvaluateSlave ();
  theSetup_GET_FOR_SLAVE(DofiSetup);
  for (int out = 0; out < DOFI_NUM_CHANNELS; ++out)
  {
    theSetup->fOutputORBits[out] = 0;
    theSetup->fOutputANDBits[out] = 0;
    for (int in = 0; in < DOFI_NUM_CHANNELS; ++in)
    {
      unsigned long long orset = (fDofiInOutOR[in][out]->isChecked() ? 1 : 0);
      theSetup->fOutputORBits[out] |= (orset << in);
      unsigned long long andset = (fDofiInOutAND[in][out]->isChecked() ? 1 : 0);
      theSetup->fOutputANDBits[out] |= (andset << in);
    }
  }

  // below the input shape elements:

  for (int in = 0; in < DOFI_NUM_CHANNELS; ++in)
     {
       theSetup->fSignalControl[in]=0;
       unsigned long long invertset = (fDofiInvertState[in]->isChecked() ? 1 : 0);
       theSetup->fSignalControl[in] |= (invertset & 0x1);
       unsigned long long delay = fDofiDelayBox[in]->value();
       theSetup->fSignalControl[in] |= (delay & 0xFFFFFF) << 16;
       unsigned long long len = fDofiLengthBox[in]->value();
       theSetup->fSignalControl[in] |= (len & 0xFFFFFF) << 40;

     }

}



void DofiGui::SetRegisters ()
{
  //std::cout << "DofiGui::SetRegisters()"<< std::endl;
// write register values from strucure with gosipcmd
  theSetup_GET_FOR_SLAVE(DofiSetup);

  // TODO: instead of single register writes, implement multiwrite in rdoficommand JAM 15-03-2023
  for(int outs=0; outs<DOFI_NUM_CHANNELS; ++outs)
  {
    unsigned long long value=theSetup->fOutputANDBits[outs];
    int address=DOFI_ANDCTRL_BASE + outs;
    int rev=WriteMuppet(fHost,fPort, address, value);
    if(rev)
    {
      printm("SetRegisters: Error writing 0x%lx to output AND register %d (0x%x)", value, outs, address);
    }
  }

  // here also multiwrite of 64 bit array starting with base
  for(int outs=0; outs<DOFI_NUM_CHANNELS; ++outs)
   {
     unsigned long long value=theSetup->fOutputORBits[outs];
     int address=DOFI_ORCTRL_BASE + outs;
     int rev=WriteMuppet(fHost,fPort, address, value);
     if(rev)
     {
       printm("SetRegisters: Error writing 0x%lx to output OR register %d (0x%x)", value, outs, address);
     }
   }

  // TODO: set the input shape/invert registers

  for(int ins=0; ins<DOFI_NUM_CHANNELS; ++ins)
     {
       unsigned long long value=theSetup->fSignalControl[ins];
       int address=DOFI_SIGNALCTRL_BASE + ins;
       int rev=WriteMuppet(fHost,fPort, address, value);
       if(rev)
       {
         printm("SetRegisters: Error writing 0x%lx to input signalctrl register %d (0x%x)", value, ins, address);
       }
     }


}

void DofiGui::GetRegisters ()
{
  //std::cout<<"DofiGui::GetRegisters ()" << std::endl;
// read register values from hardware into structure
int status=0;

theSetup_GET_FOR_SLAVE(DofiSetup);



status=MultiReadMuppet (fHost, fPort, DOFI_SIGNALCTRL_BASE, DOFI_NUM_CHANNELS,theSetup->fSignalControl);
if(status) printm("GetRegisters error %d when reading input signal registers!",status);

status=MultiReadMuppet (fHost, fPort, DOFI_ORCTRL_BASE, DOFI_NUM_CHANNELS,theSetup->fOutputORBits);
if(status) printm("GetRegisters error %d when reading output OR registers!",status);

status=MultiReadMuppet (fHost, fPort, DOFI_ANDCTRL_BASE, DOFI_NUM_CHANNELS,theSetup->fOutputANDBits);
if(status) printm("GetRegisters error %d when reading output AND registers!",status);


GetScalers();



}

void DofiGui::GetScalers ()
{
  int status=0;
  theSetup_GET_FOR_SLAVE(DofiSetup);
  theSetup->SaveScalers();// first save previous scaler values for rate calculations
  QTime nowtime=QTime::currentTime();
  int deltamillis=fScalerSampleTime.msecsTo(nowtime);
  //std::cout<<"DofiGui::GetScalers () with "<< deltamillis<<" ms difference to previous"<< std::endl;

  status=MultiReadMuppet (fHost, fPort, DOFI_INPUTSCALER_BASE, DOFI_NUM_CHANNELS,theSetup->fInputScalers);
  if(status) printm("GetRegisters error %d when reading input scalers!",status);
  status=MultiReadMuppet (fHost, fPort, DOFI_OUTPUTSCALER_BASE, DOFI_NUM_CHANNELS,theSetup->fOutputScalers);
  if(status) printm("GetRegisters error %d when reading output scalers!",status);

  theSetup->EvaluateRates(deltamillis/1000.0);
  fScalerSampleTime=nowtime;
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


void DofiGui::AutoApplyOR (int input, int output, bool on)
{
  // keep setup structure always consistent:
  theSetup_GET_FOR_SLAVE(DofiSetup);
  on ? theSetup->AddOutputOR (output, input) : theSetup->ClearOutputOR (output, input);

  // only write the output register that is concerned;
  unsigned long long value=theSetup->fOutputORBits[output];
  int address=   DOFI_ORCTRL_BASE + output;
  int rev=WriteMuppet(fHost, fPort, address, value);
  if(rev<0)
  {
    printm("Error writing to Mupppet in AutoApplyOR (%d,%d,%d)",output,input,on);
  }

}


void DofiGui::AutoApplyAND (int input, int output, bool on)
{
  // keep setup structure always consistent:
  theSetup_GET_FOR_SLAVE(DofiSetup);
  on ? theSetup->AddOutputAND (output, input) : theSetup->ClearOutputAND (output, input);

  // only write the output register that is concerned;
  unsigned long long value=theSetup->fOutputANDBits[output];
  int address=   DOFI_ANDCTRL_BASE + output;
  int rev=WriteMuppet(fHost, fPort, address, value);
  if(rev<0)
  {
    printm("Error writing to Mupppet in AutoApplyAND (%d,%d,%d)",output,input,on);
  }

}



void DofiGui::AutoApplyInvert (int input, bool on)
{
  theSetup_GET_FOR_SLAVE(DofiSetup);
  theSetup->SetInputInvert (input, on);
  unsigned long long reg = theSetup->fSignalControl[input];
  int address = DOFI_SIGNALCTRL_BASE + input;
  int rev = WriteMuppet (fHost, fPort, address, reg);
  if (rev < 0)
  {
    printm ("Error writing to Mupppet in AutoApplyInvert (%d,%d)", input, on);
  }
}

void DofiGui::AutoApplyDelay (int input, int value)
{
  theSetup_GET_FOR_SLAVE(DofiSetup);
  theSetup->SetInputDelay (input, value);
  unsigned long long reg = theSetup->fSignalControl[input];
  int address = DOFI_SIGNALCTRL_BASE + input;
  int rev = WriteMuppet (fHost, fPort, address, reg);
  if (rev < 0)
  {
    printm ("Error writing to Mupppet in AutoApplyDelay (%d,%d)", input, value);
  }
}

void DofiGui::AutoApplyLength (int input, int value)
{
  theSetup_GET_FOR_SLAVE(DofiSetup);
  theSetup->SetInputLength (input, value);
  unsigned long long reg = theSetup->fSignalControl[input];
  int address = DOFI_SIGNALCTRL_BASE + input;
  int rev = WriteMuppet (fHost, fPort, address, reg);
  if (rev < 0)
  {
    printm ("Error writing to Mupppet in AutoApplyLength (%d,%d)", input, value);
  }
}




void DofiGui::InOutOR_toggled(bool on)
{
  MUPPET_LOCK_SLOT
  int row=fDofiControlWidget->OutputORTableWidget->currentRow();
  int col=fDofiControlWidget->OutputORTableWidget->currentColumn();
  //std::cout<<"DofiGui::InOutOR_toggled("<<row<<","<<col<<","<<on<<")" << std::endl;
  MUPPET_AUTOAPPLY(AutoApplyOR(row,col,on));
  MUPPET_UNLOCK_SLOT
}

void DofiGui::InOutAND_toggled(bool on)
{
  MUPPET_LOCK_SLOT
  int row=fDofiControlWidget->OutputANDTableWidget->currentRow();
  int col=fDofiControlWidget->OutputANDTableWidget->currentColumn();
  //std::cout<<"DofiGui::InOutANDtoggled("<<row<<","<<col<<","<<on<<")" << std::endl;
  MUPPET_AUTOAPPLY(AutoApplyAND(row,col,on));
  MUPPET_UNLOCK_SLOT
}



void DofiGui::InOutOR_selected_toggled (bool on)
{
  MUPPET_LOCK_SLOT
  //std::cout << "DofiGui::InOutOR_selected_toggled(" << on << ")" << std::endl;
  QList < QTableWidgetSelectionRange > selection = fDofiControlWidget->OutputORTableWidget
      ->selectedRanges ();
  for (int i = 0; i < selection.size (); ++i)
  {
    QTableWidgetSelectionRange range = selection.at (i);
//    std::cout << "selection " << i << " is:(" << range.leftColumn () << "," << range.rightColumn ()
//        << "," << range.topRow () << "," << range.bottomRow () << ")" << std::endl;
    // here change checkboxes within ranges to the desired state:
    for (int row = range.topRow (); row <= range.bottomRow (); ++row)
    {
      for (int col = range.leftColumn (); col <= range.rightColumn (); ++col)
      {
        fDofiInOutOR[row][col]->setChecked(on);
        MUPPET_AUTOAPPLY(AutoApplyOR (row, col, on));
      }
    }
  }
  MUPPET_UNLOCK_SLOT
}


void DofiGui::InOutAND_selected_toggled (bool on)
{
  MUPPET_LOCK_SLOT
  //std::cout << "DofiGui::InOutAND_selected_toggled(" << on << ")" << std::endl;
  QList < QTableWidgetSelectionRange > selection = fDofiControlWidget->OutputANDTableWidget->selectedRanges ();
  for (int i = 0; i < selection.size (); ++i)
  {
    QTableWidgetSelectionRange range = selection.at (i);
//    std::cout << "selection " << i << " is:(" << range.leftColumn () << "," << range.rightColumn () << ","
//        << range.topRow () << "," << range.bottomRow () << ")" << std::endl;
    // here change checkboxes within ranges to the desired state:

    for (int row = range.topRow (); row <= range.bottomRow (); ++row)
    {
      for (int col = range.leftColumn (); col <= range.rightColumn (); ++col)
      {
        fDofiInOutAND[row][col]->setChecked(on);
        MUPPET_AUTOAPPLY(AutoApplyAND (row, col, on));
      }
    }
  }
  MUPPET_UNLOCK_SLOT
}


void DofiGui::InvertState_toggled(bool on)
{
  MUPPET_LOCK_SLOT
  int row=fDofiInputsWidget->InputsTableWidget->currentRow();
   //std::cout << "DofiGui::InvertState_toggled("<<row<<"," << on << ")" << std::endl;
   MUPPET_AUTOAPPLY(AutoApplyInvert(row,on));
  MUPPET_UNLOCK_SLOT
}


void DofiGui::InvertState_selected_toggled (bool on)
{
  MUPPET_LOCK_SLOT
  //std::cout << "DofiGui::InvertState_selected_toggled(" << on << ")" << std::endl;
  QList < QTableWidgetSelectionRange > selection = fDofiInputsWidget->InputsTableWidget
      ->selectedRanges ();
  for (int i = 0; i < selection.size (); ++i)
  {
    QTableWidgetSelectionRange range = selection.at (i);
//    std::cout << "selection " << i << " is:(" << range.leftColumn () << "," << range.rightColumn ()
//        << "," << range.topRow () << "," << range.bottomRow () << ")" << std::endl;
    // here change checkboxes within ranges to the desired state:
    for (int row = range.topRow (); row <= range.bottomRow (); ++row)
    {
      for (int col = range.leftColumn (); col <= range.rightColumn (); ++col)
      {
        if(col!=dofi_input_invert) continue;
        fDofiInvertState[row]->setChecked(on);
        MUPPET_AUTOAPPLY(AutoApplyInvert (row, on));
      }
    }
  }
  MUPPET_UNLOCK_SLOT
}




void DofiGui::Delay_changed(int value)
{
  MUPPET_LOCK_SLOT
   int row=fDofiInputsWidget->InputsTableWidget->currentRow();
  //std::cout << "DofiGui::Delay_changed("<<row<<"," << value << ")" << std::endl;
   MUPPET_AUTOAPPLY(AutoApplyDelay(row,value));
   MUPPET_UNLOCK_SLOT

}


void DofiGui::Delay_selected_changed(int value)
{
  MUPPET_LOCK_SLOT
  //std::cout << "DofiGui:::Delay_selected_changed(" << value << ")" << std::endl;
  QList < QTableWidgetSelectionRange > selection = fDofiInputsWidget->InputsTableWidget
      ->selectedRanges ();
  for (int i = 0; i < selection.size (); ++i)
  {
    QTableWidgetSelectionRange range = selection.at (i);
//    std::cout << "selection " << i << " is:(" << range.leftColumn () << "," << range.rightColumn ()
//        << "," << range.topRow () << "," << range.bottomRow () << ")" << std::endl;
    // here change checkboxes within ranges to the desired state:
    for (int row = range.topRow (); row <= range.bottomRow (); ++row)
    {
      for (int col = range.leftColumn (); col <= range.rightColumn (); ++col)
      {
        if(col!=dofi_input_delay) continue;
        fDofiDelayBox[row]->setValue(value);
        MUPPET_AUTOAPPLY(AutoApplyDelay(row, value));
      }

    }
  }
  MUPPET_UNLOCK_SLOT
}





void DofiGui::Length_changed(int value)
{
  MUPPET_LOCK_SLOT
   int row=fDofiInputsWidget->InputsTableWidget->currentRow();
  //std::cout << "DofiGui::Length_changed("<<row<<"," << value << ")" << std::endl;
    MUPPET_AUTOAPPLY(AutoApplyLength(row,value));
    MUPPET_UNLOCK_SLOT

}

void DofiGui::Length_selected_changed(int value)
{
  MUPPET_LOCK_SLOT
  //std::cout << "DofiGui:::Length_selected_changed(" << value << ")" << std::endl;
  QList < QTableWidgetSelectionRange > selection = fDofiInputsWidget->InputsTableWidget
      ->selectedRanges ();
  for (int i = 0; i < selection.size (); ++i)
  {
    QTableWidgetSelectionRange range = selection.at (i);
//    std::cout << "selection " << i << " is:(" << range.leftColumn () << "," << range.rightColumn ()
//        << "," << range.topRow () << "," << range.bottomRow () << ")" << std::endl;
    // here change checkboxes within ranges to the desired state:
    for (int row = range.topRow (); row <= range.bottomRow (); ++row)
    {
      for (int col = range.leftColumn (); col <= range.rightColumn (); ++col)
      {
      if(col!=dofi_input_len) continue;
        fDofiLengthBox[row]->setValue(value);
        MUPPET_AUTOAPPLY(AutoApplyLength(row, value));
      }

    }
  }
  MUPPET_UNLOCK_SLOT
}


void  DofiGui::ResetScalersBtn_clicked ()
{
  //std::cout<<"DofiGui:::ResetScalersBtn_clicked ()" << std::endl;
  MuppetSingleCommand(fHost, fPort, "-n");
}


void  DofiGui::StartScalersBtn_clicked ()
{
  //std::cout<<"DofiGui:::StartScalersBtn_clicked ()" << std::endl;
  MuppetSingleCommand(fHost, fPort, "-e");
}


void  DofiGui::StopScalersBtn_clicked ()
{
  //std::cout<<"DofiGui:::StopScalersBtn_clicked ()" << std::endl;
  MuppetSingleCommand(fHost, fPort, "-f");
}

void DofiGui::ScalerTimeout()
{
  //std::cout<<"DofiGui::ScalerTimeout()" << std::endl;
  // here get scalers and refresh ratemeters
  MUPPET_LOCK_SLOT
  GetScalers ();
  RefreshScalers();
  RefreshStatus();
  MUPPET_UNLOCK_SLOT

}

void DofiGui::ScalerTimer_changed (int on)
{
  MUPPET_LOCK_SLOT
  //std::cout << "ScalerTimer_changed to " <<  on << std::endl;
  if (on)
  {
    int period = 1000 * fDofiScalerWidget->ScalerSpinBox->value ();
    printm ("Scaler Timer has been started with %d ms period.", period);
    fScalerTimer->setInterval (period);
    fScalerTimer->start ();
  }
  else
  {
    fScalerTimer->stop ();
    printm ("Scaler Timer has been stopped.");
  }
  MUPPET_UNLOCK_SLOT
}



