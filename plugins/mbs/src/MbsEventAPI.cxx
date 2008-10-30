extern "C"
{
   #include "../evapi/typedefs.h"
  
   #ifdef GSI__LYNX
   #include "../evapi/s_filhe.h"
   #include "../evapi/s_bufhe.h"   /* GOOSY buffer header */
   #include "../evapi/s_ve10_1.h"  /* GOOSY event header          */
   #include "../evapi/s_ves10_1.h"  /* GOOSY event header          */
   #include "../evapi/s_evhe.h"    /* GOOSY spanned event header  */
   #endif
   
   #ifdef GSI__LINUX   /* Linux */
   #include "../evapi/s_filhe_swap.h"
   #include "../evapi/s_bufhe_swap.h"   /* GOOSY buffer header */
   #include "../evapi/s_ve10_1_swap.h"  /* GOOSY event header          */
   #include "../evapi/s_ves10_1_swap.h"  /* GOOSY event header          */
   #include "../evapi/s_evhe_swap.h"    /* GOOSY spanned event header  */
   #endif
   
   #ifdef GSI__WINNT           /* Windows NT */
   #include "../evapi/s_filhe_swap.h"
   #include "../evapi/s_bufhe_swap.h"   /* GOOSY buffer header */
   #include "../evapi/s_ve10_1_swap.h"  /* GOOSY event header          */
   #include "../evapi/s_ves10_1_swap.h"  /* GOOSY event header          */
   #include "../evapi/s_evhe_swap.h"    /* GOOSY spanned event header  */
   #endif
   
   #ifdef GSI__AIX
   #include "../evapi/s_filhe.h"
   #include "../evapi/s_bufhe.h"   /* GOOSY buffer header */
   #include "../evapi/s_ve10_1.h"  /* GOOSY event header          */
   #include "../evapi/s_ves10_1.h"  /* GOOSY event header          */
   #include "../evapi/s_evhe.h"    /* GOOSY spanned event header  */
   #endif
   
   #ifdef GSI__VMS   /* open vms */
   #include "../evapi/s_filhe_swap.h"
   #include "../evapi/s_bufhe_swap.h"   /* GOOSY buffer header */
   #include "../evapi/s_ve10_1_swap.h"  /* GOOSY event header          */
   #include "../evapi/s_ves10_1_swap.h"  /* GOOSY event header          */
   #include "../evapi/s_evhe_swap.h"    /* GOOSY spanned event header  */
   #endif
  
   #include "../evapi/f_evt.h"
}

#include "mbs/MbsEventAPI.h"

#include "dabc/logging.h"

#include "dabc/MemoryPool.h"

#include "mbs/MbsTypeDefs.h"

#define GETEVT__TAGFILE 4000
#define GETEVT__PUTFILE 5000


void ShowMbsError(int code, const char* info)
{
   char strbuf[256];
   f_evt_error(code, strbuf, 1); // provide text message for later output
   EOUT(("%s Error: %d %s", info, code, strbuf));
}

mbs::MbsEventInput::MbsEventInput() : 
   dabc::DataInput(),
   fMode(-1),
   fChannel(0),
   fEventHeader(0),
   fBufferHeader(0),
   fInfoHeader(0),
   fTimeout(-1),
   fFinished(true)
{
   fChannel = malloc(sizeof(s_evt_channel));
}

mbs::MbsEventInput::~MbsEventInput()
{
   Close(); 
   free(fChannel);
}
         
bool mbs::MbsEventInput::OpenFile(const char* fname, const char* tagfile)
{
   if (fMode>=0) return false; 
    
   if (tagfile==0) return Open(GETEVT__FILE, fname);
   
   int status = f_evt_get_tagopen((s_evt_channel*) fChannel,
                                   const_cast<char*>(tagfile) ,
                                   const_cast<char*>(fname),
                                   &fInfoHeader,
                                   0);

   if(status != GETEVT__SUCCESS) {
      ShowMbsError(status, "Cannot open MBS file");
      return false;
   }

   DOUT1(( " Mbs file Source --  opened %s", fname));

   fMode = GETEVT__TAGFILE;
   fFinished = false;

   return true;
}

bool mbs::MbsEventInput::OpenRemoteEventServer(const char* name, int port)
{
   if(fMode>=0) return false;
   
   if (port<=0) port = 6003;
   
   int status  = f_evt_rev_port(port);

   if(status != GETEVT__SUCCESS) {
      ShowMbsError(status, "Cannot open remote event server");
      return false;
   }

   return Open(GETEVT__REVSERV, name);
}

bool mbs::MbsEventInput::OpenStreamServer(const char* name)
{
   return Open(GETEVT__STREAM, name);
}

bool mbs::MbsEventInput::OpenEventServer(const char* name)
{
   return Open(GETEVT__EVENT, name);
}

bool mbs::MbsEventInput::OpenTransportServer(const char* name)
{
   return Open(GETEVT__TRANS, name);
}

bool mbs::MbsEventInput::OpenRFIOFile(const char* name)
{
   return Open(GETEVT__RFIO, name);
}

