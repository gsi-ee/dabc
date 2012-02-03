#include "IbTestGPU.h"

#ifdef WITH_GPU

#include <string.h>

#include "dabc/logging.h"
#include "dabc/timing.h"

opencl::Context::Context() :
   dabc::Object("Context"),
   fContext(0),
   deviceId(0),
   fDevices(0),
   maxWorkGroupSize(0),
   maxDimensions(0),
   maxWorkItemSizes(0),
   totalLocalMemory(0),
   totalGlobalMemory(0)
{
}

opencl::Context::~Context()
{
   CloseGPU();
   DOUT2(("opencl::Context destroyed"));
}

bool opencl::Context::displayDevices(cl_platform_id platform, cl_device_type deviceType)
{
    cl_int status;

    // Get platform name
    char platformVendor[1024];
    status = clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, sizeof(platformVendor), platformVendor, NULL);
    if(status!=CL_SUCCESS) {
       EOUT(("clGetPlatformInfo failed"));
       return false;
    }

    DOUT2(("Selected Platform Vendor : %s", platformVendor));

    // Get number of devices available
    cl_uint deviceCount = 0;
    status = clGetDeviceIDs(platform, deviceType, 0, NULL, &deviceCount);
    if(status!=CL_SUCCESS) {
       EOUT(("clGetDeviceIDs failed"));
       return false;
    }

    cl_device_id* deviceIds = (cl_device_id*)malloc(sizeof(cl_device_id) * deviceCount);
    if(deviceIds == NULL)
    {
        EOUT(("Failed to allocate memory(deviceIds)"));
        return 0;
    }

    // Get device ids
    status = clGetDeviceIDs(platform, deviceType, deviceCount, deviceIds, NULL);

    if(status!=CL_SUCCESS) {
       EOUT(("clGetDeviceIDs failed"));
    } else
    // Print device index and device names
    for(cl_uint i = 0; i < deviceCount; ++i)
    {
        char deviceName[1024];
        status = clGetDeviceInfo(deviceIds[i], CL_DEVICE_NAME, sizeof(deviceName), deviceName, NULL);

        if(status!=CL_SUCCESS) {
           EOUT(("clGetDeviceInfo failed %u", i));
           break;
        }

        DOUT2(("Device %u : %s", i, deviceName));
    }

    free(deviceIds);

    return true;
}

void opencl::Context::CloseGPU()
{
   cl_int status;

   deviceId = 0;
   maxWorkGroupSize = 0;
   maxDimensions = 0;
   totalLocalMemory = 0;

   if (fDevices!=0) {
      free(fDevices);
      fDevices = 0;
   }

   if (maxWorkItemSizes!=0) {
      free(maxWorkItemSizes);
      maxWorkItemSizes = 0;
   }

   if (fContext!=0) {

      status = clReleaseContext(fContext);
      if (status != CL_SUCCESS)
         EOUT(("clReleaseContext failed."));
      fContext = 0;
   }
}


