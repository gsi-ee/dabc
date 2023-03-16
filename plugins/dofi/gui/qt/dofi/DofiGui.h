#ifndef DOFIGUI_H
#define DOFIGUI_H

#include "MuppetGui.h"
#include "DofiControlWidget.h"
#include "DofiScalerWidget.h"
#include "DofiSetup.h"
#include <stdio.h>
#include <QProcess>
#include <QString>
#include <QCheckBox>


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


  /** checkbox to define which input (first index) shall be routed to output (second index).
   * checked inputs are ORED for output*/
   QCheckBox* fDofiInOutOR[DOFI_NUM_CHANNELS][DOFI_NUM_CHANNELS];

  /** checkbox to define which input (first index) shall be routed to output (second index).
    * checked inputs are ANDEDED for output*/
   QCheckBox* fDofiInOutAND[DOFI_NUM_CHANNELS][DOFI_NUM_CHANNELS];


  /** toggle general trigger state*/
  bool fTriggerOn;

  /** this flag controls if we want to have the QFW reset action on next refresh*/
  bool fDoResetQFW;


  /** update register display*/
  void RefreshView ();



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





  /** get registers and write them to config file*/
  void SaveRegisters();

  /**do change of input-output OR relation if auto apply mode is on */
  void AutoApplyOR (int output, int input, bool on);

  /**do change of input-output AND relation if auto apply mode is on */
  void AutoApplyAND (int output, int input, bool on);



public slots:

virtual void InOutOR_toggled(bool on);
virtual void InOutAND_toggled(bool on);

virtual void InOutOR_selected_toggled(bool on);
virtual void InOutAND_selected_toggled(bool on);




//virtual void InOutOR_tablechanged(int,int,int,int);
//virtual void InOutAND_tablechanged(int,int,int,int);

//
//virtual void InOutOR_Cell_changed(int,int);
//virtual void InOutAND_Cell_changed(int,int);
//
//virtual void InOutOR_Cell_doubleclicked(int,int);
//virtual void InOutAND_Cell_doubleclicked(int,int);


virtual void ResetScalersBtn_clicked ();
virtual void StartScalersBtn_clicked ();
virtual void StopScalersBtn_clicked ();
};

#endif
