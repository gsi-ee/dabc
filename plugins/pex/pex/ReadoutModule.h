/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#ifndef PEX_ReadoutModule
#define PEX_ReadoutModule

#include "dabc/ModuleAsync.h"

#include "dabc/statistic.h"
#include "mbs/Iterator.h"

namespace pex
{

extern  const char *xmlUseFsq;
extern  const char *xmlFsqServer;
extern  const char *xmlFsqFilesystem;
extern  const char *xmlFsqDestination;
extern  const char *xmlFsqPort;
extern const char *xmlFsqNode;
extern  const char *xmlFsqPass;
extern const char *xmlRun;



class ReadoutModule: public dabc::ModuleAsync
{

public:

  ReadoutModule (const std::string name, dabc::Command cmd);

  virtual void BeforeModuleStart () override;
  virtual void AfterModuleStop () override;

//  virtual void ProcessInputEvent (unsigned port) override;
//  virtual void ProcessOutputEvent (unsigned port) override;
  bool ProcessRecv(unsigned) override { return DoPexorReadout(); }
  bool ProcessSend(unsigned) override { return DoPexorReadout(); }

  virtual int ExecuteCommand(dabc::Command cmd) override;


protected:
  void SetInfo(const std::string& info, bool forceinfo);
  bool DoPexorReadout ();

  /** change file on/off state in application*/
  void ChangeFileState(bool on);

  /** Send current output buffer further. Inspired by mbs CombinerModule*/
  bool FlushBuffer();

  void ProcessTimerEvent(unsigned) override;

  std::string fEventRateName;
  std::string fDataRateName;
  std::string fInfoName;
  std::string fFileStateName;

  /** switch usage of fsq storage on/off */
  bool fUseFsq;

  /** FSQ server hostname */
  std::string fFsqServer;

  /** FSQ server socket port */
  int fFsqPort;

  /** FSQ storage destination:  0 - null, 1 - only FSQ server local, 2 - lustre, 3 -tsm, 4 - lustre+tsm,  */
  int fFsqDestination;

  /** "filesystem" prefix for FSQ storate path */
  std::string fFsqFilesystem;

  /** FSQ node (user name) */
  std::string fFsqNode;

  /** FSQ password for user name */
  std::string fFsqPass;


  /** current Run number */
  int fRunNum;

  /** remember previous file name to optionally increment run number */
  std::string fLastFileName;

  /** handle content of output buffer with write iterator class*/
  mbs::WriteIterator              fOut;

  /** used for output buffer flush timer */
  bool               fFlushFlag;

  /** period of output buffer flushing timer */
  double             fFlushTimeout;

};
}

#endif
