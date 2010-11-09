#ifndef EZCA_DEFINITIONS_H
#define EZCA_DEFINITIONS_H





namespace ezca {

/*
 * Common definition of string constants to be used in factory/application.
 * Implemented in Factory.cxx
 * */

   extern const char* typeEpicsInput;
   extern const char* nameReadoutAppClass;
   extern const char* nameReadoutModuleClass;

   /* Name of record containing the ioc identifier name- TODO Do we need this?*/
   extern const char* xmlEpicsName;

   /* Name of record containing the update flag*/
   extern const char* xmlUpdateFlagRecord;

   /* Name of record containing the id number*/
   extern const char* xmlEventIDRecord;


   /* Number of records with longword values to fetch*/
   extern const char* xmlNumLongRecords;

   /* Name prefix of longword records, will be completed by number*/
   extern const char* xmlNameLongRecords;

   /* Number of records with double values to fetch*/
   extern const char* xmlNumDoubleRecords;

   /* Name prefix of double records, will be completed by number*/
   extern const char* xmlNameDoubleRecords;

   /* Timeout for update polling loop*/
   extern const char* xmlTimeout;

   extern const char* xmlModuleName;
   extern const char* xmlModuleThread;


   /* duplicate here identifiers of other mbs payloads*/
   enum EzcaMbsTypes {
            proc_RocEvent     = 1,   //!< complete event from one roc board (iSubcrate = rocid)
            proc_ErrEvent     = 2,   //!< one or several events with corrupted data inside (iSubcrate = rocid)
            proc_MergedEvent  = 3,   //!< sorted and synchronized data from several rocs (iSubcrate = upper rocid bits)
            proc_RawData      = 4,   //!< unsorted uncorrelated data from one ROC, no SYNC markers required (mainly for FEET case)
            proc_Triglog      = 5,   //!< subevent produced by MBS directly with sync number and trigger module scalers
            proc_TRD_MADC     = 6,   //!< subevent produced by MBS directly with MADC data
            proc_TRD_Spadic   = 7,    //!< collection of data from susibo board
            proc_EPICS_Mon    = 8,    //!< epics monitor data
         };

}

#endif
