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

#ifndef DABC_DataIOTransport
#define DABC_DataIOTransport

#ifndef DABC_DataTransport
#include "dabc/DataTransport.h"
#endif

namespace dabc {
   
   class DataIOTransport : public DataTransport {
      
      public:  
         DataIOTransport(Reference port, DataInput* inp, DataOutput* out);
         virtual ~DataIOTransport();

      protected:
      
         virtual unsigned Read_Size();
         virtual unsigned Read_Start(Buffer& buf);
         virtual unsigned Read_Complete(Buffer& buf);
         virtual double Read_Timeout();
         
         virtual bool WriteBuffer(const Buffer& buf);
         virtual void FlushOutput();
         
         // these are extra methods for handling inputs/outputs
         virtual void CloseInput();
         virtual void CloseOutput();
      
         DataInput         *fInput; 
         DataOutput        *fOutput;
   };

};

#endif
