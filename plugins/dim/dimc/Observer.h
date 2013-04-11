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

#ifndef DIMC_Observer
#define DIMC_Observer

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#include "dis.hxx"
#include "dic.hxx"


#include <list>

namespace dimc {

   class Server;

   class ServiceEntry;

   /** Class registers for parameter event from the DABC and export these events to DIM
    *
    */

   class Observer : public dabc::Worker, public ::DimServer  {
      protected:

         typedef std::list<ServiceEntry*> ServiceEntriesList;

         std::string  fDNS;     //!< DIM_DNS_NODE
         unsigned     fDNSport; //!< port, default 2505

         std::string  fDimPrefix; //!< prefix for all DIM services names

         std::string  fERateName; //!< name of ratemeter exported as Rate value
         std::string  fDRateName; //!< name of ratemeter exported as Rate value
         std::string  fInfoName; //!< name of parameter exported as Info value
         std::string  fInfo2Name; //!< name of parameter exported as Info value
         std::string  fInfo3Name; //!< name of parameter exported as Info value

         Server* fServer;  //!< dim server instance

         ServiceEntriesList fEntries;

         ServiceEntry*  FindEntry(const std::string& name);
         bool CreateEntryForParameter(const std::string& parname, const std::string& dimname);
         void RemoveEntry(ServiceEntry* entry);

         /** Return dim name for parameter */
         std::string BuildDIMName(const dabc::Parameter& par, const std::string& fullname);

         dabc::Parameter FindControlObject(const char* name);

         /** Start Dim server */
         void StartDimServer(const std::string& name, const std::string& dnsnode = "", unsigned int dnsport = 0);

         /** Stop dim server */
         void StopDimServer();

         void ExtendRegistrationFor(const std::string& mask, const std::string& name);

      public:
         Observer(const std::string& name);

         virtual ~Observer();

         virtual const char* ClassName() const { return "Observer"; }

         virtual void ProcessParameterEvent(const dabc::ParameterEvent& evnt);

         virtual void errorHandler(int severity, int code, char *msg);
         virtual void exitHandler( int code );
         virtual void clientExitHandler();

         static const char* RunStatusName() { return "RunStatus"; }
   };

}

#endif
