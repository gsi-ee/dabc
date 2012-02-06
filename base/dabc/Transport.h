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

#ifndef DABC_Transport
#define DABC_Transport

#ifndef DABC_threads
#include "dabc/threads.h"
#endif

#ifndef DABC_Reference
#include "dabc/Reference.h"
#endif

#include <stdint.h>

namespace dabc {

   class Buffer;
   class Port;
   class MemoryPool;
   class Worker;

   extern const unsigned AcknoledgeQueueLength;

   class Transport {

      friend class Port;

      enum ETransportState {
         stNormal,       /** After transport created */
         stDeleting,     /** Transport deletion is started */
         stError         /** Error was detected, object starts its deletion */
      };

      protected:
         Mutex           fPortMutex;           //!< mutex should be used when port used outside thread

         Reference       fPort;                //!< reference on the port,

         Reference       fPool;                //!< reference on the memory pool

         ETransportState fTransportState;      //!< this status used together with port mutex only

         static long     gNumTransports;

         /** \brief Transport constructor, one need port before transport can be created */
         Transport(Reference port);

         /** \brief Returns port pointer.
          * When used outside the transport thread, fPortMutex should be locked
          * otherwise port may disappear in meanwhile */
         inline dabc::Port* GetPort() const { return (dabc::Port*) fPort(); }

         /** Method use to obtain pool of the port*/
         inline dabc::MemoryPool* GetPool() const { return (dabc::MemoryPool*) fPool(); }


         /** Method should be called by any transport implementation
          * to register dependencies between transport, port and memory pool */
         void RegisterTransportDependencies(Object* obj);

         /** \brief Method called when port confirms assignment - this is final step
          * Be aware that call coming from  port thread */
         virtual void PortAssigned() {}

         /** \brief Method used to produce input event for the port
          * Port mutex should be specified only when call performed not from transport thread
          * It is possible while port reference can only be changed inside thread.
          * Method returns true if port was fired and event was delivered to the port */
         bool FirePortInput(bool withmutex = false);

         /** \brief Method used to produce output event for the port
          * Port mutex should be specified only when call performed not from transport thread
          * It is possible while port reference can only be changed inside thread
          * Method returns true if port was fired and event was delivered to the port */
         bool FirePortOutput(bool withmutex = false);

         /** Methods called from Port to inform, that module starts/stops data processing.
           * One should care that call come from other thread that transport has */
         virtual void StartTransport() {}
         virtual void StopTransport() {}

         /** Method should be called from Cleanup method of worker */
         virtual void CleanupTransport();

         /** Method should be called by transport workers to remove port references
          * If true is returned, transport can be deleted while no port is exists */
         virtual void CleanupFromTransport(Object* obj);

         /** Method should be called every time when transport would like to close itself
          * Should be called from worker thread context. If true returns, one can call start object destroy */
         virtual void CloseTransport(bool witherr = false);

         /** \brief Internal DABC method. Used to start destroyment of transport itself.
          * By default it is supposed that worker calls here DeleteThis() method.
          * For special case method should be reimplemented.
          */
         virtual void DestroyTransport() {}

         /** \brief Return true if transport working.
          * Should be used only from transport thread.
          * One should check this flag in event processing to exclude some activity after
          * transport is going to shutdown state */
         bool IsTransportWorking() const { return fTransportState == stNormal; }

         /** \brief Return true if transport starts closing with error flag,
          * should be called only from transport thread */
         bool IsTransportErrorFlag() const { return fTransportState == stError; }

         /** Method called from port thread to confirm that port is accepting transport
          * If transport return false, port will delete instance via handle
          * Normally method is final stage of Port::AssignTransport call. */
         bool SetPort(Port* port);


         // FIXME: Do we need reference here

         virtual int GetParameter(const char* name) { return 0; }

      public:
         virtual ~Transport();

         virtual bool ProvidesInput() { return false; }
         virtual bool ProvidesOutput() { return false; }

         virtual bool Recv(Buffer&) = 0;
         virtual unsigned  RecvQueueSize() const = 0;
         virtual Buffer& RecvBuffer(unsigned indx) const = 0;
         virtual bool Send(const Buffer&) = 0;
         virtual unsigned SendQueueSize() = 0;
         virtual unsigned MaxSendSegments() = 0;

         /** \brief Method should return worker, which is used by transport.
          * In typical case it is just same object as transport itself (via double inheritance) */
         virtual Worker* GetWorker() = 0;

         static long NumTransports() { return gNumTransports; }
   };



#define DABC_TRANSPORT(WorkerClass)                  \
   protected:                                   \
      virtual void ObjectDestroyed(Object* obj) \
      {                                         \
         /** cleanup transport as well  */      \
         WorkerClass::ObjectDestroyed(obj);     \
         CleanupFromTransport(obj);             \
      }                                         \
                                                \
      virtual void DestroyTransport()           \
      {                                         \
         DeleteThis();                          \
      }                                         \
                                                \
      virtual void ObjectCleanup()              \
      {                                         \
         CleanupTransport();                    \
         WorkerClass::ObjectCleanup();          \
      }                                         \
                                                \
   public:                                      \
                                                \
      virtual Worker* GetWorker() { return this; }


};

#endif
