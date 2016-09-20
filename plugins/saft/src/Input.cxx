
/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "saftdabc/Input.h"
#include "saftdabc/Definitions.h"

//#include "dabc/Pointer.h"
//#include "dabc/timing.h"
//#include "dabc/Manager.h"
#include "dabc/DataTransport.h"

#include "mbs/MbsTypeDefs.h"
#include "mbs/Iterator.h"
#include "mbs/SlowControlData.h"


//#include "saftdabc/Device.h"



#include <time.h>



saftdabc::Input::Input (const saftdabc::DeviceRef &owner) :
    dabc::DataInput (),  fQueueMutex(true), fWaitingForCallback(false), fDevice(owner), fTimeout (1e-2), fUseCallbackMode(false), fSubeventId (8),fEventNumber (0),fVerbose(false)
{
  DOUT3("saftdabc::Input CTOR");
  ClearEventQueue ();
  ResetDescriptors ();
}

saftdabc::Input::~Input ()
{
  Close ();
}


void saftdabc::Input::SetTransportRef(dabc::InputTransport* trans)
   {
     fTransport.SetObject(trans);
   }

bool saftdabc::Input::Read_Init (const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
  DOUT3("saftdabc::Input::Read_Init...");
  if (!dabc::DataInput::Read_Init (wrk, cmd))
    return false;
  fTimeout = wrk.Cfg (saftdabc::xmlTimeout, cmd).AsDouble (fTimeout);
  fUseCallbackMode=wrk.Cfg (saftdabc::xmlCallbackMode, cmd).AsBool (fUseCallbackMode);
  fSubeventId = wrk.Cfg (saftdabc::xmlSaftSubeventId, cmd).AsInt (fSubeventId);
  fVerbose= wrk.Cfg (saftdabc::xmlSaftVerbose, cmd).AsBool (fVerbose);
  fInput_Names = wrk.Cfg (saftdabc::xmlInputs, cmd).AsStrVect ();
  fSnoop_Ids = wrk.Cfg (saftdabc::xmlEventIds, cmd).AsUIntVect ();
  fSnoop_Masks = wrk.Cfg (saftdabc::xmlMasks, cmd).AsUIntVect ();
  fSnoop_Offsets = wrk.Cfg (saftdabc::xmlOffsets, cmd).AsUIntVect ();
  fSnoop_Flags = wrk.Cfg (saftdabc::xmlAcceptFlags, cmd).AsUIntVect ();


  if (! (fSnoop_Ids.size () == fSnoop_Masks.size ()) &&
        (fSnoop_Ids.size () == fSnoop_Offsets.size ()) &&
        (fSnoop_Ids.size () == fSnoop_Flags.size ()))
  {
    EOUT(
        "saftdabc::Input  %s - numbers for snoop event ids %d, masks %d, offsets %d, flags %d differ!!! - Please check configuration.", wrk.GetName(),
          fSnoop_Ids.size(), fSnoop_Masks.size(), fSnoop_Offsets.size(), fSnoop_Flags.size());
    return false;
  }


  DOUT1(
      "saftdabc::Input  %s - Timeout = %e s, callbackmode:%s, subevtid:%d, %d hardware inputs, %d snoop event ids, verbose=%s ",
      wrk.GetName(), fTimeout, DBOOL(fUseCallbackMode), fSubeventId, fInput_Names.size(), fSnoop_Ids.size(), DBOOL(fVerbose));


  // There set up the software conditions
  bool rev=SetupConditions();
  if(!rev)  {
    EOUT("Could not set up all conditions!");
    return false;
  }

  fDevice.SetInfo(dabc::format("Input %s Read_Init is done!",wrk.GetName()));
  // TODO: test different variants of mainloop invokation!
  // if we had only one input, we could start the mainloop here:

  // TEST: uncomment next line for delayed starting of mainloop from remote
  fDevice.Submit(dabc::Command(saftdabc::commandRunMainloop));

  return rev;
}



bool saftdabc::Input::Close ()
{
  fDevice.ClearConditions();
  ClearEventQueue ();
  ResetDescriptors ();
  DOUT1("Input::Close");
  return true;
}

unsigned saftdabc::Input::Read_Size ()
{
  DOUT3("saftdabc::Input::Read_Size...");
  if(fUseCallbackMode) return dabc::di_DfltBufSize;

  // here may do forwarding to callback or poll with timeout if no data in queues
  dabc::LockGuard (fQueueMutex, true); // protect against saftlib callback <-Device thread
  bool nodata=fTimingEventQueue.empty();
  return (nodata ? dabc::di_RepeatTimeOut: dabc::di_DfltBufSize);



//  if(fUseCallbackMode && nodata) fWaitingForCallback=true;
//  return (nodata ? (fUseCallbackMode ? dabc::di_CallBack : dabc::di_RepeatTimeOut): dabc::di_DfltBufSize);
}

unsigned saftdabc::Input::Read_Start (dabc::Buffer& buf)
{
  dabc::LockGuard (fQueueMutex, true); // protect against saftlib callback <-Device thread
  if(fTimingEventQueue.empty())
  {
    if(fUseCallbackMode)
    {
        fWaitingForCallback=true;
        DOUT1("saftdabc::Input::Read_Start sets fWaitingForCallback=%s",DBOOL(fWaitingForCallback));
        return dabc::di_CallBack;
     }
    else
    {
      EOUT("saftdabc::Input::Read_Start() with empty queue in polling mode!");
      return dabc::di_Error;
    }

    }
  return dabc::di_Ok;
}



