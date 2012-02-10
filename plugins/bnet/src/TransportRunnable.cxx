#include "bnet/TransportRunnable.h"

bnet::TransportRunnable::TransportRunnable() :
   dabc::Runnable(),
   fMutex(),
   fNodeId(0),
   fNumNodes(1),
   fActiveNodes(0),
   fCmdId(0),
   fCmdBufferSize(0),
   fCmdDataSize(0),
   fCmdDataBuffer(0)
{
}

bnet::TransportRunnable::~TransportRunnable()
{
   if (fCmdDataBuffer!=0) {
      delete [] fCmdDataBuffer;
      fCmdDataBuffer = 0;
   }

   if (fActiveNodes!=0) {
      delete [] fActiveNodes;
      fActiveNodes = 0;
   }

   fCmdBufferSize = 0;
}


bool bnet::TransportRunnable::Configure(dabc::Module* m, dabc::Command cmd)
{
   fCmdBufferSize = 4096;
   if (fCmdBufferSize < ConnectionBufferSize() + 1024)
      fCmdBufferSize = ConnectionBufferSize() + 1024;

   fCmdDataBuffer = new char[fCmdBufferSize];

   fActiveNodes = new bool[NumNodes()];
   for (int n=0;n<NumNodes();n++)
      fActiveNodes[n] = true;

   fCmdId = cmd_None;
   fCmdDataSize = 0;

   return true;
}


bool bnet::TransportRunnable::SubmitCmd(int cmdid, void* args, int argssize)
{
   if (fCmdBufferSize<argssize) {
      EOUT(("Command arguments %d too large !!!!", argssize));
      return false;
   }

   // TODO: implement timeout instead of counter
   int cnt = 1000000;
   while (cnt-->0) {
      {
         // we apply mutex only when fCmdId value is accessed
         dabc::LockGuard lock(fMutex);
         if (fCmdId!=cmd_None) continue;
      }

      fCmdDataSize = 0;

      if (args && argssize) {
         memcpy(fCmdDataBuffer, args, argssize);
         fCmdDataSize = argssize;
      }

      // we apply mutex only when fCmdId value is accessed
      dabc::LockGuard lock(fMutex);
      fCmdId = cmdid;
      return true;
   }

   return false;
}


bool bnet::TransportRunnable::WaitCmdDone(void* res, int* ressize)
{
   // TODO: implement timeout instead of counter
   int cnt = 1000000;

   while (cnt-->0) {

      bool cmd_res(false);

      {
         // we lock only changing of command, buffer can be copied outside lock
         dabc::LockGuard lock(fMutex);
         if ((fCmdId!=cmd_Ready) && (fCmdId!=cmd_Ready)) continue;
         cmd_res = fCmdId==cmd_Ready;
         fCmdId = cmd_None;
      }


      if (res && ressize && *ressize>0) {
         if (fCmdDataSize>*ressize) {
            EOUT(("Not enough space in result array!!!"));
         } else
            *ressize = fCmdDataSize;

         if (*ressize>0)
            memcpy(res, fCmdDataBuffer, *ressize);
      }

      fCmdDataSize = 0;

      return cmd_res;
   }
   return false;
}

bool bnet::TransportRunnable::ExecuteSetActiveNodes()
{
   if (fCmdDataSize != NumNodes()) {
      EOUT(("Wrong size of active nodes array"));
      return false;
   }

   uint8_t* buff = (uint8_t*) fCmdDataBuffer;

   for (int n=0;n<NumNodes();n++)
      fActiveNodes[n] = buff>0;

   fCmdDataSize = 0;
   return true;
}


bool bnet::TransportRunnable::ExecuteCreateQPs()
{
   fCmdDataSize = 0;
   return true;
}


bool bnet::TransportRunnable::ExecuteConnectQPs()
{
   fCmdDataSize = 0;
   return true;
}


bool bnet::TransportRunnable::ExecuteCloseQPs()
{
   fCmdDataSize = 0;
   return true;
}


void* bnet::TransportRunnable::MainLoop()
{
   DOUT0(("Enter TransportRunnable::MainLoop()"));

   while (true) {

      int exeid = cmd_None;

      {
         dabc::LockGuard lock(fMutex);
         exeid = fCmdId;
      }

      if (exeid==cmd_Exit) break;

      bool exe_res(true), did_exe(false);

      switch (exeid) {
         case cmd_None:  break;
         case cmd_Ready: break;
         case cmd_Fail: break;
         case cmd_ActiveNodes:
            exe_res = ExecuteSetActiveNodes();
            did_exe = true;
            break;
         case cmd_CreateQP:
            exe_res = ExecuteCreateQPs();
            did_exe = true;
            break;
         case cmd_ConnectQP:
            exe_res = ExecuteConnectQPs();
            did_exe = true;
            break;
         case cmd_CloseQP:
            exe_res = ExecuteCloseQPs();
            did_exe = true;
            break;

         default:
            EOUT(("Why here??"));
            break;
      }

      if (did_exe) {
         dabc::LockGuard lock(fMutex);
         fCmdId = exe_res ? cmd_Ready : cmd_Fail;
      }

   }

   DOUT0(("Exit TransportRunnable::MainLoop()"));

   return 0;
}

bool bnet::TransportRunnable::SetActiveNodes(void* data, int datasize)
{
   if (!SubmitCmd(cmd_ActiveNodes, data, datasize)) return false;

   return WaitCmdDone();
}

bool bnet::TransportRunnable::CreateQPs(void* recs, int& recssize)
{
   if (!SubmitCmd(cmd_CreateQP)) return false;

   return WaitCmdDone(recs, &recssize);
}

bool bnet::TransportRunnable::ConnectQPs(void* recs, int recssize)
{
   if (!SubmitCmd(cmd_ConnectQP, recs, recssize)) return false;

   return WaitCmdDone();
}

bool bnet::TransportRunnable::CloseQPs()
{
   if (!SubmitCmd(cmd_CloseQP)) return false;

   return WaitCmdDone();
}

bool bnet::TransportRunnable::StopRunnable()
{
   // one exit command is submitted, runnable will not be
   return SubmitCmd(cmd_Exit);
}

