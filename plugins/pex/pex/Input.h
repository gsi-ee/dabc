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
#ifndef PEX_Input
#define PEX_Input

#include "dabc/DataIO.h"
#include "dabc/statistic.h"

namespace pex
{

class Device;

class Input: public dabc::DataInput
{
  friend class Device;
  friend class Transport;

public:
  Input (pex::Device*);
  virtual ~Input ();

protected:

  virtual unsigned Read_Size () override;

  virtual unsigned Read_Start (dabc::Buffer& buf) override;

  virtual unsigned Read_Complete (dabc::Buffer& buf) override;

  virtual double Read_Timeout () override;

  Device* fPexorDevice;    //< reference to device object that does all actions

  dabc::Ratemeter fErrorRate;

};
}

#endif
