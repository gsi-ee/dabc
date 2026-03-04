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
#include "pex/Device.h"
#include "dabc/logging.h"
#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/Application.h"

//#include "mbs/MbsTypeDefs.h"
//#include "mbs/Factory.h"

//#include "dabc/MemoryPool.h"
//#include "dabc/Buffer.h"
#include "dabc/Pointer.h"
#include "dabc/Port.h"

#include "pex/Factory.h"
#include "pex/Transport.h"
#include "pex/Input.h"

#include "pex/Febex3.h"
#include "pex/Poland.h"

//#include "pex/ReadoutApplication.h"

#include "pexor/PexorTwo.h"
#include "pexor/DMA_Buffer.h"

#include <vector>

//#include "pex/random-coll.h"

/** check if device object is valid. may specify return value depending on function where it is used: */
#define PEX_ASSERT_DEVICEINIT(ret)  \
  if (!fInitDone){ \
      EOUT ("**** pex Device is not initialized!\n"); \
      return ret; }

namespace pex
{
const char *xmlPexorID = "PexorID";    //< id number N of pexor device file /dev/pexor-N
const char *xmlPexorSFPSlaves = "PexorNumSlaves_";    //<  prefix for the sfp numbers 0,1,2,3 indicating how many slaves are connected input
const char *xmlPexorSlaveTypes = "PexorSlavesTypes_";
const char *xmlRawFile = "PexorOutFile";    //<  name of output lmd file
const char *xmlDMABufLen = "PexorDMALen";    //<  length of DMA buffers to allocate in driver
const char *xmlDMABufNum = "PexorDMABuffers";    //<  number of DMA buffers to allocate in driver
const char *xmlDMAScatterGatherMode = "PexorDMAScatterGather";    //<  sg mode switch
const char *xmlDMAZeroCopy = "PexorDMAZeroCopy";    //<  sg mode switch
const char *xmlFormatMbs = "PexorFormatMbs";    //<  switch Mbs formating already in transport buffer
const char *xmlSingleMbsSubevt = "PexorSingleMbsSubevent";    //<  use one single subevent for all sfps
const char *xmlMultichannelRequest = "PexorMultiTokenDMA";    //<  enable channelpattern request with combined dma for multiple sfps
const char *xmlAutoTriggerRead = "PexorAutoReadout";    //<  enable automatic readout of all configured token data in driver for each trigger
const char *xmlAutoAsyncRead = "PexorAutoAsync";    //< enable automatic triggerless readout of asynchronous sfp chains in driver
const char *xmlMbsSubevtCrate = "PexorMbsSubcrate";    //<  define crate number for subevent header
const char *xmlMbsSubevtControl = "PexorMbsControl";    //<  define crate number for subevent header
const char *xmlMbsSubevtProcid = "PexorMbsProcid";    //<  define procid number for subevent header

const char *xmlSyncRead = "PexorSyncReadout";    //<  switch readout sync mode
//onst char* xmlParallelRead	= "PexorParallelReadout"; //<  switch readout parallel token mode
const char *xmlTriggeredRead = "PexorUseTrigger";    //<  switch trigger mode
const char *xmlDmaMode = "PexorDirectDMA";    //<  switch between direct dma to host,  or token data buffering in pexor RAM
const char *xmlWaitTimeout = "PexorTriggerTimeout";    //<  specify kernel waitqueue timeout for trigger and autoread buffers

const char *xmlWaitForDataReady = "PexorTokenWaitForDataReady";    //<  token request returns only when frontend has data ready
const char *xmlUserTriggerClear= "EarlyTriggerClear"; //<- aka  "user trigger clear", using feb double buffers
const char *xmlStartDAQOnInit = "AutoStartAcquisition"; // <- start acquisition directly after init
const char *xmlInitDelay = "PexorInitDelay";    //<  sleep time after board reset until pexor is ready

const char *xmlTrixorConvTime = "TrixorConversionTime";    //<  conversion time of TRIXOR module
const char *xmlTrixorFastClearTime = "TrixorFastClearTime";    //<  fast clear time of TRIXOR module

//const char* xmlExploderSubmem	= "ExploderSubmemSize"; //<  size of exploder submem test buffer

const char *xmlModuleName = "PexorModuleName";    //<  Name of readout module instance
const char *xmlModuleThread = "PexorModuleThread";    //< Name of thread for readout module
const char *xmlDeviceName = "PexorDeviceName";
const char *xmlDeviceThread = "PexorDeviceThread";    //<  Name of thread for readout module

const char *nameReadoutAppClass = "pex::ReadoutApplication";
const char *nameDeviceClass = "pex::Device";
const char *nameTransportClass = "pex::Transport";
const char *nameReadoutModuleClass = "pex::ReadoutModule";
const char *nameInputPool = "PexorInputPool";
const char *nameOutputPool = "PexorOutputPool";

const char *commandStartAcq = "StartAcquisition";
const char *commandStopAcq = "StopAcquisition";
const char *commandInitAcq = "InitAcquisition";

const char *parDeviceDRate = "DeviceReceiveRate";

const char *parDaqRunningState = "PexorAcquisitionRunning";

}

unsigned int pex::Device::fgThreadnum = 0;

pex::Device::Device(const std::string &name, dabc::Command cmd) :
    dabc::Device(name), fKinpex(nullptr), fMbsFormat(true), fSingleSubevent(false), fSubeventSubcrate(0),
        fSubeventProcid(0), fSubeventControl(0), fWaitTimeout(1), fAqcuisitionRunning(false), fSynchronousRead(true),
        fTriggeredRead(false), fDirectDMA(true), fMultichannelRequest(false), fAutoTriggerRead(false),
        fMemoryTest(false), fSkipRequest(false), fWaitForDataReady(true),fStartDAQOnInit(true), fCurrentSFP(0), fReadLength(0),
        fTrixConvTime(0x20), fTrixFClearTime(0x10), fInitDone(false), fHasData(true), fEarlyTriggerClear(true), fNumEvents(0), fRequestMutex(true)