bool mbs::MbsEventInput::Open(int mode, const char* fname)
{
   if(fMode>=0) return false;
//std::cout << "Open of TGo4MbsSource"<< std::endl;
// open connection/file
   int status = f_evt_get_open(
                     mode,
                     const_cast<char*>(fname),
                     (s_evt_channel*) fChannel,
                     &fInfoHeader,
                     0,
                     0);
   if(status != GETEVT__SUCCESS) {
      ShowMbsError(status, "Cannot open MBS source");
      return false;
   }

   f_evt_timeout((s_evt_channel*)fChannel, fTimeout);
   
   fMode = mode;
   fFinished = false;
   
   DOUT1(( " Mbs Source --  opened input from type %d:  %s . Timeout=%d s",
               fMode, fname, fTimeout));

   return true;
}

bool mbs::MbsEventInput::Close()
{
   if(fMode<0) return true;
//std::cout << "Close of TGo4MbsSource"<< std::endl;

   int status = 0;
   
   if (fMode==GETEVT__TAGFILE) 
      status = f_evt_get_tagclose((s_evt_channel*)fChannel);
   else 
      status = f_evt_get_close((s_evt_channel*)fChannel);
      
   if(status != GETEVT__SUCCESS) {
      ShowMbsError(status, "Cannot close MBS source");
//      return false;
   }
   fMode = -1;
   fFinished = true;
   return true;
}

unsigned mbs::MbsEventInput::Read_Size()
{
   if ((fMode<0) || fFinished) return di_EndOfStream; 
   
   DOUT5(("Call ReadNextBuffer"));
    
   int status = f_evt_get_event((s_evt_channel*)fChannel,
                                (INTS4**) &fEventHeader,
                                (INTS4**) &fBufferHeader);
                                
   switch (status) {
      case GETEVT__SUCCESS: 
         return sizeof(s_evhe) + ((s_evhe*)fEventHeader)->l_dlen*2;
      case GETEVT__NOMORE:
         DOUT1(("!!!!!!!!!!!!! Read_Size File is finished !!!!!!!!!!!!"));
         fFinished = true;
         return di_EndOfStream;
   }
   
   ShowMbsError(status, "Reading event source");
      
   return di_Error;
}

unsigned mbs::MbsEventInput::Read_Complete(dabc::Buffer* buf)
{
   if ((fMode<0) || (buf==0) || (fEventHeader==0)) return di_Error;
   
   buf->SetTypeId(mbt_MbsEvs10_1);
   
   buf->SetHeaderSize(0);
   
   dabc::BufferSize_t size = GetEventBufferSize();
   
   buf->SetDataSize(size);
   
   memcpy(buf->GetDataLocation(), fEventHeader, size);

   return di_Ok;
}

void* mbs::MbsEventInput::GetEventBuffer()
{
   return fEventHeader; 
}

unsigned mbs::MbsEventInput::GetEventBufferSize()
{
   if (fEventHeader==0) return 0;
   
   return sizeof(s_evhe) + ((s_evhe*)fEventHeader)->l_dlen*2;
}

// ______________________________________________________________

mbs::MbsEventOutput::MbsEventOutput() : 
   dabc::DataOutput(),
   fMode(-1),
   fChannel(0)
{
   fChannel = malloc(sizeof(s_evt_channel));
}

   
mbs::MbsEventOutput::~MbsEventOutput()
{
   Close(); 
   free(fChannel);
}
         
bool mbs::MbsEventOutput::CreateOutputFile(const char* fname)
{
   if(fMode>=0) return false;
   
   int status = f_evt_put_open(const_cast<char*>(fname), 
                               16*1024, 4, 10, 1, 
                               (s_evt_channel*)fChannel, 0);  
   if (status != PUTEVT__SUCCESS) {
      ShowMbsError(status, "Cannot create MBS output");
      return false;
   }   

   DOUT1(( " Mbs Output --  create file %s", fname));
   
   fMode = GETEVT__PUTFILE;
   
   return true;
 
}

bool mbs::MbsEventOutput::Close()
{
   if (fMode<0) return true;
   
   int status = f_evt_put_close((s_evt_channel*)fChannel);

   if(status != GETEVT__SUCCESS) {
      ShowMbsError(status, "Cannot close MBS source");
      return false;
   }
   fMode = -1;
   return true;
}

bool mbs::MbsEventOutput::WriteBuffer(dabc::Buffer* buf)
{
   if (fMode<0) return false; 
   
   DOUT5(("Call WriteBuffer"));
   
   if (buf->GetTypeId() != mbt_MbsEvs10_1) {
      EOUT(("Buffer must contain complete MBS event, type %d != %d mbs::mbt_MbsEvs10_1", buf->GetTypeId(), mbt_MbsEvs10_1));
      return false; 
   }
   
   int status = f_evt_put_event((s_evt_channel*)fChannel,
                                (INTS4*) buf->GetDataLocation());
   
   if (status != PUTEVT__SUCCESS) {
      ShowMbsError(status, "Writing event to file"); 
      return false;
   }
   
   return true;
}

