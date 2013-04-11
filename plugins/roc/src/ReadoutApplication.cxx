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
#include "roc/ReadoutApplication.h"

#include <time.h>

#include "dabc/Parameter.h"
#include "dabc/Command.h"
#include "dabc/timing.h"
#include "dabc/CommandsSet.h"
#include "dabc/Device.h"
#include "dabc/Module.h"
#include "dabc/ConnectionRequest.h"
#include "dabc/Url.h"

#include "mbs/MbsTypeDefs.h"
#include "mbs/Factory.h"

#include "roc/defines_roc.h"
#include "roc/UdpBoard.h"
#include "roc/UdpDevice.h"
#include "roc/Commands.h"
#include "roc/CombinerModule.h"

const char* nameRocComb    = "RocComb";
const char* nameRocCalibr  = "RocCalibr";
const char* nameSusiboComb = "SusiboComb";
const char* nameTRBComb    = "TRBComb";
const char* nameTRBTrans   = "TRBTrans";
const char* nameSuperComb  = "SuperComb";

roc::ReadoutApplication::ReadoutApplication() :
   dabc::Application(roc::xmlReadoutAppClass),
   fRocBrds()
{
   CreatePar(roc::xmlIgnoreMissingEpoch).DfltBool(false);

   CreatePar(roc::xmlRawReadout).DfltBool(true);

   CreatePar(roc::xmlSyncNumber).DfltInt(0);

   CreatePar(roc::xmlSyncScaleDown).DfltInt(-1);

   CreatePar(roc::xmlNumRocs).DfltInt(3);

   for (int nr = 0; nr < NumRocs(); nr++) {
      CreatePar( dabc::format("%s%d", xmlRocIp, nr) );
      CreatePar( dabc::format("%s%d", xmlRocFebs, nr) );
   }
   CreatePar(roc::xmlMeasureADC).DfltBool(false);
   CreatePar(roc::xmlUseDLM).DfltBool(true);

   CreatePar(roc::xmlEpicsNode).DfltStr("");

   CreatePar(roc::xmlNumMbs).DfltInt(0);
   for (int n = 0; n < NumMbs(); n++) {
      CreatePar(dabc::format("%s%d", roc::xmlMbsAddr, n)).DfltStr("");
      CreatePar(dabc::format("%s%d", roc::xmlSyncSubeventId, n)).DfltInt(0);
   }

   CreatePar(roc::xmlNumSusibo).DfltInt(0);
   for (int n = 0; n < NumSusibo(); n++)
      CreatePar(dabc::format("%s%d", roc::xmlSusiboAddr, n)).DfltInt(0);

   CreatePar(roc::xmlNumSlaves).DfltInt(0);
   for (int n = 0; n < NumSlaves(); n++) {
      CreatePar(dabc::format("%s%d", roc::xmlSlaveAddr, n)).DfltStr("");
      CreatePar(dabc::format("Slave%s%d", roc::xmlSyncSubeventId, n)).DfltInt(0);
    }

   CreatePar(roc::xmlNumTRBs).DfltInt(0);
   for (int n = 0; n < NumTRBs(); n++)
      CreatePar(dabc::format("%s%d", roc::xmlTRBAddr, n)).DfltStr("");

   CreatePar("EventsInBuffer").DfltInt(1);
   CreatePar("HitDelay").DfltInt(200);
   CreatePar("PollingTimeout").DfltDouble(0.01);

   CreatePar(dabc::xmlFlushTimeout).DfltDouble(1.);

   CreatePar(mbs::xmlServerKind).DfltStr(mbs::ServerKindToStr(mbs::StreamServer));
   CreatePar(mbs::xmlCombineCompleteOnly).DfltBool(false);

   CreatePar(xmlRawFile).DfltStr("");
   CreatePar(dabc::xmlFileSizeLimit).DfltInt(0);

   CreatePar(roc::xmlSpillRoc).DfltInt(-1);
   CreatePar(roc::xmlSpillAux).DfltInt(-1);
   CreatePar(roc::xmlCalibrationPeriod).DfltDouble(-1.);
   CreatePar(roc::xmlCalibrationLength).DfltDouble(0.5);

   CreatePar(roc::xmlGet4ResetPeriod).DfltDouble(-1.);
   CreatePar(roc::xmlGet4ResetLimit).DfltDouble(-1.);

   fMasterNode = Cfg("IsMaster").AsBool(true);

   if (fMasterNode) {
      CreatePar("FilePath").DfltStr("./");
      CreatePar("RunPrefix").DfltStr("");

      CreatePar("RunNumber").DfltStr("-1"); // avoid DIM problems
      CreatePar("RunInfo", "info").SetStr("NoRun");

      CreateCmdDef("StartRun");
      CreateCmdDef("StopRun");
      CreateCmdDef("SetPrefix").AddArg("Prefix", "string", true, "te_");
      CreateCmdDef("SetRunNumber").AddArg("RunNumber", "string", true, "1");
      // JAM: currently, dim-epics interface does not support strings > 40 chars.
      // thus we cannot use parameters wrapped in xml formatting as in java gui
      // beamtime workaround: define special commands without parameters
      CreateCmdDef("SetPrefixTest");
      CreateCmdDef("SetPrefixBeam");
      CreateCmdDef("SetPrefixCosmics");
      CreateCmdDef("IncRunNumber");
      CreateCmdDef("DecRunNumber");
      CreateCmdDef("ResetRunNumber");
   }

   fRawReadout = false;
   fSuperCombiner = false;
   fDoMeasureADC = false;
   fCheckSlavesConn = false;
   fFirstSlavePort = -1;

   fFileOutPort.clear();
   fServerOutPort.clear();

   DOUT3("!!!! Readout application %s created !!!!", GetName());

   if (NumSlaves()>0)
//      RegisterForParameterEvent(std::string(nameSuperComb)+"/Input*/"+dabc::ConnectionObject::ObjectName());
        RegisterForParameterEvent(dabc::ConnectionObject::ObjectName());
//        RegisterForParameterEvent(std::string(nameSuperComb)+"*"+dabc::ConnectionObject::ObjectName());
}

