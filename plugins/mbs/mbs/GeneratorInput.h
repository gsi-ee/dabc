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

#ifndef MBS_GeneratorInput
#define MBS_GeneratorInput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

namespace mbs {

   /** \brief Input object for MBS-events generator */

   class GeneratorInput : public dabc::DataInput {
      protected:
         uint32_t    fEventCount;          ///< current event id
         uint16_t    fNumSubevents;        ///< number of subevents to generate
         uint16_t    fFirstProcId;         ///< procid of first subevent
         uint32_t    fSubeventSize;        ///< size of each subevent
         bool        fIsGo4RandomFormat;   ///< is subevents should be filled with random numbers
         uint32_t    fFullId;              ///< subevent id, if number subevents==1 and nonzero
         uint64_t    fTotalSize;           ///< total size of generated events
         uint64_t    fTotalSizeLimit;      ///< limit of generated events size

      public:
         GeneratorInput(const dabc::Url& url);
         virtual ~GeneratorInput() {}

         virtual bool Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd);

         virtual unsigned Read_Size();

         virtual unsigned Read_Complete(dabc::Buffer& buf);
   };

}


#endif
