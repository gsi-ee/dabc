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

#ifndef DABC_SocketDevice
#define DABC_SocketDevice

#ifndef DABC_Device
#include "dabc/Device.h"
#endif

#ifndef DABC_SocketThread
#include "dabc/SocketThread.h"
#endif

namespace dabc {

   class NewConnectRec;
   class SocketProtocolAddon;

   /** \brief %Device for establishing socket connections
    *
    * \ingroup dabc_all_classes
    */

   class SocketDevice : public Device {

      friend class SocketProtocolAddon;
      friend class SocketCommandChannel;

      public:
         SocketDevice(const std::string& name);
         virtual ~SocketDevice();

         virtual std::string RequiredThrdClass() const { return typeSocketThread; }

         virtual bool StartServerAddon(Command cmd, std::string& servid);

      protected:

         virtual double ProcessTimeout(double last_diff);

         virtual int ExecuteCommand(Command cmd);

         bool CleanupRecs(double tmout);

         void AddRec(NewConnectRec* rec);

         void DestroyRec(NewConnectRec* rec, bool res);

         NewConnectRec* _FindRec(const char* connid);

         void ServerProtocolRequest(SocketProtocolAddon* proc, const char* inmsg, char* outmsg);

         bool ProtocolCompleted(SocketProtocolAddon* proc, const char* inmsg);

         void RemoveProtocolAddon(SocketProtocolAddon* proc, bool res);

         int HandleManagerConnectionRequest(Command cmd);

         virtual const char* ClassName() const { return dabc::typeSocketDevice; }

         PointersVector         fConnRecs; // list of connections recs
         PointersVector         fProtocols; // list of protocol start processors
         long                   fConnCounter;
   };

}

#endif