roc::ReadoutApplication::~ReadoutApplication()
{
   DOUT3("!!!! Readout application %s destructor !!!!", GetName());
}

std::string roc::ReadoutApplication::DataServerKind() const
{
   return Par(mbs::xmlServerKind).AsStdStr();
}

std::string roc::ReadoutApplication::RocAddr(int nreadout) const
{
   if ((nreadout < 0) || (nreadout >= NumRocs())) return "";
   return Par(dabc::format("%s%d", xmlRocIp, nreadout)).AsStdStr();
}

std::string roc::ReadoutApplication::TRBAddr(int index) const
{
   if ((index < 0) || (index >= NumTRBs())) return "";
   return Par(dabc::format("%s%d", roc::xmlTRBAddr, index)).AsStdStr();
}

int roc::ReadoutApplication::SusiboAddr(int index) const
{
   if ((index < 0) || (index >= NumSusibo()))
      return 0;
   return Par(dabc::format("%s%d", roc::xmlSusiboAddr, index)).AsInt();
}

std::string roc::ReadoutApplication::SlaveAddr(int index) const
{
   if ((index < 0) || (index >= NumSlaves()))
      return "";
   return Par(dabc::format("%s%d", xmlSlaveAddr, index)).AsStdStr();
}

std::string roc::ReadoutApplication::MbsAddr(int index) const
{
   if ((index < 0) || (index >= NumMbs()))
      return "";
   return Par(dabc::format("%s%d", roc::xmlMbsAddr, index)).AsStdStr();
}

int roc::ReadoutApplication::NumOpticRocs()
{
   int cnt(0);
   for (int n = 0; n < NumRocs(); n++)
      if (roc::Board::IsOpticAddress(RocAddr(n).c_str()) != 0) cnt++;
   return cnt;
}


int roc::ReadoutApplication::NumUdpRocs()
{
   int cnt(0);
   for (int n = 0; n < NumRocs(); n++)
      if (roc::Board::IsOpticAddress(RocAddr(n).c_str()) == 0) cnt++;
   return cnt;
}


bool roc::ReadoutApplication::CreateRawOpticInput(const std::string& portname, const std::string& rocaddr, bool isfirstoptic, int numoptics)
{
   DOUT2("Creating raw optic readout for board %s", rocaddr.c_str());

   std::string opticdevname = "AbbDev";
   std::string opticthrdname = "AbbDevThrd";

   if (isfirstoptic) {

      // create ABB device
      dabc::CmdCreateDevice cmd2(roc::typeAbbDevice, opticdevname.c_str(), opticthrdname.c_str());
      // cmd2.SetBool(roc::xmlUseDLM, true);

      if (!dabc::mgr.Execute(cmd2)) {
         EOUT("Cannot create AbbDevice");
         return false;
      }

      // create transport to the
      dabc::CmdCreateTransport cmd4(portname, opticdevname, opticthrdname);

      cmd4.SetStr(roc::xmlBoardAddr, rocaddr);
      cmd4.SetPoolName(roc::xmlRocPool);

      //   cmd4.SetStr(roc::xmlBoardAddr, "none");

      // indicate that transport should produce simple MBS headers
      cmd4.SetBool("WithMbsHeader", true);
      cmd4.SetBool("ManyRocs", numoptics > 1);

      if (!dabc::mgr.Execute(cmd4)) {
         EOUT("Cannot connect splitter module to ABB device");
         return false;
      }

      DOUT0("Raw optic transport for board %s created!", rocaddr.c_str());

   } else {

      dabc::Command cmd9("AddExtraBoard");
      cmd9.SetStr(roc::xmlBoardAddr, rocaddr);
      cmd9.SetReceiver(opticdevname.c_str());
      if (!dabc::mgr.Execute(cmd9)) {
         EOUT("Cannot assign explicit ROC path %s to the AbbDevice", rocaddr.c_str());
         return false;
      }

      DOUT0("Addition board %s for raw optic transport appended!", rocaddr.c_str());
   }

   return true;
}

bool roc::ReadoutApplication::CreateRawUdpInput(const std::string& portname, const std::string& rocaddr, int rocindx)
{
   DOUT0("Creating raw Ethernet readout for board %s", rocaddr.c_str());

   std::string devname = dabc::format("Roc%udev", rocindx);
   std::string thrdname = "RocReadoutThrd";

   dabc::CmdCreateDevice cmd6(roc::typeUdpDevice, devname.c_str(), thrdname.c_str());
   cmd6.SetStr(roc::xmlBoardAddr, rocaddr);
   cmd6.SetStr(roc::xmlRole, base::roleToString(base::roleDAQ));

   if (!dabc::mgr.Execute(cmd6)) {
      EOUT("Cannot create device %s for roc %s", devname.c_str(), rocaddr.c_str());
      return false;
   }

   dabc::CmdCreateTransport cmd7(portname, devname, thrdname);
   cmd7.SetStr(roc::xmlBoardAddr, rocaddr);
   cmd7.SetBool("WithMbsHeader", true);
   cmd7.SetPoolName(roc::xmlRocPool);

   if (!dabc::mgr.Execute(cmd7)) {
      EOUT("Cannot connect readout module to device %s", devname.c_str());
      return false;
   }

   DOUT0("Raw Ethernet readout created");

   return true;
}



