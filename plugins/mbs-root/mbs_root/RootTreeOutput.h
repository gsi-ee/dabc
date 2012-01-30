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

#include "Riostream.h"
#include "Rtypes.h"

// This is class to access TTree interface from the DABC

class TTree;
class TFile;


namespace mbs_root {

    class DabcEvent;

   class RootTreeOutput : public dabc::DataOutput {
      public:

         RootTreeOutput();
         virtual ~RootTreeOutput();

         virtual bool Write_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd);

         virtual bool WriteBuffer(const dabc::Buffer& buf);

      protected:

         TTree*       fTree;
	 TFile*		fFile;
	 mbs_root::DabcEvent* fEvent;

	std::string fFileName;
	Int_t 		fSplit;	
	Int_t 		fTreeBuf;
	Int_t		fCompression;	
	Int_t		fMaxSize;

	bool Close();
	
	//mbs_root::ReadIterator	iter(buf);
   };

}

#endif
#endif
