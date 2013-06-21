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

#ifndef DABC_ROOT_RootSniffer
#define DABC_ROOT_RootSniffer

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

class TDirectory;
class TBufferFile;

namespace dabc_root {

   /** \brief %RootBinDataContainer provides access to ROOT TBuffer object for DABC
    *
    */

   class RootBinDataContainer : public dabc::BinDataContainer {
      protected:
         TBufferFile* fBuf;
      public:
         RootBinDataContainer(TBufferFile* buf);
         virtual ~RootBinDataContainer();

         virtual void* data() const;
         virtual unsigned length() const;
   };




   /** \brief %RootSniffer provides access to ROOT objects for DABC
    *
    */

   class RootSniffer : public dabc::Worker  {

      protected:
         bool fEnabled;

         /** \brief Complete snapshot of ROOT hierarchy
          *  \details Will be updated from ROOT main loop  */
         dabc::Hierarchy fHierarchy;

         dabc::Mutex fHierarchyMutex; ///< used to protect content of hierarchy

         virtual void OnThreadAssigned();

         virtual double ProcessTimeout(double last_diff);

         void FillHieararchy(dabc::Hierarchy& h, TDirectory* dir);

         virtual int ExecuteCommand(dabc::Command cmd);

      public:
         RootSniffer(const std::string& name);

         virtual ~RootSniffer();

         virtual const char* ClassName() const { return "RootSniffer"; }

         bool IsEnabled() const { return fEnabled; }

         /** Should deliver hierarchy of the ROOT objects */
         virtual void BuildHierarchy(dabc::HierarchyContainer* cont);

   };

}

#endif