bool opencl::Context::OpenGPU()
{
   cl_int status = 0;
   size_t deviceListSize;

   cl_device_type dType =  CL_DEVICE_TYPE_GPU;

   /*
    * Have a look at the available platforms and pick either
    * the AMD one if available or a reasonable default.
    */

   cl_uint numPlatforms;
   cl_platform_id platform = NULL;
   status = clGetPlatformIDs(0, NULL, &numPlatforms);

   if (status!= CL_SUCCESS) {
      EOUT(("clGetPlatformIDs failed."));
      return false;
   }

   if (numPlatforms > 0) {
       cl_platform_id* platforms = new cl_platform_id[numPlatforms];

       status = clGetPlatformIDs(numPlatforms, platforms, NULL);
       if (status!=CL_SUCCESS) {
          EOUT(("clGetPlatformIDs failed."));
          delete[] platforms;
          return false;
       }

       // select AMD platform (or last from the list)

       for (unsigned i = 0; i < numPlatforms; ++i) {
          char pbuf[100];
          status = clGetPlatformInfo(platforms[i],
                                     CL_PLATFORM_VENDOR,
                                     sizeof(pbuf),
                                     pbuf,
                                     NULL);

          if (status != CL_SUCCESS) {
             EOUT(("clGetPlatformInfo failed."));
             delete[] platforms;
             return false;
          }

          platform = platforms[i];
          if (strcmp(pbuf, "Advanced Micro Devices, Inc.")==0) break;
       }
       delete[] platforms;
   }

   if(platform == NULL)
   {
       EOUT(("NULL platform found so Exiting Application."));
       return false;
   }

   // Display available devices.
   if(!displayDevices(platform, dType))
   {
       EOUT(("displayDevices() failed"));
       return false;
   }

   /*
    * If we could find our platform, use it. Otherwise use just available platform.
    */
   cl_context_properties cps[3] =
   {
       CL_CONTEXT_PLATFORM,
       (cl_context_properties)platform,
       0
   };

   fContext = clCreateContextFromType(cps,
                                     dType,
                                     NULL,
                                     NULL,
                                     &status);

   if(status!=CL_SUCCESS) {
      EOUT(("clCreateContextFromType failed."));
      return false;
   }

   /* First, get the size of device list data */
   status = clGetContextInfo(
             fContext,
             CL_CONTEXT_DEVICES,
             0,
             NULL,
             &deviceListSize);
   if(status!=CL_SUCCESS) {
       EOUT(("clGetContextInfo failed."));
       return false;
   }

   int deviceCount = (int)(deviceListSize / sizeof(cl_device_id));
   if(deviceId >= deviceCount) {
       EOUT(("devicedId = %d should be less than device count = %d, failed", deviceId , deviceCount));
       return false;
   }

   /* Now allocate memory for device list based on the size we got earlier */
   fDevices = (cl_device_id *)malloc(deviceListSize);
   if(fDevices == NULL) {
       EOUT(("Failed to allocate memory (devices)."));
       return false;
   }

   /* Now, get the device list data */
   status = clGetContextInfo(
           fContext,
           CL_CONTEXT_DEVICES,
           deviceListSize,
           fDevices,
           NULL);

   if(status!=CL_SUCCESS) {
      EOUT(("clGetGetContextInfo failed."));
      return false;
   }

   /* Get Device specific Information */
   status = clGetDeviceInfo(
           fDevices[deviceId],
           CL_DEVICE_MAX_WORK_GROUP_SIZE,
           sizeof(size_t),
           (void *)&maxWorkGroupSize,
           NULL);

   if(status!=CL_SUCCESS) {
      EOUT(("clGetDeviceInfo CL_DEVICE_MAX_WORK_GROUP_SIZE failed."));
      return false;
   }

   status = clGetDeviceInfo(
               fDevices[deviceId],
               CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
               sizeof(cl_uint),
               (void *)&maxDimensions,
               NULL);

   if(status!=CL_SUCCESS) {
      EOUT(("clGetDeviceInfo CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS failed."));
      return false;
   }

   maxWorkItemSizes = (size_t *)malloc(maxDimensions*sizeof(size_t));

   status = clGetDeviceInfo(
               fDevices[deviceId],
               CL_DEVICE_MAX_WORK_ITEM_SIZES,
               sizeof(size_t)*maxDimensions,
               (void *)maxWorkItemSizes,
               NULL);

   if(status!=CL_SUCCESS) {
      EOUT(("clGetDeviceInfo CL_DEVICE_MAX_WORK_ITEM_SIZES failed."));
      return false;
   }

   status = clGetDeviceInfo(
               fDevices[deviceId],
               CL_DEVICE_LOCAL_MEM_SIZE,
               sizeof(cl_ulong),
               (void *)&totalLocalMemory,
               NULL);

   if(status!=CL_SUCCESS) {
      EOUT(("clGetDeviceInfo CL_DEVICE_LOCAL_MEM_SIZES failed."));
      return false;
   }

   status = clGetDeviceInfo(
               fDevices[deviceId],
               CL_DEVICE_GLOBAL_MEM_SIZE,
               sizeof(cl_ulong),
               (void *)&totalGlobalMemory,
               NULL);

   if(status!=CL_SUCCESS) {
      EOUT(("clGetDeviceInfo CL_DEVICE_GLOBAL_MEM_SIZES failed."));
      return false;
   }

   DOUT2(("Total memory local %u global = %u", totalLocalMemory, totalGlobalMemory));

   return true;
}

// ====================================================