{
  fDeviceNum = Cfg(pex::xmlPexorID, cmd).AsInt(0);
  fKinpex = new pexor::PexorTwo(fDeviceNum);
  if (!fKinpex->IsOpen())
  {
    EOUT(("**** Could not open pexor board!\n"));
    delete fKinpex;
    exit(1);
  }
  // JAM workaround:
  fKinpex->Reset();    // get rid of previous buffers of not properly closed run!
  // JAM
  fZeroCopyMode = Cfg(pex::xmlDMAZeroCopy, cmd).AsBool(false);
  DOUT1("Setting zero copy mode to %d\n", fZeroCopyMode);
  bool sgmode = Cfg(pex::xmlDMAScatterGatherMode, cmd).AsBool(false);
  fKinpex->SetScatterGatherMode(sgmode);
  DOUT1("Setting scatter gather mode to %d\n", sgmode);

  int initDelay = Cfg(pex::xmlInitDelay, cmd).AsInt(1);

  // JAM 13-02-2026: TODO adjust frontend linkspeed from setup
  // currently we only dump what is available from pexor driver library:
  DOUT1("TODO: add linkspeed settings from DABC!!!\n");
  for (int ls = 1; ls < PEXOR_MAX_SPEEDSETUP; ++ls)
  {
    DOUT1("   Available linkspeed preset %d (%s)\n", ls, gLinkspeed[ls]);
  }
// TODO fKinpex->SetLinkspeed(ls); ls from config
  //////////////

  DOUT1("Sleep %d seconds before initializing the bus\n", initDelay);
  sleep(initDelay);    // JAM 2016 - required for some kinpex board code

  fFrontendBoards.clear();
  // initialize here the connected channels:
  int sfpcount = 0;
  for (int sfp = 0; sfp < PEX_NUMSFP; sfp++)
  {

    fNumSlavesSFP[sfp] = Cfg(dabc::format("%s%d", xmlPexorSFPSlaves, sfp), cmd).AsInt(0);
    fEnabledSFP[sfp] = fNumSlavesSFP[sfp] > 0 ? true : false;
    DOUT1("Sfp %d is %s with %d slave devices.\n", sfp, (fEnabledSFP[sfp] ? "enabled" : "disabled"),
        fNumSlavesSFP[sfp]);

    if (fEnabledSFP[sfp])
    {

      // JAM2026 TODO: here evaluate slave kinds from configuration list per sfp

      fSlaveTypes[sfp].clear();

      std::vector<long int> arr = Cfg(dabc::format("%s%d", xmlPexorSlaveTypes, sfp), cmd).AsIntVect();
      for (size_t i = 0; i < arr.size(); ++i)
      {
        // create new kind of feb if not existing
        feb_kind_t kind = (feb_kind_t) arr[i];
        DOUT1("SFP %d Slave %ld has feb type %s\n", sfp, i, pex::FrontendBoard::BoardName(kind).c_str());
        dabc::Command dummy;
        //CreateFrontendBoard(kind, pex::FrontendBoard::BoardName(kind), dummy);    // JAM: if we do this here, the xml file with feb module parameters is not scanned
        fSlaveTypes[sfp].push_back(kind);    // vector index is chain index
      }

//      if (1)
//      {
//        int iret = fKinpex->InitBus(sfp, fNumSlavesSFP[sfp]);
//        if (iret)
//        {
//          EOUT("**** Error %d in PEXOR InitBus for sfp %d \n", iret, sfp);
//          delete fKinpex;
//          return;    // TODO: proper error handling
//        }
//      }
//      else
//      {
//        DOUT1("DO NOT INIT CHAIN for configured sfp %d\n", sfp);
//      }
      fDoubleBufID[sfp] = 0;
      sfpcount++;
    }
    fRequestedSFP[sfp] = false;
  }

  unsigned int size = Cfg(pex::xmlDMABufLen, cmd).AsInt(4096);
  fReadLength = size * sfpcount;    // initial value is maximum length of dma buffer times number of active chains
  DOUT1("ReadLength is %d bytes. sfpcount=%d\n", fReadLength, sfpcount);

  //fReadLength=33000;

  unsigned int numbufs = Cfg(pex::xmlDMABufNum, cmd).AsInt(20);    //GetCfgInt(pex::xmlDMABufNum,20, cmd);
  int rev = fKinpex->Add_DMA_Buffers(size, numbufs);
  if (rev)
  {
    EOUT("\n\nError %d on adding dma buffers\n", rev);
    return;
  }
  fMbsFormat = Cfg(pex::xmlFormatMbs, cmd).AsBool(true);
  fSingleSubevent = Cfg(pex::xmlSingleMbsSubevt, cmd).AsBool(false);
  fMultichannelRequest = Cfg(pex::xmlMultichannelRequest, cmd).AsBool(false);
  fAutoTriggerRead = Cfg(pex::xmlAutoTriggerRead, cmd).AsBool(false);
  fAutoAsyncRead = Cfg(pex::xmlAutoAsyncRead, cmd).AsBool(false);

  fSubeventSubcrate = Cfg(pex::xmlMbsSubevtCrate, cmd).AsInt(0);
  fSubeventProcid = Cfg(pex::xmlMbsSubevtProcid, cmd).AsInt(fDeviceNum);
  fSubeventControl = Cfg(pex::xmlMbsSubevtControl, cmd).AsInt(0);
  DOUT1("Setting mbsformat=%d, singlesubevent=%d, with subcrate:%d, procid:%d, control:%d\n", fMbsFormat,
      fSingleSubevent, fSubeventSubcrate, fSubeventProcid, fSubeventControl);

  fSynchronousRead = Cfg(pex::xmlSyncRead, cmd).AsBool(true);
  fTriggeredRead = Cfg(pex::xmlTriggeredRead, cmd).AsBool(false);
  fDirectDMA = Cfg(pex::xmlDmaMode, cmd).AsBool(true);
  fTrixConvTime = Cfg(pex::xmlTrixorConvTime, cmd).AsInt(0x200);
  fTrixFClearTime = Cfg(pex::xmlTrixorFastClearTime, cmd).AsInt(0x100);

  DOUT1("Setting synchronous=%d, triggered=%d, autotriggered=%d, autoasync=%d \n", fSynchronousRead, fTriggeredRead,
      fAutoTriggerRead, fAutoAsyncRead);
  DOUT1("Setting directdma=%d, multichannelrequest=%d\n", fDirectDMA, fMultichannelRequest);

  if (fTriggeredRead)
  {
    fWaitTimeout = Cfg(pex::xmlWaitTimeout, cmd).AsInt(2);
    fKinpex->SetWaitTimeout(fWaitTimeout);
    DOUT1("Setting trigger wait timeout to %d s\n", fWaitTimeout);
  }

  fWaitForDataReady = Cfg(pex::xmlWaitForDataReady, cmd).AsBool(true);

  fEarlyTriggerClear= Cfg(pex::xmlUserTriggerClear, cmd).AsBool(true);

  DOUT1("---------- Readout mode : wait for data ready is %s, early trigger clear is %s\n", (fWaitForDataReady? "on":"off"), (fEarlyTriggerClear ? "on":"off"));

  fStartDAQOnInit= Cfg(pex::xmlStartDAQOnInit, cmd).AsBool(true);


  CreateCmdDef(pex::commandStartAcq);
  CreateCmdDef(pex::commandStopAcq);
  CreateCmdDef(pex::commandInitAcq);

  CreatePar(pex::parDeviceDRate).SetRatemeter(false, 3.).SetUnits("kBytes");

  // todo: set here global info name or use info of readout module
  SetDevInfoParName("PexDevInfo");
  CreatePar(fDevInfoName, "info").SetSynchron(true, 2., false).SetDebugLevel(2);
  // for the moment, we create local info object

  CreatePar(pex::parDaqRunningState).Dflt(false);

  PublishPars("$CONTEXT$/PexDevice");

//  rev = InitDAQ();
//  if (rev)
//  {
//    EOUT("\n\nError %d at init DAQ!!\n", rev);
//    return;    // TODO: error handling with exceptions
//  }

  // here the memory copy test switches:
  fMemoryTest = false;    // put this to true to make memcopy performance test between driver buffer and dabc buffer
  fSkipRequest = false;

  DOUT1("Created PEXOR device %d\n", fDeviceNum);
  //fInitDone=true; // do this in subclass after constructor has finished.
}

void pex::Device::OnThreadAssigned()
{
  dabc::Device::OnThreadAssigned();

  // we can not activate timeout in constructor,
  // need to activate it here
  bool rev = ActivateTimeout(PEX_REFRESHTIMEOUT);    // enable timeout to refresh exported variables
  DOUT0("Activated timeout, result=%d\n", rev);

}

int pex::Device::InitChains()
{
  int rev = 0;
  for (int sfp = 0; sfp < PEX_NUMSFP; sfp++)
  {
    if (!fEnabledSFP[sfp])
      continue;
    rev = fKinpex->InitBus(sfp, fNumSlavesSFP[sfp]);
    if (rev)
    {
      EOUT("**** Error %d in PEXOR InitBus for sfp %d \n", rev, sfp);
      return rev;
    }
    else
    {
      DOUT1("Init gosip chain for sfp %d with %d slaves.\n", sfp, fNumSlavesSFP[sfp]);
    }
    fDoubleBufID[sfp] = 0;
  }
  return rev;
}

