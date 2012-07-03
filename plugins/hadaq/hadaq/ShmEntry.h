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

#ifndef HADAQ_ShmEntry
#define HADAQ_ShmEntry

#ifndef DABC_Parameter
#include "dabc/Parameter.h"
#endif

extern "C" {
   #include "hadaq/worker.h"
}

namespace hadaq {

//   /** type safety for record visibility bits */
//   typedef unsigned int visiblemask;
//
//   /** type safe recordtype definitions. */
//   enum recordtype {
//      ATOMIC      = 0,
//      GENERIC     = 1,
//      STATUS      = 2,
//      RATE        = 3,
//      HISTOGRAM   = 4,
//      MODULE      = 5,
//      PORT        = 6,
//      DEVICE      = 7,
//      QUEUE       = 8,
//      COMMANDDESC = 9,
//      INFO        = 10
//   };
//
//
//   /** type safety for record status definitions */
//   enum recordstat {
//      NOTSPEC    = 0,
//      SUCCESS    = 1,
//      MESSAGE    = 2,
//      WARNING    = 3,
//      ERR        = 4,
//      FATAL      = 5
//   };
//
//
//
//   /** type safety for record mode bits */
//   enum recordmode {
//      NOMODE    = 0 ,
//      ANYMODE   = 1
//   };
//

   class ShmEntry {
      public:


      protected:
         std::string fStatsName;      //!< statistics parameter name in hadaq "worker" segment
         std::string fShmName;        //!< name of shared memory file
         ::Worker* fWorker;           // reference to shmem handle

         unsigned long* fShmPtr;            //!<points to value in shared memory


//         void DeleteService();

         /** \brief Copies dabc parameter value to allocated buffer */
//         void UpdateBufferContent(const dabc::Parameter& par);

         /** \brief Converts DABC command definition in xml format, required by java gui */
//         std::string CommandDefAsXml(const dabc::CommandDefinition& src);

         /** \brief Decode xml string, produced by the java GUI */
//         bool ReadParsFromDimString(dabc::Command& cmd, const dabc::CommandDefinition& def, const char* pars);

      public:

         /** ctor will create service from given dabc parameter*/
         ShmEntry(const std::string& dabcname , const std::string& shmemname, ::Worker* handle);

         virtual ~ShmEntry();

         bool IsStatsName(const std::string& name) const { return fStatsName == name; }
         bool IsShmemName(const std::string& name) const { return fShmName == name; }

         /** Create/update service according parameter */
         //bool UpdateEntry(const dabc::Parameter& par);

         /** Update value of parameter */
         void UpdateValue(const std::string& value);

//         /** Method inherited from DIM to process command, associated with entry */
//         virtual void commandHandler();
//
//
//         void SetQuality(unsigned qual);
//         unsigned GetQuality();
//
//         void SetRecType(hadaq::recordtype t);
//         hadaq::recordtype GetRecType() const { return fRecType; }
//
//         void SetRecVisibility(hadaq::visiblemask mask);
//         hadaq::visiblemask GetRecVisibility() const { return fRecVisibility; }
//
//         void SetRecStat(hadaq::recordstat s);
//         hadaq::recordstat GetRecStat() const { return fRecStat; }
//
//         void SetRecMode(hadaq::recordmode m);
//         hadaq::recordmode GetRecMode() const { return fRecMode; }
   };

}

#endif
