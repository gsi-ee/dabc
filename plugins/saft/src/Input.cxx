
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

#include "dabc/Pointer.h"
#include "dabc/DataTransport.h"

#include "mbs/MbsTypeDefs.h"
#include "mbs/Iterator.h"
//#include "mbs/SlowControlData.h"

#include "hadaq/Iterator.h"

#include <time.h>
#include "dabc/Exception.h"


saftdabc::Input::Input (const saftdabc::DeviceRef &owner) :
    dabc::DataInput (),  fQueueMutex(false), fWaitingForCallback(false), fOverflowCount(0), fLastOverflowCount(0), fDevice(owner), fTimeout (1e-2),
    fUseCallbackMode(false), fSubeventId (8),fEventNumber (0),fVerbose(false),fSingleEvents(false)
{
  DOUT0("saftdabc::Input CTOR");
  ClearEventQueue ();
  ResetDescriptors ();
}

saftdabc::Input::~Input ()
{
  DOUT0("saftdabc::Input DTOR");
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
  fSingleEvents= wrk.Cfg (saftdabc::xmlSaftSingleEvent, cmd).AsBool (fSingleEvents);
  fVerbose= wrk.Cfg (saftdabc::xmlSaftVerbose, cmd).AsBool (fVerbose);
  fInput_Names = wrk.Cfg (saftdabc::xmlInputs, cmd).AsStrVect ();
  fSnoop_Ids = wrk.Cfg (saftdabc::xmlEventIds, cmd).AsUIntVect ();
  fSnoop_Masks = wrk.Cfg (saftdabc::xmlMasks, cmd).AsUIntVect ();
  fSnoop_Offsets = wrk.Cfg (saftdabc::xmlOffsets, cmd).AsUIntVect ();
  fSnoop_Flags = wrk.Cfg (saftdabc::xmlAcceptFlags, cmd).AsUIntVect ();

  std::string format=wrk.Cfg (saftdabc::xmlEventFormat, cmd).AsStr("MBS");

  if(format.compare(std::string("RAW"))==0)
      fEventFormat=saft_Format_Raw;
  else if (format.compare(std::string("MBS"))==0)
      fEventFormat=saft_Format_Mbs;
  else if (format.compare(std::string("HADAQ"))==0)
      fEventFormat=saft_Format_Hadaq;
  else
    fEventFormat=saft_Format_Mbs;


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
      "saftdabc::Input  %s - Timeout = %e s, callbackmode:%s, format:%s, subevtid:%d, single event:%s, %d hardware inputs, %d snoop event ids, verbose=%s ",
      wrk.GetName(), fTimeout, DBOOL(fUseCallbackMode), format.c_str(), fSubeventId, DBOOL(fSingleEvents), fInput_Names.size(), fSnoop_Ids.size(), DBOOL(fVerbose));


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
  //fDevice.ClearConditions();
  // TODO: method that just removes conditions belonging to this device?

  ClearEventQueue ();
  ResetDescriptors ();
  DOUT1("Input::Close - Overflow counts:%lu, previous:%lu",fOverflowCount, fLastOverflowCount);
  return true;
}



void saftdabc::Input::ClearEventQueue ()
 {
   dabc::LockGuard gard (fQueueMutex);
   while(!fTimingEventQueue.empty()) fTimingEventQueue.pop();
 }


unsigned saftdabc::Input::Read_Size ()
{
  try
{
  DOUT3("saftdabc::Input::Read_Size...");
#ifdef  DABC_SAFT_USE_2_0
   saftlib::wait_for_signal();
#endif
  
  if(fUseCallbackMode) return dabc::di_DfltBufSize;

  // here may do forwarding to callback or poll with timeout if no data in queues
  dabc::LockGuard gard (fQueueMutex);
  bool nodata=fTimingEventQueue.empty();
  if(fVerbose)
    {
      if(nodata) DOUT3("saftdabc::Input::Read_Size returns with timeout!");
    }
  return (nodata ? dabc::di_RepeatTimeOut: dabc::di_DfltBufSize);

//  if(fUseCallbackMode && nodata) fWaitingForCallback=true;
//  return (nodata ? (fUseCallbackMode ? dabc::di_CallBack : dabc::di_RepeatTimeOut): dabc::di_DfltBufSize);


}    // try
  catch (std::exception& ex)    // also handles std::exception
  {
    EOUT(" saftdabc::Input::Read_Size with std exception %s ", ex.what());
    return dabc::di_Error;
  }
#ifdef  DABC_SAFT_USE_2_0 
 catch (const saftbus::Error& error)
 {
   /* Catch error(s) */
   EOUT("saftdabc::Input::Read_Size with SAFTbus error %s ", error.what().c_str());
   return false;

 }
#endif  
  return dabc::di_Error;
}