int pex::Device::InitDAQ()
{
  int rev = 0;
  SetInfo("InitDaq is executed...");
  rev = InitChains();
  if (rev)
    return rev;    // TODO: error handling with exceptions
  InitTrixor();

  // first disable all frontends
  for (int sfp = 0; sfp < PEX_NUMSFP; ++sfp)
  {
    for (size_t slave = 0; slave < fSlaveTypes[sfp].size(); ++slave)
    {
      feb_kind_t kind = fSlaveTypes[sfp].at(slave);
      FrontendBoard *theBoard = GetFrontendBoard(kind);
      if (!theBoard)
      {
        EOUT("pex::Device registry error: FEB component of kind %d at sfp %d slave %ld is not yet exising!", kind, sfp,
            slave);
        continue;
      }
      rev = theBoard->Disable(sfp, slave);
      if (rev)
        return rev;    // TODO: error handling with exceptions
    }
  }

  // init all frontends
  for (int sfp = 0; sfp < PEX_NUMSFP; ++sfp)
  {
    for (size_t slave = 0; slave < fSlaveTypes[sfp].size(); ++slave)
    {
      feb_kind_t kind = fSlaveTypes[sfp].at(slave);
      FrontendBoard *theBoard = GetFrontendBoard(kind);
      if (!theBoard)
      {
        EOUT("pex::Device registry error: FEB component of kind %d at sfp %d slave %ld is not yet exising!", kind, sfp,
            slave);
        continue;
      }
      rev = theBoard->Configure(sfp, slave);
      if (rev)
        return rev;    // TODO: error handling with exceptions
    }
  }

  // enable all frontends
  for (int sfp = 0; sfp < PEX_NUMSFP; ++sfp)
  {
    for (size_t slave = 0; slave < fSlaveTypes[sfp].size(); ++slave)
    {
      feb_kind_t kind = fSlaveTypes[sfp].at(slave);
      FrontendBoard *theBoard = GetFrontendBoard(kind);
      if (!theBoard)
      {
        EOUT("pex::Device registry error: FEB component of kind %d at sfp %d slave %ld is not yet exising!", kind, sfp,
            slave);
        continue;
      }
      rev = theBoard->Enable(sfp, slave);
      if (rev)
        return rev;    // TODO: error handling with exceptions
    }
  }
  if(fStartDAQOnInit)
    rev =(StartAcquisition() ? 0 :1);
  return rev;
}

pex::Device::~Device()
{
  DOUT1("DDDDDDDDDDd pex::Device destructor \n");
  // NOTE: all time consuming cleanups and delete board is moved to object cleanup due to dabc shutdown mechanisms
}

bool pex::Device::DestroyByOwnThread()
{
//  if (fKinpex->IsOpen ())
//    {
//      if(fTriggeredRead) fKinpex->StopAcquisition (); // issue trigger 15, get out of wait for trigger
//    }
  DOUT1("DDDDDDDDDDd pex::Device DestroyByOwnThread()was called \n");
  return dabc::Device::DestroyByOwnThread();
}

void pex::Device::MapDMAMemoryPool(dabc::MemoryPool *pool)
{
  PEX_ASSERT_DEVICEINIT();
  if (!fZeroCopyMode)
    return;
  DOUT1("SSSSSSS Starting MapDMAMemoryPool for pool:%s", pool->GetName());
  // first clean up all previos buffers
  fKinpex->Reset();    // problematic when pool should change during DMA transfer?

  // then map dabc buffers to driver list:
  unsigned numbufs = pool ? pool->GetNumBuffers() : 0;
  DOUT3("pex::Device::MapDMAMemoryPool transport map pools buffers blocks: %u", numblocks);
  for (unsigned bufid = 0; bufid < numbufs; bufid++)
  {
    //if (!pool->IsMemoryBlock(blockid)) continue;
//	         dabc::BufferNum_t bufnum = pool->GetNumBuffersInBlock(blockid);
//	         for (dabc::BufferNum_t n=0; n < bufnum; n++)
//				 {
//	        	 dabc::BufferId_t bufid = dabc::CodeBufferId(blockid, n);
///////////////////////OLD ^
    unsigned bufsize = pool->GetBufferSize(bufid);
    void *addr = pool->GetBufferLocation(bufid);
    // first we map the buffer for sglist and register to driver lists:
    if (fKinpex->Register_DMA_Buffer((int*) addr, bufsize))
    {
      EOUT("\n\nError registering buffer num:%d of pool:%s, addr:%p \n", bufid, pool->GetName(), addr);
      continue;
    }
    // then tell the driver it should not use this dma buffer until we give it back:
    pexor::DMA_Buffer *taken = nullptr;
    if ((taken = fKinpex->Take_DMA_Buffer(false)) == nullptr)
    {
      EOUT("**** Could not take back DMA buffer %p for DABC!\n", addr);
      continue;
    }
    if (taken->Data() != (int*) addr)
    {
      EOUT("**** Mismatch of mapped DMA buffer %p and reserved buffer %p !\n", taken->Data(), addr);
      delete taken;
      continue;
    }
    delete taken;    // clean up wrapper for driver internal sg lists, we do not use Board class mempool!
//////////////////
    //				 }

  }    //blockid

}

void pex::Device::InitTrixor()
{
//
  if (fTriggeredRead)
  {
    //fKinpex->StopAcquisition (); // do not send stop trigger interrupt
    // TODO: setters to disable irqs in non trigger mode
    fKinpex->SetTriggerTimes(fTrixFClearTime, fTrixConvTime);
    fKinpex->ResetTrigger();
  }
}

bool pex::Device::StartAcquisition()
{
  DOUT1("pex::Device - Start Acquisition...");
  PEX_ASSERT_DEVICEINIT(false);
  bool rev = false;
  if (IsAcquisitionRunning())
  {
    DOUT1("pex::Device - Aqcuisition is already running, do not start again.");
    SetInfo("Aqcuisition is already running, do not start again.", true);
    return true;
  }
  fAqcuisitionRunning = true;
  if (IsTriggeredRead())
  {
    fKinpex->SetAutoTriggerReadout(IsAutoReadout(), true);
    rev = fKinpex->StartAcquisition();
  }

  else if(IsAutoAsync())
  {

#ifdef   IMPLICIT_ASYNC_WORKER
    rev = fKinpex->StartTriggerlessAcquisition (); // TODO: ringbuffer parameter
#else
    fKinpex->SetAutoTriggerReadout(false, true);
    rev = fKinpex->StartAcquisition();
#endif


  }


  SetInfo("Acqusition is started.");
  return rev;
}

bool pex::Device::StopAcquisition()
{
  DOUT1("pex::Device - Stop Acquisition...");
  PEX_ASSERT_DEVICEINIT(false);
  bool rev = false;
  if (!IsAcquisitionRunning())
  {
    DOUT1("pex::Device - Aqcuisition is already stopped, do not stop again.");
    SetInfo("pex::Device - Aqcuisition is already stopped, do not stop again.");
    return true;
  }
  if (fTriggeredRead)
    rev = fKinpex->StopAcquisition();
  else
    // for triggered read, do not change transport running state unless we recevied back trigger 15 from pexor
    fAqcuisitionRunning = false;

#ifdef   IMPLICIT_ASYNC_WORKER
  if(IsAutoAsync())
  {
    rev = fKinpex->StopTriggerlessAcquisition ();
  }
#endif

  SetInfo("Acqusition is stopped.");
  return rev;
}

void pex::Device::ObjectCleanup()
{
  DOUT1("_______pex::Device::ObjectCleanup...");
  StopAcquisition();
  if (fTriggeredRead)
  {
    //fKinpex->StopAcquisition ();
//     DOUT1 ("DDDDDDDDDDd pex::ObjectCleanup did stop acquisition\n");
    fKinpex->ResetTrigger();
//     DOUT1 ("DDDDDDDDDDd pex::ObjectCleanup did reset trigger\n");
  }

  fKinpex->Reset();    // throw away optionally user buffer mappings here
//   DOUT1 ("DDDDDDDDDDd pex::ObjectCleanup did board reset\n");
  fInitDone = false;
  delete fKinpex;
  dabc::Device::ObjectCleanup();
  DOUT1("_______pex::Device::ObjectCleanup leaving.");
}

int pex::Device::ExecuteCommand(dabc::Command cmd)
{
  // TODO: implement generic commands for:
  // start stop acquisition
  // open close file -> to readout module
  // enable/disable trigger
  // reset pexor/frontends (with virtual methods for user)
  // init frontends (with virtual methods for user)

  //DOUT1 ("pex::Device::ExecuteCommand-  %s", cmd.GetName ());
  bool res = false;
  if (cmd.IsName(pex::commandStartAcq))
  {
    DOUT1("Executing Command %s  ", pex::commandStartAcq);
    res = StartAcquisition();
    return cmd_bool(res);;
  }
  else if (cmd.IsName(pex::commandStopAcq))
  {
    DOUT1("Executing Command %s  ", pex::commandStopAcq);
    res = StopAcquisition();
    return cmd_bool(res);;
  }
  else if (cmd.IsName(pex::commandInitAcq))
  {
    DOUT1("Executing Command %s  ", pex::commandInitAcq);
    res = InitDAQ();
    return cmd_bool(res == 0);
  }
  else
    return dabc::Device::ExecuteCommand(cmd);
}

