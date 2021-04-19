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
         std::string fStoreInfo;   ///<! last info about storage
         bool fSortOrder;          ///<! sorting order
         int  fDefaultFill;        ///<! default fill color

         bool ClearHistogram(dabc::Hierarchy& item);

         dabc::Hierarchy FindHistogram(void *handle);

         bool ClearAllDabcHistograms(dabc::Hierarchy &folder);
         bool SaveAllHistograms(dabc::Hierarchy &folder);

      public:
         DabcProcMgr();
         virtual ~DabcProcMgr();

         void SetTop(dabc::Hierarchy& top, bool withcmds = false);

         void SetDefaultFill(int fillcol = 3) { fDefaultFill = fillcol; }

         bool IsWorking() const { return fWorkingFlag; }

         // redefine only make procedure, fill and clear should work
         base::H1handle MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* xtitle = nullptr) override;

         base::H2handle MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options = nullptr) override;

         void SetH1Title(base::H1handle h1, const char* title) override;
         void TagH1Time(base::H1handle h1) override;

         void SetH2Title(base::H2handle h2, const char* title) override;
         void TagH2Time(base::H2handle h2) override;

         virtual void ClearAllHistograms();

         void SetSortedOrder(bool on = true)  override { fSortOrder = on; }
         bool IsSortedOrder() override { return fSortOrder; }

         void AddRunLog(const char *msg) override;
         void AddErrLog(const char *msg) override;
         bool DoLog()  override { return true; }
         void PrintLog(const char *msg) override;

         bool CallFunc(const char* funcname, void* arg) override;

         bool CreateStore(const char* storename) override;
         bool CloseStore() override;

         bool CreateBranch(const char* name, const char* class_name, void** obj) override;
         bool CreateBranch(const char* name, void* member, const char* kind) override;

         bool StoreEvent() override;

         bool ExecuteHCommand(dabc::Command cmd);

         bool SaveAllHistograms() { return SaveAllHistograms(fTop); }

         std::string GetStoreInfo() const { return fStoreInfo; }
   };

}

#endif
