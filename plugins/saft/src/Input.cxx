
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
    dabc::DataInput (), fDevice(owner), fTimeout (1e-2), fUseCallbackMode(false), fSubeventId (8),fEventNumber (0)
{
  DOUT0("saftdabc::Input CTOR");
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
  DOUT0("saftdabc::Input::Read_Init...");
  if (!dabc::DataInput::Read_Init (wrk, cmd))
    return false;
  fTimeout = wrk.Cfg (saftdabc::xmlTimeout, cmd).AsDouble (fTimeout);
  fUseCallbackMode=wrk.Cfg (saftdabc::xmlCallbackMode, cmd).AsBool (fUseCallbackMode);
  fSubeventId = wrk.Cfg (saftdabc::xmlSaftSubeventId, cmd).AsInt (fSubeventId);
  fInput_Names = wrk.Cfg (saftdabc::xmlInputs, cmd).AsStrVect ();
  fSnoop_Ids = wrk.Cfg (saftdabc::xmlEventIds, cmd).AsUIntVect ();
  fSnoop_Masks = wrk.Cfg (saftdabc::xmlMasks, cmd).AsUIntVect ();
  fSnoop_Offsets = wrk.Cfg (saftdabc::xmlOffsets, cmd).AsUIntVect ();
  fSnoop_Flags = wrk.Cfg (saftdabc::xmlAcceptFlags, cmd).AsUIntVect ();

  if (fSnoop_Ids.size () != fSnoop_Masks.size ())
  {
    DOUT1(
        "Warning: saftdabc::Input  %s - number of snoop event ids %d  and number of masks %d differ!!!", wrk.GetName(), fSnoop_Ids.size(), fSnoop_Masks.size());
  }


  DOUT1(
      "saftdabc::Input  %s - Timeout = %e s, subevtid:%d, %d hardware inputs, %d snoop event ids ", wrk.GetName(), fTimeout, fSubeventId, fInput_Names.size(), fSnoop_Ids.size());


  // There set up the software conditions
  bool rev=SetupConditions();
  if(!rev)  {
    EOUT("Could not set up all conditions!");
    return false;
  }

  // TODO: test different variants of mainloop invokation!
  // if we had only one input, we could start the mainloop here:
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
  // here may do forwarding to callback or poll with timeout if no data in queues
  return (fTimingEventQueue.empty() ? (fUseCallbackMode ? dabc::di_CallBack : dabc::di_RepeatTimeOut): dabc::di_DfltBufSize);
}

unsigned saftdabc::Input::Read_Complete (dabc::Buffer& buf)
{
  DOUT0("saftdabc::Input::Read_Complete...");
// TODO: switch between mbs and hadaq output formats!


//
mbs::WriteIterator iter (buf);
// may specify special trigger type here?
//iter.evnt()->iTrigger=42;
iter.NewEvent(fEventNumber++ );
iter.NewSubevent2 (fSubeventId);
unsigned size =0;
while(!fTimingEventQueue.empty())
{
  Timing_Event theEvent=fTimingEventQueue.front();
  unsigned len=sizeof(Timing_Event);
  if(!iter.AddRawData(&theEvent, len)) break;
  size+=len;
  fTimingEventQueue.pop();
}
iter.FinishSubEvent (size);
iter.FinishEvent ();
//
buf = iter.Close ();
//
DOUT0("Read buf size = %u", buf.GetTotalSize());

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
//TODO: following code does crash!
//    for (std::vector<uint64_t>::iterator snit = fSnoop_Ids.begin (), mit = fSnoop_Masks.begin (), offit = fSnoop_Offsets
//        .begin (), flit = fSnoop_Flags.begin ();
//        snit != fSnoop_Ids.end (), mit != fSnoop_Masks.end (), offit != fSnoop_Offsets.end (), flit
//            != fSnoop_Flags.end (); ++snit, ++mit, ++offit, ++flit)
//    {
//      // TODO: may we treat the situation that snoop id is given, but masks etc is not defined as wildcard case?
//      bool rev = fDevice.RegisterEventCondition (this, *snit, *mit, *offit, (unsigned char) *flit);
//      if (!rev)
//        errcnt++;
//
//    }
//    DOUT0("SetupConditions with %d errors for snoop conditions.", errcnt);
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

  DOUT0("saftdabc::Input::EventHandler...");
  /* This is the signalhandler that treats condition events from saftlib*/

  std::string description=fDevice.GetInputDescription(event);
  DOUT0("Input::EventHandler sees event=0x%lx, param=0x%lx , deadline=0x%lx, executed=0x%lx, flags=0x%x, description:%s",
      event, param , deadline, executed, flags, description.c_str());

  fTimingEventQueue.push(Timing_Event (event, param , deadline, executed, flags, description.c_str()));
  DOUT0("TimingEventQueue is filled with %d elements", fTimingEventQueue.size());



  if (fUseCallbackMode)
  {
    // issue callback to dabc transport here:
    dabc::InputTransport* tr = dynamic_cast<dabc::InputTransport*> (fTransport ());
    if (tr != 0)
    {
      tr->Read_CallBack (dabc::di_Ok);
    }
    else
    {
      EOUT("Worker %p is not an InputTransport, can not Read_CallBack", fTransport());
    }

  }


}