dabc::Transport* pex::Device::CreateTransport(dabc::Command cmd, const dabc::Reference &port)
{
  PEX_ASSERT_DEVICEINIT(nullptr);
  //   dabc::Url url(typ);
  //
  dabc::PortRef portref = port;
  pex::Input *dinput = new pex::Input(this);

  DOUT0("~~~~~~~~~~~~~~~~~ pex::Device::CreateTransport port %s isinp %s", portref.ItemName().c_str(),
      DBOOL (portref.IsInput ()));
  pex::Transport *transport = new pex::Transport(this, dinput, cmd, portref);
  DOUT1("pex::Device::CreateTransport creates new transport instance %p", transport);
  DOUT3(("Device thread %p\n", thread ().GetObject ()));
  return transport;
}

bool pex::Device::ExistsFrontendType(feb_kind_t kind)
{
  for (size_t ix = 0; ix < fFrontendBoards.size(); ++ix)
  {
    if (fFrontendBoards[ix]->IsType(kind))
      return true;
  }
  return false;
}

pex::FrontendBoard* pex::Device::CreateFrontendBoard(feb_kind_t kind, const std::string &modulename, dabc::Command cmd)
{
  DOUT1(" pex::Device::CreateFrontendBoard for type %s ...\n", pex::FrontendBoard::BoardName(kind).c_str());
  pex::FrontendBoard *theboard = GetFrontendBoard(kind);
  if (theboard)
  {
    DOUT1("    --- pex::Device::CreateFrontendBoard finds existing FEB entity (%p) %s.\n",theboard, theboard->GetName());
    return theboard;    // if called from factory when scanning the module tag in xml file. just pass the existing board
  }
  switch (kind)
  {
    case FEB_FEBEX3:
      theboard = new pex::Febex3(modulename, cmd);
      break;
    case FEB_POLAND:
      theboard = new pex::Poland(modulename, cmd);
      break;
    case FEB_NONE:
    case FEB_FEBEX4:
    case FEB_TAMEX:
    case FEB_CTDC:
    case FEB_FOOT:
    default:
      theboard = nullptr;

  };
  RegisterFrontendBoard(theboard);
  DOUT1("    --- pex::Device::CreateFrontendBoard has created new FEB entity.\n");
  return theboard;
}
/** Add existing frontend board component to the readout*/
void pex::Device::RegisterFrontendBoard(pex::FrontendBoard *feb)
{
  if (!feb)
    return;
  fFrontendBoards.push_back(feb);
  feb->SetDevice(this);
}

pex::FrontendBoard* pex::Device::GetFrontendBoard(feb_kind_t kind)
{
  pex::FrontendBoard *theboard = nullptr;
  for (size_t ix = 0; ix < fFrontendBoards.size(); ++ix)
  {
    theboard = fFrontendBoards[ix];
    if (theboard->IsType(kind))
      return theboard;
  }
  return nullptr;
}

int pex::Device::RequestToken(dabc::Buffer &buf, bool synchronous)
{
  PEX_ASSERT_DEVICEINIT(dabc::di_Error);
  DOUT3("RequestToken is called");

  if (!NextSFP())
  {
    EOUT(("**** No SFP channel is enabled in configuration!\n"));
    return dabc::di_Error;
  }
  if (fTriggeredRead)
  {
    // wait for trigger fired before fetching data:
    if (!fKinpex->WaitForTrigger())
    {
      // case of timeout, need dabc retry?
      EOUT(("**** Timout of trigger, retry dabc request.. \n"));
      return dabc::di_RepeatTimeOut;
    }

  }

  // new: decide if we have regular dma or zero copy:
  int *bptr = nullptr;
  unsigned int headeroffset = 0;
  if (fZeroCopyMode)
  {
    // get id and data offset
    bptr = (int*) buf.SegmentPtr();
    if (fMbsFormat)
      headeroffset = sizeof(mbs::EventHeader) + sizeof(mbs::SubeventHeader) + sizeof(int);

    DOUT3("Device RequestToken uses headeroffset :%x, mbs event:0x%x, subevent:0x%x", headeroffset,
        sizeof(mbs::EventHeader), sizeof(mbs::SubeventHeader));
    //
    // make buffer available for driver DMA:
    pexor::DMA_Buffer wrapper(bptr, buf.SegmentSize());
    if (fKinpex->Free_DMA_Buffer(&wrapper))
    {
      EOUT("**** Could not make buffer %p available for DMA!\n", bptr);
      return dabc::di_Error;
    }
  }

  // now request token from board at current sfp:

  int bufflag = (fWaitForDataReady ? fDoubleBufID[fCurrentSFP] | 2 : fDoubleBufID[fCurrentSFP]);
  pexor::DMA_Buffer *tokbuf = fKinpex->RequestToken(fCurrentSFP, bufflag, synchronous, fDirectDMA, bptr, headeroffset);    // synchronous dma mode here
  if ((long int) tokbuf == -1)    // TODO: handle error situations by exceptions later!
  {
    EOUT("**** Error in PEXOR Token Request from sfp %d !\n", fCurrentSFP);
    return dabc::di_SkipBuffer;
  }
  fDoubleBufID[fCurrentSFP] = fDoubleBufID[fCurrentSFP] == 0 ? 1 : 0;
  if (!synchronous)
    return dabc::di_Ok;

  if (fTriggeredRead && !fEarlyTriggerClear)
    fKinpex->ResetTrigger();
  return (CopyOutputBuffer(tokbuf, buf));

}

int pex::Device::RequestMultiToken(dabc::Buffer &buf, bool synchronous, uint16_t trigtype)
{
  DOUT3("pex::Device::RequestMultiToken with synchronous=%d ", synchronous);
  pexor::DMA_Buffer *tokbuf = nullptr;
  unsigned int channelmask = 0;
  for (int ix = 0; ix < PEX_NUMSFP; ++ix)
  {
    if (fEnabledSFP[ix])
      channelmask |= (1 << ix);
  }
  DOUT2("pex::Device::RequestMultiToken with channelmask:0x%x", channelmask);
  int bufflag = (fWaitForDataReady ? fDoubleBufID[0] | 2 : fDoubleBufID[0]);
  tokbuf = fKinpex->RequestMultiToken(channelmask, bufflag, synchronous, fDirectDMA);
  DOUT3("pex::Device::RequestAllTokens gets dma buffer 0x%x", tokbuf);

  if ((long int) tokbuf == -1)    // TODO: handle error situations by exceptions later!
  {
    EOUT("**** Error in PEXOR Multi Token Request with mask %x !\n", channelmask);
    return dabc::di_Error;
  }
  fDoubleBufID[0] = fDoubleBufID[0] == 0 ? 1 : 0;    // in this mode, we only use double buffer id of first sfp

  if (!synchronous)
    return dabc::di_Ok;
  if (fTriggeredRead && !fEarlyTriggerClear)
    fKinpex->ResetTrigger();
  return (CopyOutputBuffer(tokbuf, buf, trigtype));

}

int pex::Device::RequestReceiveParallelTokens(dabc::Buffer &buf, uint16_t trigtype)
{
  DOUT3("pex::Device::RequestReceiveParallelTokens");
  pexor::DMA_Buffer *tokbuf = nullptr;
  unsigned int channelmask = 0;
  for (int ix = 0; ix < PEX_NUMSFP; ++ix)
  {
    if (fEnabledSFP[ix])
      channelmask |= (1 << ix);
  }
  DOUT2("pex::Device::RequestReceiveParallelTokens with channelmask:0x%x", channelmask);
  int bufflag = (fWaitForDataReady ? fDoubleBufID[0] | 2 : fDoubleBufID[0]);
  tokbuf = fKinpex->RequestReceiveAllTokens(channelmask, bufflag, nullptr, 0);
  DOUT3("pex::Device::RequestReceiveParallelTokens gets dma buffer 0x%x", tokbuf);

  if ((long int) tokbuf == -1)    // TODO: handle error situations by exceptions later!
  {
    EOUT("**** Error in PEXOR RequestReceiveParallelTokens  with mask %x !\n", channelmask);
    return dabc::di_Error;
  }
  fDoubleBufID[0] = fDoubleBufID[0] == 0 ? 1 : 0;    // in this mode, we only use double buffer id of first sfp

  if (fTriggeredRead && !fEarlyTriggerClear)
    fKinpex->ResetTrigger();
  return (CopyOutputBuffer(tokbuf, buf, trigtype));
}

