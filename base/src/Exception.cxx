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
#include "dabc/Exception.h"

#include "dabc/logging.h"

dabc::Exception::Exception() throw() : 
   std::exception(),
   fWhat("exception") 
{ 
   DOUT2(( "Generic exception")); 
}

dabc::Exception::Exception(const char* info) throw() : 
   std::exception(),
   fWhat(info) 
{ 
   DOUT2(( "Exception %s", info)); 
}

dabc::Exception::~Exception() throw() 
{
}