bool roc::ReadoutApplication::CreateRocCombiner(const char* modulename,
      int numoutputs, bool skiperrordata,
      roc::BoardsVector& brds)
{
   int numoptic(0);
   for (unsigned n = 0; n < brds.size(); n++)
      if (roc::Board::IsOpticAddress(brds[n].rocname.c_str()) != 0)
         numoptic++;

   dabc::CmdCreateModule cmd1("roc::CombinerModule", modulename, "RocCombThrd");
   cmd1.SetStr(dabc::xmlPoolName, roc::xmlRocPool);
   cmd1.SetInt(dabc::xmlNumInputs, brds.size());
   cmd1.SetInt(dabc::xmlNumOutputs, numoutputs);
   cmd1.SetBool(roc::xmlSkipErrorData, skiperrordata);

   bool res = dabc::mgr.Execute(cmd1);
   DOUT3("Create ROC combiner module = %s", DBOOL(res));
   if (!res) return false;

   std::string devname, devtype, thrdname, opticdevname, opticthrdname;

   if (numoptic > 0) {
      // create optic device first

      opticdevname = "AbbDev";
      opticthrdname = "AbbDevThrd";

      dabc::CmdCreateDevice cmd2(roc::typeAbbDevice, opticdevname.c_str(),
            opticthrdname.c_str());
//      cmd2.SetBool(roc::xmlUseDLM, true);

      if (!dabc::mgr.Execute(cmd2)) {
         EOUT("Cannot create AbbDevice");
         return false;
      }

      brds.addDLMDev(opticdevname);

      if (numoptic > 1) {
         DOUT0("Create optical splitter module");

         dabc::CmdCreateModule cmd3("roc::SplitterModule", "Splitter", "RocCombThrd");
         cmd3.SetStr(dabc::xmlPoolName, roc::xmlRocPool);
         cmd3.SetInt(dabc::xmlNumInputs, 1);
         cmd3.SetInt(dabc::xmlNumOutputs, numoptic);
         if (!dabc::mgr.Execute(cmd3)) {
            EOUT("Cannot create optic Splitter module");
            return false;
         }

         dabc::CmdCreateTransport cmd4("Splitter/Input", opticdevname, opticthrdname);
         cmd4.SetStr(roc::xmlBoardAddr, "none");
         cmd4.SetPoolName(roc::xmlRocPool);

         if (!dabc::mgr.Execute(cmd4)) {
            EOUT("Cannot connect splitter module to ABB device");
            return false;
         }
      }
   }

   int splitterid(0);

   for (unsigned n = 0; n < brds.size(); n++) {

      DOUT3("Start configure ROC%u", n);

      std::string rocaddr = brds[n].rocname;
      std::string portname = dabc::format("%s/Input%u", modulename, n);

      bool isoptic = roc::Board::IsOpticAddress(rocaddr.c_str()) != 0;

      int infoid = -1;

      if (isoptic) {

         devname = opticdevname;

         bool cmd5_res = true;

         if (numoptic == 1) {
            dabc::CmdCreateTransport cmd5(portname, opticdevname, opticthrdname);
            cmd5.SetStr(roc::xmlBoardAddr, rocaddr);
            cmd5.SetPoolName(roc::xmlRocPool);
            cmd5_res = (dabc::mgr.Execute(cmd5) == dabc::cmd_true);
         } else {
            dabc::mgr.Connect(dabc::format("Splitter/Output%d", splitterid), portname);
            infoid = splitterid;
            splitterid++;
         }
         if (!cmd5_res) {
           EOUT("Cannot provide connection for ROC %s", rocaddr.c_str());
           return false;
         }

      } else {

         devname = dabc::format("Roc%udev", n);
         thrdname = "RocReadoutThrd";
         //thrdname = dabc::format("RocDev%uThrd", n);

         brds.addDLMDev(devname);

         dabc::CmdCreateDevice cmd6(roc::typeUdpDevice, devname.c_str(),
               thrdname.c_str());
         cmd6.SetStr(roc::xmlBoardAddr, rocaddr);
         cmd6.SetStr(roc::xmlRole, base::roleToString(base::roleDAQ));

         if (!dabc::mgr.Execute(cmd6)) {
            EOUT("Cannot create device %s for roc %s", devname.c_str(), rocaddr.c_str());
            return false;
         }

         dabc::CmdCreateTransport cmd7(portname, devname, thrdname);
         cmd7.SetStr(roc::xmlBoardAddr, rocaddr);
         cmd7.SetPoolName(roc::xmlRocPool);


         if (!dabc::mgr.Execute(cmd7)) {
            EOUT("Cannot connect readout module to device %s", devname.c_str());
            return false;
         }
      }

      roc::CmdGetBoardPtr cmd8;
      cmd8.SetStr(roc::xmlBoardAddr, rocaddr);
      cmd8.SetReceiver(devname.c_str());
      roc::Board* brd = 0;
      if (dabc::mgr.Execute(cmd8))
         brd = (roc::Board*) cmd8.GetPtr(roc::CmdGetBoardPtr::Board());

      if (brd == 0) {
         EOUT("Cannot get board pointer from device %s", devname.c_str());
         return false;
      }

      brds.setBoard(n, brd, devname);

      if (infoid >= 0) {
         dabc::Command cmd9("AddExtraBoard");
         cmd9.SetStr(roc::xmlBoardAddr, rocaddr);
         cmd9.SetReceiver(opticdevname.c_str());
         if (!dabc::mgr.Execute(cmd9)) {
            EOUT("Cannot assign explicit ROC path %s to the AbbDevice", rocaddr.c_str());
            return false;
         }

         // add information to splitter
         dabc::Command cmd10("AddROC");
         cmd10.SetInt("ROCID", brd->rocNumber());
         cmd10.SetReceiver("Splitter");
         if (!dabc::mgr.Execute(cmd10)) {
            EOUT("Cannot ADD ROCID %d to splitter", brd->rocNumber());
            return false;
         }
      }

      dabc::Command cmd11("ConfigureInput");

      cmd11.SetInt("Input", n);
      cmd11.SetBool("IsUdp", brd->getTransportKind() == roc::kind_UDP);
      cmd11.SetInt("ROCID", brd->rocNumber());
      cmd11.SetInt("Format", brd->getMsgFormat());
      cmd11.SetReceiver(modulename);
      if (!dabc::mgr.Execute(cmd11)) {
         EOUT("Cannot configure input %u of combiner module!!!", n);
         return false;
      }

      DOUT2("Did configure input%d as ROC%u addr:%s", n, brd->rocNumber(), rocaddr.c_str());
   }

   return true;
}