int pex::Device::ReceiveTokenBuffer(dabc::Buffer &buf)
{
  // for asynchronous request, we need to put here again the check for zero copy dma:
  int *bptr = nullptr;
  unsigned int headeroffset = 0;
  if (fZeroCopyMode)
  {
    bptr = (int*) buf.SegmentPtr();
    if (fMbsFormat)
      headeroffset = sizeof(mbs::EventHeader) + sizeof(mbs::SubeventHeader) + sizeof(int);
  }
  pexor::DMA_Buffer *tokbuf = fKinpex->WaitForToken(fCurrentSFP, fDirectDMA, bptr, headeroffset);
  //if (tokbuf == 0)
  if ((tokbuf == nullptr) || ((long int) tokbuf == -1))
  {
    EOUT("**** Error in PEXOR ReceiveTokenBuffer from sfp %d !\n", fCurrentSFP);
    return dabc::di_Error;
    //return dabc::di_SkipBuffer;
  }
  if (fTriggeredRead&& !fEarlyTriggerClear )
    fKinpex->ResetTrigger();

  return (CopyOutputBuffer(tokbuf, buf));

}

int pex::Device::RequestAllTokens(dabc::Buffer &buf, bool synchronous, uint16_t trigtype)
{
  DOUT3("pex::Device::RequestAllTokens with synchronous=%d ", synchronous);

  dabc::LockGuard(fRequestMutex, true);    // for asynchronous request from several SFPS may need to protect our flags

  static pexor::DMA_Buffer *tokbuf[PEX_NUMSFP];
  if ((fMemoryTest && !fSkipRequest) || (!fMemoryTest))
  {
    for (fCurrentSFP = 0; fCurrentSFP < PEX_NUMSFP; ++fCurrentSFP)
    {
      tokbuf[fCurrentSFP] = nullptr;
      if (!fEnabledSFP[fCurrentSFP])
        continue;

      if (!synchronous && fRequestedSFP[fCurrentSFP])
        continue;    // no second request if data has not yet been received

      int bufflag = (fWaitForDataReady ? fDoubleBufID[fCurrentSFP] | 2 : fDoubleBufID[fCurrentSFP]);
      tokbuf[fCurrentSFP] = fKinpex->RequestToken(fCurrentSFP, bufflag, synchronous, fDirectDMA);
      DOUT3("pex::Device::RequestAllTokens gets dma buffer 0x%x for sfp:%d ", tokbuf[fCurrentSFP], fCurrentSFP);

      if (!synchronous)
        fRequestedSFP[fCurrentSFP] = true;    // mark request for receiver function
      if ((long int) tokbuf[fCurrentSFP] == -1)    // TODO: handle error situations by exceptions later!
      {
        EOUT("**** Error in PEXOR Token Request from sfp %d !\n", fCurrentSFP);
        return dabc::di_Error;
      }
      fDoubleBufID[fCurrentSFP] = fDoubleBufID[fCurrentSFP] == 0 ? 1 : 0;
    }

  }
  if (!synchronous)
    return dabc::di_Ok;
  fSkipRequest = true;
  if (fTriggeredRead && !fEarlyTriggerClear)
  {
    fKinpex->ResetTrigger();
    DOUT3("RRRRRRRRRRRRRRRR pex::Device::RequestAllTokens has reset trigger!\n");
  }

  return (CombineTokenBuffers(tokbuf, buf, trigtype));

}

int pex::Device::ReceiveAllTokenBuffer(dabc::Buffer &buf, uint16_t trigtype)
{
  DOUT2("pex::Device::ReceiveAllTokenBuffer...\n");
  ///////////////// JAM DEBUG
  static unsigned long emptytokbufcount[4] = { 0, 0, 0, 0 };

  ///////////////
  dabc::LockGuard(fRequestMutex, true);    // for asynchronous request from several SFPS may need to protect our flags
  static pexor::DMA_Buffer *tokbuf[PEX_NUMSFP];
  static int oldbuflen = 0;
  for (int sfp = 0; sfp < PEX_NUMSFP; ++sfp)
  {
    tokbuf[sfp] = nullptr;
    if (!fEnabledSFP[sfp])
      continue;
    if (!IsSynchronousRead() && !fRequestedSFP[sfp])
      continue;

    if ((fMemoryTest && !fSkipRequest) || !fMemoryTest)
    {
      tokbuf[sfp] = fKinpex->WaitForToken(sfp, fDirectDMA, nullptr, 0, IsSynchronousRead());    // error messages supressed for sync read
      if ((tokbuf[sfp] == nullptr) || ((long int) tokbuf[sfp] == -1))
      {
        if (IsSynchronousRead())    // no error on timeout for asynchronous triggerless mode!
        {
          EOUT("**** Error in PEXOR ReceiveAllTokenBuffer from sfp %d !\n", sfp);
          return dabc::di_Error;
          //return dabc::di_SkipBuffer;
        }
        else
        {
          tokbuf[sfp] = nullptr;    // mark currently empty "sfp subevent" for Combine function (for -1 return value of WaitForToken)
          emptytokbufcount[sfp]++;

          if ((emptytokbufcount[sfp] > 10000) && (emptytokbufcount[sfp] % 10000) == 0)
          {
            DOUT0("pex::Device::ReceiveAllTokenBuffer sfp %d has seen  %ld empty tokens\n", sfp, emptytokbufcount[sfp]);
            DOUT0("******** DUMP fRequestedSFP[%d]=%d , fDoubleBufID[%d]=%d, hasdata=%d\n", sfp, fRequestedSFP[sfp],
                sfp, fDoubleBufID[sfp], fHasData);
          }
          continue;
        }

      }
      else
      {
        fRequestedSFP[sfp] = false;    // data was received, reset the request flag
        emptytokbufcount[sfp] = 0;

      }
      oldbuflen = tokbuf[sfp]->UsedSize();
      DOUT3("pex::Device::ReceiveAllTokenBuffer got token buffer of len %d\n", oldbuflen);
    }
    else
    {
      tokbuf[sfp] = fKinpex->Take_DMA_Buffer();    // get empty buffer to emulate sync
      tokbuf[sfp]->SetUsedSize(oldbuflen);    // set to length of real dma buffer
      DOUT3("pex::Device::ReceiveAllTokenBuffer set dummy buffer len to %d\n", oldbuflen);
    }
  }
  if (!fSkipRequest)
    fSkipRequest = true;    // switch off all subsequent requests after the first

  if (fTriggeredRead && !fEarlyTriggerClear)
  {
    fKinpex->ResetTrigger();
    DOUT3("RRRRRRRRRRRRRRRR pex::Device::ReceiveAllTokenBuffer has reset trigger!\n");
  }

  return (CombineTokenBuffers(tokbuf, buf, trigtype));
}

int pex::Device::ReceiveAutoTriggerBuffer(dabc::Buffer &buf, uint8_t &trigtype)
{
  pexor::DMA_Buffer *trigbuf = fKinpex->WaitForTriggerBuffer();
  if (trigbuf == nullptr)
  {
    EOUT("**** Error in ReceiveAutoTriggerBuffer\n");
    return dabc::di_SkipBuffer;
  }
  else if ((long int) trigbuf == -1)
  {
    EOUT(("**** Timout of trigger in ReceiveAutoTriggerBuffer, retry...\n"));
    fKinpex->ResetTrigger();
    return dabc::di_RepeatTimeOut;
  }

  trigtype = fKinpex->GetTriggerType();
  if (trigtype == 14 || trigtype == 15)
  {
    // workaround for empty dma buffers when processing special triggers
    // do not copy anything, just free dma buffers and account event:
    if (!fZeroCopyMode)
    {
      fKinpex->Free_DMA_Buffer(trigbuf);    // put dma buffer back to free lists
    }
    else
    {
      delete trigbuf;    // for zero copy mode, this is just a temporary wrapper. Will be freed before request
    }
    fNumEvents++;    // for unique header sequence number
    return 0;
  }
  return (CopyOutputBuffer(trigbuf, buf, trigtype));
}

