#include "dabc/logging.h"

#include "dabc/FileIO.h"
#include "dabc/ModuleAsync.h"
#include "dabc/ModuleSync.h"
#include "dabc/Port.h"
#include "dabc/PoolHandle.h"
#include "dabc/Buffer.h"
#include "dabc/Manager.h"
#include "dabc/timing.h"

#include "mbs/EventAPI.h"
#include "mbs/Factory.h"
#include "mbs/MbsTypeDefs.h"
#include "mbs/Iterator.h"
#include "mbs/LmdInput.h"
#include "mbs/LmdOutput.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <list>


void IterateBuffer(dabc::Buffer* buf)
{
   if (buf==0) return;

   mbs::ReadIterator iter(buf);

   while (iter.NextEvent()) {
      DOUT1(("Event %u", iter.evnt()->EventNumber()));

      while (iter.NextSubEvent())
         DOUT1(("Subevent crate %u", iter.subevnt()->iSubcrate));
   }
}




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
               if (fBufferCnt<10) IterateBuffer(buf);

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

   dabc::Command* cmd = new dabc::CmdCreateInputTransport("Repeater/Ports/Input", mbs::typeLmdInput);
   cmd->SetStr(mbs::xmlFileName, inpfile);
   bool res = mgr.Execute(cmd);

   cmd = new dabc::CmdCreateOutputTransport("Repeater/Ports/Output", mbs::typeLmdOutput);
   cmd->SetStr(mbs::xmlFileName, outfile);
   res = mgr.Execute(cmd);

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
   mbs::EvapiInput* inp = new mbs::EvapiInput();
   long cnt = 0;

   if (inp->OpenFile(fname)) {

      dabc::Buffer* buf = 0;

      while ((buf = inp->ReadBuffer())>0) {
         if (cnt++<10)
            IterateBuffer(buf);

         dabc::Buffer::Release(buf);
      }
   }

   DOUT1(("File:%s Get %d events", fname, cnt));

   delete inp;
}

void TestMbsTrServer(const char* server)
{
   mbs::EvapiInput* inp = new mbs::EvapiInput();
   long cnt = 0;

   if (inp->OpenTransportServer(server)) {

      while ((inp->Read_Size()<=dabc::DataInput::di_ValidSize) && (cnt<10000)) {
         inp->Read_Complete(0);
         if (cnt<100)
            DOUT1((" Event buffer %p", inp->GetEventBuffer()));
         cnt++;
      }
   }

   DOUT1(("File:%s Get %d events", server, cnt));

   delete inp;
}

void TestMbsInputFile(const char* fname)
{
   mbs::LmdInput* inp = new mbs::LmdInput(fname);

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
         IterateBuffer(buf);

      cnt++;
   }

   dabc::Buffer::Release(buf);

   DOUT1(("File:%s Has %d buffers", fname, cnt));

   delete inp;
}

void TestMbsIO(const char* fname, const char* outname)
{
   mbs::EvapiInput* inp = new mbs::EvapiInput();

   mbs::EvapiOutput* out = new mbs::EvapiOutput();

   ::remove(outname);

   long cnt = 0;

   DOUT1(("TestMbsIO start %s", fname));

   if (inp->OpenFile(fname) && out->CreateOutputFile(outname)) {

      dabc::Buffer* buf = dabc::Buffer::CreateBuffer(1024);

      unsigned sz = 0;

      while ((sz = inp->Read_Size()) <= dabc::DataInput::di_ValidSize) {

         buf->RellocateBuffer(sz);

         inp->Read_Complete(buf);

         if (cnt<10) IterateBuffer(buf);

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

//   return 0;

//   TestList();
//   return 0;

//   TestMbsIO("gauss.lmd","gauss_out.lmd");
//   TestMbsTrServer("x86g-4");
//   TestMbsInpFile("gauss.lmd");
//   TestMbsInpFile("x1.lmd");

//   TestMbsStream("test.lmd");

//   TestMbsInputFile("test.lmd");

   return 0;


//   TestMbsCombiner("test.lmd","testoutput.lmd");
   TestMbsInputFile("test.lmd");
   TestMbsInputFile("testoutput.lmd");
   return 0;

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