bool roc::ReadoutApplication::CreateAppModules()
{
   fCalibrState = calNONE;
   fFileOutPort.clear();

   DOUT2("CreateAppModules starts...");

   fRawReadout = Par(roc::xmlRawReadout).AsBool(true);

   bool res = false;

   dabc::lgr()->SetLogLimit(10000000);

   DOUT2("Create ROC POOL");

   dabc::mgr.CreateMemoryPool(roc::xmlRocPool);

   bool isepics = !EpicsNode().empty();

   int num_super_inputs = 0; // number of inputs in super combiner
   int nsuperinp = 0;        // index used for connections


   int num_optic_rocs = NumOpticRocs();
   int num_udp_rocs = NumUdpRocs();

   if (DoRawReadout()) {
      num_super_inputs = NumMbs() + NumSlaves() + num_udp_rocs + (num_optic_rocs>0 ? 1 : 0) +
                         NumSusibo() + (NumTRBs()>0 ? 1 : 0)  + (isepics ? 1 : 0);

      fSuperCombiner = true;
   } else {
      fDoMeasureADC = (NumRocs() > 0) && Par(roc::xmlMeasureADC).AsBool(false);
      num_super_inputs = NumMbs() + NumSlaves() + (NumRocs() > 0 ? 1 : 0)
                        + (NumSusibo() > 0 ? 1 : 0) + (NumTRBs() > 0 ? 1 : 0) + (isepics ? 1 : 0);
      fSuperCombiner = (num_super_inputs > 1) || ((num_super_inputs==1) && (NumRocs()==0));

      if (NumMbs() > 2) {
         EOUT("For a moment maximum two MBS input are supported in synchronized readout");
         exit(345);
      }

   }

//   if (dabc::mgr.NumNodes()>1)
//      dabc::mgr.CreateDevice(dabc::typeSocketDevice, "ConnDev");


   // as very first step, create super combiner module,
   // in raw readout it will be the only module

   if (DoSuperCombiner()) {

      if (DoRawReadout()) {
         dabc::CmdCreateModule cmd4("dabc::MultiplexerModule", nameSuperComb, "SuperCombThrd");
         cmd4.SetStr(dabc::xmlPoolName, roc::xmlRocPool);
         cmd4.SetInt(dabc::xmlNumInputs, num_super_inputs);
         cmd4.SetInt(dabc::xmlNumOutputs, 2);
         cmd4.SetStr("DataRateName","RawData");
         res = dabc::mgr.Execute(cmd4);
         DOUT3("Create RAW super combiner module = %s", DBOOL(res));
         if (!res) return false;

         fFileOutPort = dabc::mgr.FindModule(nameSuperComb).OutputName(2);
         fServerOutPort = dabc::mgr.FindModule(nameSuperComb).OutputName(1);

      } else {
         dabc::CmdCreateModule cmd4("mbs::CombinerModule", nameSuperComb, "SuperCombThrd");
         cmd4.SetStr(dabc::xmlPoolName, roc::xmlRocPool);
         cmd4.SetInt(dabc::xmlNumInputs, num_super_inputs);
         cmd4.SetInt(dabc::xmlNumOutputs, 2);
         cmd4.SetBool(mbs::xmlCombineCompleteOnly,
               Par(mbs::xmlCombineCompleteOnly).AsBool(false));
         cmd4.SetStr(mbs::xmlCombinerRatesPrefix, "Super");

         cmd4.SetInt(mbs::xmlEvidMask, 0xffffff); // only 24 bit used in sync counter

         res = dabc::mgr.Execute(cmd4);
         DOUT3("Create super combiner module = %s", DBOOL(res));
         if (!res) return false;

         fServerOutPort = dabc::format("%s/Output0", nameSuperComb);
         fFileOutPort = dabc::format("%s/Output1", nameSuperComb);
      }
   }

   // as first inputs, connect MBS to supercombiner
   for (int nmbs = 0; nmbs < NumMbs(); nmbs++) {
      dabc::CmdCreateTransport cmd5(
            dabc::format("%s/Input%d", nameSuperComb, nsuperinp),  MbsAddr(nmbs), "MbsReadoutThrd");
      cmd5.SetPoolName(roc::xmlRocPool);
      res = dabc::mgr.Execute(cmd5);
      DOUT1("Created MBS client transport for node %s : result is %s", MbsAddr(nmbs).c_str(), DBOOL(res));
      if (!res) return false;

      dabc::Command cmd6("ConfigureInput");
      cmd6.SetInt("Port", nsuperinp);
      cmd6.SetBool("RealMbs", true);
      cmd6.SetBool("RealEvntNum", false);
      cmd6.SetUInt("EvntSrcFullId",
            Par(dabc::format("%s%d", roc::xmlSyncSubeventId, nmbs)).AsInt(0));
      cmd6.SetUInt("EvntSrcShift", 0);
      cmd6.SetStr("RateName", "MbsData");

      cmd6.SetReceiver(nameSuperComb);
      res = dabc::mgr.Execute(cmd6);

      DOUT1("Configure special MBS case as supercombiner input %d : result is %s", nsuperinp, DBOOL(res));
      if (!res) return false;

      nsuperinp++;
   }

   // second, connect slaves

   for (int nslave = 0; nslave < NumSlaves(); nslave++) {

      if (!ConnectSlave(nslave, nsuperinp)) return false;

      if (fFirstSlavePort<0) fFirstSlavePort = nsuperinp;

      nsuperinp++;
   }


   // than connect ROCs

   if (NumRocs() > 0) {

      fRocBrds.returnBoards();

      if (DoRawReadout()) {
         bool isfirstoptic(true);

         for (int nroc = 0; nroc < NumRocs(); nroc++) {
            std::string portname = dabc::format("%s/Input%d", nameSuperComb, nsuperinp);

            if (roc::Board::IsOpticAddress(RocAddr(nroc).c_str()) != 0) {
               res = CreateRawOpticInput(portname, RocAddr(nroc), isfirstoptic, num_optic_rocs);
               if (!res) return false;

               if (isfirstoptic) nsuperinp++;

               isfirstoptic = false;
            } else {

               res = CreateRawUdpInput(portname, RocAddr(nroc), nroc);
               if (!res) return false;
               nsuperinp++;
            }


         }

      } else {
         for (int n = 0; n < NumRocs(); n++)
            fRocBrds.addRoc( RocAddr(n), Par(dabc::format("%s%d", xmlRocFebs, n)).AsStdStr() );

         res = CreateRocCombiner(nameRocComb, // module name
                 2, // num output
                 fSuperCombiner, // skip error data
                 fRocBrds);

         if (res && fDoMeasureADC)
            for (unsigned n = 0; n < fRocBrds.size(); n++)
               for (int nfeb = 0; nfeb < 2; nfeb++)
                  for (int nadc = 0; nadc < fRocBrds.numAdc(n, nfeb); nadc++) {
                     std::string parname = dabc::format("ROC%u_FEB%d_ADC%d",
                           fRocBrds[n].brd->rocNumber(), nfeb, nadc);
                     CreatePar(parname);
                  }

         if (!res) return false;


         if (DoSuperCombiner()) {

            dabc::Command cmd7("ConfigureInput");
            cmd7.SetInt("Port", nsuperinp);
            cmd7.SetBool("RealMbs", false);
            cmd7.SetBool("RealEvntNum", true);
            cmd7.SetReceiver(nameSuperComb);
            res = dabc::mgr.Execute(cmd7);
            DOUT1("Configure ROC as supercombiner input %d : result is %s", nsuperinp, DBOOL(res));
            if (!res) return false;

            dabc::mgr.Connect(dabc::format("%s/Output0", nameRocComb),
                              dabc::format("%s/Input%d", nameSuperComb, nsuperinp));

            nsuperinp++;
         } else {
            fServerOutPort = dabc::format("%s/Output0", nameRocComb);
            fFileOutPort = dabc::format("%s/Output1", nameRocComb);
         }
      }

      // SwitchCalibrationMode(false);
   }

   if (NumSusibo() > 0) {

      if (!DoRawReadout()) {
         dabc::CmdCreateModule cmd41("mbs::CombinerModule", nameSusiboComb, "SusiboCombThrd");
         cmd41.SetStr(dabc::xmlPoolName, roc::xmlRocPool);
         cmd41.SetInt(dabc::xmlNumInputs, NumSusibo());
         cmd41.SetInt(dabc::xmlNumOutputs, 1);
         cmd41.SetBool(mbs::xmlCombineCompleteOnly, false);
         cmd41.SetInt(mbs::xmlEvidMask, 0xffffff); // only 24 bit used in sync counter
         cmd41.SetStr(mbs::xmlCombinerRatesPrefix, "Spadic");
         res = dabc::mgr.Execute(cmd41);
         DOUT3("Create susibo combiner module = %s", DBOOL(res));
         if (!res) return false;
      }

      for (int n = 0; n < NumSusibo(); n++) {

         std::string portname;

         if (DoRawReadout())
            portname = dabc::format("%s/Input%d", nameSuperComb, nsuperinp++);
         else
            portname = dabc::format("%s/Input%d", nameSusiboComb, n);

         dabc::CmdCreateTransport cmd42(portname, "spadic::SusiboInput");

         cmd42.SetStr("SusiboAddr-", dabc::format("SusiboInput%d", n));
         cmd42.SetInt("SusiboID-", SusiboAddr(n));
         cmd42.SetInt("EventsInBuffer", Par("EventsInBuffer").AsInt(1));
         cmd42.SetInt("HitDelay", Par("HitDelay").AsInt(200));
         cmd42.SetDouble("PollingTimeout", Par("PollingTimeout").AsDouble(0.01));
         cmd42.SetPoolName(roc::xmlRocPool);

         cmd42.SetInt("SusiboMbsFormat", true);

         res = dabc::mgr.Execute(cmd42);
         if (res==0) return false;
      }

      if (!DoRawReadout()) {
         dabc::mgr.Connect(dabc::format("%s/Output0", nameSusiboComb),
                           dabc::format("%s/Input%d", nameSuperComb, nsuperinp));

         dabc::Command cmd71("ConfigureInput");
         cmd71.SetInt("Port", nsuperinp);
         cmd71.SetBool("RealMbs", false);
         cmd71.SetBool("RealEvntNum", true);
         cmd71.SetBool("Optional", true);
         cmd71.SetReceiver(nameSuperComb);
         res = dabc::mgr.Execute(cmd71);
         DOUT1("Configure Susibo as supercombiner input %d : result is %s", nsuperinp, DBOOL(res));
         if (!res) return false;

         nsuperinp++;
      }
   }

   if (NumTRBs() > 0) {

      dabc::CmdCreateModule cmd3("hadaq::CombinerModule", nameTRBComb, "TRBCombThrd");
      cmd3.SetStr(dabc::xmlPoolName, roc::xmlRocPool);
      cmd3.SetInt(dabc::xmlNumInputs, NumTRBs());
      cmd3.SetInt(dabc::xmlNumOutputs, 1);
      res = dabc::mgr.Execute(cmd3);
      DOUT0("Create TRB combiner module = %s", DBOOL(res));
      if (res==0) return false;

      for (int n=0;n<NumTRBs();n++) {

         dabc::CmdCreateTransport cmd31(dabc::format("%s/Input%d", nameTRBComb, n), TRBAddr(n), "HadaqUdpThrd");
         cmd31.SetInt("HadaqUdpBuffer", 200000);
         cmd31.SetPoolName(roc::xmlRocPool);

         res = dabc::mgr.Execute(cmd31);
         DOUT0("Create input %d addr %s TRB combiner module = %s", n, TRBAddr(n).c_str(), DBOOL(res));
         if (res==0) return false;
      }

      dabc::CmdCreateModule cmd32("hadaq::MbsTransmitterModule", nameTRBTrans, "TRBOnlineThrd");
      cmd32.SetStr(dabc::xmlPoolName, roc::xmlRocPool);
      cmd32.SetInt(dabc::xmlNumInputs, 1);
      cmd32.SetInt(dabc::xmlNumOutputs, 1);
      res = dabc::mgr.Execute(cmd32);
      DOUT0("Create TRB transmitter module = %s", DBOOL(res));
      if (res==0) return false;
      dabc::mgr.Connect(dabc::mgr.FindModule(nameTRBComb).OutputName(),
                        dabc::mgr.FindModule(nameTRBTrans).InputName());

      // connect to super combiner at the end
      dabc::mgr.Connect(dabc::mgr.FindModule(nameTRBTrans).OutputName(),
                        dabc::mgr.FindModule(nameSuperComb).InputName(nsuperinp));

      if (!DoRawReadout()) {
         dabc::Command cmd72("ConfigureInput");
         cmd72.SetInt("Port", nsuperinp);
         cmd72.SetBool("RealMbs", false);
         cmd72.SetBool("RealEvntNum", true);
         cmd72.SetBool("Optional", true);
         cmd72.SetReceiver(nameSuperComb);
         res = dabc::mgr.Execute(cmd72);
         DOUT1("Configure TRB as supercombiner input %d : result is %s", nsuperinp, DBOOL(res));
         if (!res) return false;
      }

      nsuperinp++;
   }

   if (isepics) {
      dabc::CmdCreateTransport cmd8(
            dabc::format("%s/Input%d", nameSuperComb, nsuperinp),
            EpicsNode(), "MbsReadoutThrd");
      cmd8.SetDouble(dabc::xmlConnTimeout, 5.);
      cmd8.SetPoolName(roc::xmlRocPool);
      res = dabc::mgr.Execute(cmd8);
      DOUT0("Created EPICS client transport result is %s", DBOOL(res));
      if (!res) return false;

      dabc::Command cmd9("ConfigureInput");
      cmd9.SetInt("Port", nsuperinp);
      cmd9.SetBool("RealMbs", false);
      cmd9.SetBool("RealEvntNum", false);
      cmd9.SetBool("NoEvntNum", true);
      cmd9.SetReceiver(nameSuperComb);
      res = dabc::mgr.Execute(cmd9);

      DOUT1("Configure special EPICS case as supercombiner input %d : result is %s", nsuperinp, DBOOL(res));
      if (!res) return false;

      nsuperinp++;
   }

   if (!OutputFileName().empty() && !fFileOutPort.empty()) {
      if (!StartFile(OutputFileName())) return false;
   }

   if (!DataServerKind().empty() && !fServerOutPort.empty()) {

      ///// connect module to mbs server:

      std::string kind = DataServerKind();
      if (kind.find("mbs://") == std::string::npos)
         kind = std::string("mbs://") + kind;

      dabc::CmdCreateTransport cmd13(fServerOutPort, kind, "MbsServerThrd");

      // cmd13.SetStr(mbs::xmlServerKind, DataServerKind());

      // no need to set pool name for output transport
      //cmd13.SetPoolName(roc::xmlRocPool);

      res = dabc::mgr.Execute(cmd13);
      DOUT0("Connected module port %s to Mbs server res = %s", fServerOutPort.c_str(), DBOOL(res));
      if (!res) return false;
   }

   return true;
}

