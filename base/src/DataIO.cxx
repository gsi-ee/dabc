/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "dabc/DataIO.h"

#include "dabc/Buffer.h"

dabc::Buffer dabc::DataInput::ReadBuffer()
{
   unsigned sz = Read_Size();

   dabc::Buffer buf;

   if (sz>di_ValidSize) return buf;
    
   buf = dabc::Buffer::CreateBuffer(sz);
    
   if (buf.null()) return buf;
   
   if (Read_Start(buf) != di_Ok) {
      buf.Release();
      return buf;
   }
    
   if (Read_Complete(buf) != di_Ok) {
      buf.Release();
      return buf;
   }
    
   return buf;
}
