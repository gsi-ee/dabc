#include "dabc/logging.h"

#include "dabc/FileIO.h"
#include "dabc/ModuleAsync.h"
#include "dabc/ModuleSync.h"
#include "dabc/Port.h"
#include "dabc/PoolHandle.h"
#include "dabc/Buffer.h"
#include "dabc/Manager.h"
#include "dabc/timing.h"

#include "mbs/MbsEventAPI.h"
#include "mbs/Factory.h"
#include "mbs/MbsTypeDefs.h"
#include "mbs/MbsDataInput.h"
#include "mbs/MbsOutputFile.h"
#include "mbs/MbsInputFile.h"
#include "mbs/MbsTransport.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <list>


class MbsTest1RepeaterModule : public dabc::ModuleAsync {
   protected:
      dabc::PoolHandle* fPool;
      dabc::Port*         fInpPort;
      dabc::Port*         fOutPort;
      int                 fBufferCnt;
      bool                fWaitForStop;
   public:
      MbsTest1RepeaterModule(const char* name = "Repeater") :
         dabc::ModuleAsync(name)
      {
         // we can use small buffer, while now we can allocate flexible buffer
         int buffersize = 1024;

         fPool = CreatePool("MyPool", 100, buffersize);

         fInpPort = CreateInput("Input", fPool, 5);

         fOutPort = CreateOutput("Output", fPool, 5);

         fBufferCnt = 0;

         fWaitForStop = false;
      }

      void PerformRepeater()
      {
         while (!fInpPort->InputBlocked() && !fOutPort->OutputBlocked())
         {
            dabc::Buffer* buf = 0;
            fInpPort->Recv(buf);

//            DOUT1(("Recv buffer %p", buf));

            if (buf==0) continue;

            if (buf->GetTypeId() == dabc::mbt_EOF) {
               DOUT1(("Find EOF in input stream"));
               fWaitForStop = true;
            } else  {
               if (fBufferCnt<10)
                  if (buf->GetTypeId() == mbs::mbt_MbsEvs10_1)
                     mbs::IterateEvent(buf->GetDataLocation());
                  else
                  if (buf->GetTypeId() == mbs::mbt_Mbs100_1)
                     mbs::IterateBuffer(buf->GetDataLocation(), fBufferCnt==0);

               fBufferCnt++;
            }

            fOutPort->Send(buf);
         }
      }

      void ProcessInputEvent(dabc::Port*)
      {
         PerformRepeater();
      }

      void ProcessOutputEvent(dabc::Port*)
      {
         PerformRepeater();

         if (fWaitForStop && (fOutPort->OutputPending()==0)) {
            DOUT1(("THIS IS MUST BE FULL STOP OF THE MODULE"));
            Stop();
            DOUT1(("STOP is CALLED, but not yet executed"));
         }
      }

      void AfterModuleStop()
      {
         DOUT1(("Repeater module is finished,  buffers:%d", fBufferCnt));
      }
};

void TestMbsFileRepeater(const char* inpfile, const char* outfile, bool new_format)
{
   ::remove(outfile);

   dabc::Manager mgr("LocalHost");

//   new mbs::Factory(&mgr);

   MbsTest1RepeaterModule* m = new MbsTest1RepeaterModule("Repeater");

   mgr.MakeThreadForModule(m, "Thrd1");

   mgr.CreateMemoryPools();

   const char* format = new_format ? mbs::Factory::NewFileType() : mbs::Factory::FileType();

   bool res = mgr.CreateDataInputTransport("Repeater/Ports/Input", "", format, inpfile);
   res = res && mgr.CreateDataOutputTransport("Repeater/Ports/Output", "", format, outfile);

   DOUT1(("Init repeater module() res = %s", DBOOL(res)));

   dabc::SetDebugLevel(3);

   m->Start();

   DOUT1(("Start called"));

   for (int n=0;n<100;n++) {
      dabc::MicroSleep(100000);
      if (m->WorkStatus()<=0) break;
   }

   dabc::SetDebugLevel(10);

   DOUT1(("Work finished"));

   m->Stop();

   DOUT1(("Finish repeater module()"));

   dabc::SetDebugLevel(1);
}

void TestMbsInpFile(const char* fname)
{
   mbs::MbsEventInput* inp = new mbs::MbsEventInput();
   long cnt = 0;

   if (inp->OpenFile(fname)) {

      dabc::Buffer* buf = 0;

      while ((buf = inp->ReadBuffer())>0) {
         if (cnt++<10)
            mbs::IterateEvent(buf->GetDataLocation());

         dabc::Buffer::Release(buf);
      }
   }

   DOUT1(("File:%s Get %d events", fname, cnt));

   delete inp;
}

void TestMbsTrServer(const char* server)
{
   mbs::MbsEventInput* inp = new mbs::MbsEventInput();
   long cnt = 0;

   if (inp->OpenTransportServer(server)) {

      while ((inp->Read_Size()<=dabc::DataInput::di_ValidSize) && (cnt<10000)) {
         inp->Read_Complete(0);
         if (cnt<100)
            mbs::IterateEvent(inp->GetEventBuffer());
         cnt++;
      }
   }

   DOUT1(("File:%s Get %d events", server, cnt));

   delete inp;
}

