#ifndef DOFIGUI_H
#define DOFIGUI_H

#include "MuppetGui.h"
#include "DofiControlWidget.h"
#include "DofiScalerWidget.h"
#include "DofiSetup.h"
#include <stdio.h>
#include <QProcess>
#include <QString>



class DofiGui: public MuppetGui
{
  Q_OBJECT

public:
  DofiGui (QWidget* parent = 0);
  virtual ~DofiGui ();


  virtual MuppetSetup* CreateSetup()
     {
       return new DofiSetup();
     }


protected:


  /** reference to the mdi embedded window with main controlss*/
  DofiControlWidget* fDofiControlWidget;

  /** reference to the embedded scaler displaywidget */
  DofiScalerWidget* fDofiScalerWidget;


  /** toggle general trigger state*/
  bool fTriggerOn;

  /** this flag controls if we want to have the QFW reset action on next refresh*/
  bool fDoResetQFW;


  /** update register display*/
  void RefreshView ();



 /** helper function for broadcast: get shown set up and put it immediately to hardware.*/
  void ApplyGUISettings();

//  /** helper function for broadcast: get shown set up and put it immediately to hardware.*/
//   void ApplyQFWSettings();
//
//   /** helper function for broadcast: get shown set up and put it immediately to hardware.*/
//   void ApplyDACSettings();
//
//
//   /** helper function for broadcast: get shown set up and put it immediately to hardware.*/
//   void ApplyFanSettings();
//
//   /** helper function for broadcast: get shown set up and put it immediately to hardware.*/
//   void ApplyCSASettings();

  /** helper function for broadcast: rest current poland slave*/
  virtual void ResetSlave ();

//  /** helper function for broadcast: do offset scan for current poland slave*/
//   void ScanOffsets ();

   /** helper function for broadcast: dump samples for current poland slave*/
   virtual void DumpSlave ();

  /** copy values from gui to internal status object*/
  void EvaluateView ();

//  /** copy gui contents of sensors tab to setup structure*/
//   void EvaluateFans();
//
//   /** copy gui contents of CSA tab to setup structure*/
//   void EvaluateCSA();


  /** copy sfp and slave from gui to variables*/
  //void EvaluateSlave ();

//  /** find out measurement mode from selected combobox entry.*/
//  void EvaluateMode();
//
//
//
//
//  /** update measurement range in combobox entry*/
//    void RefreshMode();

//    /** refresh view of general trigger state*/
//    void RefreshTrigger();

  /** set register from status structure*/
  void SetRegisters ();

  /** get register contents to status structure*/
  void GetRegisters ();



//  /** refresh view of temp/fan sensors from status structure*/
//   void RefreshSensors();
//
//
//  /** read temp sensors into status structure*/
//   void GetSensors ();
//
//   /** set fan speed value*/
//   void SetFans ();

//   /** set CSA properties value*/
//   void ApplyCSA();
//
//   /** Refresh view of CSA contents*/
//    void RefreshCSA();

  /** get registers and write them to config file*/
  void SaveRegisters();




//  /** Apply DAC setup to frontends*/
//  void ApplyDAC();
//
//  /** Refresh view of DAC contents*/
//  void RefreshDAC();
//
//  /** Refresh view of DAC mode*/
//  void RefreshDACMode();
//
//  /** Refresh widget (enable/disable) of DAC mode*/
//   void EnableDACModeWidgets(int mode);
//
//
//  /** copy gui contents of DAC tab to setup structure*/
//  void EvaluateDAC();
//
//  void GetSample(DofiSample* theSample);
//
//#ifdef USE_PEXOR_LIB
//// this function stolen and adopted from polandtest:
//int UnpackQfw (pexor::DMA_Buffer* tokbuf, DofiSample* theSample);
//#endif


public slots:



//  virtual void OffsetBtn_clicked ();
//  virtual void DACMode_changed(int ix);
//  virtual void TriggerBtn_clicked ();
//  virtual void QFWResetBtn_clicked();
//  virtual void QFW_changed ();
//  virtual void DAC_changed ();
//  virtual void Fan_changed ();
//  virtual void CSA_changed ();
//  virtual void CSA_spinbox_changed (int value);
//  virtual void CSA_lineEdit_changed();
//
//  virtual void ShowSample ();
};

#endif
