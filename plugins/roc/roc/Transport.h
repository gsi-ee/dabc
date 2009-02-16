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
#ifndef ROC_Transport
#define ROC_Transport

#include "dabc/DataTransport.h"

class SysCoreBoard;

namespace roc {
   class Device;

   class Transport : public dabc::DataTransport {
      friend class Device;

      public:
         Transport(roc::Device* dev, dabc::Port* port, int bufsize, const char* boardIp, int trWindow);
         virtual ~Transport();

         SysCoreBoard* GetBoard() const { return fxBoard; }

         void ComplteteBufferReading();

      protected:

         virtual void StartTransport();

         virtual void StopTransport();


         virtual unsigned Read_Size();

         virtual unsigned Read_Start(dabc::Buffer* buf);

         virtual unsigned Read_Complete(dabc::Buffer* buf);

         virtual double GetTimeout() { return 0.01; }

      private:

         /** shortcut pointer to board object for this transport */
         SysCoreBoard* fxBoard;

         /** index of board in controller */
         unsigned fuBoardId;

         int      fBufferSize; // configured buffer size
         int      fTransWidnow; // transport window
         int      fReqNumMsgs; // required number of messages

         // for integrity checks
         bool     fIntegrityChecks;
         unsigned fLastEvent;
         bool     fIsLastEvent;
         unsigned fLastEventStep;
         unsigned fLastEpoch;
   };

}

#endif
