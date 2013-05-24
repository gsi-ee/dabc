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

#ifndef DIMC_ServiceEntry
#define DIMC_ServiceEntry

#include "dis.hxx"
#include "dic.hxx"

#ifndef DABC_Parameter
#include "dabc/Parameter.h"
#endif

namespace dimc {

   /** type safety for record visibility bits */
   typedef unsigned int visiblemask;

   /** type safe recordtype definitions. */
   enum recordtype {
      ATOMIC      = 0,
      GENERIC     = 1,
      STATUS      = 2,
      RATE        = 3,
      HISTOGRAM   = 4,
      MODULE      = 5,
      PORT        = 6,
      DEVICE      = 7,
      QUEUE       = 8,
      COMMANDDESC = 9,
      INFO        = 10
   };


   /** type safety for record status definitions */
   enum recordstat {
      NOTSPEC    = 0,
      SUCCESS    = 1,
      MESSAGE    = 2,
      WARNING    = 3,
      ERR        = 4,
      FATAL      = 5
   };


   /** type safety for record mode bits */
   enum recordmode {
      NOMODE    = 0 ,
      ANYMODE   = 1
   };


   /** \brief Entry for each variable which is exported to DIM control  */
   class ServiceEntry : public ::DimCommandHandler {
      public:
         enum EServiceKind {
            kindNone,           ///< unspecified kind
            kindInt,            ///< integer
            kindDouble,         ///< double
            kindString,         ///< string
            kindRate,           ///< ratemeter
            kindStatus,         ///< status record
            kindInfo,           ///< info record
            kindCommand         ///< command + definition
         };


      protected:
         std::string fDabcName;    ///< parameter name in DABC

         std::string fDimName;     ///< parameter name in DIM

         ::DimService* fService;   ///< service associated with parameter/data reference

         ::DimCommand* fCmd;       ///< dim command

         void* fBuffer;            ///< allocated space for dim variable

         unsigned fBufferLen;      ///< size of allocated buffer

         EServiceKind  fKind;      ///< kind of the server

         recordtype   fRecType;
         recordstat   fRecStat;
         visiblemask  fRecVisibility;
         recordmode   fRecMode;

         void DeleteService();

         /** \brief Copies dabc parameter value to allocated buffer */
         void UpdateBufferContent(const dabc::Parameter& par);

         /** \brief Converts DABC command definition in xml format, required by java gui */
         std::string CommandDefAsXml(const dabc::CommandDefinition& src);

         /** \brief Decode xml string, produced by the java GUI */
         bool ReadParsFromDimString(dabc::Command& cmd, const dabc::CommandDefinition& def, const char* pars);

      public:

         /** ctor will create service from given dabc parameter*/
         ServiceEntry(const std::string& dabcname, const std::string& dimname);

         virtual ~ServiceEntry();

         bool IsDabcName(const std::string& name) const { return fDabcName == name; }

         /** Create/update service according parameter */
         bool UpdateService(const dabc::Parameter& par, EServiceKind kind = kindNone );

         /** Update value of parameter */
         void UpdateValue(const std::string& value);

         /** Method inherited from DIM to process command, associated with entry */
         virtual void commandHandler();


         void SetQuality(unsigned qual);
         unsigned GetQuality();

         void SetRecType(dimc::recordtype t);
         dimc::recordtype GetRecType() const { return fRecType; }

         void SetRecVisibility(dimc::visiblemask mask);
         dimc::visiblemask GetRecVisibility() const { return fRecVisibility; }

         void SetRecStat(dimc::recordstat s);
         dimc::recordstat GetRecStat() const { return fRecStat; }

         void SetRecMode(dimc::recordmode m);
         dimc::recordmode GetRecMode() const { return fRecMode; }
   };

}

#endif
