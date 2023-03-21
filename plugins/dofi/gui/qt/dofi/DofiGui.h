#ifndef DOFIGUI_H
#define DOFIGUI_H

#include "MuppetGui.h"
#include "DofiControlWidget.h"
#include "DofiScalerWidget.h"
#include "DofiInputsWidget.h"
#include "DofiSetup.h"
#include <stdio.h>
#include <QProcess>
#include <QString>
#include <QCheckBox>
#include <QSpinBox>
#include <QRadioButton>
#include <QTime>


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

  /** reference to the embedded input signal shape control */
   DofiInputsWidget* fDofiInputsWidget;

  /** checkbox to define which input (first index) shall be routed to output (second index).
   * checked inputs are ORED for output*/
   QCheckBox* fDofiInOutOR[DOFI_NUM_CHANNELS][DOFI_NUM_CHANNELS];

  /** checkbox to define which input (first index) shall be routed to output (second index).
    * checked inputs are ANDEDED for output*/
   QCheckBox* fDofiInOutAND[DOFI_NUM_CHANNELS][DOFI_NUM_CHANNELS];

   /** radiobutton in table that defines inverted state of input*/
   QRadioButton* fDofiInvertState[DOFI_NUM_CHANNELS];


   /** spinbox in table that defines input delay value*/
   QSpinBox* fDofiDelayBox[DOFI_NUM_CHANNELS];

   /** spinbox in table that defines input length value*/
   QSpinBox* fDofiLengthBox[DOFI_NUM_CHANNELS];



   /** timer for periodic update of scalers*/
    QTimer* fScalerTimer;

    /** most recent time of scaler update */
    QTime fScalerSampleTime;



  /** update register display*/
  void RefreshView ();

  void RefreshScalers ();

  void RefreshControlBits ();

  void RefreshInputs ();


  /** helper function for broadcast: rest current poland slave*/
  virtual void ResetSlave ();

//  /** helper function for broadcast: do offset scan for current poland slave*/
//   void ScanOffsets ();

   /** helper function for broadcast: dump samples for current poland slave*/
   virtual void DumpSlave ();

  /** copy values from gui to internal status object*/
  void EvaluateView ();


  /** set register from status structure*/
  void SetRegisters ();

  /** get register contents to status structure*/
  void GetRegisters ();


  /** get scaler registers only*/
    void GetScalers ();


  /** get registers and write them to config file*/
  void SaveRegisters();

  /**do change of input-output OR relation if auto apply mode is on */
  void AutoApplyOR (int output, int input, bool on);

  /**do change of input-output AND relation if auto apply mode is on */
  void AutoApplyAND (int output, int input, bool on);

  /** change invert state for input channel if auto apply mode is on*/
  void AutoApplyInvert(int input, bool on);

  /** change signal delay for input channel if auto apply mode is on*/
   void AutoApplyDelay(int input, int value);

   /** change signal length for input channel if auto apply mode is on*/
   void AutoApplyLength(int input, int value);



public slots:

virtual void InOutOR_toggled(bool on);
virtual void InOutAND_toggled(bool on);
virtual void InOutOR_selected_toggled(bool on);
virtual void InOutAND_selected_toggled(bool on);

virtual void InvertState_toggled(bool on);
virtual void Delay_changed(int value);
virtual void Length_changed(int value);
virtual void InvertState_selected_toggled(bool on);
virtual void Delay_selected_changed(int value);
virtual void Length_selected_changed(int value);

virtual void ResetScalersBtn_clicked ();
virtual void StartScalersBtn_clicked ();
virtual void StopScalersBtn_clicked ();

virtual void ScalerTimeout();
virtual void ScalerTimer_changed (int on);


};

#endif
