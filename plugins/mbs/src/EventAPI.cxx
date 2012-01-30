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

#include "mbs/EventAPI.h"

#include "dabc/logging.h"

#include "dabc/MemoryPool.h"

#include "mbs/MbsTypeDefs.h"

#define GETEVT__TAGFILE 4000
#define GETEVT__PUTFILE 5000

const char* mbs::xmlEvapiType                     = "EvapiType";

const char* mbs::xmlEvapiFile                     = "EvapiFile";
const char* mbs::xmlEvapiRFIOFile                 = "EvapiRFIO";
const char* mbs::xmlEvapiTransportServer          = "EvapiTransport";
const char* mbs::xmlEvapiStreamServer             = "EvapiStream";
const char* mbs::xmlEvapiEventServer              = "EvapiEvent";
const char* mbs::xmlEvapiRemoteEventServer        = "EvapiRemote";

const char* mbs::xmlEvapiSourceName               = "Name";
const char* mbs::xmlEvapiTagFile                  = "TagFile";
const char* mbs::xmlEvapiRemoteEventServerPort    = "Port";
const char* mbs::xmlEvapiTimeout                  = "Timeout";

const char* mbs::xmlEvapiOutFile                  = "EvapiOutFile";
const char* mbs::xmlEvapiOutFileName              = "OutputFile";


void ShowMbsError(int code, const char* info)
{
   char strbuf[256];
   f_evt_error(code, strbuf, 1); // provide text message for later output
   EOUT(("%s Error: %d %s", info, code, strbuf));
}

mbs::EvapiInput::EvapiInput(const char* kind, const char* name) :
   dabc::DataInput(),
   fKind(kind ? kind : ""),
   fName(name ? name : ""),
   fTagFile(),
   fRemotePort(0),
   fTimeout(-1),
   fMode(-1),
   fChannel(0),
   fEventHeader(0),
   fBufferHeader(0),
   fInfoHeader(0),
   fFinished(true)
{
   fChannel = malloc(sizeof(s_evt_channel));
}

mbs::EvapiInput::~EvapiInput()
{
   Close();
   free(fChannel);
}

bool mbs::EvapiInput::Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   fKind = wrk.Cfg(xmlEvapiType, cmd).AsStdStr(fKind);
   fName = wrk.Cfg(xmlEvapiSourceName, cmd).AsStdStr(fName);

   if (fKind==xmlEvapiFile) {
      fTagFile = wrk.Cfg(xmlEvapiTagFile, cmd).AsStdStr(fTagFile);
      return OpenFile(fName.c_str(), fTagFile.c_str());
   } else
   if (fKind==xmlEvapiRFIOFile) {
      return OpenRFIOFile(fName.c_str());
   } else
   if (fKind==xmlEvapiTransportServer) {
      fTimeout = wrk.Cfg(xmlEvapiTimeout, cmd).AsInt(fTimeout);
      return OpenTransportServer(fName.c_str());
   } else
   if (fKind==xmlEvapiStreamServer) {
      fTimeout = wrk.Cfg(xmlEvapiTimeout, cmd).AsInt(fTimeout);
      return OpenStreamServer(fName.c_str());
   } else
   if (fKind==xmlEvapiEventServer) {
      fTimeout = wrk.Cfg(xmlEvapiTimeout, cmd).AsInt(fTimeout);
      return OpenEventServer(fName.c_str());
   } else
   if (fKind==xmlEvapiRemoteEventServer) {
      fRemotePort = wrk.Cfg(xmlEvapiRemoteEventServerPort, cmd).AsInt(fRemotePort);
      fTimeout = wrk.Cfg(xmlEvapiTimeout, cmd).AsInt(fTimeout);
      return OpenRemoteEventServer(fName.c_str(), fRemotePort);
   }

   return false;
}


bool mbs::EvapiInput::OpenFile(const char* fname, const char* tagfile)
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

bool mbs::EvapiInput::OpenRemoteEventServer(const char* name, int port)
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

bool mbs::EvapiInput::OpenStreamServer(const char* name)
{
   return Open(GETEVT__STREAM, name);
}

bool mbs::EvapiInput::OpenEventServer(const char* name)
{
   return Open(GETEVT__EVENT, name);
}

bool mbs::EvapiInput::OpenTransportServer(const char* name)
{
   return Open(GETEVT__TRANS, name);
}

bool mbs::EvapiInput::OpenRFIOFile(const char* name)
{
   return Open(GETEVT__RFIO, name);
}

bool mbs::EvapiInput::Open(int mode, const char* fname)
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

bool mbs::EvapiInput::Close()
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

unsigned mbs::EvapiInput::Read_Size()
{
   if ((fMode<0) || fFinished) return dabc::di_EndOfStream;

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
         return dabc::di_EndOfStream;
   }

   ShowMbsError(status, "Reading event source");

   return dabc::di_Error;
}

unsigned mbs::EvapiInput::Read_Complete(dabc::Buffer& buf)
{
   if ((fMode<0) || buf.null() || (fEventHeader==0)) return dabc::di_Error;

   buf.SetTypeId(mbt_MbsEvents);

   dabc::BufferSize_t size = GetEventBufferSize();

   buf.SetTotalSize(size);

   buf.CopyFrom(fEventHeader, size);

   return dabc::di_Ok;
}

void* mbs::EvapiInput::GetEventBuffer()
{
   return fEventHeader;
}

unsigned mbs::EvapiInput::GetEventBufferSize()
{
   if (fEventHeader==0) return 0;

   return sizeof(s_evhe) + ((s_evhe*)fEventHeader)->l_dlen*2;
}

// ______________________________________________________________

mbs::EvapiOutput::EvapiOutput(const char* fname) :
   dabc::DataOutput(),
   fFileName(fname ? fname : 0),
   fMode(-1),
   fChannel(0)
{
   fChannel = malloc(sizeof(s_evt_channel));
}


mbs::EvapiOutput::~EvapiOutput()
{
   Close();
   free(fChannel);
}

bool mbs::EvapiOutput::Write_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   fFileName = wrk.Cfg(xmlEvapiOutFileName, cmd).AsStdStr(fFileName);

   return CreateOutputFile(fFileName.c_str());
}

bool mbs::EvapiOutput::CreateOutputFile(const char* fname)
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

bool mbs::EvapiOutput::Close()
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

bool mbs::EvapiOutput::WriteBuffer(const dabc::Buffer& buf)
{
   if (fMode<0) return false;

   DOUT5(("Call WriteBuffer"));

   if (buf->GetTypeId() != mbt_MbsEvents) {
      EOUT(("Buffer must contain complete MBS event, type %d != %d mbs::mbt_MbsEvents", buf->GetTypeId(), mbt_MbsEvents));
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

