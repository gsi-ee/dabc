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

#include "fesa/defines.h"

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
            EOUT("Exception caught in subscribe: %s %s", ex.getType(), ex.getMessage());
         } catch (...) {
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
           EOUT("Exception caught in subscribe: %s %s", ex.getType(), ex.getMessage());
        } catch (...) {
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
            EOUT( "Exception caught in GSIVoltageHandler: %s %s", ex.getType(), ex.getMessage());
          }
          catch (...)
          {
             EOUT("Unknown exception caught in handleReply");
          }
      }
};

#endif

fesa::Monitor::Monitor(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fHierarchyMutex(),
   fHierarchy(),
   fRDAService(0),
   fDevice(0),
   fHandlers(0)
{
   fHierarchy.Create("fesa-monitor");

   fServerName = Cfg("Server", cmd).AsStdStr();
   fDeviceName = Cfg("Device", cmd).AsStdStr();
   fCycle = Cfg("Cycle", cmd).AsStdStr();

   std::vector<std::string> services = Cfg("Services", cmd).AsStrVector();

   CreateTimer("update", 1., false);

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
      dabc::Hierarchy item = fHierarchy.CreateChild(services[n]);
      item.EnableHistory(100, "value");

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
//   DOUT0("Process timer");
}



int fesa::Monitor::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("GetBinary")) {

      std::string itemname = cmd.GetStdStr("Item");

      dabc::LockGuard lock(fHierarchyMutex);

      dabc::Hierarchy item = fHierarchy.FindChild(itemname.c_str());

      if (item.null()) {
         EOUT("No find item %s to get binary", itemname.c_str());
         return dabc::cmd_false;
      }

      dabc::Buffer buf;

      if (cmd.GetBool("history"))
         buf = item.ExecuteHistoryRequest(cmd);
      else {
      }

      if (buf.null()) {
         EOUT("No find binary data for item %s", itemname.c_str());
         return dabc::cmd_false;
      }

      cmd.SetRawData(buf);

      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}


void fesa::Monitor::BuildWorkerHierarchy(dabc::HierarchyContainer* cont)
{
   // indicate that we are bin producer of down objects

   dabc::LockGuard lock(fHierarchyMutex);

   // do it here, while all properties of main node are ignored when hierarchy is build
   dabc::Hierarchy(cont).Field(dabc::prop_binary_producer).SetStr(ItemName());

   fHierarchy()->BuildHierarchy(cont);
}

void fesa::Monitor::ReportServiceChanged(const std::string& name, const rdaData* value)
{
   // DOUT0("REPORT FESA SERVICE %s = %5.3f", name.c_str());

   dabc::LockGuard lock(fHierarchyMutex);

   dabc::Hierarchy item = fHierarchy.FindChild(name.c_str());
   if (item.null()) return;

#ifdef WITH_FESA

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
            item.Field(tag).SetBool(entry->extractBoolean());
            break;
         }
         /// indicates a boolean array value.
         case rdaDataEntry::TYPE_BOOLEAN_ARRAY: {
            unsigned long size(0);
            const bool* arr = entry->getBooleanArray(size);
            item.Field(tag).SetBoolArray(arr, size);
            break;
         }
         /// indicates a byte value.
         case rdaDataEntry::TYPE_BYTE: {
            item.Field(tag).SetInt((int)entry->extractByte());
            break;
         }
         /// indicates a byte array value.
         case rdaDataEntry::TYPE_BYTE_ARRAY: {
            unsigned long size(0);
            const signed char* arr = entry->getByteArray(size);
            std::vector<int> vect;
            for (unsigned n=0;n<size;n++) vect.push_back((int)arr[n]);
            item.Field(tag).SetIntVector(vect);
            break;
         }
         /// indicates a short value.
         case rdaDataEntry::TYPE_SHORT: {
            item.Field(tag).SetInt((int)entry->extractShort());
            break;
         }
         /// indicates a short array value.
         case rdaDataEntry::TYPE_SHORT_ARRAY: {
            unsigned long size(0);
            const short* arr = entry->getShortArray(size);
            std::vector<int> vect;
            for (unsigned n=0;n<size;n++) vect.push_back((int)arr[n]);
            item.Field(tag).SetIntVector(vect);
            break;
         }
         /// indicates an integer value.
         case rdaDataEntry::TYPE_INT: {
            item.Field(tag).SetInt(entry->extractInt());
            break;
         }
         /// indicates an integer array value.
         case rdaDataEntry::TYPE_INT_ARRAY: {
            unsigned long size(0);
            const int* arr = entry->getIntArray(size);
            item.Field(tag).SetIntArray(arr,size);
            break;
         }
         /// indicates a long long value.
         case rdaDataEntry::TYPE_LONG: {
            item.Field(tag).SetInt((int)entry->extractLong());
            break;
         }
         /// indicates a long long array value.
         case rdaDataEntry::TYPE_LONG_ARRAY: {
            unsigned long size(0);
            const long long* arr = entry->getLongArray(size);
            std::vector<int> vect;
            for (unsigned n=0;n<size;n++) vect.push_back((int)arr[n]);
            item.Field(tag).SetIntVector(vect);
            break;
         }
         /// indicates a float value.
         case rdaDataEntry::TYPE_FLOAT: {
            item.Field(tag).SetDouble(entry->extractFloat());
            break;
         }
         /// indicates a float array value.
         case rdaDataEntry::TYPE_FLOAT_ARRAY: {
            unsigned long size(0);
            const float* arr = entry->getFloatArray(size);
            std::vector<double> vect;
            for (unsigned n=0;n<size;n++) vect.push_back(arr[n]);
            item.Field(tag).SetDoubleVector(vect);
            break;
         }
         /// indicates a double value.
         case rdaDataEntry::TYPE_DOUBLE: {
            item.Field(tag).SetDouble(entry->extractDouble());
            break;
         }
         /// indicates a double array value.
         case rdaDataEntry::TYPE_DOUBLE_ARRAY: {
            unsigned long size(0);
            const double* arr = entry->getDoubleArray(size);
            item.Field(tag).SetDoubleArray(arr, size);
            break;
         }
         /// indicates a string value.
         case rdaDataEntry::TYPE_STRING: {
            item.Field(tag).SetStr(entry->getString());
            break;
         }
         /// indicates a string array value.
         case rdaDataEntry::TYPE_STRING_ARRAY: {
            unsigned long size(0);
            const char** arr = entry->getStringArray(size);
            std::vector<std::string> vect;
            for (unsigned n=0;n<size;n++) vect.push_back(arr[n]);
            item.Field(tag).SetStrVector(vect);
            break;
         }
      }
   }

#endif
}