int pex::Device::ReceiveAutoAsyncBuffer(dabc::Buffer &buf)
{
  pexor::DMA_Buffer *trigbuf = fKinpex->RequestReceiveAsyncTokens();
  if (trigbuf == nullptr)
  {
    return dabc::di_RepeatTimeOut;    // polling for data mode
  }
  else if ((long int) trigbuf == -1)
  {
    EOUT("**** Error in ReceiveAutoAsyncBuffer\n");
    return dabc::di_SkipBuffer;
  }
  else
  {
    return (CopyOutputBuffer(trigbuf, buf, PEXOR_TRIGTYPE_NONE));
  }
}

int pex::Device::ReceiveAutoAsyncBufferPolling(dabc::Buffer &buf)
{
  pexor::DMA_Buffer *trigbuf = fKinpex->RequestReceiveAsyncTokensPolling();
  if (trigbuf == nullptr)
  {
    EOUT("**** TimeOut in ReceiveAutoAsyncBufferPolling\n");
    return dabc::di_RepeatTimeOut;    //poll again from userland, but this is not save
  }
  else if ((long int) trigbuf == -1)
  {
    EOUT("**** Error in ReceiveAutoAsyncBufferPolling\n");
    return dabc::di_Error;    // no explicit polling here, this is a real error.
  }
  else
  {
    return (CopyOutputBuffer(trigbuf, buf, PEXOR_TRIGTYPE_NONE));
  }
}

int pex::Device::ReceiveNextAsyncBuffer(dabc::Buffer &buf)
{
  pexor::DMA_Buffer *trigbuf = fKinpex->ReceiveNextAsyncBuffer();
  if (trigbuf == nullptr)
  {
    DOUT3("**** TimeOut in ReceiveNextAsyncBuffer\n");
    return dabc::di_RepeatTimeOut;    //poll again explicitely from userland
  }
  else if ((long int) trigbuf == -1)
  {
    EOUT("**** Error in ReceiveNextAsyncBuffer, NEVER COME HERE\n");
    return dabc::di_Error;    // no explicit polling here, this is a real error.
  }
  else
  {
    return (CopyOutputBuffer(trigbuf, buf, PEXOR_TRIGTYPE_NONE));
  }
}

int pex::Device::CopyOutputBuffer(pexor::DMA_Buffer *tokbuf, dabc::Buffer &buf, uint16_t trigtype)
{
  dabc::Pointer ptr(buf);
  unsigned int filled_size = 0, used_size = 0;
  if (fMbsFormat)
  {
    mbs::EventHeader *evhdr = PutMbsEventHeader(ptr, fNumEvents, trigtype);
    used_size += sizeof(mbs::EventHeader);

    mbs::SubeventHeader *subhdr = PutMbsSubeventHeader(ptr,
        ((fSingleSubevent || fMultichannelRequest) ? fSubeventSubcrate : fCurrentSFP), fSubeventControl,
        fSubeventProcid);

    filled_size += sizeof(mbs::SubeventHeader);
    used_size += sizeof(mbs::SubeventHeader);

    // here account for zero copy alignment: headers+int give 32 bytes before payload
    // fill padding space with pattern like in mbs:
    if (PutMbsPaddingWords(ptr, 1) < 0)
      return dabc::di_SkipBuffer;
    filled_size += sizeof(int);
    used_size += sizeof(int);
    // UsedSize contains the real received token data length, as set by driver
    //subhdr->SetRawDataSize (tokbuf->UsedSize () + sizeof(int));
    filled_size += tokbuf->UsedSize();
    evhdr->SetSubEventsSize(filled_size);
    subhdr->SetRawDataSize(filled_size - sizeof(mbs::SubeventHeader));

    buf.SetTypeId(mbs::mbt_MbsEvents);
  }
  DOUT3("Token buffer size:%ld, used size%ld, target buffer size:%d\n", tokbuf->Size(), tokbuf->UsedSize(),
      buf.GetTotalSize());


  if (tokbuf->UsedSize() + used_size > buf.GetTotalSize())
  {
    EOUT("Token buffer used size %ld + header sizes %d exceed available target buffer length %d \n", tokbuf->UsedSize(),
        used_size, buf.GetTotalSize());
    EOUT("Mbs Event header size is %ld;  Mbs subevent header sizes: %ld \n", sizeof(mbs::EventHeader),
        sizeof(mbs::SubeventHeader));
    EOUT("Mbs event filled size  %d\n", filled_size);
    EOUT("**** Error in PEXOR Token Request size, skip buffer!\n");
    return dabc::di_SkipBuffer;
  }

  used_size += tokbuf->UsedSize();

  if (!fZeroCopyMode)
  {
    ptr.copyfrom(tokbuf->Data(), tokbuf->UsedSize());
    fKinpex->Free_DMA_Buffer(tokbuf);    // put dma buffer back to free lists
  }
  else
  {
    delete tokbuf;    // for zero copy mode, this is just a temporary wrapper. Will be freed before request
  }
  buf.SetTotalSize(used_size);
  fNumEvents++;
  return used_size;

}

int pex::Device::CombineTokenBuffers(pexor::DMA_Buffer **src, dabc::Buffer &buf, uint16_t trigtype)
{
  // for asynch triggerless mode, we have to check here if there is data anyhow:
  fHasData = false;
  for (int sfp = 0; sfp < PEX_NUMSFP; ++sfp)
  {
    if (src[sfp] != nullptr)
    {
      fHasData = true;
      break;
    }
  }
  if (!fHasData)
    return dabc::di_SkipBuffer;
  // end check for triggerless readout

  dabc::Pointer ptr(buf);
  DOUT3("pex::Device::CombineTokenBuffers initial pointer is 0x%x, dabc buffer size: %ld", ptr.ptr (), buf.GetTotalSize()); // JAM26
  unsigned int filled_size = 0, used_size = 0;
  mbs::EventHeader *evhdr = nullptr;
  mbs::SubeventHeader *subhdr = nullptr;
  if (fMbsFormat)
  {
    evhdr = PutMbsEventHeader(ptr, fNumEvents, trigtype);    // TODO: get current trigger type from trixor and set
    if (evhdr == nullptr)
      return dabc::di_SkipBuffer;    // buffer too small error
    used_size += sizeof(mbs::EventHeader);
    if (fSingleSubevent)
    {
      // one common subevent for data of all sfps:
      subhdr = PutMbsSubeventHeader(ptr, fSubeventSubcrate, fSubeventControl, fSubeventProcid);
      if (subhdr == nullptr)
        return dabc::di_SkipBuffer;    // buffer too small error
      used_size += sizeof(mbs::SubeventHeader);
      filled_size += sizeof(mbs::SubeventHeader);
      // here account for zero copy alignment: headers+int give 32 bytes before payload
      // necessary for the explodertest unpacker up to now
      // we need some padding words here for mbs tailored unpacker:
      if (PutMbsPaddingWords(ptr, 1) < 0)
        return dabc::di_SkipBuffer;
      used_size += sizeof(int);
      filled_size += sizeof(int);
      // TODO: can we switch padding behaviour by parameter?
    }
  }
  DOUT3("pex::Device::CombineTokenBuffers output pointer after mbs header is 0x%x", ptr.ptr ());
  for (int sfp = 0; sfp < PEX_NUMSFP; ++sfp)
  {
    if (src[sfp] == nullptr)
      continue;
    int increment = CopySubevent(src[sfp], ptr, sfp);
    if (increment < 0)
      return increment;    // error on copy, propagate error type upwards
    used_size += increment;
    filled_size += increment;
    DOUT3("pex::Device::CombineTokenBuffers after sfp %d : used size:%d filled size:%d", (int) sfp, used_size,
        filled_size);
  }
  if (fMbsFormat)
  {
    if (fSingleSubevent)
      subhdr->SetRawDataSize(filled_size - sizeof(mbs::SubeventHeader));
    evhdr->SetSubEventsSize(filled_size);
    buf.SetTypeId(mbs::mbt_MbsEvents);
  }
  buf.SetTotalSize(used_size);
  fNumEvents++;
  //sleep(1); // JAM debug
  return used_size;

}

