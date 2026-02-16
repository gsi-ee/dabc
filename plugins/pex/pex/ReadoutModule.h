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

namespace pex
{

class ReadoutModule: public dabc::ModuleAsync
{

public:

  ReadoutModule (const std::string name, dabc::Command cmd);

  virtual void BeforeModuleStart () override;
  virtual void AfterModuleStop () override;

  virtual void ProcessInputEvent (unsigned port) override;
  virtual void ProcessOutputEvent (unsigned port) override;

  virtual int ExecuteCommand(dabc::Command cmd) override;


protected:
  void SetInfo(const std::string& info, bool forceinfo);
  void DoPexorReadout ();

  /** change file on/off state in application*/
  void ChangeFileState(bool on);

  std::string fEventRateName;
  std::string fDataRateName;
  std::string fInfoName;
  std::string fFileStateName;

};
}

#endif