bool roc::ReadoutApplication::StartFile(const std::string& filename)
{
   if (fFileOutPort.empty()) {
      EOUT("Cannot create file output - no appropriate port specified");
      return false;
   }


   std::string arg = filename;
   if (filename.find("lmd://") == std::string::npos)
      arg = std::string("lmd://") + filename;


   dabc::CmdCreateTransport cmd11(fFileOutPort, arg);

//   cmd11.SetStr(mbs::xmlFileName, filename);
   // no need to set pool for output transport
   //cmd11.SetPoolName(roc::xmlRocPool);

   if (fMasterNode) {
      cmd11.SetStr("InfoPar", "RunInfo");
   } else {
      CreatePar("FileInfo","info").SetSynchron(true, 2., false).SetStr("nofile");
      cmd11.SetStr("InfoPar", "FileInfo"); // inform transport which parameter should be used for information
   }

   bool res = dabc::mgr.Execute(cmd11);
   DOUT1("Create lmd file %s for %s, res = %s", filename.c_str(), fFileOutPort.c_str(), DBOOL(res));
   return res;
}

bool roc::ReadoutApplication::StopFile()
{
   if (fFileOutPort.find(nameSuperComb)!=0) {
      EOUT("File port connected not to the super combiner!!!");
      dabc::PortRef port = dabc::mgr.FindPort(fFileOutPort);
      port.Disconnect();
   }

   return dabc::mgr.FindModule(nameSuperComb).Execute(mbs::comStopFile);

// TODO: replace later by the new code

//   dabc::PortRef port = dabc::mgr.FindPort(fFileOutPort);
//   return port.Disconnect();
}