unsigned saftdabc::Input::Read_Complete (dabc::Buffer& buf)
{
  DOUT5("saftdabc::Input::Read_Complete...");

  dabc::LockGuard (fQueueMutex, true);    // protect against saftlib callback <-Device thread

  DOUT3("saftdabc::Input::Read_Read_Complete with fWaitingForCallback=%s",DBOOL(fWaitingForCallback));

  // TODO: switch between mbs and hadaq output formats!
  mbs::WriteIterator iter (buf);
// may specify special trigger type here?
//iter.evnt()->iTrigger=42;
  iter.NewEvent (fEventNumber++);
  iter.NewSubevent2 (fSubeventId);
  unsigned size = 0;
  while (!fTimingEventQueue.empty ())
  {
    Timing_Event theEvent = fTimingEventQueue.front ();
    unsigned len = sizeof(Timing_Event);
    if (!iter.AddRawData (&theEvent, len))
      break;
    size += len;
    fTimingEventQueue.pop ();
    fDevice.AddEventStatistics(1);
  }
  iter.FinishSubEvent (size);
  iter.FinishEvent ();
  buf = iter.Close ();
//
  DOUT3("Read buf size = %u", buf.GetTotalSize());

  return dabc::di_Ok;
}

bool saftdabc::Input::SetupConditions ()
{
  // PART I: treat the input conditions
  // this is mostly stolen from saft-io-ctl.cpp
  try
  {
    unsigned errcnt (0);

    // just loop over all DABC configured inputs, checking is done in device class
    for (std::vector<std::string>::iterator confit = fInput_Names.begin (); confit != fInput_Names.end (); ++confit)
    {
      bool rev = fDevice.RegisterInputCondition (this, *confit);
      if (!rev)
        errcnt++;
    }    // for

    DOUT0("SetupConditions with %d errors for input conditions.", errcnt);
    if (errcnt > 0)
      return false;

    // PART II: treat any WR events on the line:
    for (std::vector<uint64_t>::iterator snit = fSnoop_Ids.begin (), mit = fSnoop_Masks.begin (), offit = fSnoop_Offsets
        .begin (), flit = fSnoop_Flags.begin ();
        snit != fSnoop_Ids.end (), mit != fSnoop_Masks.end (), offit != fSnoop_Offsets.end (), flit
            != fSnoop_Flags.end (); ++snit, ++mit, ++offit, ++flit)
    {
      // TODO: may we treat the situation that snoop id is given, but masks etc is not defined as wildcard case?
      bool rev = fDevice.RegisterEventCondition (this, *snit, *mit, *offit, (unsigned char) *flit);
      if (!rev)
        errcnt++;

    }
    DOUT0("SetupConditions with %d errors for snoop conditions.", errcnt);
    if (errcnt > 0)
      return false;
    return true;

  }    // end try
  catch (const Glib::Error& error)
  {
    EOUT("Glib error %s in SetupConditions", error.what().c_str());
    return false;
  }

  return true;
}


void saftdabc::Input::EventHandler (guint64 event, guint64 param, guint64 deadline, guint64 executed, guint16 flags)
{

  DOUT3("saftdabc::Input::EventHandler...");
  /* This is the signalhandler that treats condition events from saftlib*/
  dabc::LockGuard(fQueueMutex,true); // protect against Transport thread

 std::string description=fDevice.GetInputDescription(event);
if(fVerbose)
{
  DOUT0("Input::EventHandler sees event=0x%lx, param=0x%lx , deadline=0x%lx, executed=0x%lx, flags=0x%x, description:%s",
      event, param , deadline, executed, flags, description.c_str());
  DOUT0("Formatted Date:%s",
      saftdabc::tr_formatDate(executed, PMODE_VERBOSE).c_str());
  DOUT0("Eventid:%s",
       saftdabc::tr_formatActionEvent(event,PMODE_VERBOSE).c_str());
}
  fTimingEventQueue.push(Timing_Event (event, param , deadline, executed, flags, description.c_str()));
  DOUT3("TimingEventQueue is filled with %d elements", fTimingEventQueue.size());

  fDevice.SetInfo(dabc::format("Received %s at %s!",
      saftdabc::tr_formatActionEvent(event,PMODE_VERBOSE).c_str(),
      saftdabc::tr_formatDate(executed, PMODE_VERBOSE).c_str()));

  DOUT3("saftdabc::Input::EventHandler with fWaitingForCallback=%s",DBOOL(fWaitingForCallback));
  if (fUseCallbackMode && fWaitingForCallback)
  {
      // do not call Read_CallBack again during transport running
    // issue callback to dabc transport here:
    dabc::InputTransport* tr = dynamic_cast<dabc::InputTransport*> (fTransport ());
    if (tr != 0)
    {
      //unsigned datasize=fTimingEventQueue.size()* sizeof(saftdabc::Timing_Event) + sizeof(mbs::EventHeader) + sizeof(mbs::SubeventHeader);
      // todo: adjust if using different output data format, e.g. hadtu

      unsigned datasize=dabc::di_DfltBufSize; // always use full buffer anyway.
      tr->Read_CallBack (datasize);
      fWaitingForCallback=false;
      DOUT1("saftdabc::Input::EventHandler after Read_CallBack, fWaitingForCallback=%s",DBOOL(fWaitingForCallback));
    }
    else
    {
      EOUT("Worker %p is not an InputTransport, can not Read_CallBack", fTransport());
    }

  }


}

