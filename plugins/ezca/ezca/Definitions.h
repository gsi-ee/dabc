#ifndef EZCA_DEFINITIONS_H
#define EZCA_DEFINITIONS_H


// this is switch to put double values instead of scaled integers
#define EZCA_useDOUBLES 1

// scaling factor for double values when converted to ints
#define EZCA_DOUBLESCALE 1000000000


#define EZCA_typeEpicsInput 	  "ezca::EpicsInput"
#define EZCA_nameReadoutAppClass  "ezca::Readout"
#define EZCA_nameReadoutModuleClass "ezca::MonitorModule"

#define EZCA_nameUpdateCommand "ezca::OnUpdate"


#define EZCA_xmlEpicsName "EpicsIdentifier"

#define EZCA_xmlUpdateFlagRecord "EpicsFlagRec"

#define EZCA_xmlEventIDRecord "EpicsEventIDRec"

#define EZCA_xmlNumLongRecords "EpicsNumLongRecs"

#define EZCA_xmlNameLongRecords "EpicsLongRec-"

#define EZCA_xmlNumDoubleRecords "EpicsNumDoubleRecs"

#define EZCA_xmlNameDoubleRecords "EpicsDoubleRec-"

#define EZCA_xmlTimeout "EpicsPollingTimeout"

#define EZCA_xmlEpicsSubeventId "EpicsSubeventId"

#define EZCA_xmlModuleName "EpicsModuleName"
#define EZCA_xmlModuleThread "EpicsModuleThread"

#define EZCA_xmlCommandReceiver "EpicsDabcCommandReceiver"

namespace ezca {

/*
 * Common definition of string constants to be used in factory/application.
 * Implemented in Factory.cxx
 * */

   extern const char* typeEpicsInput;
   extern const char* nameReadoutAppClass;
   extern const char* nameReadoutModuleClass;
   extern const char* nameUpdateCommand;

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

   /* full subevent id for epics data*/
   extern const char* xmlEpicsSubeventId;

   extern const char* xmlModuleName;
   extern const char* xmlModuleThread;


   extern const char* xmlCommandReceiver;


   /* duplicate here identifiers of other mbs payloads*/
//   enum EzcaMbsTypes {
//            proc_RocEvent     = 1,   //!< complete event from one roc board (iSubcrate = rocid)
//            proc_ErrEvent     = 2,   //!< one or several events with corrupted data inside (iSubcrate = rocid)
//            proc_MergedEvent  = 3,   //!< sorted and synchronized data from several rocs (iSubcrate = upper rocid bits)
//            proc_RawData      = 4,   //!< unsorted uncorrelated data from one ROC, no SYNC markers required (mainly for FEET case)
//            proc_Triglog      = 5,   //!< subevent produced by MBS directly with sync number and trigger module scalers
//            proc_TRD_MADC     = 6,   //!< subevent produced by MBS directly with MADC data
//            proc_TRD_Spadic   = 7,    //!< collection of data from susibo board
//            proc_EPICS_Mon    = 10,    //!< epics monitor data
//         };

}

#endif
