#ifndef EZCA_DEFINITIONS_H
#define EZCA_DEFINITIONS_H


// this is switch to put double values instead of scaled integers
#define EZCA_useDOUBLES 1

// scaling factor for double values when converted to ints
#define EZCA_DOUBLESCALE 1000000000


namespace ezca {

/*
 * Common definition of string constants to be used in factory/application.
 * Implemented in Factory.cxx
 * */

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

   /** timeout for ezca operation */
   extern const char* xmlEzcaTimeout;

   /** retry counter for ezca operation */
   extern const char* xmlEzcaRetryCount;

   /** retry counter for ezca operation */
   extern const char* xmlEzcaDebug;

   /** indicates if error should be automatically printed */
   extern const char* xmlEzcaAutoError;

   extern const char* xmlCommandReceiver;
   extern const char* nameUpdateCommand;


}

#endif