bool opencl::ContextRef::OpenGPU()
{
   if (GetObject()) return true;

   Context* ctx = new Context;

   if (!ctx->OpenGPU()) {
      delete ctx;
      return false;
   }

   SetObject(ctx, true);
   SetTransient(false);
   return true;
}

// ======================================================


opencl::Memory::Memory(const ContextRef& ctx, unsigned sz) :
  fMem(0),
  fSize(0)
{
   if (ctx.null() || (sz==0)) return;

   cl_int status = 0;

   /* Create input imaginary buffer */
   fMem = clCreateBuffer(
           ctx.context(),
           CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
           sz,
           0,
           &status);

   if(status != CL_SUCCESS)
      EOUT(("clCreateBuffer failed. (buffer_r)"));
   else
      fSize = sz;
}

opencl::Memory::~Memory()
{
   if (fSize==0) return;

   cl_int status = clReleaseMemObject(fMem);

   if (status != CL_SUCCESS)
      EOUT(("clReleaseMemObject failed."));
}

// ====================================================

opencl::CommandsQueue::CommandsQueue(ContextRef ctx) :
   fQueue(0)
{
   if (ctx.null()) {
      EOUT(("Empty context!!!"));
      return;
   }

   cl_command_queue_properties prop = 0;
   cl_int status = 0;
   fQueue = clCreateCommandQueue(
                ctx.context(),
                ctx.deviceid(),
                prop,
                &status);

   if(status != 0) {
      fQueue = 0;
      EOUT(("clCreateCommandQueue failed."));
   }
}

opencl::CommandsQueue::~CommandsQueue()
{
   cl_int status = clReleaseCommandQueue(fQueue);
   if(status!=CL_SUCCESS) EOUT(("clReleaseCommandQueue failed."));
   fQueue = 0;
}


bool opencl::CommandsQueue::SubmitWrite(QueueEvent& evt, Memory& mem, void* src, unsigned copysize)
{
   if (copysize==0) copysize = mem.size();

   if (mem.null() || (src==0) || (copysize==0)) return false;

   cl_int   status;

   /* Kernel Does an inplace FFT */
   status = clEnqueueWriteBuffer(
               fQueue,
               mem.mem(),
               CL_FALSE,
               0,
               copysize,
               src,
               0,
               NULL,
               &evt);
   if(status != CL_SUCCESS) {
       EOUT(("clEnqueueWriteBuffer failed."));
       return false;
   }

   return Flush();
}

bool opencl::CommandsQueue::SubmitRead(QueueEvent& evt, Memory& mem, void* tgt, unsigned copysize)
{
   if (copysize==0) copysize = mem.size();

   if (mem.null() || (tgt==0) || (copysize==0)) return false;

   cl_int   status;

   /* Kernel Does an inplace FFT */
   status = clEnqueueReadBuffer(
               fQueue,
               mem.mem(),
               CL_FALSE,
               0,
               copysize,
               tgt,
               0,
               NULL,
               &evt);
   if(status != CL_SUCCESS) {
       EOUT(("clEnqueueReadBuffer failed."));
       return false;
   }

   return Flush();
}

bool opencl::CommandsQueue::Flush()
{
   cl_int status = clFlush(fQueue);
   if (status != CL_SUCCESS) {
      EOUT(("clFlush failed"));
      return false;
   }
   return true;
}


/** -1 - error, 0 - not complete, 1 - complete */
int opencl::CommandsQueue::CheckComplete(QueueEvent& evt)
{
   cl_int eventStatus = CL_QUEUED;

   cl_int status = clGetEventInfo(
                    evt,
                    CL_EVENT_COMMAND_EXECUTION_STATUS,
                    sizeof(cl_int),
                    &eventStatus,
                    NULL);

   if(status!=CL_SUCCESS) {
      EOUT(("clGetEventInfo failed."));
      return -1;
   }

   return eventStatus == CL_COMPLETE ? 1 : 0;
}


bool opencl::CommandsQueue::WaitComplete(QueueEvent& evt, double tm)
{
   dabc::TimeStamp start = dabc::Now();

   do {
      switch (CheckComplete(evt)) {
         case 1: return true;
         case -1: return false;
         default: break;
      }

   } while (!start.Expired(tm));

   return false;
}


#else

// this is just placeholder in the case when GPU soft is not available


#endif

