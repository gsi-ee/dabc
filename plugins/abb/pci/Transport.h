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
#ifndef PCI_Transport
#define PCI_Transport

#include "dabc/DataTransport.h"

namespace pci {

   class BoardDevice;

   class Transport : public dabc::DataTransport {
      friend class BoardDevice;

      public:
         Transport(BoardDevice* dev, dabc::Port* port);
         virtual ~Transport();

      protected:

         virtual unsigned Read_Size();

         virtual unsigned Read_Start(dabc::Buffer* buf);

         virtual unsigned Read_Complete(dabc::Buffer* buf);

         virtual bool WriteBuffer(dabc::Buffer* buf);

         virtual void ProcessPoolChanged(dabc::MemoryPool* pool);

         BoardDevice* fPCIDevice;

   };
}

#endif
