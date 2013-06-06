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

#ifndef DABC_SocketTransport
#define DABC_SocketTransport

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

#ifndef DABC_NetworkTransport
#include "dabc/NetworkTransport.h"
#endif

#ifndef DABC_Queue
#include "dabc/Queue.h"
#endif

namespace dabc {

   /** \brief Specific implementation of network transport for socket
    *
    * \ingroup dabc_all_classes
    */

   class SocketNetworkInetrface : public SocketIOAddon,
                                  public NetworkInetrface {
      protected:

         typedef Queue<uint32_t> RecIdsQueue;

         char*       fHeaders;
         RecIdsQueue fSendQueue;
         RecIdsQueue fRecvQueue;
         int         fRecvStatus;  ///< 0 - idle, 1 - header, 2 - front buffer from recv queue
         uint32_t    fRecvRecid;   ///< if of the record, used for data receiving (status != 0)
         int         fSendStatus;  ///< 0 - idle, 1 - sending
         uint32_t    fSendRecid;   ///< id of the active send record

         std::string fMcastAddr;   ///< mcast address
         int         fMcastPort;   ///< mcast port
         bool        fMcastRecv;   ///< is mcast recv

         virtual long Notify(const std::string&, int);

         virtual void OnSendCompleted();

         virtual void OnRecvCompleted();

      public:
         SocketNetworkInetrface(int fd, bool datagram = false);
         virtual ~SocketNetworkInetrface();

         /** \brief Set mcast address, required to correctly close socket */
         void SetMCastAddr(const std::string addr, int port, bool recv)
         {
            fMcastAddr = addr;
            fMcastPort = port;
            fMcastRecv = recv;
         }

         virtual void AllocateNet(unsigned fulloutputqueue, unsigned fullinputqueue);
         virtual void SubmitSend(uint32_t recid);
         virtual void SubmitRecv(uint32_t recid);

   };
}

#endif
