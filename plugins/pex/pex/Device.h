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
#ifndef PEX_Device
#define PEX_Device

#include <vector>
#include <string>
#include <stdint.h>

#include "dabc/Device.h"
#include "dabc/Object.h"
#include "dabc/MemoryPool.h"
#include "dabc/threads.h"

namespace pexor
{
class PexorTwo;
class DMA_Buffer;
}

#include "mbs/MbsTypeDefs.h"
#include "pex/FrontendBoard.h"

///** number of connected sfps*/
//#define PEX_NUMSFP 4
//
///** maximum number of connected slaves per sfp*/
//#define PEX_MAXSLAVES 16

/* interval in seconds to refresh exported control variables*/
#define PEX_REFRESHTIMEOUT 1.0

/* test switch to check difference between new atomic parallel token request*/
#define LIBPEXOR_ATOMIC_IOCTLS 1

/** switch between implicit polling in driver (locked), and explicit dabc polling
 * in case of free running async readout from different fast sfp chains*/
//#define IMPLICIT_ASYNC_POLLING 1
/** this switches to polling in asynchronous kernel worker*/
//#define IMPLICIT_ASYNC_WORKER 1

namespace pex
{

extern const char *xmlPexorID;    //< id number N of pexor device file /dev/pexor-N
extern const char *xmlPexorSFPSlaves;    //< prefix for the sfp numbers 0,1,2,3 indicating number of slave devices connected
extern const char *xmlPexorSlaveTypes;    //< prefix for the sfp numbers 0,1,2,3 indicating slave types
extern const char *xmlRawFile;    //< name of output lmd file
extern const char *xmlDMABufLen;    //< length of DMA buffers to allocate in driver
extern const char *xmlDMABufNum;    //< number of DMA buffers
extern const char *xmlDMAScatterGatherMode;    //< switch scatter gather dma on/off
extern const char *xmlDMAZeroCopy;    //< switch zero copy scatter gather dma on/off
extern const char *xmlExploderSubmem;    //< exploder submem size for testbuffer
extern const char *xmlFormatMbs;    //< enable mbs formating already in device transport
extern const char *xmlSingleMbsSubevt;    //<  use one single subevent for all sfps
extern const char *xmlMultichannelRequest;    //<  enable channelpattern request with combined dma for multiple sfps
extern const char *xmlAutoTriggerRead;    //<  enable automatic readout of all configured token data in driver for each trigger
extern const char *xmlAutoAsyncRead;    //<  enable automatic triggerless readout of all configured token data in driver
extern const char *xmlMbsSubevtCrate;    //<  define crate number for subevent header
extern const char *xmlMbsSubevtControl;    //<  define crate number for subevent header
extern const char *xmlMbsSubevtProcid;    //<  define procid number for subevent header
extern const char *xmlSyncRead;    //< switch synchronous or asynchronous token dma
extern const char *xmlTriggeredRead;    //< switch triggered or polling mode readout
extern const char *xmlDmaMode;          //<  switch between direct dma to host,  or token data buffering in pexor RAM
extern const char *xmlWaitTimeout;    //<  specify kernel waitqueue timeout for trigger and autoread buffers

extern const char *xmlWaitForDataReady;    //<  token request returns only when frontend has data ready
extern const char *xmlUserTriggerClear; //<- aka  "user trigger clear", using feb double buffers
extern const char *xmlStartDAQOnInit; // <- start acquisition directly after init
extern const char *xmlInitDelay;        //<  sleep time after board reset until pexor is ready

extern const char *xmlTrixorConvTime;    //< conversion time of TRIXOR module
extern const char *xmlTrixorFastClearTime;    //< fast clear time of TRIXOR module
extern const char *xmlModuleName;    //< Name of readout module instance
extern const char *xmlModuleThread;    //< Name of readout thread
extern const char *xmlDeviceName;    //< Name of device instance
extern const char *xmlDeviceThread;    //< Name of readout thread

#ifdef PEXOR_USE_WHITERABBIT
extern const char *xmlTLUaddress; //< wishbone address of the white rabbit time latch unit on the PEXARIA (if any);
extern const char *xmlWRSubID; //< user defined subsystem id of this timestamped data stream
#endif



extern const char *nameReadoutAppClass;
extern const char *nameDeviceClass;
extern const char *nameTransportClass;
extern const char *nameReadoutModuleClass;
extern const char *nameInputPool;
extern const char *nameOutputPool;

extern const char *commandStartAcq;
extern const char *commandStopAcq;
extern const char *commandInitAcq;

extern const char *parDeviceDRate;
extern const char *parDaqRunningState;

class Device: public dabc::Device
{

public:

