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
#ifndef DABC_RootTreeOutput
#define DABC_RootTreeOutput

#ifndef __CINT__

//#ifndef DABC_DataIO
#include "dabc/DataIO.h"
//#endif

class TTree;

namespace mbs_root {

    class DabcEvent;

   class RootTreeOutput : public dabc::FileOutput {
      public:

         RootTreeOutput(const dabc::Url& url);
         virtual ~RootTreeOutput();

         virtual bool Write_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd);

         virtual unsigned Write_Buffer(dabc::Buffer& buf);

      protected:

         TTree*         fTree;
	 mbs_root::DabcEvent* fEvent;

	 int 		fSplit;	
	 int 		fTreeBuf;
	 int		fCompression;	
	 int		fMaxSize;

	 bool Close();
	
	//mbs_root::ReadIterator	iter(buf);
   };

}

#endif
#endif
