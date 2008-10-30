#include "dabc/DataIOTransport.h"

dabc::DataIOTransport::DataIOTransport(Device* dev, Port* port, DataInput* inp, DataOutput* out) :
   DataTransport(dev, port, inp!=0, out!=0),
   fInput(inp),
   fOutput(out)
{
DOUT3(("ddddddddddd dabc::DataIOTransport %p CONSTRUCTOR",this));
}
   
dabc::DataIOTransport::~DataIOTransport()
{
 DOUT3(("ddddddddddd dabc::DataIOTransport %p DESTRUCTOR",this));
 CloseInput();
 CloseOutput();
}

unsigned dabc::DataIOTransport::Read_Size() 
{ 
   return fInput ? fInput->Read_Size() : DataInput::di_Error; 
}

unsigned dabc::DataIOTransport::Read_Start(Buffer* buf)
{
   return fInput ? fInput->Read_Start(buf) : DataInput::di_Error; 
}

unsigned dabc::DataIOTransport::Read_Complete(Buffer* buf) 
{ 
   return fInput ? fInput->Read_Complete(buf) : DataInput::di_Error; 
}

double dabc::DataIOTransport::Read_Timeout() 
{ 
   return fInput ? fInput->GetTimeout() : 0.1; 
}

bool dabc::DataIOTransport::WriteBuffer(Buffer* buf) 
{ 
   return fOutput ? fOutput->WriteBuffer(buf) : false; 
}

void dabc::DataIOTransport::FlushOutput() 
{ 
   if (fOutput) fOutput->Flush(); 
}

void dabc::DataIOTransport::CloseInput() 
{ 
   DOUT3(("dabc::DataIOTransport::CloseInput() deletes input "));
   delete fInput; 
   fInput = 0;
}

void dabc::DataIOTransport::CloseOutput() 
{ 
   DOUT3(("dabc::DataIOTransport::CloseOutput() deletes output "));   
   delete fOutput; 
   fOutput = 0;  
}
