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

#ifndef MBS_EventAPI
#define MBS_EventAPI

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef DABC_string
#include "dabc/string.h"
#endif


// This is wrapper for standard MBS event API

namespace mbs {

   // this is name of configuration item with type of event api data source
   extern const char* xmlEvapiType;

   // following 6 types are valid values for xmlEvapiType
   extern const char* xmlEvapiFile;
   extern const char* xmlEvapiRFIOFile;
   extern const char* xmlEvapiTransportServer;
   extern const char* xmlEvapiStreamServer;
   extern const char* xmlEvapiEventServer;
   extern const char* xmlEvapiRemoteEventServer;

   extern const char* xmlEvapiSourceName;            // name of file, server and so on
   extern const char* xmlEvapiTagFile;               // additional for xmlEvapiFile
   extern const char* xmlEvapiRemoteEventServerPort; // additional for xmlEvapiRemoteEventServer
   extern const char* xmlEvapiTimeout;               // additional for sockets

   extern const char* xmlEvapiOutFile;
   extern const char* xmlEvapiOutFileName;

   class EvapiInput : public dabc::DataInput {
      public:
         EvapiInput(const char* kind = 0, const char* name = 0);
         virtual ~EvapiInput();

         virtual bool Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd);

         void SetTimeout(int timeout = -1) { fTimeout = timeout; }

         bool OpenFile(const char* fname, const char* tagfile = 0);
         bool OpenRemoteEventServer(const char* name, int port);
         bool OpenStreamServer(const char* name);
         bool OpenEventServer(const char* name);
         bool OpenTransportServer(const char* name);
         bool OpenRFIOFile(const char* name);
         bool Close();

         void* GetEventBuffer();
         unsigned GetEventBufferSize();

         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(dabc::Buffer& buf);

      protected:

         /** Kind of event api input  */
         std::string      fKind;

         /** Name of event api input  */
         std::string      fName;

         std::string      fTagFile;

         int              fRemotePort;

         /** Timeout in seconds for mbs getevent. If -1, no timeout (default)  */
         int fTimeout;

         bool Open(int mode, const char* fname);

         int fMode;  //   Mode of operation, -1 - not active

         /** Event channel structure used by event source. */
         void* fChannel; // s_evt_channel*

         void* fEventHeader;  // s_ve10_1*

         /** Points to the current gsi buffer structure filled by the event source. */
         void* fBufferHeader; // s_bufhe*

         /** Reference to header info delivered by source. */
         char* fInfoHeader; //  s_filhe*

         /** Indicates, if input file stream is normally finished
           * One can only close input at the end */
         bool fFinished;
   };

   class EvapiOutput : public dabc::DataOutput {
      public:

         EvapiOutput(const char* fname = 0);
         virtual ~EvapiOutput();

         virtual bool Write_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd);

         bool CreateOutputFile(const char* name);
         bool Close();

         virtual bool WriteBuffer(const dabc::Buffer& buf);

      protected:

         std::string fFileName;

         int fMode; // current working mode, -1 - not active

         void* fChannel; // s_evt_channel*
   };

}

#endif
