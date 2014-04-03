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

#include "fesa/Monitor.h"

#include <stdlib.h>
#include <math.h>

#ifdef WITH_FESA
#include <cmw-rda/RDAService.h>
#include <cmw-rda/Config.h>
#include <cmw-rda/DeviceHandle.h>
#include <cmw-rda/ReplyHandler.h>


class rdaDabcHandler : public rdaReplyHandler
{
  public:
    fesa::Monitor* fPlayer;
    std::string fService;
    rdaRequest* fRequest;
    rdaData fContext;

    rdaDabcHandler(fesa::Monitor* p, const std::string& serv) :
       rdaReplyHandler(),
       fPlayer(p),
       fService(serv),
       fRequest(0),
       fContext()
       {
       }

       bool subscribe(rdaDeviceHandle* device, const std::string& cycle)
       {
         try {
            fRequest = device->monitorOn(fService.c_str(), cycle.c_str(), false, this, fContext);
            return fRequest!=0;
         } catch (const rdaException& ex) {
            if (fPlayer) fPlayer->ReportServiceError(fService, dabc::format("%s : %s", ex.getType(),ex.getMessage()));
            EOUT("Exception caught in subscribe: %s %s", ex.getType(), ex.getMessage());
         } catch (...) {
            if (fPlayer) fPlayer->ReportServiceError(fService, "subscribe - unknown error");
            EOUT("Unknown exception caught in subscribe");
         }

         return false;
      }

      bool unsubscribe(rdaDeviceHandle* device)
      {
         try {
           if (fRequest) device->monitorOff(fRequest);
           fRequest = 0;
           return true;
        } catch (const rdaException& ex) {
            if (fPlayer) fPlayer->ReportServiceError(fService, dabc::format("%s : %s", ex.getType(),ex.getMessage()));
           EOUT("Exception caught in subscribe: %s %s", ex.getType(), ex.getMessage());
        } catch (...) {
           if (fPlayer) fPlayer->ReportServiceError(fService, "unsubscribe - unknown error");
           EOUT("Unknown exception caught in unsubscribe");
        }
        return false;
      }


      virtual void handleReply(const rdaRequest& rq, const rdaData& value)
      {
          try
          {
             if (fPlayer) fPlayer->ReportServiceChanged(fService, &value);
          }
          catch (const rdaException& ex)
          {
             if (fPlayer) fPlayer->ReportServiceError(fService, dabc::format("%s : %s", ex.getType(),ex.getMessage()));
             EOUT( "Exception caught in GSIVoltageHandler: %s %s", ex.getType(), ex.getMessage());
          }
          catch (...)
          {
             if (fPlayer) fPlayer->ReportServiceError(fService, "handleReply - unknown error");
             EOUT("Unknown exception caught in handleReply");
          }
      }
};

#endif

fesa::Monitor::Monitor(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fHierarchy(),
   fRDAService(0),
   fDevice(0),
   fHandlers(0),
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

   fHierarchy.Create("fesa-monitor", true);

   fServerName = Cfg("Server", cmd).AsStr();
   fDeviceName = Cfg("Device", cmd).AsStr();
   fCycle = Cfg("Cycle", cmd).AsStr();

   double period = Cfg("FesaPeriod", cmd).AsDouble(1);
   fSubeventId = Cfg("FesaSubeventId", cmd).AsUInt(fSubeventId);
   fFlushTime = Cfg(dabc::xmlFlushTimeout,cmd).AsDouble(10.);

   if (fDoRec)
      CreateTimer("update", (period>0.01) ? period : 0.01, false);

   std::vector<std::string> services = Cfg("Services", cmd).AsStrVect();

   #ifdef WITH_FESA
   if (!fServerName.empty() && !fDeviceName.empty() && (services.size()>0)) {
      fRDAService = rdaRDAService::init();

      try {
         fDevice = fRDAService->getDeviceHandle(fDeviceName.c_str(), fServerName.c_str());
      } catch (...) {
         EOUT("Device %s on server %s not found", fDeviceName.c_str(), fServerName.c_str());
      }
   }
   #endif

   for (unsigned n=0;n<services.size();n++) {
      dabc::Hierarchy item = fHierarchy.CreateHChild(services[n]);
      item.EnableHistory(100);

      if (fDevice!=0) {
         #ifdef WITH_FESA
         rdaDabcHandler* handler = new rdaDabcHandler(this, services[n]);
         if (handler->subscribe(fDevice, fCycle))
           fHandlers.push_back(handler);
         else
           delete handler;
         #endif
      }
   }

   Publish(fHierarchy, "FESA/Monitor");
}