bool roc::ReadoutApplication::ConnectSlave(int nslave, int ninp)
{
   dabc::CmdCreateTransport cmd21(
         dabc::format("%s/Input%d", nameSuperComb, ninp),
         SlaveAddr(nslave), "SlaveReadoutThrd");
   cmd21.SetDouble(dabc::xmlConnTimeout, 5.);
   cmd21.SetPoolName(roc::xmlRocPool);

   bool res = dabc::mgr.Execute(cmd21);

   int slavesubevid = Par(dabc::format("Slave%s%d", roc::xmlSyncSubeventId, nslave)).AsInt(0);

   DOUT1("Created client transport for slave %s syncid %d: result is %s",
           SlaveAddr(nslave).c_str(), slavesubevid, DBOOL(res));

   if (!res) {
      DOUT0("==============================================");
      DOUT0(" ++++++++++ SLAVE %d WAS NOT CONNECTED +++++++++", nslave);
      DOUT0("==============================================");
      if (slavesubevid!=1) return false;
   }

   if (slavesubevid==0) {
      DOUT1("Configure slave %d input as normal MBS input", nslave);
   } else
   if (slavesubevid==1) {
      DOUT1("Configure slave %d input as optional MBS input - can be used for FASP", nslave);
      dabc::Command cmd22("ConfigureInput");
      cmd22.SetInt("Port", ninp);
      cmd22.SetBool("RealMbs", false);
      cmd22.SetBool("RealEvntNum", true);
      cmd22.SetBool("Optional", true);
      cmd22.SetReceiver(nameSuperComb);
      res = dabc::mgr.Execute(cmd22);
      DOUT1("Configure client for slave %d as optional supercombiner input %d : result is %s",
              nslave, ninp, DBOOL(res));
   } else {
      DOUT1("Configure slave %d input as ubnormal MBS input with event number in subeventid 0x%x ", nslave, slavesubevid);

      dabc::Command cmd22("ConfigureInput");
      cmd22.SetInt("Port", ninp);
      cmd22.SetBool("RealMbs", false);
      cmd22.SetBool("RealEvntNum", false);
      cmd22.SetUInt("EvntSrcFullId", slavesubevid);
      cmd22.SetUInt("EvntSrcShift", 0);
      cmd22.SetStr("RateName", "SlaveData");

      cmd22.SetReceiver(nameSuperComb);
      res = dabc::mgr.Execute(cmd22);
      DOUT1("Configure client for slave %d as supercombiner input %d : result is %s",
              nslave, ninp, DBOOL(res));
   }
   return res;
}


