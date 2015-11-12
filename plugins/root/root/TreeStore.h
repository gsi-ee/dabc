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

#ifndef ROOT_TreeStore
#define ROOT_TreeStore

#ifndef DABC_Worker
#include "dabc/Worker.h"
#endif

class TFile;
class TTree;

namespace root {

   class TreeStore : public dabc::LocalWorker {
      protected:

         TFile* fFile;
         TTree* fTree;

         void CloseTree();

         virtual int ExecuteCommand(dabc::Command);

      public:
         TreeStore(const std::string& name);
         virtual ~TreeStore();

   };
}

#endif
