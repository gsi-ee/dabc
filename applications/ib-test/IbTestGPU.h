#ifndef IbTestGPU_h
#define IbTestGPU_h

#ifdef WITH_GPU

#include "CL/cl.h"

namespace opencl {
  
  

  class Buffer {
     cl_mem   fMem;
     public:
     Buffer() {}
     virtual ~Buffer() {}
     
  };

  class Context {
    cl_context fContext;
  public:
    Context() {}
    virtual ~Context() {}
  };

   
}


#else

// this is just placeholder in the case when GPU soft is not available

namespace opencl {

  class Buffer {
     public:
    Buffer() {}
    virtual ~Buffer() {}
     
  };

  class Context {
  public:
    Context() {}
    virtual ~Context() {}
  };

}


#endif



#endif
