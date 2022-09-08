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

#include "ezca/EpicsInput.h"

#include "dabc/Pointer.h"
#include "dabc/timing.h"
#include "dabc/Manager.h"
#include "mbs/MbsTypeDefs.h"
#include "mbs/Iterator.h"
#include "mbs/SlowControlData.h"

#include "ezca/Definitions.h"

#include <ctime>


#include "tsDefs.h"
#include "cadef.h"

#include "ezca.h"


ezca::EpicsInput::EpicsInput(const std::string &name) :
   dabc::DataInput(),
   fName(name),
   fTimeout(1),
   fSubeventId(8),
   fLastFlagValue(0),
   fEventNumber(0),
   fCounter(0),
   fEzcaTimeout(-1.),
   fEzcaRetryCnt(-1),
   fEzcaDebug(false),
   fEzcaAutoError(false)
{
   ResetDescriptors();
}

ezca::EpicsInput::~EpicsInput()
{
   Close();
}

bool ezca::EpicsInput::Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd)
{
   if (!dabc::DataInput::Read_Init(wrk, cmd)) return false;

   fName = wrk.Cfg(ezca::xmlEpicsName, cmd).AsStr(fName);
   fTimeout = wrk.Cfg(ezca::xmlTimeout, cmd).AsDouble(fTimeout);
   fSubeventId = wrk.Cfg(ezca::xmlEpicsSubeventId, cmd).AsInt(fSubeventId);

   fUpdateFlagRecord = wrk.Cfg(ezca::xmlUpdateFlagRecord,cmd).AsStr();
   fIDNumberRecord = wrk.Cfg(ezca::xmlEventIDRecord,cmd).AsStr();
   fUpdateCommandReceiver = wrk.Cfg(ezca::xmlCommandReceiver,cmd).AsStr("");

   fLongRecords = wrk.Cfg(ezca::xmlNameLongRecords, cmd).AsStrVect();
   fLongValues.resize(fLongRecords.size(), 0);
   fLongRes.resize(fLongRecords.size(), false);

   fDoubleRecords = wrk.Cfg(ezca::xmlNameDoubleRecords, cmd).AsStrVect();
   fDoubleValues.resize(fDoubleRecords.size(), 0.);
   fDoubleRes.resize(fDoubleRecords.size(), false);

   fEzcaTimeout = wrk.Cfg(ezca::xmlEzcaTimeout, cmd).AsDouble(fEzcaTimeout);
   fEzcaRetryCnt = wrk.Cfg(ezca::xmlEzcaRetryCount, cmd).AsInt(fEzcaRetryCnt);
   fEzcaDebug = wrk.Cfg(ezca::xmlEzcaDebug, cmd).AsBool(fEzcaDebug);
   fEzcaAutoError = wrk.Cfg(ezca::xmlEzcaAutoError, cmd).AsBool(fEzcaAutoError);

   DOUT1("EpicsInput %s - Timeout = %e s, subevtid:%d, update flag:%s id:%s ",
         fName.c_str(), fTimeout,fSubeventId, fUpdateFlagRecord.c_str(),fIDNumberRecord.c_str());

   return true;
}


bool ezca::EpicsInput::Close()
{
   ResetDescriptors();
   DOUT1("EpicsInput::Close");
   return true;
}


unsigned ezca::EpicsInput::Read_Size()
{

   if (fEzcaTimeout>0) ezcaSetTimeout(fEzcaTimeout);

   if (fEzcaRetryCnt>0) ezcaSetRetryCount(fEzcaRetryCnt);

   if (fEzcaDebug) ezcaDebugOn();
              else ezcaDebugOff();

   if (fEzcaAutoError) ezcaAutoErrorMessageOn();
                  else ezcaAutoErrorMessageOff();

   if (!fUpdateFlagRecord.empty()) {
      long flag = 0;

      // first check the flag record:
      if (CA_GetLong(fUpdateFlagRecord, flag) != 0) {
         EOUT("Cannot get update record %s", fUpdateFlagRecord.c_str());
         return dabc::di_RepeatTimeOut;
      }

      if (flag == fLastFlagValue) { // read on every change of flag if not 0
         //oldflag = flag;
         DOUT3("EpicsInput::Read_Size same flag value, repeat after timeout %3.1f", fTimeout);
         return dabc::di_RepeatTimeOut;
      }

      fLastFlagValue = flag;

      if (!fUpdateCommandReceiver.empty()) {
         dabc::Command cmd(ezca::nameUpdateCommand);
         cmd.SetReceiver(fUpdateCommandReceiver);
         cmd.SetInt(xmlUpdateFlagRecord, flag);
         dabc::mgr.Submit(cmd);
      }
      // if this is nonzero, read other records and write buffer
   } else {

      // use last flag as switch to timeout reading of data
      if(fLastFlagValue) {
         fLastFlagValue = 0;
//         DOUT0("Return timeout");
         return dabc::di_RepeatTimeOut;
      }
      fLastFlagValue = 1;
   }

//   DOUT0("ezca::EpicsInput::Read_Size middle %s", fIDNumberRecord.c_str());

   if (!fIDNumberRecord.empty()) {
//      ezcaAutoErrorMessageOn();
//      ezcaDebugOn();
      DOUT1("Event number record reading %s retr %d tmout %5.3f", fIDNumberRecord.c_str(), ezcaGetRetryCount(), ezcaGetTimeout());
      if (CA_GetLong(fIDNumberRecord, fEventNumber) != 0) {
         EOUT("Cannot read event number from record %s", fIDNumberRecord.c_str());
         return dabc::di_RepeatTimeOut;
      }
      DOUT1("Event number record reading %s = %ld ", fIDNumberRecord.c_str(), fEventNumber);
   } else {
      fEventNumber = fCounter;
   }

   return dabc::di_DfltBufSize;
}


