#ifndef IbTestGPU_h
#define IbTestGPU_h

#ifndef DABC_Object
#include "dabc/Object.h"
#endif


#ifdef WITH_GPU

#include "CL/cl.h"

namespace opencl {

   class ContextRef;

   class Context : public dabc::Object {
      friend class ContextRef;

      protected:
         cl_context     fContext;
         int            deviceId;
         cl_device_id  *fDevices;           /**< CL device list */
         size_t         maxWorkGroupSize;   /**< Device Specific Information */
         cl_uint        maxDimensions;
         size_t        *maxWorkItemSizes;
         cl_ulong       totalLocalMemory;
         cl_ulong       totalGlobalMemory;

         cl_device_id deviceid() { return fDevices && (deviceId>=0) ? fDevices[deviceId] : 0; }

         Context();

         bool displayDevices(cl_platform_id platform, cl_device_type deviceType);

         bool OpenGPU();

         void CloseGPU();

      public:
         virtual ~Context();
   };


   class ContextRef : public dabc::Reference {

      static bool transient_refs() { return false; }

      DABC_REFERENCE(ContextRef, dabc::Reference, Context)

      public:
         bool OpenGPU();

         cl_context context() const { return GetObject() ? GetObject()->fContext : 0; }

         cl_device_id deviceid() const { return GetObject() ? GetObject()->deviceid() : 0; }
   };



   class Memory {
      protected:
         cl_mem     fMem;
         unsigned   fSize;
      public:
         Memory(const ContextRef& ref, unsigned sz);
         virtual ~Memory();

         unsigned size() const { return fSize; }

         bool null() const { return size() == 0; }

         cl_mem& mem() { return fMem; }

   };


   typedef cl_event QueueEvent;

   class CommandsQueue {
      protected:
         cl_command_queue fQueue;   /**< CL command queue */

      public:
         CommandsQueue(ContextRef ctx);
         virtual ~CommandsQueue();

         bool SubmitWrite(QueueEvent& evt, Memory& mem, void* src, unsigned copysize = 0);

         bool SubmitRead(QueueEvent& evt, Memory& mem, void* tgt, unsigned copysize = 0);

         bool Flush();

         /** -1 - error, 0 - not complete, 1 - complete */
         int CheckComplete(QueueEvent& evt);

         bool WaitComplete(QueueEvent& evt, double tm = 1.);

   };

}


#else

// this is just placeholder in the case when GPU soft is not available

namespace opencl {

   class Dummy {
      public:
         Dummy() {}
         virtual ~Dummy() {}

   };

   typedef int Context;

   typedef int ContextRef;

   typedef int Memory;

   typedef int QueueEvent;

   typedef int CommandsQueue;
}


#endif



#endif
