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
#include "dabc/DataIO.h"

#include "dabc/Buffer.h"

dabc::Buffer* dabc::DataInput::ReadBuffer()
{
   unsigned sz = Read_Size();
    
   if (sz>di_ValidSize) return 0;
    
   dabc::Buffer* buf = dabc::Buffer::CreateBuffer(sz);
    
   if (buf==0) return 0;
   
   if (Read_Start(buf) != di_Ok) {
      dabc::Buffer::Release(buf); 
      return 0;
   }
    
   if (Read_Complete(buf) != di_Ok) {
      dabc::Buffer::Release(buf); 
      return 0;
   }
    
   return buf;
}
