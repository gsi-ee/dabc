#ifndef MUPPETGUI_H
#define MUPPETGUI_H

//#include "ui_MuppetGui.h"
#include "ui_MuppetMainwindow.h"
#include <stdio.h>
#include <stdint.h>
#include <QProcess>
#include <QString>
#include <QSettings>

class QSignalMapper;



#define MUPPET_BROADCAST_ACTION(X) X



// JAM2020: redefine this macros for usage in non-subclasses
#define MUPPET_LOCK_SLOT \
if(MuppetGui::fInstance->IsSlotGuard()) return; \
MuppetGui::fInstance->SetSlotGuard(true);

#define MUPPET_UNLOCK_SLOT \
MuppetGui::fInstance->SetSlotGuard(false);


/** JAM2018: the following pattern is used in most of the gui slots for autoapply:*/
#define MUPPET_AUTOAPPLY(X)\
if (IsAutoApply())\
     {\
       MUPPET_BROADCAST_ACTION(X);\
     }\

#define theSetup_GET_FOR_SLAVE(X) \
    X* theSetup = dynamic_cast<X*>(fSetup); \
    if(theSetup==0) {\
      printm("--- Could not get setup structure X for %s:%d ",fHost,fPort); \
      return;\
    }

#define theSetup_GET_FOR_SLAVE_RETURN(X) \
    X* theSetup = dynamic_cast<X*>(fSetup); \
    if(theSetup==0) {\
      printm("--- Could not get setup structure X for %s:%d ",fHost,fPort); \
      return -1;\
    }


/** also need to declare this here without library include!*/
 void printm (char *, ...);



/** the (A)BC for all frontend setup structures */
class MuppetSetup
{

  public:

  MuppetSetup(){;}
  virtual ~MuppetSetup(){;}
  virtual void Dump(){printm("Empty status structure - Please implement a new frontend first");}

};

enum MuppetTextColor_t
{
  muppet_black,
  muppet_red,
  muppet_green,
  muppet_yellow,
  muppet_blue,
  muppet_red_background,
  muppet_green_background,
  muppet_yellow_background,
  muppet_blue_background
};



class MuppetGui: public QMainWindow, public Ui::MuppetMainWindow
{
  Q_OBJECT

public:
  MuppetGui (QWidget* parent = 0);
  virtual ~MuppetGui ();

  int GetNumberBase(){return fNumberBase;}


  bool IsAutoApply(){return checkBox_AA->isChecked();}


  void AppendTextWindow (const QString& text);

  void AppendTextWindow (const char *txt)
         {
           QString buf (txt);
           AppendTextWindow (buf);
         }

  void FlushTextWindow();


  /** singleton pointer to forward mbspex lib output, also useful without mbspex lib:*/
static MuppetGui* fInstance;


	bool IsSlotGuard(){
		return fSlotGuard;
	}

	void SetSlotGuard(bool on){
		fSlotGuard=on;
	}





protected:

#if QT_VERSION >= QT_VERSION_CHECK(4,6,0)
  QProcessEnvironment fEnv;
#endif


  /** handle mainwindow settings by this*/
  QSettings* fSettings;

  /* for mdi windows Signals*/
  QSignalMapper*     fWinMapper;

  /** Name of the gui implementation*/
  QString fImplementationName;

  /** Versionstring to be printed on terminal*/
  QString fVersionString;

   /* setup structure. for gui framework use pointers + factory method*/
  MuppetSetup* fSetup;


  /** text debug mode*/
  bool fDebug;

  /** save configuration file instead of setting device values*/
  bool fSaveConfig;

  /** protect mutually dependent slots by this.*/
   bool fSlotGuard;

   /** indicate full screen mode*/
   bool fFullScreen;


   /** may change display of subwindows (tabs=true, separate windows=false)mode*/
   bool fMdiTabViewMode;

  /** base for number display (10 or 16)*/
  int fNumberBase;


  /** Hostname for the DABC command server*/
  QString fHost;

/** TCP port for the DABC command server*/
  int fPort;


  /** configuration output file handle*/
    FILE* fConfigFile;