void TestMbsTrServerNew(const char* server, int port = 8000, const char* outfile = "test.lmd")
{
   mbs::MbsDataInput* inp = new mbs::MbsDataInput();

   if (inp->Open(server, port)) {
     DOUT1(("Open server %s port:%d", server, port));
   } else {
     DOUT1(("Cannot open server %s port:%d", server, port));
     delete inp;
     return;
   }

   mbs::MbsOutputFile* out = new mbs::MbsOutputFile(outfile);
   if (!out->Init()) {
      EOUT(("Cannot create output file %s", outfile));
      delete out;
      delete inp;
      return;
   }

   long cnt = 0;

   unsigned sz = 0;

   dabc::Buffer* buf = dabc::Buffer::CreateBuffer(1024);

   while (((sz = inp->Read_Size())<=dabc::DataInput::di_ValidSize) && (cnt<100)) {

      buf->RellocateBuffer(sz);

      if (inp->Read_Complete(buf)!=dabc::DataInput::di_Ok) break;

      if (cnt<10)
         mbs::NewIterateBuffer(buf);

      cnt++;

      out->WriteBuffer(buf);
   }

   dabc::Buffer::Release(buf);

   DOUT1(("Transport:%s Get %d buffers sz1:%d sz2:%d", server, cnt,
            sizeof(mbs::sMbsFileHeader), sizeof(mbs::sMbsBufferHeader)));

   delete out;
   delete inp;
}

void TestMbsInputFile(const char* fname)
{
   mbs::MbsInputFile* inp = new mbs::MbsInputFile(fname);

   if (inp->Init()) {
     DOUT1(("Open file %s", fname));
   } else {
     DOUT1(("Cannot open file %s", fname));
     delete inp;
     return;
   }

   long cnt = 0;

   unsigned sz = 0;

   dabc::Buffer* buf = dabc::Buffer::CreateBuffer(1024);

   while ((sz = inp->Read_Size()) <= dabc::DataInput::di_ValidSize) {

      buf->RellocateBuffer(sz);

      if (inp->Read_Complete(buf)!=dabc::DataInput::di_Ok) break;
      if (cnt<5)
         mbs::IterateBuffer(buf->GetDataLocation(), cnt==0);

//      if (cnt<5)
//         mbs::NewIterateBuffer(buf);

      cnt++;
   }

   dabc::Buffer::Release(buf);

   DOUT1(("File:%s Has %d buffers", fname, cnt));

   delete inp;
}

void TestNewGenerator()
{
   dabc::Buffer* buf = dabc::Buffer::CreateBuffer(16*1024);
   mbs::NewGenerateBuffer(buf, 4, 100);
   mbs::NewIterateBuffer(buf);
   dabc::Buffer::Release(buf);
}

void TestMbsIO(const char* fname, const char* outname)
{
   mbs::MbsEventInput* inp = new mbs::MbsEventInput();

   mbs::MbsEventOutput* out = new mbs::MbsEventOutput();

   ::remove(outname);

   long cnt = 0;

   DOUT1(("TestMbsIO start %s", fname));

   if (inp->OpenFile(fname) && out->CreateOutputFile(outname)) {

      dabc::Buffer* buf = dabc::Buffer::CreateBuffer(1024);

      unsigned sz = 0;

      while ((sz = inp->Read_Size()) <= dabc::DataInput::di_ValidSize) {

         buf->RellocateBuffer(sz);

         inp->Read_Complete(buf);

         if (cnt<10)
            mbs::IterateEvent(buf->GetDataLocation());

         out->WriteBuffer(buf);

         cnt++;
      }

      dabc::Buffer::Release(buf);
   }

   DOUT1(("File:%s Get %d events", fname, cnt));

   delete inp;
   delete out;
}


   typedef struct abc {
     int16_t v1;
     int32_t v2;
     int32_t v3;
     int16_t v4;
   };


void TestList()
{
   std::list<int> mylist;

   mylist.push_back(123);

   std::list<int>::iterator iter = mylist.begin();

   DOUT1(("Value = %d sizeof = %d", *iter, sizeof(iter)));

   *mylist.rbegin() = 321;

   DOUT1(("Value = %d", *iter));
}


int main(int numc, char* args[])
{
   dabc::SetDebugLevel(1);

   TestMbsTrServerNew("x86g-4.gsi.de", 6000, "test.lmd");
//   return 0;

//   TestList();
//   return 0;

//   TestMbsIO("gauss.lmd","gauss_out.lmd");
//   TestMbsTrServer("x86g-4");
//   TestMbsInpFile("gauss.lmd");
//   TestMbsInpFile("x1.lmd");

//   TestMbsStream("test.lmd");

//   TestMbsInputFile("test.lmd");
//   TestNewGenerator();

   return 0;


//   TestMbsCombiner("test.lmd","testoutput.lmd");
   TestMbsInputFile("test.lmd");
   TestMbsInputFile("testoutput.lmd");
   return 0;



   TestMbsTrServerNew("lxg0500", 8000, "test.lmd");

   TestMbsInputFile("test.lmd");
   TestMbsInputFile("gauss.lmd");
   TestMbsInputFile("x1.lmd");
   return 0;


//   TestMbsIO("x1.lmd", "x1_out.lmd");
//   TestMbsIO("gauss.lmd", "gauss_out.lmd");

   TestMbsFileRepeater("x1.lmd", "x1_out.lmd", false);
   TestMbsFileRepeater("test.lmd", "test_out.lmd", true);
//   TestMbsInpFile("x1_out.lmd");
   return 0;
}
