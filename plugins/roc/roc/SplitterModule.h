/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/

#ifndef ROC_SPLITTERMODULE_H
#define ROC_SPLITTERMODULE_H

#include "dabc/ModuleAsync.h"
#include "dabc/Pointer.h"
#include "dabc/timing.h"

#include "roc/BoardsVector.h"

#include <map>

namespace roc {

   class SplitterModule : public dabc::ModuleAsync {

      protected:

         struct OutputRec {

            unsigned           nout;
            dabc::Buffer       buf;
            dabc::Pointer      ptr;
            unsigned           len;
            dabc::TimeStamp    lasttm;
            bool               wassent;

            OutputRec() :
               nout(0), buf(), ptr(), len(0), lasttm(), wassent(false) {}

            OutputRec(const OutputRec& r) :
               nout(r.nout), buf(r.buf), ptr(r.ptr), len(r.len), lasttm(r.lasttm), wassent(r.wassent) {}
         };

         typedef std::map<unsigned, OutputRec> OutputMap;

         virtual int ExecuteCommand(dabc::Command cmd);

         bool FlushNextBuffer();
         void CheckBuffersFlush(bool forceunsent = false);

         double               fFlushTime;
         OutputMap            fMap;

      public:

         SplitterModule(const char* name, dabc::Command cmd);
         virtual ~SplitterModule();

         virtual void BeforeModuleStart();
         virtual void AfterModuleStop();

         virtual bool ProcessRecv(unsigned port);

         virtual bool ProcessSend(unsigned port);

         virtual void ProcessTimerEvent(unsigned timer);

   };
}

#endif