unsigned ezca::EpicsInput::Read_Complete(dabc::Buffer& buf)
{
   fCounter++;

//   DOUT0("Buffer size %u", buf.GetTotalSize());

   dabc::TimeStamp tm = dabc::Now();

   if ((NumLongRecords() + NumDoubleRecords()) > 0) {

      ezcaStartGroup();

      // now the data values for each record in order:
      for (unsigned ix = 0; ix < NumLongRecords(); ix++) {
         fLongValues[ix] = 0;
         fLongRes[ix] = true;
         int ret = CA_GetLong(GetLongRecord(ix), fLongValues[ix]);
         if (ret!=EZCA_OK) EOUT("Request long %s Ret = %s", GetLongRecord(ix).c_str(), CA_RetCode(ret));
      }

      for (unsigned ix = 0; ix < NumDoubleRecords(); ix++) {
         fDoubleValues[ix] = 0;
         fDoubleRes[ix] = true;
         int ret = CA_GetDouble(GetDoubleRecord(ix), fDoubleValues[ix]);
         if (ret!=EZCA_OK) EOUT("Request double %s Ret = %s", GetDoubleRecord(ix).c_str(), CA_RetCode(ret));
      }

      int *rcs = nullptr, nrcs = 0;

      if (ezcaEndGroupWithReport(&rcs, &nrcs) != EZCA_OK) {
         EOUT("EZCA error %s", CA_ErrorString().c_str());
         for (unsigned i=0; i< (unsigned) nrcs; i++)
            if (i<fLongRecords.size()) {
               unsigned ix = i;
               fLongRes[ix] = (rcs[i]==EZCA_OK);
               if (!fLongRes[ix])
                  EOUT("Problem getting long %s ret %s", fLongRecords[ix].c_str(), CA_RetCode(rcs[i]));
            } else {
               unsigned ix = i - fLongRecords.size();
               fDoubleRes[ix] = (rcs[i]==EZCA_OK);
               if (!fDoubleRes[ix])
                  EOUT("Problem getting double %s ret %s", fDoubleRecords[ix].c_str(), CA_RetCode(rcs[i]));
            }
      }

      ezcaFree(rcs);
   }

   DOUT2("EpicsInput:: readout time is = %7.5f s", tm.SpentTillNow());

   mbs::SlowControlData rec;

   for (unsigned ix = 0; ix < fLongRecords.size(); ++ix)
      if (fLongRes[ix]) rec.AddLong(fLongRecords[ix], fLongValues[ix]);

   for (unsigned ix = 0; ix < fDoubleRecords.size(); ++ix)
      if (fDoubleRes[ix]) rec.AddDouble(fDoubleRecords[ix], fDoubleValues[ix]);


   rec.SetEventId(fEventNumber);
   rec.SetEventTime(time(nullptr));

   mbs::WriteIterator iter(buf);

   iter.NewEvent(fEventNumber);
   iter.NewSubevent2(fSubeventId);

   unsigned size = rec.Write(iter.rawdata(), iter.maxrawdatasize());

   if (size==0) {
      EOUT("Fail to write data into MBS subevent");
   }

   iter.FinishSubEvent(size);
   iter.FinishEvent();

   buf = iter.Close();

//   DOUT0("Read buf size = %u", buf.GetTotalSize());

   return dabc::di_Ok;
}


int ezca::EpicsInput::CA_GetLong(const std::string &name, long& val)
{
   int rev = ezcaGet((char*) name.c_str(), ezcaLong, 1, &val);
   if(rev!=EZCA_OK)
      EOUT("%s", CA_ErrorString().c_str());
   else
      DOUT3("EpicsInput::CA_GetLong(%s) = %d",name.c_str(),val);
   return rev;
}

int ezca::EpicsInput::CA_GetDouble(const std::string &name, double& val)
{
   int rev=ezcaGet((char*) name.c_str(), ezcaDouble, 1, &val);
   if(rev!=EZCA_OK)
      EOUT("%s", CA_ErrorString().c_str());
   else
      DOUT3("EpicsInput::CA_GetDouble(%s) = %f",name.c_str(),val);
   return rev;
}

std::string ezca::EpicsInput::CA_ErrorString()
{
   std::string res;

   char *error_msg_buff = nullptr;
   ezcaGetErrorString(nullptr, &error_msg_buff);
   if (error_msg_buff) res = error_msg_buff;
   ezcaFree(error_msg_buff);

   return res;
}

const char* ezca::EpicsInput::CA_RetCode(int ret)
{
   switch (ret) {
      case EZCA_OK:                return "EZCA_OK";
      case EZCA_INVALIDARG:        return "EZCA_INVALIDARG";
      case EZCA_FAILEDMALLOC:      return "EZCA_FAILEDMALLOC";
      case EZCA_CAFAILURE:         return "EZCA_CAFAILURE";
      case EZCA_UDFREQ:            return "EZCA_UDFREQ";
      case EZCA_NOTCONNECTED:      return "EZCA_NOTCONNECTED";
      case EZCA_NOTIMELYRESPONSE:  return "EZCA_NOTIMELYRESPONSE";
      case EZCA_INGROUP:           return "EZCA_INGROUP";
      case EZCA_NOTINGROUP:        return "EZCA_NOTINGROUP";
      case EZCA_ABORTED:           return "EZCA_ABORTED";
   }

   return "unknown";
}
