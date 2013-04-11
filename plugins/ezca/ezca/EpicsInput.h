#ifndef EZCA_EPICSINPUT_H
#define EZCA_EPICSINPUT_H

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#include "dabc/logging.h"

#include <string>
#include <vector>
#include <stdint.h>


namespace ezca {

   /*
    * The epics easy channel access data input implementation
    *
    */

   class EpicsInput: public dabc::DataInput {
      protected:
         std::string fName;

         /* timeout (in seconds) for readout polling. */
         double fTimeout;

         /* full id number for epics subevent*/
         unsigned fSubeventId;

         /** Remember previous flag value which should be change to start readout */
         long fLastFlagValue;

         /** Event number, read from EPICS record */
         long fEventNumber;

         /** Counter of events, used as mbs event id when event number is not provided */
         long fCounter;

         /** Timeout for ezca readout */
         double fEzcaTimeout;

         /** Number of retry in ezca readout */
         int fEzcaRetryCnt;

         /** Switch on/off epics debug messages */
         bool fEzcaDebug;

         /** Automatic error printing */
         bool fEzcaAutoError;

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
         std::vector<long> fLongValues;

         /* contains names of all double  values to be requested*/
         std::vector<std::string> fDoubleRecords;
         std::vector<double> fDoubleValues;

         /** Complete descriptor of long/double variables, packed into mbs event */
         std::string fDescriptor;

         void ResetDescriptors()
         {
            fUpdateFlagRecord = "";
            fIDNumberRecord = "";
            fUpdateCommandReceiver="";
            fLongRecords.clear();
            fLongValues.clear();
            fDoubleRecords.clear();
            fDoubleValues.clear();
            fDescriptor.clear();
         }


         void AddLongRecord(const std::string& name)
         {
            fLongRecords.push_back(name);
            fLongValues.push_back(0);
            fDescriptor.clear();
         }

         unsigned NumLongRecords() const { return fLongRecords.size(); }

         const std::string& GetLongRecord(unsigned i) const { return fLongRecords[i]; }

         void AddDoubleRecord(const std::string& name)
         {
            fDoubleRecords.push_back(name);
            fDoubleValues.push_back(0.);
            fDescriptor.clear();
         }

         unsigned NumDoubleRecords() const { return fDoubleRecords.size(); }

         const std::string& GetDoubleRecord(unsigned i) const { return fDoubleRecords[i]; }

         void BuildDescriptor();

         /* Wrapper for ezca get with long values. Contains error message handling.
          * Returns ezca error code.*/
         int CA_GetLong(const std::string& name, long& val);

         /* Wrapper for ezca get with double values. Contains error message handling
          * Returns ezca error code.*/
         int CA_GetDouble(const std::string& name, double& val);

         /** Return error string with error description */
         std::string CA_ErrorString();

         const char* CA_RetCode(int ret);

         bool Close();

      public:
         EpicsInput(const std::string& name = "");
         virtual ~EpicsInput();

         virtual bool Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd);

         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(dabc::Buffer& buf);

         virtual double Read_Timeout() { return fTimeout; }
   };

}

#endif
