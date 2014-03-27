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

#include "ezca/Player.h"

#include <sys/timeb.h>

#include "tsDefs.h"
#include "cadef.h"
#include "ezca.h"

#include "ezca/Definitions.h"

#include "dabc/Publisher.h"
#include "mbs/MbsTypeDefs.h"
#include "mbs/Iterator.h"

ezca::Player::Player(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fEzcaTimeout(-1.),
   fEzcaRetryCnt(-1),
   fEzcaDebug(false),
   fEzcaAutoError(false),
   fTimeout(1),
   fSubeventId(8),
   fLongRecords(),
   fLongValues(),
   fDoubleRecords(),
   fDoubleValues(),
   fDescriptor(),
   fEventNumber(0)
{
   EnsurePorts(0, 0, dabc::xmlWorkPool);

   fEzcaTimeout = Cfg(ezca::xmlEzcaTimeout, cmd).AsDouble(fEzcaTimeout);
   fEzcaRetryCnt = Cfg(ezca::xmlEzcaRetryCount, cmd).AsInt(fEzcaRetryCnt);
   fEzcaDebug = Cfg(ezca::xmlEzcaDebug, cmd).AsBool(fEzcaDebug);
   fEzcaAutoError = Cfg(ezca::xmlEzcaAutoError, cmd).AsBool(fEzcaAutoError);

   fTimeout = Cfg(ezca::xmlTimeout, cmd).AsDouble(fTimeout);
   fSubeventId = Cfg(ezca::xmlEpicsSubeventId, cmd).AsUInt(fSubeventId);

   fLongRecords = Cfg("Long", cmd).AsStrVect();
   fLongValues.resize(fLongRecords.size());

   fDoubleRecords = Cfg("Doubles", cmd).AsStrVect();
   fDoubleValues.resize(fDoubleRecords.size());

   fDescriptor.clear();

   fWorkerHierarchy.Create("EZCA");


   for (unsigned ix = 0; ix < fLongRecords.size(); ++ix) {
      dabc::Hierarchy item = fWorkerHierarchy.GetHChild(GetItemName(fLongRecords[ix]), true);
      item.SetField(dabc::prop_kind, "rate");
      item.EnableHistory(100);
   }

   for (unsigned ix = 0; ix < fDoubleRecords.size(); ++ix) {
      dabc::Hierarchy item = fWorkerHierarchy.GetHChild(GetItemName(fDoubleRecords[ix]), true);
      item.SetField(dabc::prop_kind, "rate");
      item.EnableHistory(100);
   }


   /*
   dabc::Hierarchy item = fWorkerHierarchy.CreateChild("BeamRate2");
   item.SetField(dabc::prop_kind, "rate");
   item.EnableHistory(100);

   fServerName = Cfg("Server", cmd).AsStr();
   fDeviceName = Cfg("Device", cmd).AsStr();
   fCycles = Cfg("Cycles", cmd).AsStr();
   fService = Cfg("Service", cmd).AsStr();
   fField = Cfg("Field", cmd).AsStr();

   */

   if (fTimeout<=0) fTimeout = 0.001;

   CreateTimer("update", fTimeout, false);

   Publish(fWorkerHierarchy, "EZCA");
}

ezca::Player::~Player()
{
}

std::string ezca::Player::GetItemName(const std::string& ezcaname)
{
   std::string res = ezcaname;

   size_t pos = 0;

   while ((pos = res.find_first_of(":")) != std::string::npos)
      res[pos] = '/';

   return res;
}


void ezca::Player::OnThreadAssigned()
{
   dabc::ModuleAsync::OnThreadAssigned();

   if (fEzcaTimeout>0) ezcaSetTimeout(fEzcaTimeout);

   if (fEzcaRetryCnt>0) ezcaSetRetryCount(fEzcaRetryCnt);

   if (fEzcaDebug) ezcaDebugOn();
   else ezcaDebugOff();

   if (fEzcaAutoError) ezcaAutoErrorMessageOn();
   else ezcaAutoErrorMessageOff();
}

void ezca::Player::ProcessTimerEvent(unsigned timer)
{
   if (!DoEpicsReadout()) return;

   if (NumOutputs() > 0)
      SendDataToOutputs();

   for (unsigned ix = 0; ix < fLongRecords.size(); ++ix) {
      fWorkerHierarchy.GetHChild(GetItemName(fLongRecords[ix])).SetField("value", fLongValues[ix]);
   }

   for (unsigned ix = 0; ix < fDoubleRecords.size(); ++ix) {
      fWorkerHierarchy.GetHChild(GetItemName(fDoubleRecords[ix])).SetField("value", fDoubleValues[ix]);
   }

   fWorkerHierarchy.MarkChangedItems();
}


int ezca::Player::ExecuteCommand(dabc::Command cmd)
{
   return dabc::ModuleAsync::ExecuteCommand(cmd);
}


void ezca::Player::SendDataToOutputs()
{
   if (fDescriptor.empty()) BuildDescriptor();

   dabc::Buffer buf = TakeBuffer();
   if (buf.null()) return;

   fEventNumber++;

   mbs::WriteIterator iter(buf);

   iter.NewEvent(fEventNumber);
   iter.NewSubevent2(fSubeventId);

   uint32_t number = fEventNumber;
   iter.AddRawData(&number, sizeof(number));

   struct timeb s_timeb;
   ftime(&s_timeb);
   number = s_timeb.time;
   iter.AddRawData(&number, sizeof(number));

   uint64_t num64 = fLongValues.size();
   iter.AddRawData(&num64, sizeof(num64));

   for (unsigned ix = 0; ix < fLongValues.size(); ++ix) {
      int64_t val = fLongValues[ix]; // machine independent representation here
      iter.AddRawData(&val, sizeof(val));
   }

   // header with number of double records
   num64 = fDoubleValues.size();
   iter.AddRawData(&num64, sizeof(num64));

   // data values for double records:
   for (unsigned ix = 0; ix < fDoubleValues.size(); ix++) {
      double tmpval = fDoubleValues[ix]; // should be always 8 bytes
      iter.AddRawData(&tmpval, sizeof(tmpval));
   }

   // copy description of record names at subevent end:
   iter.AddRawData(fDescriptor.c_str(), fDescriptor.size());

   iter.FinishSubEvent();
   iter.FinishEvent();

   buf = iter.Close();
   SendToAllOutputs(buf);
}


void ezca::Player::BuildDescriptor()
{
   fDescriptor.clear();

   fDescriptor.append(dabc::format("%u ", (unsigned) fLongRecords.size()));
   fDescriptor.append(1,'\0');

   for (unsigned ix = 0; ix < fLongRecords.size(); ++ix) {
      // record the name of just written process variable:
      fDescriptor.append(fLongRecords[ix]);
      fDescriptor.append(1,'\0');
   }

   fDescriptor.append(dabc::format("%u ", (unsigned) fDoubleRecords.size()));
   fDescriptor.append(1,'\0');

   for (unsigned ix = 0; ix < fDoubleRecords.size(); ix++) {
      // record the name of just written process variable:
      fDescriptor.append(fDoubleRecords[ix]);
      fDescriptor.append(1,'\0');
   }

   while (fDescriptor.size() % 4 != 0) fDescriptor.append(1,'\0');
}



bool ezca::Player::DoEpicsReadout()
{
   if ((fLongRecords.size()==0) && (fDoubleRecords.size()==0)) return false;

   dabc::TimeStamp tm = dabc::Now();

   bool res = true;

   ezcaStartGroup();

   // now the data values for each record in order:
   for (unsigned ix = 0; ix < fLongRecords.size(); ix++) {
      fLongValues[ix] = 0;
      int ret = CA_GetLong(fLongRecords[ix], fLongValues[ix]);
      if (ret==EZCA_OK) continue;
      EOUT("Request long %s Ret = %s", fLongRecords[ix].c_str(), CA_RetCode(ret));
      res = false;
   }

   for (unsigned ix = 0; ix < fDoubleRecords.size(); ix++) {
      fDoubleValues[ix] = 0;
      int ret = CA_GetDouble(fDoubleRecords[ix], fDoubleValues[ix]);
      if (ret==EZCA_OK) continue;
      EOUT("Request double %s Ret = %s", fDoubleRecords[ix].c_str(), CA_RetCode(ret));
      res = false;
   }


   int *rcs(0), nrcs(0);

   if (ezcaEndGroupWithReport(&rcs, &nrcs) != EZCA_OK) {
      res = false;
      EOUT("EZCA error %s", CA_ErrorString().c_str());
      for (unsigned i=0; i< (unsigned) nrcs; i++)
         if (rcs[i] != EZCA_OK) {
            std::string vname = i<fLongRecords.size() ? fLongRecords[i] : fDoubleRecords[i-fLongRecords.size()];
            EOUT("Problem getting %s ret %s", vname.c_str(), CA_RetCode(rcs[i]));
         }
   }

   ezcaFree(rcs);

   DOUT0("Epics readout time is = %7.5f s res = %s", tm.SpentTillNow(), DBOOL(res));

   return res;
}


int ezca::Player::CA_GetLong(const std::string& name, long& val)
{
   int rev = ezcaGet((char*) name.c_str(), ezcaLong, 1, &val);
   if(rev!=EZCA_OK)
      EOUT("%s", CA_ErrorString().c_str());
   else
      DOUT3("EpicsInput::CA_GetLong(%s) = %d",name.c_str(),val);
   return rev;
}

int ezca::Player::CA_GetDouble(const std::string& name, double& val)
{
   int rev=ezcaGet((char*) name.c_str(), ezcaDouble, 1, &val);
   if(rev!=EZCA_OK)
      EOUT("%s", CA_ErrorString().c_str());
   else
      DOUT3("EpicsInput::CA_GetDouble(%s) = %f",name.c_str(),val);
   return rev;
}

std::string ezca::Player::CA_ErrorString()
{
   std::string res;

   char *error_msg_buff(0);
   ezcaGetErrorString(NULL, &error_msg_buff);
   if (error_msg_buff!=0) res = error_msg_buff;
   ezcaFree(error_msg_buff);

   return res;
}

const char* ezca::Player::CA_RetCode(int ret)
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
