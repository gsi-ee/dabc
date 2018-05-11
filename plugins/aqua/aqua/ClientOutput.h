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

   /** \brief Client transport for A1 DAQ
    *
    **/

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
         double                fReconnectTmout;
         std::string           fServerName;
         int                   fServerPort;

         SendHeader fSendHdr;

         OutputState           fState;
         int64_t               fBufCounter;

         virtual void OnThreadAssigned();
         virtual void OnSendCompleted();
         virtual void OnRecvCompleted();

         virtual void OnSocketError(int errnum, const std::string &info);

         virtual double ProcessTimeout(double lastdiff);

         virtual WorkerAddon* Write_GetAddon() { return this; }

         bool ConnectAquaServer();

         void MakeCallBack(unsigned arg);


      public:

         ClientOutput(dabc::Url& url);
         virtual ~ClientOutput();

         virtual bool Write_Init();

         virtual unsigned Write_Check();
         virtual unsigned Write_Buffer(dabc::Buffer& buf);
         virtual unsigned Write_Complete();

         virtual double Write_Timeout();


   };
}


#endif
