#include "mbs/Factory.h"

#include "dabc/string.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"

#include "mbs/EventAPI.h"
#include "mbs/MbsDataInput.h"
#include "mbs/LmdInput.h"
#include "mbs/LmdOutput.h"
#include "mbs/Device.h"


mbs::Factory mbsfactory("mbs");

dabc::Device* mbs::Factory::CreateDevice(dabc::Basic* parent, const char* classname, const char* devname, dabc::Command*)
{
   if (strcmp(classname, "MbsDevice")!=0) return 0;

   DOUT5(("Creating MBS device with name %s", devname));

   return new mbs::Device(parent, devname);
}

dabc::DataInput* mbs::Factory::CreateDataInput(const char* typ, const char* name, dabc::Command* cmd)
{
   if ((typ==0) || (strlen(typ)==0)) return 0;

   DOUT3(("Factory::CreateDataInput %s", typ));

   if (strcmp(typ, LmdFileType())==0) {

      return new mbs::LmdInput();

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
   if (strcmp(typ, NewTransportType())==0) {
      mbs::MbsDataInput* inp = new mbs::MbsDataInput();

      int mbs_port = cmd ? cmd->GetInt("Port", 6000) : 6000;

      DOUT1(("Trying to connect to node %s port %d", name, mbs_port));

      if (inp->Open(name, mbs_port)) return inp;
      delete inp;
   } else
   if (strcmp(typ, xmlEvapiType) == 0) {
      return new mbs::EvapiInput(xmlEvapiFile, name);
   } else
   if (strcmp(typ, xmlEvapiFile) == 0) {
      return new mbs::EvapiInput(xmlEvapiFile, name);
   } else
   if (strcmp(typ, xmlEvapiRFIOFile) == 0) {
      return new mbs::EvapiInput(xmlEvapiRFIOFile, name);
   } else
   if (strcmp(typ, xmlEvapiTransportServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiTransportServer, name);
   } else
   if (strcmp(typ, xmlEvapiStreamServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiStreamServer, name);
   } else
   if (strcmp(typ, xmlEvapiEventServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiEventServer, name);
   } else
   if (strcmp(typ, xmlEvapiRemoteEventServer) == 0) {
      return new mbs::EvapiInput(xmlEvapiRemoteEventServer, name);
   } else
   if (strcmp(typ, FileType())==0) {
      mbs::EvapiInput* inp = new mbs::EvapiInput();
      if (inp->OpenFile(name, cmd ? cmd->GetStr("MbsTagFile", 0) : 0)) return inp;
      delete inp;
   } else
   if (strcmp(typ, "MbsTransport")==0) {
      mbs::EvapiInput* inp = new mbs::EvapiInput();
      if (inp->OpenTransportServer(name)) return inp;
      delete inp;
   } else
   if (strcmp(typ, "MbsStream")==0) {
      mbs::EvapiInput* inp = new mbs::EvapiInput();
      if (inp->OpenStreamServer(name)) return inp;
      delete inp;
   } else
   if (strcmp(typ, "MbsEvent")==0) {
      mbs::EvapiInput* inp = new mbs::EvapiInput();
      if (inp->OpenEventServer(name)) return inp;
      delete inp;
   } else
   if (strcmp(typ, "MbsRemote")==0) {
      mbs::EvapiInput* inp = new mbs::EvapiInput();
      if (inp->OpenRemoteEventServer(name, cmd ? cmd->GetInt("MbsPort", 6009) : 6009)) return inp;
      delete inp;
   }

   return 0;
}


dabc::DataOutput* mbs::Factory::CreateDataOutput(const char* typ, const char* name, dabc::Command* cmd)
{

   if ((typ==0) || (strlen(typ)==0)) return 0;

   DOUT3(("Factory::CreateDataOutput typ:%s", typ));

   if (strcmp(typ, LmdFileType())==0) {

      return new mbs::LmdOutput();

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
   if (strcmp(typ, xmlEvapiOutFile) == 0) {
      return new mbs::EvapiOutput(name);
   } else
   if (strcmp(typ, FileType())==0) {
      mbs::EvapiOutput* out = new mbs::EvapiOutput();
      if (out->CreateOutputFile(name)) return out;
      delete out;
   }

   return 0;
}