  friend class pex::FrontendBoard;

  Device(const std::string &name, dabc::Command cmd);
  virtual ~Device();

  /** need to start timeout here*/
  void OnThreadAssigned() override;

  /** define timout timer to refresh monitoring stat variables*/
  virtual double ProcessTimeout(double last_diff) override;

  /** here we may insert some actions to the device cleanup methods*/
  virtual bool DestroyByOwnThread() override;

  /** for zero copy DMA: map complete dabc pool for sg DMA of driver
   * NOTE: currently not used for standard daq, deprecated function of previous tests with emulated sg dma.
   * For pexor direct token dma mode, sg emulation is not applicable!*/
  void MapDMAMemoryPool(dabc::MemoryPool *pool);

  /** Request token from current sfp. If synchronous is true, fill output buffer.
   * if mbs formating is enabled, put mbs headers into buffer
   * If synchronous mode false, return before getting dma buffer,
   * needs to call ReceivetokenBuffer afterwards.
   * NOTE: this method is not used for default daq case, kept for user convencience to be called
   * in optional reimplementation of ReadStart/ReadComplete interface*/
  virtual int RequestToken(dabc::Buffer &buf, bool synchronous = true);

  /** Request tokens from all enabled sfp by channelpattern. If synchronous is true, fill output buffer.
   * if mbs formating is enabled, put mbs headers into buffer
   * If synchronous mode false, return before getting dma buffer,
   * needs to call ReceivetokenBuffer afterwards.*/
  virtual int RequestMultiToken(dabc::Buffer &buf, bool synchronous = true, uint16_t trigtype = mbs::tt_Event);

  /** Request tokens in parallel from all enabled sfp by channelpattern and
   *  fill single output buffer by DMA from PEXOR internal memory.
   * if mbs formating is enabled, put mbs headers into buffer
   * This method blocks until buffer buf is complete.
   * data readout is protected in kernel driver agains*/
  virtual int RequestReceiveParallelTokens(dabc::Buffer &buf, uint16_t trigtype = mbs::tt_Event);

  /** Receive token buffer of currently active sfp after asynchronous RequestToken call.
   * NOTE: this method is not used for default daq case, kept for user convencience to be called
   * in optional reimplementation of ReadStart/ReadComplete interface */
  int ReceiveTokenBuffer(dabc::Buffer &buf);

  /** for parallel readout mode: send request to all connected sfp chains in parallel.
   * If synchronous mode false, return before getting dma buffer,
   * needs to call ReceiveAllTokenBuffer afterwards
   * for synchronous mode true, fill one dabc buffer with subevents of different channels*/
  int RequestAllTokens(dabc::Buffer &buf, bool synchronous = true, uint16_t trigtype = mbs::tt_Event);

  /** Receive dma buffers from token request on all channels and copy to dabc buffer buf.
   * Optionally data is formatted with mbs event and subevent headers. MBS trigger type may be specified
   * depending on trixor trigger or triggerless readout.*/
  int ReceiveAllTokenBuffer(dabc::Buffer &buf, uint16_t trigtype = mbs::tt_Event);

  /** For automatic kernelmodule trigger readout mode: wait for next filled buffer.
   * Copy and format it to dabc buffer. Pass actual trigtype back to caller.*/
  int ReceiveAutoTriggerBuffer(dabc::Buffer &buf, uint8_t &trigtype);

