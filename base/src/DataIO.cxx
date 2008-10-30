#include "dabc/DataIO.h"

#include "dabc/Buffer.h"

dabc::Buffer* dabc::DataInput::ReadBuffer()
{
   unsigned sz = Read_Size();
    
   if (sz>di_ValidSize) return 0;
    
   dabc::Buffer* buf = dabc::Buffer::CreateBuffer(sz);
    
   if (buf==0) return 0;
   
   if (Read_Start(buf) != di_Ok) {
      dabc::Buffer::Release(buf); 
      return 0;
   }
    
   if (Read_Complete(buf) != di_Ok) {
      dabc::Buffer::Release(buf); 
      return 0;
   }
    
   return buf;
}
