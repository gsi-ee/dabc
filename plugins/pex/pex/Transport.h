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
#ifndef PEX_Transport
#define PEX_Transport

#include "dabc/DataTransport.h"
#include "dabc/statistic.h"

namespace pex
{

class Device;
class Input;

class Transport: public dabc::InputTransport
{
  friend class Device;
  friend class Input;

public:
  Transport (pex::Device*, pex::Input* inp, dabc::Command cmd, const dabc::PortRef& inpport);
  virtual ~Transport ();

protected:

  virtual void ProcessPoolChanged (dabc::MemoryPool* pool) override;

  pex::Device* fPexorDevice;

  pex::Input* fPexorInput;

  virtual bool StartTransport () override;

  virtual bool StopTransport () override;

};
}

#endif
