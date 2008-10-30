#include "mbs/MbsFactory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"

#include "mbs/MbsEventAPI.h"
#include "mbs/MbsInputFile.h"
#include "mbs/MbsOutputFile.h"
#include "mbs/MbsDataInput.h"
#include "mbs/LmdInput.h"
#include "mbs/LmdOutput.h"
#include "mbs/MbsDevice.h"


mbs::MbsFactory mbsfactory("mbs");


dabc::Device* mbs::MbsFactory::CreateDevice(dabc::Basic* parent, const char* classname, const char* devname, dabc::Command*)
{
   if (strcmp(classname, "MbsDevice")!=0) return 0;

   DOUT5(("Creating MBS device with name %s", devname));
   
   return new mbs::MbsDevice(parent, devname);
}

dabc::DataInput* mbs::MbsFactory::CreateDataInput(const char* typ, const char* name, dabc::Command* cmd)
{
   if ((typ==0) || (strlen(typ)==0)) return 0;
    
   DOUT3(("MbsFactory::CreateDataInput %s", typ));
   
   if (strcmp(typ, LmdFileType())==0) {
      int nummulti = 0;
      int firstmulti = 0;
      
      if (cmd) {
         nummulti = cmd->GetInt("NumMulti", 0);
         firstmulti = cmd->GetInt("FirstMulti", 0);
      }
       
      mbs::LmdInput* inp = new mbs::LmdInput(name, nummulti, firstmulti);
      if (inp->Init()) return inp;
      delete inp;
       
   } else
   
   if (strcmp(typ, NewFileType())==0) {
      const char* iotyp = 0;
      int nummulti = 0;
      int firstmulti = 0;
      
      if (cmd) {
         iotyp = dabc::CmdCreateDataTransport::GetIOTyp(cmd);
         nummulti = cmd->GetInt("NumMulti", 0);
         firstmulti = cmd->GetInt("FirstMulti", 0);
      }
       
      mbs::MbsInputFile* inp = new mbs::MbsInputFile(name, iotyp, nummulti, firstmulti);
      if (inp->Init()) return inp;
      delete inp;
   } else
   if (strcmp(typ, NewTransportType())==0) {
      mbs::MbsDataInput* inp = new mbs::MbsDataInput();
      
      int mbs_port = cmd ? cmd->GetInt("Port", 6000) : 6000;
      
      DOUT1(("Trying to connnect to node %s port %d", name, mbs_port));
      
      if (inp->Open(name, mbs_port)) return inp;
      delete inp;   
   } else 
   if (strcmp(typ, FileType())==0) {
      mbs::MbsEventInput* inp = new mbs::MbsEventInput();
      if (inp->OpenFile(name, cmd ? cmd->GetStr("MbsTagFile", 0) : 0)) return inp;
      delete inp;
   } else 
   if (strcmp(typ, "MbsTransport")==0) {
      mbs::MbsEventInput* inp = new mbs::MbsEventInput();
      if (inp->OpenTransportServer(name)) return inp;
      delete inp;
   } else 
   if (strcmp(typ, "MbsStream")==0) {
      mbs::MbsEventInput* inp = new mbs::MbsEventInput();
      if (inp->OpenStreamServer(name)) return inp;
      delete inp;
   } else 
   if (strcmp(typ, "MbsEvent")==0) {
      mbs::MbsEventInput* inp = new mbs::MbsEventInput();
      if (inp->OpenEventServer(name)) return inp;
      delete inp;
   } else 
   if (strcmp(typ, "MbsRemote")==0) {
      mbs::MbsEventInput* inp = new mbs::MbsEventInput();
      if (inp->OpenRemoteEventServer(name, cmd ? cmd->GetInt("MbsPort", 6009) : 6009)) return inp;
      delete inp;
   } 
   
   return 0;
}


dabc::DataOutput* mbs::MbsFactory::CreateDataOutput(const char* typ, const char* name, dabc::Command* cmd)
{
    
   if ((typ==0) || (strlen(typ)==0)) return 0;

   DOUT3(("MbsFactory::CreateDataOutput typ:%s", typ)); 

   if (strcmp(typ, LmdFileType())==0) {

      unsigned sizelimit_mb = 0;
      int nummulti = 0;
      int firstmulti = 0;
      
      if (cmd) {
         sizelimit_mb = cmd->GetUInt("SizeLimit", 0);
         nummulti = cmd->GetInt("NumMulti", 0);
         firstmulti = cmd->GetInt("FirstMulti", 0);
      }

      mbs::LmdOutput* out = new mbs::LmdOutput(name, sizelimit_mb, nummulti, firstmulti);

      if (out->Init()) return out;

      delete out;
       
   } else

   if (strcmp(typ, NewFileType())==0) {
      const char* iotyp = 0;
      unsigned sizelimit = 0;
      int nummulti = 0;
      int firstmulti = 0;
      
      if (cmd) {
         iotyp = dabc::CmdCreateDataTransport::GetIOTyp(cmd);
         sizelimit = cmd->GetUInt("SizeLimit", 0);
         nummulti = cmd->GetInt("NumMulti", 0);
         firstmulti = cmd->GetInt("FirstMulti", 0);
      }

      mbs::MbsOutputFile* out = new mbs::MbsOutputFile(name, iotyp, sizelimit, nummulti, firstmulti);

      if (out->Init()) return out;

      delete out;
   } else 
   if (strcmp(typ, FileType())==0) {
      mbs::MbsEventOutput* out = new mbs::MbsEventOutput();
      if (out->CreateOutputFile(name)) return out;
      delete out;
   }

   return 0;
}