bool roc::ReadoutApplication::AfterAppModulesStopped()
{
   if (fCalibrState == calON)
      SwitchCalibrationMode(false);
   return true;
}

bool roc::ReadoutApplication::BeforeAppModulesDestroyed()
{
   fRocBrds.returnBoards();

   return dabc::Application::BeforeAppModulesDestroyed();
}

bool roc::ReadoutApplication::IsModulesRunning()
{
   if (!DoRawReadout()) {
   
      if ((NumRocs() > 0) && !dabc::mgr.FindModule(nameRocComb).IsRunning())
         return false;

      if ((NumSusibo() > 0) && !dabc::mgr.FindModule(nameSusiboComb).IsRunning())
         return false;

      if ((NumTRBs() > 0) && 
         (!dabc::mgr.FindModule(nameTRBComb).IsRunning() || !dabc::mgr.FindModule(nameTRBTrans).IsRunning()))
            return false;
   }

   if (DoSuperCombiner() && !dabc::mgr.FindModule(nameSuperComb).IsRunning())
      return false;

   return true;
}

int roc::ReadoutApplication::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName(roc::CmdCalibration::CmdName())) {
      bool flag = cmd.GetBool(roc::CmdCalibration::FlagName(), true);
      return cmd_bool(SwitchCalibrationMode(flag));
   } else if (cmd.IsName("StartRun")) {
      std::string fname = Par("FilePath").AsStdStr();
      fname+=Par("RunPrefix").AsStdStr();
      int number = Par("RunNumber").AsInt(-1);
      if (number >= 0)
         fname += dabc::format("run%d.lmd", number);
      if (StartFile(fname)) {
         Par("RunInfo").SetStr(std::string("Start file ")+fname);
         return dabc::cmd_true;
      }
      Par("RunInfo").SetStr(std::string("Fail open file ")+fname);
      return dabc::cmd_false;
   } else if (cmd.IsName("StopRun")) {

      bool res = StopFile();
      int number = Par("RunNumber").AsInt(-1);
      if (number >= 0)
         Par("RunNumber").SetInt(number + 1);
      Par("RunInfo").SetStr("NoRun");
      return cmd_bool(res);
   } else if (cmd.IsName("SetPrefix")) {
      // todo: get par from string
      std::string prename = Cfg("Prefix", cmd).AsStdStr();
      Par("RunPrefix").SetStr(prename);
      DOUT1("Setting Prefix to %s", prename.c_str());
      return dabc::cmd_true;
   } else if (cmd.IsName("SetRunNumber")) {
      int number = Cfg("RunNumber", cmd).AsInt();
      Par("RunNumber").SetInt(number);
      DOUT1("Setting Run number to %d", number);
      return dabc::cmd_true;
   } else if (cmd.IsName("SetPrefixTest")) {
      std::string prename = "Te_";
      Par("RunPrefix").SetStr(prename);
      DOUT1("Setting Prefix to %s", prename.c_str());
      return dabc::cmd_true;
   } else if (cmd.IsName("SetPrefixBeam")) {
      std::string prename = "Be_";
      Par("RunPrefix").SetStr(prename);
      DOUT1("Setting Prefix to %s", prename.c_str());
      return dabc::cmd_true;
   } else if (cmd.IsName("SetPrefixCosmics")) {
      std::string prename = "Co_";
      Par("RunPrefix").SetStr(prename);
      DOUT1("Setting Prefix to %s", prename.c_str());
      return dabc::cmd_true;
   } else if (cmd.IsName("IncRunNumber")) {
      int number = Par("RunNumber").AsInt(-1);
      if (number >= 0)
         Par("RunNumber").SetInt(number + 1);
      return dabc::cmd_true;
   } else if (cmd.IsName("DecRunNumber")) {
      int number = Par("RunNumber").AsInt(-1);
      if (number >= 0) {
         number -= 1;
         if (number < 0)
            number = 0;
         Par("RunNumber").SetInt(number);
         return dabc::cmd_true;
      }
   } else if (cmd.IsName("ResetRunNumber")) {
       Par("RunNumber").SetInt(0);
       return dabc::cmd_true;
   } else if (cmd.IsName("ResetAllGet4")) {
      fRocBrds.ResetAllGet4();
      return dabc::cmd_true;
   } else if (cmd.IsName("ezca::OnUpdate")) {
      int id = cmd.GetInt("EpicsFlagRec");
      fRocBrds.produceSystemMessage(id+1000);
      DOUT0("Has EZCA command rec = %d", id);
      return dabc::cmd_true;
   }

   return dabc::Application::ExecuteCommand(cmd);
}

