// $Id$

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

#ifndef DIMC_Player
#define DIMC_Player

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

#include <map>

#include <dic.hxx>

class DimBrowser;
class DimInfo;

namespace dimc {

   /** \brief Player of DIM data
    *
    * Module builds hierarchy for discovered DIM variables
    *
    **/

   class Player : public dabc::ModuleAsync,
                  public DimInfoHandler {

      protected:

         struct MyEntry {
            DimInfo* info;
            int   flag;
            MyEntry() : info(0), flag(0) {}
            MyEntry(const MyEntry& src) : info(src.info), flag(src.flag) {}
         };

         typedef std::map<std::string, MyEntry> DimServicesMap;

         std::string    fDimDns;      ///<  name of DNS server
         std::string    fDimMask;     ///<  mask of DIM services
         double         fDimPeriod;   ///<  how often DIM records will be checked, default 1 s
         ::DimBrowser*  fDimBr;       ///<  dim browser
         DimServicesMap fDimInfos;    ///< all subscribed DIM services
         dabc::TimeStamp fLastScan;   ///< last time when DIM services were scanned
         char           fNoLink[10];  ///< buffer used to detect nolink
         bool           fNeedDnsUpdate; ///< if true, should update DNS structures


         virtual void OnThreadAssigned();

         void ScanDimServices();

      public:
         Player(const std::string& name, dabc::Command cmd = 0);
         virtual ~Player();

         virtual void ProcessTimerEvent(unsigned timer);

         virtual int ExecuteCommand(dabc::Command cmd);

         virtual void infoHandler();

   };
}


#endif
