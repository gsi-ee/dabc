// $Id$

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

#include "mbs/MonitorSlowControl.h"

//#include <cstdlib>
//#include <cmath>


mbs::MonitorSlowControl::MonitorSlowControl(const std::string &name, const std::string &prefix, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fRec(),
   fDoRec(false),
   fSubeventId(8),
   fEventNumber(0),
   fLastSendTime(),
   fIter(),
   fFlushTime(10)
{
   EnsurePorts(0, 0, dabc::xmlWorkPool);
   fDoRec = NumOutputs() > 0;

   double period = Cfg(prefix + "Period", cmd).AsDouble(1);
   fSubeventId = Cfg(prefix + "SubeventId", cmd).AsUInt(fSubeventId);
   fFlushTime = Cfg(dabc::xmlFlushTimeout,cmd).AsDouble(10.);

   if (fDoRec)
      CreateTimer("update", (period>0.01) ? period : 0.01);
}

mbs::MonitorSlowControl::~MonitorSlowControl()
{
}

void mbs::MonitorSlowControl::ProcessTimerEvent(unsigned timer)
{
   if (fDoRec && (TimerName(timer) == "update"))
      SendDataToOutputs();
}

unsigned mbs::MonitorSlowControl::GetNextEventTime()
{
   return time(NULL);
}


void mbs::MonitorSlowControl::SendDataToOutputs()
{
   unsigned nextsize = GetRecRawSize();

   if (fIter.IsAnyEvent() && !fIter.IsPlaceForEvent(nextsize, true)) {

      // if output is blocked, do not produce data
      if (!CanSendToAllOutputs()) return;

      dabc::Buffer buf = fIter.Close();
      SendToAllOutputs(buf);

      fLastSendTime.GetNow();
   }

   if (!fIter.IsBuffer()) {
      dabc::Buffer buf = TakeBuffer();
      // if no buffer can be taken, skip data
      if (buf.null()) { EOUT("Cannot take buffer for FESA data"); return; }
      fIter.Reset(buf);
   }

   if (!fIter.IsPlaceForEvent(nextsize, true)) {
      EOUT("EZCA event %u too large for current buffer size", nextsize);
      return;
   }

   unsigned evid = GetNextEventNumber();

   fRec.SetEventId(evid);
   fRec.SetEventTime(GetNextEventTime());

   fIter.NewEvent(evid);
   fIter.NewSubevent2(fSubeventId);

   unsigned size = WriteRecRawData(fIter.rawdata(), fIter.maxrawdatasize());

   if (size==0) {
      EOUT("Fail to write data into MBS subevent");
   }

   fIter.FinishSubEvent(size);
   fIter.FinishEvent();

   if (fLastSendTime.Expired(fFlushTime) && CanSendToAllOutputs()) {
      dabc::Buffer buf = fIter.Close();
      SendToAllOutputs(buf);
      fLastSendTime.GetNow();
   }
}