bool roc::ReadoutApplication::SwitchCalibrationMode(bool on)
{
   if ((fCalibrState == calON) && on)
      return true;

   if ((fCalibrState == calOFF) && !on)
      return true;

   DOUT0("SwitchCalibration %s %5.2f", DBOOL(on), dabc::Now().AsDouble());

   if (on && fDoMeasureADC) {
      roc::MessagesVector* vect = fRocBrds.readoutExtraMessages();

      if (vect != 0) {

         for (unsigned n = 0; n < vect->size(); n++)
            if (vect->at(n).isSysMsg()
                  && vect->at(n).getSysMesType() == roc::SYSMSG_ADC) {
               uint32_t val = vect->at(n).getSysMesData();

               std::string parname = dabc::format("ROC%u_FEB%d_ADC%d",
                     vect->at(n).getRocNumber(), val >> 31, (val >> 24) & 0x7f);

               Par(parname).SetInt(val & 0xffffff);

//               dabc::RateParameter* rate = dynamic_cast<dabc::RateParameter*> (FindPar(parname));
//               if (rate) rate->ChangeRate(val & 0xffffff);
            }

         roc::CmdMessagesVector cmd(vect);
         cmd.SetReceiver(nameRocComb);
         dabc::mgr.Submit(cmd);
      }
   }

   fRocBrds.autoped_switch(on);

   //  DLM 10/11 not implemented for calibration switch on/off
   // fRocBrds.issueDLM(on ? 10 : 11);

   fCalibrState = on ? calON : calOFF;

   return true;
}

void roc::ReadoutApplication::ProcessParameterEvent(const dabc::ParameterEvent& evnt)
{
    // here one should analyze

   DOUT0("Get change event for connection %s value %s", evnt.ParName().c_str(), evnt.ParValue().c_str());

   if (evnt.ParValue() == dabc::ConnectionObject::GetStateName(dabc::ConnectionObject::sBroken)) {
      DOUT0("Activate reconnection timeout for slaves");
      fCheckSlavesConn = true;
      ActivateTimeout(3);
   }
}

double roc::ReadoutApplication::ProcessTimeout(double last_diff)
{
   double res = dabc::Application::ProcessTimeout(last_diff);

   // retry at least every
   if (fCheckSlavesConn) {

      DOUT0("!!! Check reconnection for slaves !!!");

      dabc::ModuleRef m = dabc::mgr.FindModule(nameSuperComb);
      if (m.null()) { EOUT("Did not found super combiner !!!"); return res; }

      bool tryagain = false;

      for (int nslave = 0; nslave < NumSlaves(); nslave++) {

         if (m.IsInputConnected(nslave)) continue;

         if (!ConnectSlave(nslave, fFirstSlavePort+nslave)) tryagain = true;
      }

      if (tryagain) {
         if ((res<0) || (res>3)) res = 3;
      } else
         fCheckSlavesConn = false;
   }

   return res;
}