  /** open configuration file for writing*/
    int OpenConfigFile(const QString& fname);

    /** guess what...*/
    int CloseConfigFile();

    /** append text to currently open config file*/
    int WriteConfigFile(const QString& text);


//  /** update febex device index display*/
  void RefreshStatus ();

  /** Change text and background color on given label*/
  void RefreshColouredLabel(QLabel* label, const QString text, MuppetTextColor_t color);

// /** update initilized chain display and slave limit*/
//  void RefreshChains();


//  /** copy hostname and port from gui to variables*/
  void EvaluateSlave ();
//
//  /** retrieve slave configuration from driver*/
//  void GetSFPChainSetup();
//

  /** Read from spi address at raspi host:port, returns value*/
  unsigned long long ReadMuppet (QString& host, int port, int address);

  /** Read from spi address next times words at raspi host:port. values are written into destbuffer*/
  int MultiReadMuppet (QString& host, int port, int baseaddress, int times, unsigned long long* destbuffer);

  /** Write value to spi address at raspi host:port*/
  int WriteMuppet (QString& host, int port, int address, unsigned long long value);

  /** Save value to currently open *.dof configuration file*/
  int SaveMuppet (QString& host, int port, int address, unsigned long long value);

  /** Execute a single command with simple argument passed as flag string */
  int MuppetSingleCommand (QString& host, int port, QString flag);

  /** execute (rdofi) command in shell. Return value is output of command*/
  QString ExecuteMuppetCmd (QString& command,  int timeout=5000);



  void DebugTextWindow (const char*txt)
  {
    if (fDebug)
      AppendTextWindow (txt);
  }
  void DebugTextWindow (const QString& text)
  {
    if (fDebug)
      AppendTextWindow (text);
  }


  /**
   * The following methods are the interface to the implementation:
   */

  /** update gui display from status structure*/
  virtual void RefreshView ();

  /** put gui values into status structure*/
   virtual void EvaluateView ();

  /** get register contents from hardware to status structure*/
   virtual void GetRegisters ();

   /** set register contents from status structure to hardware*/
   virtual void SetRegisters ();

   /** evaluate gui display and put it immediately to hardware.
    * default beaviour is using just  EvaluateView() and SetRegisters (),
    * this may be overwritten*/
   virtual void ApplyGUISettings();


   /** get registers and write them to config file*/
   virtual void SaveRegisters();

   /** generic reset/init call for sfp slave*/
   virtual void ResetSlave();

   /** generic reset/init call for dumpint sfp slave registers*/
   virtual void DumpSlave();

   /** save configuration to a file*/
   virtual void SaveConfig();


   /** apply configuration from a file to the hardware.
    * Subclass implementation may call this functions with gosipwait  time in microseconds
    * for crucial frontend timing*/
   virtual void ApplyFileConfig(int gosipwait=0);


   /* Factory method for the frontend setup structures.
    * To be re-implemented in subclass*/
   virtual MuppetSetup* CreateSetup()
     {
       return new MuppetSetup();
     }



   virtual void closeEvent( QCloseEvent * ce );



   void storePanelGeometry(QWidget* w, const QString& kind);

   QSize lastPanelSize(const QString& kind, int dfltwidth=450, int dfltheight=250);

   QPoint lastPanelPos(const QString& kind);


public slots:
  virtual void ShowBtn_clicked();
  virtual void ApplyBtn_clicked ();
  virtual void ResetSlaveBtn_clicked ();
  virtual void TestBtn_clicked ();
  virtual void DumpBtn_clicked ();
  virtual void ClearOutputBtn_clicked ();
  virtual void ConfigBtn_clicked ();
  virtual void SaveConfigBtn_clicked ();
  virtual void DebugBox_changed (int on);
  virtual void HexBox_changed(int on);

  virtual void about();
  void windowsMenuAboutToShow();
  void windowsMenuActivated( int id );
  void MinAllWindows();
  void ToggleFullScreenSlot();
  void ToggleSubwindowModeSlot();

  /** read Qt settings (window size etc.)*/
   virtual void ReadSettings();

   /** write Qt settings (window size etc.)*/
   virtual void WriteSettings();


};

#endif
