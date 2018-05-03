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

#ifndef HADAQ_SorterModule
#define HADAQ_SorterModule

#ifndef DABC_ModuleAsync
#include "dabc/ModuleAsync.h"
#endif

#ifndef DABC_Pointer
#include "dabc/Pointer.h"
#endif

#include <vector>

namespace hadaq {


/** \brief Sorts HADAQ subevents according to trigger number
 *
 * Need to be applied when TRB send provides events not in order they appear
 * Or when network adapter provides UDP packets not in order
 */

   class SorterModule : public dabc::ModuleAsync {

   public:
         struct SubsRec {
            void*     subevnt;  //!< direct pointer on subevent
            uint32_t  trig;     //!< trigger number
            uint32_t  buf;      //!< buffer indx
            uint32_t  sz;       //!< padded size
         };

         struct SubsComp {
            SorterModule* m;
            SubsComp(SorterModule* _m) : m(_m) {}
            // use in std::sort for sorting elements of std::vector<SubsRec>
            bool operator() (const SubsRec& l,const SubsRec& r) { return m->Diff(l.trig, r.trig) > 0; }
         };


      int       fFlushCnt;
      int       fBufCnt;          //!< total number of buffers
      int       fLastRet;         //!< debug
      uint32_t  fTriggersRange;   //!< valid range for the triggers, normally 0x1000000
      uint32_t  fLastTrigger;     //!< last trigger copied into output
      unsigned  fNextBufIndx;     //!< next buffer which could be processed
      unsigned  fReadyBufIndx;    //!< input buffer index which could be send directly
      std::vector<SubsRec> fSubs; //!< vector with subevents data in the buffers
      dabc::Buffer fOutBuf;       //!< output buffer
      dabc::Pointer fOutPtr;      //!< place for new data

      void DecremntInputIndex(unsigned cnt=1);

      bool RemoveUsedSubevents(unsigned num);

      bool retransmit();

      virtual int ExecuteCommand(dabc::Command cmd);

   public:
      SorterModule(const std::string& name, dabc::Command cmd = nullptr);

      virtual bool ProcessBuffer(unsigned) { return retransmit(); }

      virtual bool ProcessRecv(unsigned) { return retransmit(); }

      virtual bool ProcessSend(unsigned) { return retransmit(); }

      virtual void ProcessTimerEvent(unsigned);

      int Diff(uint32_t trig1, uint32_t trig2)
      {
         int res = (int) (trig2) - trig1;
         if (res > ((int) fTriggersRange)/2) return res - fTriggersRange;
         if (res < ((int) fTriggersRange)/-2) return res + fTriggersRange;
         return res;
      }


   };

}


#endif
