#ifndef DABC_StandaloneManager
#define DABC_StandaloneManager

#include "dabc/Manager.h"

#include "dabc/threads.h"

#include <vector>

#include <list>

namespace dabc {

   class CommandChannelModule;
   class StateMachineModule;
   class Command;
   class Parameter;

   class StandaloneManager : public Manager  {

      struct ParamReg {
         Parameter* par; // pointer on parameter
         bool ismaster; // indicate if parameter is master (observed by slave)
         int srcnode;    // nodeid where original parameter is situated
         int tgtnode;    // nodeid where dependent parameter is situated
         String remname; // name of source/target parameter on remote node
         bool active;    // indicates if information is delivered to/from remote host
         String defvalue; // value of parameter if dependent parameter is not connected

         ParamReg(Parameter* p) : par(p), ismaster(false), srcnode(0), tgtnode(0), remname(), active(false), defvalue() {}
         ParamReg(const ParamReg& src) :
            par(src.par), ismaster(src.ismaster), srcnode(src.srcnode), tgtnode(src.tgtnode), remname(src.remname), active(src.active), defvalue(src.defvalue) {}
      };

      typedef std::list<ParamReg> ParamRegList;

      public:
         StandaloneManager(int nodeid, int numnodes, bool usesm = true);
         virtual ~StandaloneManager();

         void ConnectCmdChannel(int numnodes, int deviceid = 1, const char* controllerID = "file.txt");
         // deviceid=1 for socket, =2 for verbs

         void ConnectCmdChannelOld(int numnodes, int deviceid = 1, const char* controllerID = "file.txt");
         // deviceid=1 for socket, =2 for verbs

         virtual void ParameterEvent(Parameter* par, int event);

         virtual bool HasClusterInfo() { return IsMainManager(); }
         virtual int NumNodes();
         virtual int NodeId() const { return fNodeId; }
         virtual bool IsNodeActive(int num);
         virtual const char* GetNodeName(int num);

         virtual bool Subscribe(Parameter* par, int remnode, const char* remname);
         virtual bool Unsubscribe(Parameter* par);

         virtual bool IsMainManager() { return fIsMainMgr; }

      protected:

         void DisconnectCmdChannels();
         void WaitDisconnectCmdChannel();

         virtual bool CanSendCmdToManager(const char* mgrname);
         virtual bool SendOverCommandChannel(const char* managername, const char* cmddata);

         int DefinePortId(const char* managername);

         virtual int ExecuteCommand(Command* cmd);

         Parameter* FindControlParameter(const char* name);

         void SubscribedParChanged(ParamReg& reg);

         void CheckSubscriptionList();

         bool                  fIsMainMgr;
         String                fMainMgrName;
         std::vector<String>   fClusterNames;
         std::vector<bool>     fClusterActive;

         int                   fNodeId;
         CommandChannelModule *fCmdChannel;
         String                fCmdDeviceId;
         String                fCmdDevName;

         Condition             fStopCond;

         std::list<ParamReg>   fParReg;
   };
};

#endif
