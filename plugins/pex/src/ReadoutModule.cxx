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
  const char *xmlRun = "RunNumber";

}

pex::ReadoutModule::ReadoutModule (const std::string name, dabc::Command  cmd  ) :
    dabc::ModuleAsync (name), fOut(), fFlushFlag(true)
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

  fRunNum=Cfg(pex::xmlRun, cmd).AsInt(0); // user may optionally set the beginning run number
  fFlushTimeout = Cfg(dabc::xmlFlushTimeout, cmd).AsDouble(1.);
  if (fFlushTimeout>0.) CreateTimer("FlushTimer", fFlushTimeout);



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



bool pex::ReadoutModule::DoPexorReadout ()
{
  dabc::Buffer inbuf;
  uint32_t eventsize=0;
  DOUT3 ("pex::DoPexorReadout\n");
  try
  {
          inbuf = Recv ();
          //ref=RecvQueueItem(0, 0);
          if (!inbuf.null ())
          {
            eventsize=inbuf.GetTotalSize ();
            Par (fDataRateName).SetValue ((double)(eventsize) / 1024. / 1024.);
            Par (fEventRateName).SetValue (1);
            DOUT3 ("pex::ReadoutModule::DoPexorReadout - has input buffer size: total %d bytes  \n",eventsize);

            // from mbs Combiner Module: get new buffer
            if (fOut.IsBuffer() && !fOut.IsPlaceForEvent(eventsize- sizeof(mbs::EventHeader),false )) //raw data is behind subevent header! use CopyEventFrom
              {
                if (!FlushBuffer()) return false;
              }
            if (!fOut.IsBuffer())
              {
                dabc::Buffer buf = TakeBuffer();
                if (buf.null()) return false;
                if (!fOut.Reset(buf)) {
                  EOUT("Cannot use buffer for output - hard error!!!!");
                  buf.Release();
                  dabc::mgr.StopApplication();
                  return false;
                }
              }
            if (!fOut.IsPlaceForEvent(eventsize - sizeof(mbs::EventHeader),false ))
              {
                EOUT("Event size %lu too big for buffer, skip event completely", (long unsigned) (eventsize));
              }
            else
            {
               // copy input buffer to output iterator
              //  We do not need information of input event headers (read iterator in mbs combiner); just copy complete payload here
              dabc::Pointer inptr(inbuf);
              fOut.CopyEventFrom(inptr, true);
            }
            return true;
          }
  }
  catch (dabc::Exception& e)
  {
    DOUT1 ("pex::ReadoutModule::DoPexorReadout - raised dabc exception %s", e.what ());
    inbuf.Release ();
    // how do we treat this?
  }
  catch (std::exception& e)
  {
    DOUT1 ("pex::ReadoutModule::DoPexorReadout - raised std exception %s ", e.what ());
    inbuf.Release ();
  }
  catch (...)
  {
    DOUT1 ("pex::ReadoutModule::DoPexorReadout - Unexpected exception!!!");
    throw;
  }
  return false;
}

int pex::ReadoutModule::ExecuteCommand (dabc::Command cmd)
{

  // this is section taken from mbs combiner
  if (cmd.IsName (mbs::comStartFile))
  {
    std::string fname = cmd.GetStr (dabc::xmlFileName);    //"filename")
    int maxsize = cmd.GetInt (dabc::xml_maxsize, 30);    // maxsize
    bool usefsq = cmd.GetBool (pex::xmlUseFsq, fUseFsq);    // FSQ mode

    if (fLastFileName != fname)
      fRunNum = 0;    // reset file sequence number if user has changed the run name - later TODO: run number configuration

    std::string url;
    if (usefsq)
    {
      url =
          dabc::format (
              "%s://%s_%03d?%s=%d&ltsm&ltsmUseFSD=true&ltsmNode=%s&ltsmPass=%s&ltsmFsname=%s&ltsmFSDServerName=%s&ltsmFSDServerPort=%d&ltsmFSQDestination=%d",
              mbs::protocolLmd, fname.c_str (), fRunNum, dabc::xml_maxsize, maxsize, fFsqNode.c_str (), fFsqPass.c_str (),
              fFsqFilesystem.c_str (), fFsqServer.c_str (), fFsqPort, fFsqDestination);
    }
    else
    {
      url = dabc::format ("%s://%s_%03d?%s=%d", mbs::protocolLmd, fname.c_str (), fRunNum, dabc::xml_maxsize, maxsize);
    }

    fLastFileName = fname;
    fRunNum++;

    EnsurePorts (0, 2);
    bool res = dabc::mgr.CreateTransport (OutputName (1, true), url);
    DOUT0("Started file %s res = %d", url.c_str (), res);
    SetInfo (dabc::format ("Execute StartFile for %s, result=%d", url.c_str (), res), true);
    ChangeFileState (true);
    return cmd_bool (res);
  }
  else if (cmd.IsName (mbs::comStopFile))
  {
    FindPort (OutputName (1)).Disconnect ();
    SetInfo ("Stopped file", true);
    ChangeFileState (false);
    return dabc::cmd_true;
  }
  else if (cmd.IsName (mbs::comStartServer))
  {
    if (NumOutputs () < 1)
    {
      EOUT("No ports was created for the server");
      return dabc::cmd_false;
    }
    std::string skind = cmd.GetStr (mbs::xmlServerKind);

    int port = cmd.GetInt (mbs::xmlServerPort, 6666);
    std::string url = dabc::format ("mbs://%s?%s=%d", skind.c_str (), mbs::xmlServerPort, port);
    EnsurePorts (0, 1);
    bool res = dabc::mgr.CreateTransport (OutputName (0, true));
    DOUT0("Started server %s res = %d", url.c_str (), res);
    SetInfo (dabc::format ("Execute StartServer for %s, result=%d", url.c_str (), res), true);
    return cmd_bool (res);
  }
  else if (cmd.IsName (mbs::comStopServer))
  {
    FindPort (OutputName (0)).Disconnect ();
    SetInfo ("Stopped server", true);
    return dabc::cmd_true;
  }

  return dabc::ModuleAsync::ExecuteCommand (cmd);
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


// JAM 20-04-26 - stolen from mbs CombinerModule
bool pex::ReadoutModule::FlushBuffer()
{
   DOUT3("pex::ReadoutModule::FlushBuffer() ....");
   if (fOut.IsEmpty() || !fOut.IsBuffer()) return false;
   if (!CanSendToAllOutputs()) return false;
   dabc::Buffer buf = fOut.Close();
   DOUT3("Send buffer of size = %d", buf.GetTotalSize());
   SendToAllOutputs(buf);
   fFlushFlag = false; // indicate that next flush timeout one not need to send buffer
   return true;
}

void pex::ReadoutModule::ProcessTimerEvent(unsigned)
{
   if (fFlushFlag) {
      unsigned cnt = 0;
      while (IsRunning() && (cnt < 100) && DoPexorReadout()) ++cnt;
      FlushBuffer();
   }
   fFlushFlag = true;
}