unsigned saftdabc::Input::Read_Start (dabc::Buffer& buf)
{
  try
  {
    dabc::LockGuard gard (fQueueMutex);
    if (fTimingEventQueue.empty ())
    {
      if (fUseCallbackMode)
      {
        fWaitingForCallback = true;
        DOUT1("saftdabc::Input::Read_Start sets fWaitingForCallback=%s", DBOOL(fWaitingForCallback));
        return dabc::di_CallBack;
      }
      else
      {
        EOUT("saftdabc::Input::Read_Start() with empty queue in polling mode!");
        return dabc::di_Error;
      }

    }
    return dabc::di_Ok;

  }    // try
  catch (std::exception& ex)    // also handles std::exception
  {
    EOUT(" saftdabc::Input::Read_Start with std exception %s ", ex.what());
    return dabc::di_Error;
  }

  return dabc::di_Error;
}


unsigned saftdabc::Input::Read_Complete (dabc::Buffer& buf)
{

  try
  {
    DOUT5("saftdabc::Input::Read_Complete...");
    switch (fEventFormat)
    {
      case saft_Format_Raw:
        return (Write_Raw (buf));
        break;
      case saft_Format_Hadaq:
        return (Write_Hadaq (buf));
        break;
      case saft_Format_Mbs:
      default:
        return (Write_Mbs (buf));
        break;
    };
    return dabc::di_Error;
  }    // try

  
  #ifdef DABC_SAFT_USE_2_0
catch (const saftbus::Error& error)
  {
    EOUT("saftdabc::Input::Read_Complete with saftbus error %s", error.what().c_str());
    return dabc::di_Error;
  }
#else
  catch (const Glib::Error& error)
  {
    EOUT("saftdabc::Input::Read_Complete with Glib error %s", error.what().c_str());
    return dabc::di_Error;
  }
#endif

  catch (std::exception& ex)    // also handles std::exception
  {
    EOUT(" saftdabc::Input::Read_Complete with std exception %s ", ex.what());

    return dabc::di_Error;
  }

}


 unsigned saftdabc::Input::Write_Mbs (dabc::Buffer& buf)
 {
   DOUT5("saftdabc::Input::Write_Mbs...");
   dabc::LockGuard gard (fQueueMutex); // protect against saftlib callback <-Device thread
   mbs::WriteIterator iter (buf);
    if(!iter.IsPlaceForEvent(sizeof(Timing_Event), true))
      {
        // NEVER COME HERE for correctly configured pool?
        EOUT("saftdabc::Input::Write_Mbs: buffer too small for a single Timing Event!");
        return dabc::di_SkipBuffer;
      }
    iter.NewEvent (fEventNumber++);
    iter.NewSubevent2 (fSubeventId);
    iter.evnt()->iTrigger=SAFT_DABC_TRIGTYPE;
    if(fVerbose) DOUT0("saftdabc::Input::Write_Mbs begins new event %d, mutex=0x%x, instance=0x%x",
        fEventNumber, &fQueueMutex, (unsigned long) this);
    //unsigned size = 0;
    unsigned ec=0;
    while (!fTimingEventQueue.empty ())
    {
      if(fSingleEvents && (ec==1))
        {
          if(fVerbose){
            DOUT0("saftdabc::Input::Write_Mbs has filled single event, closing container.");
          }
          break;
        }


      ec++;
      if (!iter.IsPlaceForRawData(sizeof(Timing_Event)))
          {
              // following check does not work, since rawdata pointer is updated in FinishSubEvent only ?
              //uint32_t rest= iter.maxrawdatasize() - (uint32_t) ((ulong) iter.rawdata() - (ulong) iter.subevnt());
              // no public access in mbs interator to remaining size. workaround:
              if(fVerbose)
              {
              uint32_t rest= iter.maxrawdatasize() - ec * sizeof(Timing_Event);
              DOUT0("saftdabc::Input::Write_Mbs - buffer remaining size is %d bytes too small for next timing event!",
                  rest);
              //fVerbose=true; // switch on full debug for the following things
              }
              break;
          }


      Timing_Event theEvent = fTimingEventQueue.front ();
      if(fVerbose)
      {
        DOUT0("saftdabc::Input::Write_Mbswith queue length %u",fTimingEventQueue.size());
        char txt[1024];
        theEvent.InfoMessage(txt,1024);
        DOUT0("saftdabc::Input::Write_Mbs sees event: %s",txt);
      }
      unsigned len = sizeof(Timing_Event);
      if (!iter.AddRawData (&theEvent, len)){

        DOUT0("saftdabc::Input::Write_Mbs could not add data of len=%ld to subevent of maxraw=%ld, queuelen=%u",
            len, iter.maxrawdatasize(), fTimingEventQueue.size());
        break;
      }
      fTimingEventQueue.pop ();

      // Disable event statistics, not usable with blocked device thread
      //fDevice.AddEventStatistics(1);
    }
    iter.FinishSubEvent();
    iter.FinishEvent ();
    buf = iter.Close ();
  //
    //DOUT3("Read buf size = %u", buf.GetTotalSize());
    if(fVerbose) DOUT0("saftdabc::Input::Write_Mbs closes new event %d, read buffer size=%u", fEventNumber, buf.GetTotalSize());
    return dabc::di_Ok;
 }




 unsigned saftdabc::Input::Write_Hadaq (dabc::Buffer& buf)
 {
   DOUT5("saftdabc::Input::Write_Hadaq...");
     dabc::LockGuard gard (fQueueMutex); // protect against saftlib callback <-Device thread
     hadaq::WriteIterator iter (buf);

     // probably this is redundant with the following check in NewEvent:
     if(!iter.IsPlaceForEvent(sizeof(Timing_Event)))
        {
          // NEVER COME HERE for correctly configured pool?
          EOUT("saftdabc::Input::Write_Hadaq: buffer too small for a single Timing Event!");
          return dabc::di_SkipBuffer;
        }

      // we misuse the subevent id as run number here. should be ignored by combiner module anyway.
      if(!iter.NewEvent (fEventNumber++, fSubeventId, sizeof(hadaq::RawSubevent)+ sizeof(Timing_Event)))
        {
        // NEVER COME HERE for correctly configured pool?
        EOUT("saftdabc::Input::Write_Hadaq: NewEvent fails to hold single Timing Event!");
        return dabc::di_SkipBuffer;
        }

      iter.NewSubevent(sizeof(Timing_Event), SAFT_DABC_TRIGTYPE);
      char* rawbase= (char*) iter.rawdata();
      char* cursor=rawbase;


      if(fVerbose) DOUT0("saftdabc::Input::Write_Hadaq begins new event %d, mutex=0x%x, instance=0x%x",
          fEventNumber, &fQueueMutex, (unsigned long) this);
      unsigned rawsize = sizeof(hadaq::RawSubevent); // =0 account header in rawsize for hadtu?;
      unsigned ec=0;
      while (!fTimingEventQueue.empty ())
      {
        if(fSingleEvents && (ec==1))
          {
            if(fVerbose){
              DOUT0("saftdabc::Input::Write_Hadaq has filled single event, closing container.");
            }
            break;
          }


        ec++;
        if((cursor - rawbase + sizeof(Timing_Event) ) > iter.maxrawdatasize())
            {
                if(fVerbose)
                {
                uint32_t rest= iter.maxrawdatasize() - ec * sizeof(Timing_Event);
                DOUT0("saftdabc::Input::Write_Hadaq - buffer remaining size is %d bytes too small for next timing event!",
                    rest);
                //fVerbose=true; // switch on full debug for the following things
                }
                break;
            }


        Timing_Event theEvent = fTimingEventQueue.front ();
        if(fVerbose)
        {
          DOUT0("saftdabc::Input::Write_Hadaq with queue length %u",fTimingEventQueue.size());
          char txt[1024];
          theEvent.InfoMessage(txt,1024);
          DOUT0("saftdabc::Input::Write_Hadaq sees event: %s",txt);
        }

        unsigned len = sizeof(Timing_Event);
        memcpy(cursor, &theEvent,len);
        rawsize+=len;
        cursor+=len;

        fTimingEventQueue.pop ();

        // Disable event statistics, not usable with blocked device thread
        //fDevice.AddEventStatistics(1);
      } // while timing queue is not empty

      iter.FinishSubEvent(rawsize);
      iter.FinishEvent ();
      buf = iter.Close ();
      if(fVerbose) DOUT0("saftdabc::Input::Write_Hadaq closes new event %d, read buffer size=%u", fEventNumber, buf.GetTotalSize());
      return dabc::di_Ok;
 }

 unsigned saftdabc::Input::Write_Raw (dabc::Buffer& buf)
 {
   // dummy:
   while (!fTimingEventQueue.empty ())
     fTimingEventQueue.pop ();
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
  
  
  #ifdef DABC_SAFT_USE_2_0
catch (const saftbus::Error& error)
  {
    EOUT("SAFTbus error %s in SetupConditions", error.what().c_str());
  }
#else  
  catch (const Glib::Error& error)
  {
    EOUT("Glib error %s in SetupConditions", error.what().c_str());
    return false;
  }
#endif

  catch (std::exception& ex)
   {
     EOUT("std exception %s in SetupConditions", ex.what());
     return false;
   }

  return true;
}

