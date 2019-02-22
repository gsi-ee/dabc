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
#ifndef USER_Factory
#define USER_Factory

#include "dabc/Factory.h"

namespace user
{

class Factory: public dabc::Factory
{
public:

  Factory (const std::string name) :
      dabc::Factory (name)
  {
  }
  virtual dabc::DataInput* CreateDataInput(const std::string& typ);


  virtual void Initialize();

};

}    // namespace

#endif