  /** For automatic kernelmodule asynchronous triggerless readout mode:
   * request token data from each configured sfp. Receive data via dma from pexor memory from the sfps that are ready.
   * Copy and format them to dabc buffer. Next time this method is called only those sfps are requested again
   * that have already send their data, the others are checked if they have finished the previous token request.
   * This method is intended to be used in "polling for data" mode without a trigger signal*/
  int ReceiveAutoAsyncBuffer(dabc::Buffer &buf);

  /** For automatic kernelmodule asynchronous triggerless readout mode:
   * request token data from each configured sfp. Receive data via dma from pexor memory from the sfps that are ready.
   * Copy and format them to dabc buffer. This method will always return a buffer filled from these requests, or an error.
   * In contrast to ReceiveAutoAsyncBuffer, polling is done inside kernel module and not handled by dabc time out.
   * This method is intended to be used in "polling for data" mode without a trigger signal*/
  int ReceiveAutoAsyncBufferPolling(dabc::Buffer &buf);

  /** For automatic kernelmodule asynchronous triggerless readout mode, third variant:
   *  Just fetch next buffer in kernel module output queue (used buffers).
   *  If nothing in it, try again after dabc timeout.
   * */
  int ReceiveNextAsyncBuffer(dabc::Buffer &buf);

  virtual const char* ClassName() const override
  {
    return "pex::Device";
  }

  unsigned int GetDeviceNumber()
  {
    return fDeviceNum;
  }

  virtual int ExecuteCommand(dabc::Command cmd) override;

  virtual dabc::Transport* CreateTransport(dabc::Command cmd, const dabc::Reference &port) override;

  unsigned int GetReadLength()
  {
    return fReadLength;
  }

  bool IsSynchronousRead()
  {
    return fSynchronousRead;
  }

  bool IsTriggeredRead()
  {
    return fTriggeredRead;
  }
  bool IsAutoReadout()
  {
    return fAutoTriggerRead;
  }

  bool IsAutoAsync()
  {
    return fAutoAsyncRead;
  }

  bool IsMultichannelMode()
  {
    return fMultichannelRequest;
  }
  bool IsDirectDMA()
  {
    return fDirectDMA;
  }

  /** initialize trixor depending on the setup*/
  void InitTrixor();

  /** start data taking with trigger*/
  bool StartAcquisition();

  /** stop data taking with trigger*/
  bool StopAcquisition();

  bool IsAcquisitionRunning()
  {
    return fAqcuisitionRunning;
  }

  /**
   * (re-)initialize the known gosip chains.
   */
  int InitChains();

  /** generic initialization function for daq and frontends.
   * To be overwritten in subclass and callable by command interactively, without shutting down
   * application.*/
  virtual int InitDAQ();

  /**  Forwarded interface for user defined readout.*/
  virtual double Read_Timeout();

  /** Forwarded interface for user defined readout. Here we handle the timeout case for
   * triggerless async readout*/
  virtual unsigned Read_Size();

  /** Forwarded interface for user defined readout:
   * User code may overwrite the default behaviour (gosip token dma)
   * For example, optionally some register settings may be added to buffer contents*/
  virtual unsigned Read_Start(dabc::Buffer &buf);

  /** Forwarded interface for user defined readout:
   * User code may overwrite the default behaviour (gosip token dma)
   * For example, optionally some register settings may be added to buffer contents*/
  virtual unsigned Read_Complete(dabc::Buffer &buf);

  /** interface for user subclass to implement different readout variants depending on the triggertype.
   * The default implementation will issue retry/timeout on start/stop acquisition trigger and
   * a standard token request with direct dma for all other trigger types*/
  virtual int User_Readout(dabc::Buffer &buf, uint8_t trigtype);

  /** JAM 2026: need getter method for the frontend components*/
  pexor::PexorTwo* GetKinpexHandle()
  {
    return fKinpex;
  }