#ifdef DABC_SAFT_USE_2_0
    void saftdabc::Input::EventHandler (uint64_t event, uint64_t param, uint64_t deadline, uint64_t executed, uint16_t flags)
#else
void saftdabc::Input::EventHandler (guint64 event, guint64 param, guint64 deadline, guint64 executed, guint16 flags)
#endif
{

  DOUT5("saftdabc::Input::EventHandler...");
  /* This is the signalhandler that treats condition events from saftlib*/
  try
  {
    dabc::LockGuard gard (fQueueMutex);// protect against Transport thread
    std::string description = fDevice.GetInputDescription (event);
    // here check if we have input condition, then substract the offset:
    if(description.compare(std::string(NON_IO_CONDITION_LABEL))!=0)
      {
        deadline -= IO_CONDITION_OFFSET; // like in saft-io-ctl
      }
 

    if (fVerbose)
    {
      DOUT0("saftdabc::Input::EventHandler holds mutex 0x%x, instance=0x%x",(unsigned long) &fQueueMutex, (unsigned long) this);
      DOUT0(
          "Input::EventHandler sees event=0x%lx, param=0x%lx , deadline=0x%lx, executed=0x%lx, flags=0x%x, description:%s", event, param, deadline, executed, flags, description.c_str());
      DOUT0("Formatted Date:%s", saftdabc::tr_formatDate(deadline, PMODE_VERBOSE).c_str());
      DOUT0("Eventid:%s", saftdabc::tr_formatActionEvent(event,PMODE_VERBOSE).c_str());
/// Disable set info, since info parameter can not be monitored due to blocked device thread in glib mainloop
//      fDevice.SetInfo (
//        dabc::format ("Received %s at %s!", saftdabc::tr_formatActionEvent (event, PMODE_VERBOSE).c_str (),
//            saftdabc::tr_formatDate (deadline, PMODE_VERBOSE).c_str ()));
    }




    uint64_t doverflow= fOverflowCount-fLastOverflowCount;
     if (fVerbose)
    {
      DOUT0("saftdabc::Input::EventHandler sees overflowcount=%lu, previous=%lu, delta=%lu",fOverflowCount, fLastOverflowCount, doverflow);
    }

    fLastOverflowCount=fOverflowCount;
    fTimingEventQueue.push (Timing_Event (event, param, deadline, executed, flags, doverflow, description.c_str ()));
    DOUT3("TimingEventQueue is filled with %d elements", fTimingEventQueue.size());
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

        unsigned datasize = dabc::di_DfltBufSize;    // always use full buffer anyway.
        tr->Read_CallBack (datasize);
        fWaitingForCallback = false;
        DOUT1("saftdabc::Input::EventHandler after Read_CallBack, fWaitingForCallback=%s", DBOOL(fWaitingForCallback));
      }
      else
      {
        EOUT("Worker %p is not an InputTransport, can not Read_CallBack", fTransport());
      }

    }    //  if (fUseCallbackMode &

  }    // try
  
#ifdef DABC_SAFT_USE_2_0
catch (const saftbus::Error& error)
  {
    EOUT("saftdabc::Input::EventHandler with saftbus error %s", error.what().c_str());
  }
#else
catch (const Glib::Error& error)
  {
    EOUT("saftdabc::Input::EventHandler with Glib error %s", error.what().c_str());
  }
#endif  
  
  catch (std::exception& ex)
  {
    EOUT(" saftdabc::Input::EventHandler with std exception %s ", ex.what());
  }

}




#ifdef DABC_SAFT_USE_2_0
void saftdabc::Input::OverflowHandler(uint64_t count)
#else
void saftdabc::Input::OverflowHandler(guint64 count)
#endif
{
  DOUT1("saftdabc::Input::OverflowHandler with counts=%lu",count);
  dabc::LockGuard gard(fQueueMutex); // protect also the counter?
  fOverflowCount=count;
  DOUT3("saftdabc::Input::OverflowHandler sees overflowcount=%lu, previous=%lu",fOverflowCount, fLastOverflowCount);
}


