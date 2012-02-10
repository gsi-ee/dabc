#ifndef BNET_TransportRunnable
#define BNET_TransportRunnable

#include "dabc/threads.h"

#include "dabc/Module.h"

namespace bnet {

   class TransportRunnable : public dabc::Runnable {
      protected:

         dabc::Mutex fMutex; /** Main mutex for communication with the rest of the world*/

         int    fNodeId; // node number
         int    fNumNodes;   // total number of node
         bool*  fActiveNodes; // arrays with active node flags

         int    fCmdId;         // current command id - 0 is empty
         int    fCmdBufferSize; // maximum size of data for command
         int    fCmdDataSize;   // current size of data in command
         char*  fCmdDataBuffer; // data itself

         enum RunnableCommands {
            cmd_None,       // no any command
            cmd_Ready,      // command execution is done
            cmd_Fail,       // command execution failed
            cmd_Exit,       // exit main loop
            cmd_ActiveNodes,// set flags which nodes are active and which are not
            cmd_CreateQP,   // create connection handles - queue pair in IB
            cmd_ConnectQP,  // create connection handles - queue pair in IB
            cmd_CloseQP     // create connection handles - queue pair in IB
         };

         int NodeId() const { return fNodeId; }
         int NumNodes() const { return fNumNodes; }
         bool IsActiveNode(int id) const { return true; }


         /*************** Methods, used from other threads ******************/

         /** Method should be used from module thread to submit new command to the runnable */
         bool SubmitCmd(int cmdid, void* args = 0, int argssize = 0);

         /** Method should be used from module thread to verify that command execution is finished */
         bool WaitCmdDone(void* res = 0, int* ressize = 0);

         /** Method which is only allow to call from the runnable thread itself */
         virtual bool ExecuteSetActiveNodes();
         virtual bool ExecuteCreateQPs();
         virtual bool ExecuteConnectQPs();
         virtual bool ExecuteCloseQPs();

         virtual void RunnableCancelled() {}

      public:

         // dabc::Mutex& mutex() { return fMutex; }

         TransportRunnable();
         virtual ~TransportRunnable();

         void SetNodeId(int id, int num) { fNodeId = id; fNumNodes = num; }

         int GetCmdBufferSize() const { return fCmdBufferSize; }


         virtual bool Configure(dabc::Module* m, dabc::Command cmd);

         virtual void* MainLoop();

         // =================== Interface commands to the runnable ===================

         // size required for storing all connection handles of local node
         virtual int ConnectionBufferSize() { return 1024; }

         virtual void ResortConnections(void*, void*) { }

         bool SetActiveNodes(void* data, int datasize);
         bool CreateQPs(void* recs, int& recssize);
         bool ConnectQPs(void* recs, int recssize);
         bool CloseQPs();
         bool StopRunnable();
   };

}

#endif