  /** Create and add frontend component of given slave kind. Does nothing if we already have one.*/
  pex::FrontendBoard* CreateFrontendBoard(feb_kind_t kind, const std::string &modulename, dabc::Command cmd);

protected:
  virtual void ObjectCleanup() override;

  /** Check if our readout already has component ofr given kind of gosip slave*/
  bool ExistsFrontendType(feb_kind_t kind);

  /** Add existing frontend board component to the readout*/
  void RegisterFrontendBoard(pex::FrontendBoard *feb);

  /** access the frontend component of kind */
  pex::FrontendBoard* GetFrontendBoard(feb_kind_t kind);

  /** Insert mbs event header at location ptr in external buffer. Eventnumber will define event
   * sequence number, trigger marks current trigger type.
   * ptr is shifted to place after event header afterwards.
   * Return value is handle to event header structure
   * */
  mbs::EventHeader* PutMbsEventHeader(dabc::Pointer &ptr, mbs::EventNumType eventnumber, uint16_t trigger =
      mbs::tt_Event);

  /** Insert mbs subevent header at location ptr in external buffer. Id number subcrate, control nd procid
   * can be defined.ptr is shifted to place after subevent header afterwards.
   * Return value is handle to subevent header structure
   */
  mbs::SubeventHeader* PutMbsSubeventHeader(dabc::Pointer &ptr, int8_t subcrate, int8_t control, int16_t procid);

  /** Insert num padding words at location of ptr and increment ptr.
   * Padding words are formatted in mbs convention like 0xaddNNII*/
  int PutMbsPaddingWords(dabc::Pointer &ptr, uint8_t num);

  /** copy contents of received dma buffer and optionally format for mbs.*/
  int CopyOutputBuffer(pexor::DMA_Buffer *src, dabc::Buffer &dest, uint16_t trigtype = mbs::tt_Event);

  /** copy contents of received dma buffers src to destination buffer and optionally format for mbs.
   * mbs style trigger type can be set for event header.*/
  int CombineTokenBuffers(pexor::DMA_Buffer **src, dabc::Buffer &dest, uint16_t trigtype = mbs::tt_Event);

  /** copy contents of dma token buffers src to subevent field pointed at by coursor; optionally format for mbs
   * returns increment of used size in target buffer. Use sfpnum as subevent identifier*/
  int CopySubevent(pexor::DMA_Buffer *src, dabc::Pointer &cursor, char sfpnum);

  /** switch sfp input index to next enabled one. returns false if no sfp is enabled in setup.
   * For round robin readout of single sfps. Not used for default triggered daq implementation.*/
  bool NextSFP();

  void SetDevInfoParName(const std::string &name)
  {
    fDevInfoName = name;
  }

  void SetInfo(const std::string &info, bool forceinfo = true);

protected:

  pexor::PexorTwo *fKinpex;

  /** number X of pexor device (/dev/pexor-X) */
  unsigned int fDeviceNum;

  /** Name of info parameter for device messages*/
  std::string fDevInfoName;

  /** if true we put mbs headers already into transport buffer.
   * will contain subevents for each connected sfp*/
  bool fMbsFormat;

  /** For mbsformat: if true, use a single mbs subevent containing data of all sfps.
   * Subevent identifier (subcrate, procid, control) is configured by user via paramters.
   * Otherwise (default) buffer will contain one mbs subevent for each sfp, with sfpnumber labelling the
   * subcrate number.*/
  bool fSingleSubevent;

  /** For mbsformat: defines subevent subcrate id for case fSingleSubevent=true*/
  unsigned int fSubeventSubcrate;

  /** For mbsformat: defines subevent procid*/
  unsigned int fSubeventProcid;

  /** For mbsformat: defines subevent control*/
  unsigned int fSubeventControl;






  /** wait timeout in seconds for kernel receive queues*/
  int fWaitTimeout;

