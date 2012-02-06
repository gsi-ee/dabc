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

#ifndef DABC_Port
#define DABC_Port

#ifndef DABC_ModuleItem
#include "dabc/ModuleItem.h"
#endif

#ifndef DABC_Transport
#include "dabc/Transport.h"
#endif

#ifndef DABC_Buffer
#include "dabc/Buffer.h"
#endif

#ifndef DABC_ConnectionRequest
#include "dabc/ConnectionRequest.h"
#endif

namespace dabc {

   class Port;
   class PortRef;
   class Module;
   class PoolHandle;

   class PortException : public ModuleItemException {
      public:
         PortException(Port* port, const char* info) throw();
         Port* GetPort() const throw();
   };

   class PortInputException : public PortException {
      public:
         PortInputException(Port* port, const char* info = "Input Error") :
            PortException(port, info)
         {
         }
   };

   class PortOutputException : public PortException {
      public:
         PortOutputException(Port* port, const char* info = "Output Error") :
            PortException(port, info)
         {
         }
   };

   class Port : public ModuleItem {

      friend class Module;
      friend class Transport;
      friend class PortRef;

      protected:
         Reference         fPoolHandle; //!< reference on the pool handle to get some configuration parameters
         Reference         fPool;       //!< reference on the pool itself to ensure that pool will remain all the time
         unsigned          fInputQueueCapacity;
         unsigned          fInputPending;
         unsigned          fOutputQueueCapacity;
         unsigned          fOutputPending;
         unsigned          fInlineDataSize;
         Parameter         fInpRate;
         Parameter         fOutRate;

         Reference         fTrHandle;  //!< reference on object which implements transport
         Transport*        fTransport; //!< direct pointer on the transport

         /** \brief Inherited method, should cleanup everything */
         virtual void ObjectCleanup();

         /** \brief Inherited method, should check if transport handle cleanup */
         virtual void ObjectDestroyed(Object*);

         virtual int ExecuteCommand(Command cmd);

         virtual void DoStart();
         virtual void DoStop();

         virtual void ProcessEvent(const EventId&);

         Port(Reference parent,
              const char* name,
              PoolHandle* pool,
              unsigned recvqueue,
              unsigned sendqueue);
         virtual ~Port();

      public:

         virtual const char* ClassName() const { return "Port"; }

         /** Return reference on existing request object.
          * If object not exists and force flag is specified, new request object will be created */
         ConnectionRequest GetConnReq(bool force = false);

         /** Create connection request to specified url.
          * If connection to other dabc port is specified, isserver flag should identify which side is server
          * and which is client during connection establishing
          * TODO: one should try in future avoid isserver flag completely, it can be ruled later by connection manager
          */
         ConnectionRequest MakeConnReq(const std::string& url, bool isserver = false);

         /** Remove connection request - it does not automatically means that port will be disconnected */
         void RemoveConnReq();

          //  here methods for config settings
         PoolHandle* GetPoolHandle() const;
         unsigned InputQueueCapacity() const { return fInputQueueCapacity; }
         unsigned OutputQueueCapacity() const { return fOutputQueueCapacity; }
         unsigned InlineDataSize() const { return fInlineDataSize; }

         void ChangeInlineDataSize(unsigned sz) { if (!IsConnected()) fInlineDataSize = sz; }
         void ChangeInputQueueCapacity(unsigned sz) { if (!IsConnected()) fInputQueueCapacity = sz; }
         void ChangeOutputQueueCapacity(unsigned sz) { if (!IsConnected()) fOutputQueueCapacity = sz; }

         unsigned NumOutputBuffersRequired() const;
         unsigned NumInputBuffersRequired() const;

         /** Returns pointer on memory pool
          * Pointer will be requested from the pool handle.
          * Once pointer acquired it will be stored in the reference -
          * mean pool will not disappear until port is existing */
         MemoryPool* GetMemoryPool();

         /** Method used to assign transport to the port */
         bool AssignTransport(Reference handle, Transport* tr, bool sync = false);

         bool IsConnected() const { return fTransport!=0; }
         void Disconnect();

         bool IsInput() const { return fTransport ? fTransport->ProvidesInput() : false; }
         bool IsOutput() const { return fTransport ? fTransport->ProvidesOutput() : false; }

         unsigned OutputQueueSize() { return fTransport ? fTransport->SendQueueSize() : 0; }
         unsigned OutputPending() const { return fOutputPending; }
         bool CanSend() const { return !IsConnected() || (OutputPending() < OutputQueueCapacity()); }

         /** Send buffer via port.
          * After this call buffer reference will be empty -
          * user will not be able to access data from the buffer.
          * Means, following code will fail:
          *    Buffer buf = Pool()->TakeBuffer(1024);
          *    buf.CopyFromStr("test string as buffer value");
          *    Output(0)->Send(buf);
          *    buf.CopyFromStr("fail while buffer do not reference any memory");
          *
          * To keep reference on the buffer, one should use duplicate method
          *    Output(0)->Send(buf.Duplicate());
          *
          * Be aware that buffer after such operation still has reference on same memory and
          * one do not allowed to change this memory until transport is not finish its transfer.
          */
         bool Send(const Buffer& buf) throw (PortOutputException);

         unsigned MaxSendSegments() { return fTransport ? fTransport->MaxSendSegments() : 0; }

         /** Defines how many buffers can be preserved in the input queue */
         unsigned InputQueueSize() { return fTransport ? fTransport->RecvQueueSize() : 0; }

         /** Defines how many buffers already exists in the input queue */
         unsigned InputPending() const { return fInputPending; }

         /** Returns true, when input queue is full and cannot get more buffers */
         bool InputQueueFull() { return InputPending() == InputQueueSize(); }

         /** Returns true if user can get (receive) buffer from the port */
         bool CanRecv() const { return IsConnected() && (InputPending() > 0); }

         Buffer Recv() throw (PortInputException);

         bool HasInputBuffer(unsigned indx = 0) const { return indx < InputPending(); }
         Buffer& InputBuffer(unsigned indx = 0) const;
         Buffer& FirstInputBuffer() const { return InputBuffer(0); }
         Buffer& LastInputBuffer() const { return InputBuffer(InputPending()-1); }

         bool SkipInputBuffers(unsigned num=1);

         void SetInpRateMeter(const Parameter& ref);
         void SetOutRateMeter(const Parameter& ref);

         inline bool FireInput() { return FireEvent(evntInput); }
         inline bool FireOutput() { return FireEvent(evntOutput); }

         int GetTransportParameter(const char* name);
   };


   class PortRef : public ModuleItemRef {

      DABC_REFERENCE(PortRef, ModuleItemRef, Port)

      bool IsConnected() const;

      bool Disconnect();

      Reference GetPool();
   };

}

#endif