int pex::Device::CopySubevent(pexor::DMA_Buffer *tokbuf, dabc::Pointer &cursor, char sfpnum)
{
  unsigned int filled_size = 0;
  // JAM26
  bool debug=false;
  if(tokbuf->UsedSize()>30000) debug=true;
  if(debug)
    DOUT0("pex::Device::CopySubevent has dma buffer %p (used size:%ld) for sfp %d, output cursor pointer :%p", tokbuf,tokbuf->UsedSize(), (int ) sfpnum,
      cursor.ptr());

  if (fMbsFormat && !fSingleSubevent)
  {
    mbs::SubeventHeader *subhdr = PutMbsSubeventHeader(cursor, sfpnum, fSubeventControl, fSubeventProcid);
    if (subhdr == nullptr)
      return dabc::di_SkipBuffer;    // buffer too small error

    filled_size += sizeof(mbs::SubeventHeader);
    // here account for zero copy alignment: headers+int give 32 bytes before payload
    // fill with padding word descriptor like in mbs:
    if (PutMbsPaddingWords(cursor, 1) < 0)
      return dabc::di_SkipBuffer;
    filled_size += sizeof(int);
    subhdr->SetRawDataSize(tokbuf->UsedSize() + sizeof(int));
  }
  cursor.copyfrom(tokbuf->Data(), tokbuf->UsedSize());
  cursor.shift(tokbuf->UsedSize());    // NOTE: you have to shift current pointer yourself after copyfrom!!
  DOUT3("pex::Device::CopySubevent output cursor pointer after copyfrom  and shift is:0x%x", cursor.ptr ());
  filled_size += tokbuf->UsedSize();
  fKinpex->Free_DMA_Buffer(tokbuf);
  if(debug){
  DOUT0("---------- token used size :%ld", tokbuf->UsedSize());
  DOUT0("---------- filledsize :%d", filled_size);
  sleep(1);
  }
  return filled_size;
}

mbs::EventHeader* pex::Device::PutMbsEventHeader(dabc::Pointer &ptr, mbs::EventNumType eventnumber, uint16_t trigger)
{
  // check if header would exceed buffer length.
  if (ptr.rawsize() < sizeof(mbs::EventHeader))
  {
    DOUT0("pex::Device::PutMbsEventHeader fails because no more space in buffer, restsize=%d bytes", ptr.rawsize());
    return nullptr;
  }

  mbs::EventHeader *evhdr = (mbs::EventHeader*) ptr();
  evhdr->Init(eventnumber);
  // put here trigger type
  evhdr->iTrigger = trigger;
  //fKinpex->GetTriggerType();

  ptr.shift(sizeof(mbs::EventHeader));
  return evhdr;
}

mbs::SubeventHeader* pex::Device::PutMbsSubeventHeader(dabc::Pointer &ptr, int8_t subcrate, int8_t control,
    int16_t procid)
{
  if (ptr.rawsize() < sizeof(mbs::SubeventHeader))
  {
    DOUT0("pex::Device::PutMbsSubeventHeader fails because no more space in buffer, restsize=%d bytes", ptr.rawsize());
    return nullptr;
  }
  mbs::SubeventHeader *subhdr = (mbs::SubeventHeader*) ptr();
  subhdr->Init();
  subhdr->iProcId = procid;
  subhdr->iSubcrate = subcrate;
  subhdr->iControl = control;
  ptr.shift(sizeof(mbs::SubeventHeader));
  return subhdr;
}

int pex::Device::PutMbsPaddingWords(dabc::Pointer &ptr, uint8_t num)
{
// TODO: check if padding words would exceed buffer length.
  if (ptr.rawsize() < num * sizeof(int))
  {
    DOUT0("pex::Device::PutMbsPaddingWords fails because no more space in buffer, restsize=%d bytes, paddingwords=%d",
        ptr.rawsize(), num);
    return -1;
  }

  for (uint8_t k = 0; k < num; k++)
  {
    int *pl_dat = (int*) ptr();
    *pl_dat = 0xadd00000 + (num << 8) + k;
    ptr.shift(sizeof(int));
  }
  return num;
}

bool pex::Device::NextSFP()
{
  int loopcounter = 0;
  do
  {
    fCurrentSFP++;
    if (fCurrentSFP >= PEX_NUMSFP)
      fCurrentSFP = 0;
    if (loopcounter++ > PEX_NUMSFP)
      return false;    // no sfp is enabled,error
  } while (!fEnabledSFP[fCurrentSFP]);
  return true;
}

double pex::Device::Read_Timeout()
{
  if (!IsAcquisitionRunning())
    return 1;
  else
    return 0.5e-3;    //1.0e-3; // 10s JAM - timeout for triggerless polling mode TODO: configure in device
}

unsigned pex::Device::Read_Size()
{
  PEX_ASSERT_DEVICEINIT(dabc::di_Error);
  int res = GetReadLength();
  DOUT3("Read_Size()=%d\n", res);
  if (IsTriggeredRead() || IsSynchronousRead())
  {
    return res;
  }
  else
  {
    dabc::LockGuard(fRequestMutex, true);    // for asynchronous request from several SFPS may need to protect our flags
    if (!IsAcquisitionRunning())
    {

      DOUT0("pex::Device::Read_Size: acquisition is stopped.\n");
      return dabc::di_RepeatTimeOut;
    }
    else if (!fHasData)
    {
      fHasData = true;    // next time we want to retry reading
      DOUT3("pex::Device::Read_Size: no data on polling, transport timeout...\n");
      return dabc::di_RepeatTimeOut;
    }
    else
      return res;
  }
}

unsigned pex::Device::Read_Start(dabc::Buffer &buf)
{
  PEX_ASSERT_DEVICEINIT(dabc::di_Error);
  if (IsTriggeredRead() || IsSynchronousRead() || IsAutoAsync())
  {
    return dabc::di_Ok;    // synchronous mode, all handled in Read_Complete
  }
  else
  {
    return (unsigned) (RequestAllTokens(buf, false));
  }
}

unsigned pex::Device::Read_Complete(dabc::Buffer &buf)
{
  PEX_ASSERT_DEVICEINIT(dabc::di_Error);
  if (!IsAcquisitionRunning())
  {
    DOUT0("pex::Device::Read_Complete: acquisition is stopped, transport timeout...\n");
    return dabc::di_RepeatTimeOut;
  }
  int res = dabc::di_Ok;
  int retsize = 0;

  // on trigger, we always read all sfp channels! so always "parallel" mode for dabc
  if (IsTriggeredRead() || IsSynchronousRead())
  {
    uint8_t trigtype = 0;
    if (IsTriggeredRead())
    {
      if (IsAutoReadout())
      {
        retsize = ReceiveAutoTriggerBuffer(buf, trigtype);
        if (retsize < 0)
          return retsize;
      }
      else
      {

        // wait for trigger fired before fetching data:
        if (!fKinpex->WaitForTrigger())
        {
          // case of timeout, need dabc retry?
          EOUT(("**** Timout of trigger, retry dabc request.. \n"));
          fKinpex->ResetTrigger();
          return dabc::di_RepeatTimeOut;
        }

        // trigger is received, optionally dump something:
        if (dabc::lgr()->GetDebugLevel() > 1)
          fKinpex->DumpTriggerStatus();
        trigtype = fKinpex->GetTriggerType();
      }    // if else autoreadout

    }
    else
    {
      trigtype = PEXOR_TRIGTYPE_NONE;
    }
    retsize = User_Readout(buf, trigtype);
  }
  else if (IsAutoAsync())
  {
    // implicit free running mode with asynchronous sfps
#if defined(IMPLICIT_ASYNC_POLLING)
    retsize = ReceiveAutoAsyncBufferPolling(buf);
#elif defined(IMPLICIT_ASYNC_WORKER)
    retsize = ReceiveNextAsyncBuffer(buf);
#else
    retsize = ReceiveAutoAsyncBuffer(buf);
#endif
  }
  else
  {
    // asynchronous dabc readout without trigger - special case not used for standard daq
    retsize = ReceiveAllTokenBuffer(buf);
  }
  if (retsize < 0)
    return retsize;
  Par(pex::parDeviceDRate).SetValue(((double) retsize) / 1024.);
  //fReadLength=retsize; // do not always adjust receive buffer length by default!
  return (unsigned) res;
}