  /** flag for aquisition running state*/
  bool fAqcuisitionRunning;

  /** if true, use synchrounous readout of token and dma. otherwise, decouple token request
   * from DMA buffer receiving*/
  bool fSynchronousRead;

  /** if true, request data only when trigger interrupt was received.
   * Otherwise request data immediately (polling mode)*/
  bool fTriggeredRead;

  /** mode how token data is put to host buffers:
   * if true, dma of each channel's token data will be written directly to receiving buffer
   * if false, token data will be stored in pexor memory first and then transferred to host buffers separately
   * this mode is evaluated in kernel module*/
  bool fDirectDMA;

  /** if true, data is requested by frontends with sfp channel pattern
   * and driver-intrinsic filling of dma buffer from all channels.
   * Otherwise, use sequential round-robin token request with separate dma buffers combined in application.
   * The latter may also separate different mbs subvevents for each sfp.*/
  bool fMultichannelRequest;

  /** if true, data readout will be done automatically in driver kernel module.
   * The already filled token buffer is fetched for each incoming trigger  */
  bool fAutoTriggerRead;

  /** if true, data readout will be done automatically in driver kernel module.
   * The already filled token buffer is fetched without trigger from asynchronously running sfp chains1  */
  bool fAutoAsyncRead;

  /** flag to switch on memory speed measurements without acquisition*/
  bool fMemoryTest;

  /** switch to skip daq request*/
  bool fSkipRequest;

  /** zero copy DMA into dabc buffers*/
  bool fZeroCopyMode;

  /** array indicating which sfps are connected for readout*/
  bool fEnabledSFP[PEX_NUMSFP];

  /** array indicating which sfps have already been requested for data token.
   * This is used for triggerless asynchronous readout.*/
  bool fRequestedSFP[PEX_NUMSFP];

  /** array indicating number of slaves in chain at each sfp*/
  unsigned int fNumSlavesSFP[PEX_NUMSFP];

  /** specifies which feb device is at what slave position */
  std::vector<pex::feb_kind_t> fSlaveTypes[PEX_NUMSFP];

  /** specifies which feb device is at what slave position */
  std::vector<pex::FrontendBoard*> fFrontendBoards;

  /** id number of current exploder double buffer to request (0,1)*/
  int fDoubleBufID[PEX_NUMSFP];

  /** if this flag is true, token request will wait until front end provides data ready state*/
  bool fWaitForDataReady;

  /** if true, acquisition is started at initialization. otherwise, explicit start DAQ command is required */
  bool fStartDAQOnInit;

  /** index of currently read sfp. Used for the simple round robin readout into one transport*/
  unsigned char fCurrentSFP;

  /** actual payload length of read buffer*/
  unsigned int fReadLength;

  /** trixor conversion time window (100ns units)*/
  unsigned short fTrixConvTime;

  /** trixor fast clear time window (100ns units)*/
  unsigned short fTrixFClearTime;

  /*** counter for transport threads, for unique naming.*/
  static unsigned int fgThreadnum;

  /** set true if initialization of board is successful*/
  bool fInitDone;

  /** flag for triggerless read out to issue a timeout on next retry */
  bool fHasData;


  /** clear trigger before requesting data. this is the MBS "user trigger clear" mode */
  bool fEarlyTriggerClear;


  /** white rabbit subsystem id. arbitratily user defined, for later data stream stiching */
   int fWRSubsystem;

   /** wishbone address of trigger time latch unit on the white rabbit receiver (pexaria). depends on WR firmware version*/
   unsigned long fTLUAddress;

  /** Event number since device init*/
  unsigned int fNumEvents;

  /** Mutex to protect flags for asynchronous requests*/
  dabc::Mutex fRequestMutex;

};

/** most generic pexor readout implementation.
 * Required to settle initialization flag*/
class GenericDevice: public pex::Device
{
public:

  GenericDevice(const std::string &name, dabc::Command cmd);

};

}    // namespace

#endif
