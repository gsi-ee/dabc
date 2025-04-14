// $Id$

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

#ifndef AQUA_ClientOutput
#define AQUA_ClientOutput


#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

namespace aqua {

   /** \brief Client transport for A1 DAQ */

   struct SendHeader {
      int64_t counter;
      int64_t bufsize;
   };

   class ClientOutput : public dabc::SocketIOAddon,
                        public dabc::DataOutput {
      protected:

         enum OutputState {
            oDisconnected,        // when server not connected
            oReady,               // socket ready to send new data
            oSendingBuffer,       // when buffer can be send
            oCompleteSend,        // when send was completed
            oError                // error state
         };

         dabc::TimeStamp       fLastConnect;
         double                fReconnectTmout{0};
         std::string           fServerName;
         int                   fServerPort{0};

         SendHeader fSendHdr;

         OutputState           fState{oDisconnected};
         int64_t               fBufCounter{0};

         void OnThreadAssigned() override;
         void OnSendCompleted() override;
         void OnRecvCompleted() override;

         void OnSocketError(int errnum, const std::string &info) override;

         double ProcessTimeout(double lastdiff) override;

         WorkerAddon* Write_GetAddon() override { return this; }

         bool ConnectAquaServer();

         void MakeCallBack(unsigned arg);

      public:

         ClientOutput(dabc::Url& url);
         virtual ~ClientOutput();

         bool Write_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd) override;
         unsigned Write_Check() override;
         unsigned Write_Buffer(dabc::Buffer& buf) override;
         unsigned Write_Complete() override;
         double Write_Timeout() override;

   };
}


#endif
