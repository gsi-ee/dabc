// $Id$

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

#include "dabc/Pointer.h"
#include "user/Input.h"
#include <unistd.h>
#include <cmath>

#include "dabc/Port.h"
#include "dabc/logging.h"

#include "mbs/Iterator.h"

user::Input::Input (dabc::Url& url) :
    dabc::DataInput (), fURL (url), fNumEvents(0)
// provide input and output buffers
{
  DOUT2 ("Created new user::Input\n");
  fReadLength = url.GetOptionInt("bufsize", 65536);

  // for mbs header format configuration:
  fSubeventSubcrate= url.GetOptionInt("cratid", 0);
  fSubeventProcid = url.GetOptionInt("procid", 1);
  fSubeventControl = url.GetOptionInt("ctrlid", 7);
  fSubeventSize = url.GetOptionInt("size", 32);
  fEarlyTriggerClear = url.HasOption("earlytrigclr");
  fDebug = url.HasOption("debug");

  if(fDebug)  DOUT0 ("Created new user::Input with read length %d, crate:%d, procid:%d, control:%d\n",
        fReadLength,fSubeventSubcrate, fSubeventProcid, fSubeventControl);

}

user::Input::~Input ()
{
   User_Cleanup();
}


bool user::Input::Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   DOUT1 ("Read_Init is executed...");
   if(User_Init() != 0) return false;
   return dabc::DataInput::Read_Init(wrk,cmd);
}


double user::Input::Read_Timeout ()
  {
    return  0.1; // timeout in seconds in case of  dabc::di_RepeatTimeOut
  }


unsigned user::Input::Read_Size ()
{
  return ( GetReadLength ()); // will specify buffer size for next event
}

unsigned user::Input::Read_Start (dabc::Buffer& buf)
{
  DOUT2 ("Read_Start() with bufsize %d\n", buf.GetTotalSize ());
  return dabc::di_Ok;    // synchronous mode, all handled in Read_Complete
}

unsigned user::Input::Read_Complete (dabc::Buffer& buf)
{
  DOUT3 ("Read_Complete() with buffer size :%d\n", buf.GetTotalSize());
  int trigtype=User_WaitForTrigger ();
  if (!trigtype)
        {
          // case of timeout, need dabc retry?
          EOUT(("**** Timout of trigger, retry dabc request.. \n"));
          return dabc::di_RepeatTimeOut;
        }
  if(fEarlyTriggerClear) User_ResetTrigger (); // prepare for next trigger before readout of data (for external DAQ buffering)
  buf.SetTypeId (mbs::mbt_MbsEvents);
  mbs::WriteIterator iter(buf);
  iter.NewEvent(fNumEvents);
  mbs::EventHeader* evhdr = (mbs::EventHeader*) iter.rawdata();
  // put here DAQ trigger type:
  evhdr->iTrigger = trigtype;

  iter.NewSubevent(fSubeventSize, fSubeventSubcrate, fSubeventControl, fSubeventProcid);
  uint32_t* pldat = (uint32_t*) iter.rawdata();
  unsigned long subsize=0;
  int res = User_Readout(pldat, subsize);

  if(!fEarlyTriggerClear) User_ResetTrigger (); // prepare for next trigger only after readout has finished
  if ((unsigned) res == dabc::di_SkipBuffer)
  {
    return dabc::di_SkipBuffer;
  }
  if ((unsigned) res == dabc::di_RepeatTimeOut)
  {
    if(fDebug) DOUT1 ("user::Input() returns with timeout\n");
    return dabc::di_RepeatTimeOut;
  }
  if ((unsigned) res == dabc::di_Error)
   {
      EOUT("user::Input() returns with Error\n");
      return dabc::di_Error;
   }

  if(!iter.FinishSubEvent(subsize))
  {
    EOUT("user::Input() could not finish subevent with size:%ld\n", (long) subsize);
    return dabc::di_Error;
  }
  if(!iter.FinishEvent())
  {
    EOUT("user::Input() could not finish event \n");
    return dabc::di_Error;
  }
  buf = iter.Close();
  if(fDebug)
   {
      if(fNumEvents % 10000 ==0)
      {
         DOUT0("UserReadout has send %d events to input, last size:%ld, return value:%d, total buffers size:%d\n",
                 fNumEvents, (long) subsize,res, buf.GetTotalSize());
      }
   }
  fNumEvents++;
  return (unsigned) res;
}


////////////////////////////////////////////////////////////////////////////////////////////
// below are the user defined functions for proprietary DAQ
// they can be changed already here
// it is also possible to develop a subclass of user::Input and re-implement it there
////////////////////////////////////////////////////////////////////////////////////////////

int user::Input::User_Init ()
{
   DOUT0("Executing User_Init() function for crate:%d, procid:%d, control:%d",
         fSubeventSubcrate, fSubeventProcid, fSubeventControl);
   // do antything reasonable here to setup your readout device
   // parameters can be passed to this function by extracting URL in the constructor and defining member variables
   return 0; // nonzero return value will mark error in the framework
}

int user::Input::User_Cleanup()
{
   DOUT0("Executing User_Cleanup() function");
   return 0;
}

int user::Input::User_WaitForTrigger ()
{
   // this is just a dummy implementation, causing a frequent polling readout.
   // todo: implement check of external readout trigger source
   int trigtype = mbs::tt_Event;
   usleep(500);
   return trigtype;
}

int user::Input::User_ResetTrigger()
 {
   // todo: reset trigger state in readout device
    return 0;
}

int user::Input::User_Readout (uint32_t* pdat, unsigned long& size)
 {
    unsigned numval = 100;
    for (unsigned nval=0;nval<numval;nval++)
         {
            if(size>=fSubeventSize)
                {
                   DOUT0("User_Readout: data would exceed subevent size %d ,please adjust setup",fSubeventSize);
                   return dabc::di_Error;
                }
            *pdat++ = (uint32_t) Gauss_Rnd(nval*500 + 1000, 500./(nval+1));
             size+=sizeof(uint32_t);
         }
    return dabc::di_Ok;
}



double user::Input::Gauss_Rnd(double mean, double sigma)
{
   // this function is taken from mbs random source to provide some data for standard go4 examples
   double x, y, z;
   z = 1.* rand() / RAND_MAX;
   y = 1.* rand() / RAND_MAX;
   x = z * 6.28318530717958623;
   return mean + sigma*sin(x)*sqrt(-2*log(y));
}
