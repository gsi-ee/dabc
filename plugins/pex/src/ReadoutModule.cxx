/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009- 
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH 
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/
#include "pex/ReadoutModule.h"
#include "pex/Commands.h"

#include "dabc/logging.h"
#include "dabc/Port.h"
#include "dabc/Manager.h"
#include "mbs/MbsTypeDefs.h"

namespace pex
{
  const char *xmlUseFsq = "UseFsq";    //< switch usage of FSQ lustre/tape storage server
  const char *xmlFsqServer = "FsqServername";
  const char *xmlFsqFilesystem = "FsqFSname";
  const char *xmlFsqDestination = "FsqDestination";
  const char *xmlFsqPort = "FsqPort";
  const char *xmlFsqNode = "FsqNode";
  const char *xmlFsqPass = "FsqPass";
}

pex::ReadoutModule::ReadoutModule (const std::string name, dabc::Command  cmd  ) :
    dabc::ModuleAsync (name)
{
  EnsurePorts (1, 1, dabc::xmlWorkPool);
  std::string ratesprefix = "Pexor";

  fEventRateName = ratesprefix + "Events";
  fDataRateName = ratesprefix + "Data";
  fInfoName = ratesprefix + "Info";
  fFileStateName= ratesprefix + "FileOn";
  CreatePar (fEventRateName).SetRatemeter (false, 3.).SetUnits ("Ev");
  CreatePar (fDataRateName).SetRatemeter (false, 3.).SetUnits ("Mb");
  CreatePar(fInfoName, "info").SetSynchron(true, 2., false).SetDebugLevel(2);
  CreatePar(fFileStateName).Dflt(false);

  Par (fDataRateName).SetDebugLevel (1);
  Par (fEventRateName).SetDebugLevel (1);


  fUseFsq= Cfg(pex::xmlUseFsq, cmd).AsBool(false);
  fFsqServer= Cfg(xmlFsqServer, cmd).AsStr("lxfsq11");
  fFsqPort= Cfg(xmlFsqPort, cmd).AsInt(7625);
  fFsqDestination= Cfg(xmlFsqDestination, cmd).AsInt(4);
  fFsqFilesystem= Cfg(xmlFsqFilesystem, cmd).AsStr("/lustre");
  fFsqNode= Cfg(xmlFsqNode, cmd).AsStr("myaccount");
  fFsqPass= Cfg(xmlFsqPass, cmd).AsStr("secret");








  CreateCmdDef(mbs::comStartFile)
        .AddArg(dabc::xmlFileName, "string", true)
        .AddArg(dabc::xmlFileSizeLimit, "int", false, 1000) //;
        .AddArg(pex::xmlUseFsq, "bool", false, false);






     CreateCmdDef(mbs::comStopFile);

     CreateCmdDef(mbs::comStartServer)
        .AddArg(mbs::xmlServerKind, "string", true, mbs::ServerKindToStr(mbs::StreamServer))
        .AddArg(mbs::xmlServerPort, "int", false, 6901);
     CreateCmdDef(mbs::comStopServer);


  PublishPars("$CONTEXT$/PexReadout");


  DOUT1("pex::ReadoutModule configured with FSQ=%d (server=%s, node=%s, port=%d, filesystem=%s)", fUseFsq, fFsqServer.c_str(), fFsqNode.c_str(), fFsqPort, fFsqFilesystem.c_str());

}

void pex::ReadoutModule::BeforeModuleStart ()
{
  DOUT1 ("pex::ReadoutModule::BeforeModuleStart");

}

void pex::ReadoutModule::AfterModuleStop ()
{
  DOUT1 ("pex::ReadoutModule finished. Rate %5.1f Mb/s", Par (fDataRateName).Value ().AsDouble ());

}

void pex::ReadoutModule::ProcessInputEvent (unsigned /* port*/ )
{
  DoPexorReadout ();
}

void pex::ReadoutModule::ProcessOutputEvent (unsigned /* port*/  )
{
  DoPexorReadout ();
}

void pex::ReadoutModule::DoPexorReadout ()
{
  dabc::Buffer ref;
  DOUT3 ("pex::DoPexorReadout\n");
  try
  {
    while (CanRecv ())
    {
      if (!CanSendToAllOutputs ())
      {
        DOUT3 ("pex::ReadoutModule::DoPexorReadout - can not send to all outputs. skip event \n");
        return;
      }
      ref = Recv ();
      if (!ref.null ())
      {
        Par (fDataRateName).SetValue ((double)(ref.GetTotalSize ()) / 1024. / 1024.);
        DOUT3 ("pex::ReadoutModule::DoPexorReadout - has buffer size: total %d bytes  \n",ref.GetTotalSize ());
        SendToAllOutputs (ref);
        Par (fEventRateName).SetValue (1);
      }
    }



  }
  catch (dabc::Exception& e)
  {
    DOUT1 ("pex::ReadoutModule::DoPexorReadout - raised dabc exception %s", e.what ());
    ref.Release ();
    // how do we treat this?
  }
  catch (std::exception& e)
  {
    DOUT1 ("pex::ReadoutModule::DoPexorReadout - raised std exception %s ", e.what ());
    ref.Release ();
  }
  catch (...)
  {
    DOUT1 ("pex::ReadoutModule::DoPexorReadout - Unexpected exception!!!");
    throw;
  }

}

