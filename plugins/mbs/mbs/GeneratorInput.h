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

#ifndef MBS_GeneratorInput
#define MBS_GeneratorInput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

namespace mbs {

   /** \brief Input object for MBS-events generator
    *
    * It can generate MBS events for testing. Configuration in xml file look like:
    * ~~~{.xml}
    * <InputPort name="Input0" url="lmd://Generator?size=32&numsub=2&tmout=10" queue="5"/>
    * ~~~
    *
    * Following parameters are supported:
    *   numsub  - number of subevents
    *   procid  - procid of first subevent
    *   size    - size of each subevent
    *   go4     - if true (default), generates go4-specific data, used in all examples
    *   fullid  - fullid of first subevent
    *   total   - size limit of generated events in MB
    *   tmout   - delay between two generated buffers (in ms)
    * */

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
         double      fGenerTimeout;        ///< timeout used to avoid 100% CPU load
         bool        fTimeoutSwitch;       ///< boolean used to generate timeouts

      public:
         GeneratorInput(const dabc::Url& url);
         virtual ~GeneratorInput() {}

         virtual bool Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd);

         virtual unsigned Read_Size();

         virtual unsigned Read_Complete(dabc::Buffer& buf);

         virtual double Read_Timeout() { return fGenerTimeout; }

   };

}


#endif