int pex::Device::User_Readout(dabc::Buffer &buf, uint8_t trigtype)
{
  //int res = dabc::di_Ok;
  int retsize = 0;
  unsigned int filled_size = 0, used_size = 0;
  mbs::EventHeader *evhdr = nullptr;
  mbs::SubeventHeader *subhdr = nullptr;

  switch (trigtype)
  {
    // do not read out for start or stop trigger:
    case PEXOR_TRIGTYPE_START:
    case PEXOR_TRIGTYPE_STOP:
      switch (trigtype)
      {
        case PEXOR_TRIGTYPE_START:

          DOUT1("pex::Device::User_Readout finds start trigger :%d !!", trigtype);
          SetInfo(dabc::format("User_Readout finds start trigger :%d !!", trigtype));

          // in most generic way, it is problematic to define double buffer id at start,
          // since we have no means here to intialize any frontend to same number!
          // we leave this to subclass devices and hope that daq and frontends keep consistency...
          //InitDAQ(); // need this to sync frontend buf ids ? is it enough? no
          // JAM2016: need to reset frontend double buffer id after start to 0
          // otherwise we might end up in unwanted "user trigger clear mode" (or no readout at all...)
//          for (int sfp = 0; sfp < PEX_NUMSFP; sfp++)
//          {
//            if (!fEnabledSFP[sfp])
//              continue;
//            fDoubleBufID[sfp] = 0;
//          }
//          DOUT1("pex::Device::User_Readout resets initial bufid 0 \n");
          fAqcuisitionRunning = true;
          break;

        case PEXOR_TRIGTYPE_STOP:

          DOUT1("pex::Device::User_Readout finds stop trigger :%d !!", trigtype);
          SetInfo(dabc::format("User_Readout finds stop trigger :%d !!", trigtype));
          fAqcuisitionRunning = false;
          break;

        default:

          break;
      }
      ;
// JAM 2026 moved this to start stop trigger type only
      if (!IsAutoReadout())
        fKinpex->ResetTrigger();

      if (fMbsFormat)
      {
        // as default, we deliver empty event and subevent just marking trigger type and ids:
        dabc::Pointer ptr(buf);
        evhdr = PutMbsEventHeader(ptr, fNumEvents, trigtype);    // TODO: get current trigger type from trixor and set
        if (evhdr == nullptr)
          return dabc::di_SkipBuffer;    // buffer too small error
        used_size += sizeof(mbs::EventHeader);
        subhdr = PutMbsSubeventHeader(ptr, fSubeventSubcrate, fSubeventControl, fSubeventProcid);
        if (subhdr == nullptr)
          return dabc::di_SkipBuffer;    // buffer too small error
        used_size += sizeof(mbs::SubeventHeader);
        filled_size += sizeof(mbs::SubeventHeader);
        /////////////
        // put some dummy payload data here:
//              if(PutMbsPaddingWords(ptr, 1)<0)
//                return dabc::di_SkipBuffer;
//              used_size += sizeof(int);
//              filled_size += sizeof(int);
        /////////////
        subhdr->SetRawDataSize(filled_size - sizeof(mbs::SubeventHeader));
        evhdr->SetSubEventsSize(filled_size);
        buf.SetTypeId(mbs::mbt_MbsEvents);
        buf.SetTotalSize(used_size);
        retsize = used_size;
      }
      else
      {
        // no mbs format (very hypothetical): just skip buffer
        return dabc::di_SkipBuffer;
      }
      //return dabc::di_RepeatTimeOut;

      break;

//    case PEXOR_TRIGTYPE_STOP:
//      {
//        DOUT1("pex::Device::User_Readout finds stop trigger :%d !!", trigtype);
//        SetInfo(dabc::format("User_Readout finds stop trigger :%d !!", trigtype));
//        if (!IsAutoReadout ())
//          fKinpex->ResetTrigger ();
//        fAqcuisitionRunning = false;
//        // TODO: do not repeat with timeout, but return event buffer with start trigger type to data stream
//        //return dabc::di_SkipBuffer;
//
//        return dabc::di_RepeatTimeOut;    // need this timeout for proper shutdown?
//      }
//      break;

    case PEXOR_TRIGTYPE_NONE:    // we may put different behaviour for triggerless readout here
    default:
      trigtype = mbs::tt_Event;    // for the moment, triggerless events are marked as regular data events

      if (!IsAutoReadout())    // only request further data if we do not have already automatically filled buffer
      {

        if(fEarlyTriggerClear)
          fKinpex->ResetTrigger(); // new JAM 3-3-2026

        if (IsMultichannelMode())
        {
          // NOTE: asynchronous channelmask request does not work principally with direct DMA to host buffer!
          // leads to severe machine crash due to pexor dma (all sfp will do dma to first buffer simultaneously?)
          // this mode is not reasonable!!!!!!
          if (IsDirectDMA())
          {
            EOUT("pex::Device::  PexorMultiTokenDMA Mode does not work with direct DMA! Please change config.\n");
            exit(1);
          }

#ifdef LIBPEXOR_ATOMIC_IOCTLS
          retsize = RequestReceiveParallelTokens(buf, trigtype);
#else
            unsigned res = RequestMultiToken (buf, false);    // does not work as synchronous request! dma buffers are still separately send
            if ((unsigned) res != dabc::di_Ok)
              return res;    // propagate error type
            retsize = ReceiveAllTokenBuffer (buf, trigtype);
#endif
        }
        else
        {

          retsize = RequestAllTokens(buf, true, trigtype);    // need synchronous mode here, otherwise mixup between sfps and dma buffer contents!
          //////////////////////////////// trigger reset is optionally done in receive before buffer combining/copying
        }
      }
      else
      {
        retsize = buf.GetTotalSize();    //account already filled buffer size for ratemeter
      }
      break;
  };    // switch
  Par(pex::parDaqRunningState).SetValue(fAqcuisitionRunning);
  return retsize;
}

void pex::Device::SetInfo(const std::string &info, bool forceinfo)
{
//   DOUT0("SET INFO: %s", info.c_str());

  dabc::InfoParameter par;

  if (!fDevInfoName.empty())
    par = Par(fDevInfoName);

  par.SetValue(info);
  if (forceinfo)
    par.FireModified();
}

double pex::Device::ProcessTimeout(double /* last_diff */)
{
  //Par(pex::parDaqRunningState).SetValue(fAqcuisitionRunning);
  dabc::Parameter par = Par(pex::parDaqRunningState);
  par.SetValue(fAqcuisitionRunning);
  DOUT3("pex::Device::ProcessTimeout with dt= %f s - acuisition state=%d, parameter=%d!!", last_diff,
      fAqcuisitionRunning, Par(pex::parDaqRunningState).Value().AsBool());
  dabc::Hierarchy chld = fWorkerHierarchy.FindChild(pex::parDaqRunningState);
  if (!chld.null())
  {
    par.ScanParamFields(&chld()->Fields());
    fWorkerHierarchy.MarkChangedItems();
  }
  else
  {
    DOUT0("pex::Device::ProcessTimeout could not find parameter %s", pex::parDaqRunningState);
  }
  // here workaround for missing update of dabc run state:
//    std::string statename=std::string("/App/")+dabc::Application::StateParName();
//    dabc::Parameter statepar=Par(statename);
//    if (statepar.null())
//      DOUT0("pex::Device::ProcessTimeout could not find parameter %s", statename.c_str());
//      dabc::Hierarchy statechld = fWorkerHierarchy.GetParent()->FindChildRef(statename.c_str());
//        if (!statechld.null())
//        {
//             statepar.ScanParamFields(&statechld()->Fields());
//             statechld.MarkChangedItems();
//             //(dabc::HierarchyContainer*) (fWorkerHierarchy.GetParent())->MarkChangedItems();
//
//        }
//        else
//        {
//            DOUT0("pex::Device::ProcessTimeout could not find parameter child %s", statename.c_str());
//        }

  return PEX_REFRESHTIMEOUT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// the most simple readout implementation:

pex::GenericDevice::GenericDevice(const std::string &name, dabc::Command cmd) :
    pex::Device(name, cmd)
{
  DOUT1("Constructing GenericDevice...\n");
  //PublishPars("$CONTEXT$/PexDevice");
  fInitDone = true;
  // initial start acquisition here, not done from transport start anymore:
  //StartAcquisition();
}

