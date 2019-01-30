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

#ifndef STREAM_DabcProcMgr
#define STREAM_DabcProcMgr

#include "base/ProcMgr.h"

#include "dabc/Hierarchy.h"
#include "dabc/Command.h"
#include "dabc/Worker.h"

namespace stream {

   class DabcProcMgr : public base::ProcMgr {
      protected:

         dabc::Hierarchy fTop;
         bool fWorkingFlag;

         dabc::LocalWorkerRef fStore;
         std::string fStoreInfo;  //!< last info about storage

         bool ClearHistogram(dabc::Hierarchy& item);

         bool ClearAllHistograms(dabc::Hierarchy &folder);
         bool SaveAllHistograms(dabc::Hierarchy &folder);

      public:
         DabcProcMgr();
         virtual ~DabcProcMgr();

         void SetTop(dabc::Hierarchy& top, bool withcmds = false);

         bool IsWorking() const { return fWorkingFlag; }

         // redefine only make procedure, fill and clear should work
         virtual base::H1handle MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle = 0);

         virtual base::H2handle MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options = 0);

         virtual void SetH1Title(base::H1handle h1, const char* title);

         virtual void SetH2Title(base::H2handle h2, const char* title);


         virtual void AddRunLog(const char *msg);
         virtual void AddErrLog(const char *msg);
         virtual bool DoLog() { return true; }

         virtual bool CallFunc(const char* funcname, void* arg);

         virtual bool CreateStore(const char* storename);
         virtual bool CloseStore();

         virtual bool CreateBranch(const char* name, const char* class_name, void** obj);
         virtual bool CreateBranch(const char* name, void* member, const char* kind);

         virtual bool StoreEvent();

         bool ExecuteHCommand(dabc::Command cmd);

         std::string GetStoreInfo() const { return fStoreInfo; }
   };

}

#endif