int pex::ReadoutModule::ExecuteCommand(dabc::Command cmd)
{

  // this is section taken from mbs combiner
  if (cmd.IsName(mbs::comStartFile)) {

    std::string fname = cmd.GetStr(dabc::xmlFileName); //"filename")
    int maxsize = cmd.GetInt(dabc::xml_maxsize, 30); // maxsize
    bool usefsq = cmd.GetBool(pex::xmlUseFsq, fUseFsq); // FSQ mode
    std::string url;
    if(usefsq)
    {
      url= dabc::format("%s://%s?%s=%d&ltsm&ltsmUseFSD=true&ltsmNode=%s&ltsmPass=%s&ltsmFsname=%s&ltsmFSDServerName=%s&ltsmFSDServerPort=%d&ltsmFSQDestination=%d",
          mbs::protocolLmd, fname.c_str(), dabc::xml_maxsize, maxsize, fFsqNode.c_str(),fFsqPass.c_str(), fFsqFilesystem.c_str(), fFsqServer.c_str(), fFsqPort, fFsqDestination);
      // JAM26 for hades this looks like:
      //&ltsm&ltsmServer=${LTSMSERVER}&ltsmNode=${LTSMNODE}&ltsmPass=${LTSMPASSWD}&ltsmFsname=${LTSMFSNAME}&ltsmOwner=hadesdst&ltsmUseFSD=${USEFSD}&ltsmFSDServerName=${FSDSERVER}&ltsmFSDServerPort=${FSDPORT}&ltsmMaxSessionFiles=${LTSMSESSIONFILES}&ltsmDaysubfolders=true&ltsmFSQDestination=${FSQDEST}&slave" queue="5000" onerror="exit"/>
    }
    else
    {
      url= dabc::format("%s://%s?%s=%d", mbs::protocolLmd, fname.c_str(), dabc::xml_maxsize, maxsize);
    }



    EnsurePorts(0, 2);
    bool res = dabc::mgr.CreateTransport(OutputName(1, true), url);
    DOUT0("Started file %s res = %d", url.c_str(), res);
    SetInfo(dabc::format("Execute StartFile for %s, result=%d",url.c_str(), res), true);
    ChangeFileState(true);
    return cmd_bool(res);
     } else
     if (cmd.IsName(mbs::comStopFile)) {
        FindPort(OutputName(1)).Disconnect();
        SetInfo("Stopped file", true);
        ChangeFileState(false);
        return dabc::cmd_true;
     } else
     if (cmd.IsName(mbs::comStartServer)) {
        if (NumOutputs()<1) {
           EOUT("No ports was created for the server");
           return dabc::cmd_false;
        }
        std::string skind = cmd.GetStr(mbs::xmlServerKind);

        int port = cmd.GetInt(mbs::xmlServerPort, 6666);
        std::string url = dabc::format("mbs://%s?%s=%d", skind.c_str(), mbs::xmlServerPort,  port);
        EnsurePorts(0, 1);
        bool res = dabc::mgr.CreateTransport(OutputName(0, true));
        DOUT0("Started server %s res = %d", url.c_str(), res);
        SetInfo(dabc::format("Execute StartServer for %s, result=%d",url.c_str(), res), true);
        return cmd_bool(res);
     } else
     if (cmd.IsName(mbs::comStopServer)) {
        FindPort(OutputName(0)).Disconnect();
        SetInfo("Stopped server", true);
        return dabc::cmd_true;
     }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}






void pex::ReadoutModule::SetInfo(const std::string& info, bool forceinfo)
{
//   DOUT0("SET INFO: %s", info.c_str());

   dabc::InfoParameter par;

   if (!fInfoName.empty()) par = Par(fInfoName);

   par.SetValue(info);
   if (forceinfo)
      par.FireModified();
}


void pex::ReadoutModule::ChangeFileState(bool on)
{
  dabc::Parameter par=Par (fFileStateName);
  par.SetValue (on);
  dabc::Hierarchy chld = fWorkerHierarchy.FindChild(fFileStateName.c_str());
  if (!chld.null())
  {
       par.ScanParamFields(&chld()->Fields());
       fWorkerHierarchy.MarkChangedItems();
       DOUT0("ChangeFileState to %d", on);
  }
  else
  {
      DOUT0("ChangeFileState Could not find parameter %s", fFileStateName.c_str());
  }





}
