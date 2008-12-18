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

namespace dabc {

   class Port;
   class PoolHandle;
   class RateParameter;

   class Module;

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

      protected:
         PoolHandle*       fPool;
         unsigned          fInputQueueCapacity;
         unsigned          fInputPending;
         unsigned          fOutputQueueCapacity;
         unsigned          fOutputPending;
         BufferSize_t      fUsrHeaderSize;
         Transport*        fTransport;
         RateParameter*    fInpRate;
         RateParameter*    fOutRate;

         virtual int ExecuteCommand(Command* cmd);

         virtual void DoStart();
         virtual void DoStop();

         inline void FireInput() { FireEvent(evntInput); }
         inline void FireOutput() { FireEvent(evntOutput); }

         virtual void ProcessEvent(EventId evid);

         Port(Basic* parent,
              const char* name,
              PoolHandle* pool,
              unsigned recvqueue,
              unsigned sendqueue,
              BufferSize_t usrheadersize = 0);
         virtual ~Port();

      public:

         virtual const char* MasterClassName() const { return "Port"; }
         virtual const char* ClassName() const { return "Port"; }
         virtual bool UseMasterClassName() const { return true; }

          //  here methods for config settings
         PoolHandle* Pool() const { return fPool; }
         MemoryPool* GetMemoryPool() const;
         unsigned InputQueueCapacity() const { return fInputQueueCapacity; }
         unsigned OutputQueueCapacity() const { return fOutputQueueCapacity; }
         BufferSize_t UserHeaderSize() const { return fUsrHeaderSize; }
         void ChangeUserHeaderSize(BufferSize_t sz) { if (!IsConnected()) fUsrHeaderSize = sz; }
         void ChangeInputQueueCapacity(unsigned sz) { if (!IsConnected()) fInputQueueCapacity = sz; }
         void ChangeOutputQueueCapacity(unsigned sz) { if (!IsConnected()) fOutputQueueCapacity = sz; }
         unsigned NumOutputBuffersRequired() const;
         unsigned NumInputBuffersRequired() const;

         bool AssignTransport(Transport* tr, CommandClientBase* cli = 0);
         bool IsConnected() const { return fTransport!=0; }
         void Disconnect();

         bool IsInput() const { return fTransport ? fTransport->ProvidesInput() : false; }
         bool IsOutput() const { return fTransport ? fTransport->ProvidesOutput() : false; }

         unsigned OutputQueueSize() { return fTransport ? fTransport->SendQueueSize() : 0; }
         unsigned OutputPending() const { return fOutputPending; }
         bool OutputQueueBlocked() const { return OutputPending() >= OutputQueueCapacity(); }
         bool OutputBlocked() const { return IsConnected() && OutputQueueBlocked(); }

         unsigned MaxSendSegments() { return fTransport ? fTransport->MaxSendSegments() : 0; }

         bool Send(Buffer* buf) throw (PortOutputException);

         unsigned InputQueueSize() { return fTransport ? fTransport->RecvQueueSize() : 0; }
         unsigned InputPending() const { return fInputPending; }
         bool InputQueueBlocked() const { return InputPending() == 0; }
         bool InputBlocked() const { return !IsConnected() || InputQueueBlocked(); }
         bool InputQueueFull() { return InputPending() == InputQueueSize(); }

         Buffer* InputBuffer(unsigned indx = 0) const { return (fTransport && (indx<fInputPending)) ? fTransport->RecvBuffer(indx) : 0; }
         Buffer* FirstInputBuffer() const { return InputBuffer(0); }
         Buffer* LastInputBuffer() const { return InputBuffer(InputPending()-1); }

         bool SkipInputBuffers(unsigned num=1);

         bool Recv(Buffer* &buf) throw (PortInputException);

         void SetInpRateMeter(RateParameter* p) { fInpRate = p; }
         void SetOutRateMeter(RateParameter* p) { fOutRate = p; }
   };

}

#endif