fesa::Monitor::~Monitor()
{
   #ifdef WITH_FESA

   for (unsigned n=0;n<fHandlers.size();n++) {
      fHandlers[n]->unsubscribe(fDevice);
      delete fHandlers[n];
   }
   fHandlers.clear();

   #endif

}

void fesa::Monitor::ProcessTimerEvent(unsigned timer)
{
   if (fDoRec) SendDataToOutputs();
}

void fesa::Monitor::ReportServiceError(const std::string& name, const std::string& err)
{
   dabc::LockGuard lock(fHierarchy.GetHMutex());

   dabc::Hierarchy item = fHierarchy.GetHChild(name);
   if (item.null()) return;

   item.SetField(dabc::prop_error, err);

   item.MarkChangedItems();
}


void fesa::Monitor::ReportServiceChanged(const std::string& name, const rdaData* value)
{
   // DOUT0("REPORT FESA SERVICE %s = %5.3f", name.c_str());

   dabc::LockGuard lock(fHierarchy.GetHMutex());

   dabc::Hierarchy item = fHierarchy.GetHChild(name);
   if (item.null()) return;

   if (item.HasField(dabc::prop_error))
      item.RemoveField(dabc::prop_error);

#ifdef WITH_FESA

   // TODO: mark here that we start change hierarchy item, but not immidiately change version

   unsigned n = 0;
   // first of all, delete non-existing fields
   while (n < item.NumFields()) {
      std::string fname = item.FieldName(n++);
      if ((fname.find("dabc:") == 0) || value->contains(fname.c_str())) continue;
      item.RemoveField(fname);
      n = 0; // start search from beginning
   }

   rdaDataIterator iter(*value);

   while (iter.hasNext()) {
      rdaDataEntry* entry = iter.next();

      const char* tag = entry->tag();
      if ((tag==0) || (*tag==0)) {
         EOUT("There is no tag specified in field of service %s", name.c_str());
         continue;
      }
      
      // DOUT0("Service %s tag %s", name.c_str(), tag);

      switch (entry->getValueType()) {
         /// indicates that a DataEntry does not contain anything.
         case rdaDataEntry::TYPE_NULL: break;
         /// indicates a boolean value.
         case rdaDataEntry::TYPE_BOOLEAN: {
            item.SetField(tag, entry->extractBoolean());
            if (fDoRec) fRec.AddLong(tag, entry->extractBoolean(), true);
            break;
         }
         /// indicates a boolean array value.
         case rdaDataEntry::TYPE_BOOLEAN_ARRAY: {
            unsigned long size(0);
            const bool* arr = entry->getBooleanArray(size);
            std::vector<int64_t> vect;
            for (unsigned n=0;n<size;n++) vect.push_back(arr[n] ? 1 : 0);
            item.SetField(tag, vect);
            break;
         }
         /// indicates a byte value.
         case rdaDataEntry::TYPE_BYTE: {
            item.SetField(tag, (int64_t)entry->extractByte());
            if (fDoRec) fRec.AddLong(tag, (int64_t)entry->extractByte(), true);
            break;
         }
         /// indicates a byte array value.
         case rdaDataEntry::TYPE_BYTE_ARRAY: {
            unsigned long size(0);
            const signed char* arr = entry->getByteArray(size);
            std::vector<int64_t> vect;
            for (unsigned n=0;n<size;n++) vect.push_back((int64_t)arr[n]);
            item.SetField(tag, vect);
            break;
         }
         /// indicates a short value.
         case rdaDataEntry::TYPE_SHORT: {
            item.SetField(tag, (int64_t)entry->extractShort());
            if (fDoRec) fRec.AddLong(tag, (int64_t)entry->extractShort(), true);
            break;
         }
         /// indicates a short array value.
         case rdaDataEntry::TYPE_SHORT_ARRAY: {
            unsigned long size(0);
            const short* arr = entry->getShortArray(size);
            std::vector<int64_t> vect;
            for (unsigned n=0;n<size;n++) vect.push_back((int64_t)arr[n]);
            item.SetField(tag, vect);
            break;
         }
         /// indicates an integer value.
         case rdaDataEntry::TYPE_INT: {
            item.SetField(tag, entry->extractInt());
            if (fDoRec) fRec.AddLong(tag, entry->extractInt(), true);
            break;
         }
         /// indicates an integer array value.
         case rdaDataEntry::TYPE_INT_ARRAY: {
            unsigned long size(0);
            const int* arr = entry->getIntArray(size);

            std::vector<int64_t> vect;
            for (unsigned n=0;n<size;n++) vect.push_back(arr[n]);
            item.SetField(tag, vect);
            break;
         }
         /// indicates a long long value.
         case rdaDataEntry::TYPE_LONG: {
            item.SetField(tag, (int64_t)entry->extractLong());
            if (fDoRec) fRec.AddLong(tag, entry->extractLong(), true);
            break;
         }
         /// indicates a long long array value.
         case rdaDataEntry::TYPE_LONG_ARRAY: {
            unsigned long size(0);
            const long long* arr = entry->getLongArray(size);
            std::vector<int64_t> vect;
            for (unsigned n=0;n<size;n++) vect.push_back((int64_t)arr[n]);
            item.SetField(tag, vect);
            break;
         }
         /// indicates a float value.
         case rdaDataEntry::TYPE_FLOAT: {
            item.SetField(tag, entry->extractFloat());
            if (fDoRec) fRec.AddDouble(tag, entry->extractFloat(), true);
            break;
         }
         /// indicates a float array value.
         case rdaDataEntry::TYPE_FLOAT_ARRAY: {
            unsigned long size(0);
            const float* arr = entry->getFloatArray(size);
            std::vector<double> vect;
            for (unsigned n=0;n<size;n++) vect.push_back(arr[n]);
            item.SetField(tag, vect);
            break;
         }
         /// indicates a double value.
         case rdaDataEntry::TYPE_DOUBLE: {
            item.SetField(tag, entry->extractDouble());
            if (fDoRec) fRec.AddDouble(tag, entry->extractDouble(), true);
            break;
         }
         /// indicates a double array value.
         case rdaDataEntry::TYPE_DOUBLE_ARRAY: {
            unsigned long size(0);
            const double* arr = entry->getDoubleArray(size);
            std::vector<double> vect;
            for (unsigned n=0;n<size;n++) vect.push_back(arr[n]);
            item.SetField(tag, vect);
            break;
         }
         /// indicates a string value.
         case rdaDataEntry::TYPE_STRING: {
            item.SetField(tag, entry->getString());
            break;
         }
         /// indicates a string array value.
         case rdaDataEntry::TYPE_STRING_ARRAY: {
            unsigned long size(0);
            const char** arr = entry->getStringArray(size);
            std::vector<std::string> vect;
            for (unsigned n=0;n<size;n++) vect.push_back(arr[n]);
            item.SetField(tag, vect);
            break;
         }
      }
   }

   // TODO: here apply new version and create history entry
#endif

   item.MarkChangedItems();
}


void fesa::Monitor::SendDataToOutputs()
{
   unsigned nextsize = 0;

   {
      dabc::LockGuard lock(fHierarchy.GetHMutex());
      nextsize = fRec.GetRawSize();
   }

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

   fEventNumber++;

   fRec.SetEventId(fEventNumber);
   fRec.SetEventTime(time(NULL));

   fIter.NewEvent(fEventNumber);
   fIter.NewSubevent2(fSubeventId);

   unsigned size = 0;

   {
      dabc::LockGuard lock(fHierarchy.GetHMutex());
      size = fRec.Write(fIter.rawdata(), fIter.maxrawdatasize());
   }

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
