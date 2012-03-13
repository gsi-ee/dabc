#ifndef EZCA_EPICSINPUT_H
#define EZCA_EPICSINPUT_H

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#include "dabc/logging.h"

// this one is from EPICS:
#include "tsDefs.h"
#include "cadef.h"
// this one from ezca extension:
#include "ezca.h"

#include <string>
#include <vector>
#include <stdint.h>



namespace ezca {

   class InfoDescriptor {
      protected:
         /* the update flag record is looked at for all read attempts
          * only if this one switches to some "updated" state,
          * all the rest of the set up variables are requested
          * and put into the output buffer*/
         std::string fUpdateFlagRecord;

         /* the id number of the current dataset, i.e. the mbs event number*/
         std::string fIDNumberRecord;

         /* name of the dabc command receiver that shall be informed
          * whenever the PVs are updated*/
         std::string fUpdateCommandReceiver;

         /* contains names of all long integer values to be requested*/
         std::vector<std::string> fLongRecords;

         /* string expression for number of long records. For descriptor block*/
         std::string fNumLongString;

         /* real length of all name strings in long records list*/
         size_t fLongRecordStringsize;

         /* contains names of all double  values to be requested*/
         std::vector<std::string> fDoubleRecords;

         std::string fNumDoubleString;

         /* real length of all name strings in long records list*/
         size_t fDoubleRecordStringsize;



         // question: what kind of possible data types are expected to be put into mbs subevent payload?
         //      ezcaByte
         //      ezcaString
         //      ezcaShort
         //      ezcaLong
         //      ezcaFloat
         //      ezcaDouble
         // for the moment, we restrict to long and double
      public:

         InfoDescriptor()
         {
            Reset();
         }
         virtual ~InfoDescriptor()
         {
            ;
         }

         void Reset()
         {
            fUpdateFlagRecord = "";
            fIDNumberRecord = "";
            fUpdateCommandReceiver="";
            fLongRecords.clear();
            fDoubleRecords.clear();
            fLongRecordStringsize = 0;
            fDoubleRecordStringsize = 0;
         }

         const std::string& GetUpdateRecord()
         {
            return fUpdateFlagRecord; //.c_str();
         }

         void SetUpdateRecord(std::string name)
         {
            fUpdateFlagRecord = name;
         }

         const std::string& GetIDRecord()
         {
            return fIDNumberRecord; //.c_str();
         }

         void SetIDRecord(std::string name)
         {
            fIDNumberRecord = name;
         }

         const std::string& GetUpdateCommandReceiver()
         {
            return fUpdateCommandReceiver; //.c_str();
         }

         void SetUpdateCommandReceiver(std::string name)
         {
            fUpdateCommandReceiver = name;
         }


         void AddLongRecord(std::string name)
         {
            fLongRecords.push_back(name);
            fLongRecordStringsize+=name.size()+1; // account terminating \0 in buffer
            char lbuf[128];
            snprintf(lbuf,128,"%d ", NumLongRecords());
            fNumLongString=lbuf;
         }

         const char* GetLongRecord(size_t i)
         {
            try {
               return fLongRecords.at(i).c_str();
            } catch (...) // for out of range exc etc.
            {
               return 0;
            }
         }

         unsigned int NumLongRecords()
         {
            return fLongRecords.size();
         }

         const std::string & NumLongRecordsString()
            {
               return fNumLongString;
            }

         void AddDoubleRecord(std::string name)
         {
            fDoubleRecords.push_back(name);
            fDoubleRecordStringsize+=name.size()+1; // account terminating \0 in buffer
            char lbuf[128];
            snprintf(lbuf,128,"%d ", NumDoubleRecords());
            fNumDoubleString=lbuf;
         }

         const char* GetDoubleRecord(size_t i)
         {
            try {
               return fDoubleRecords.at(i).c_str();
            } catch (...) // for out of range exc etc.
            {
               return 0;
            }
         }

         unsigned int NumDoubleRecords()
         {
            return fDoubleRecords.size();
         }

         const std::string & NumDoubleRecordsString()
           {
              return fNumDoubleString;
           }

         size_t SizeofDoubleRecords()
         {
            return fDoubleRecordStringsize;
         }

         size_t SizeofLongRecords()
         {
            return fLongRecordStringsize;
         }

   };

   /*
    * The epics easy channel access data input implementation
    *
    */

   class EpicsInput: public dabc::DataInput {
      public:
         EpicsInput(const char* name = 0, uint32_t bufsize = 0x10000);
         virtual ~EpicsInput();

         virtual bool Read_Init(const dabc::WorkerRef& wrk,
               const dabc::Command& cmd);

         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(dabc::Buffer& buf);

         virtual double GetTimeout()
         {
            return fTimeout;
         }

      protected:
         std::string fName;

         /* aggregation of all record names to be read.
          * Configurable from  xml*/
         ezca::InfoDescriptor fInfoDescr;

         /* size of preallocated memory buffer from pool*/
         unsigned int fBufferSize;

         /* timeout (in seconds) for readout polling. */
         double fTimeout;

         /* full id number for epics subevent*/
         unsigned int fSubeventId;

         /* Wrapper for ezca get with long values. Contains error message handling.
          * Returns ezca error code.*/
         int CA_GetLong(char* name, long& val);

         /* Wrapper for ezca get with double values. Contains error message handling
          * Returns ezca error code.*/
         int CA_GetDouble(char* name, double& val);

         bool Close();
   };

}

#endif
