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

#ifndef SAFT_DEFINITIONS_H
#define SAFT_DEFINITIONS_H


/** JAM we use the same definitions from saft-io-ctl. Is there any include file here? */
#define ECA_EVENT_ID_LATCH     UINT64_C(0xfffe000000000000) /* FID=MAX & GRPID=MAX-1 */
#define ECA_EVENT_MASK_LATCH   UINT64_C(0xfffe000000000000)
#define IO_CONDITION_OFFSET    5000


/** this marks conditions that are not input conditions:*/
#define NON_IO_CONDITION_LABEL "WR_Event"

/** from eca_flags.h not yet installed to saftlib system includes:*/
#define ECA_LATE      0
#define ECA_EARLY     1
#define ECA_CONFLICT  2
#define ECA_DELAYED   3
#define ECA_VALID     4
#define ECA_OVERFLOW  5
#define ECA_MAX_FULL  6

#define ECA_LINUX         0
#define ECA_WBM           1
#define ECA_EMBEDDED_CPU  2
#define ECA_SCUBUS        3

/** these flags are dabc proprietary up to now, since saftlib uses separate boolean setters:*/
#define SAFT_DABC_ACCEPT_LATE      (1 << ECA_LATE)
#define SAFT_DABC_ACCEPT_EARLY     (1 << ECA_EARLY)
#define SAFT_DABC_ACCEPT_CONFLICT  (1 << ECA_CONFLICT)
#define SAFT_DABC_ACCEPT_DELAYED   (1 << ECA_DELAYED)

/** length of descriptor text field in timing event structure*/
#define SAFT_DABC_DESCRLEN 16

/** tag the timing events with special trigger type. configurable later?*/
#define SAFT_DABC_TRIGTYPE 0xA



namespace saftdabc {

/*
 * Common definition of string constants to be used in factory/application.
 * Implemented in Factory.cxx
 * */

/** name of the saft device, e.g. "baseboard"*/
extern const char* xmlDeviceName;

/** EXPLODER input name items that should be latched with timestamp*/
extern const char* xmlInputs;

/** WR event masks to snoop*/
extern const char* xmlMasks;

/** WR event ids to snoop*/
extern const char* xmlEventIds;

/** WR event offsets to snoop*/
extern const char* xmlOffsets;

/** WR event accept flags to snoop*/
extern const char* xmlAcceptFlags;


/** time out for polling mode*/
extern const char* xmlTimeout;

/** switch between polling for data or callback mode*/
extern const char* xmlCallbackMode;

/** switch between silent or verbose event receiver  mode*/
extern const char* xmlSaftVerbose;

/* full subevent id for timestamp data*/
extern const char* xmlSaftSubeventId;

/* switch to fill single timing event for each (MBS or hadaq) Event container*/
extern const char* xmlSaftSingleEvent;

/* specify output event format: mbs, hadaq, or raw*/
extern const char* xmlEventFormat;

/** Command to invoke the glib/dbus mainloop*/
extern const char* commandRunMainloop;

/** Name of event rate parameter*/
extern const char* parEventRate;

/** \brief The saftlib input event data structure.
 */
class Timing_Event {

 public:

   uint64_t fEvent;
   uint64_t fParam;
   uint64_t fDeadline;
   uint64_t fExecuted;
   uint64_t fFlags;
   uint64_t fOverflows;
   char     fDescription[SAFT_DABC_DESCRLEN]; //< specifies input type. we use simple char field for mbs buffers etc.

   Timing_Event (uint64_t event = 0, uint64_t param = 0, uint64_t deadline = 0, uint64_t executed = 0,
       uint64_t flags = 0, uint64_t overflows = 0,const char *description = nullptr) :
       fEvent (event), fParam (param), fDeadline (deadline), fExecuted (executed), fFlags (flags), fOverflows(overflows)
   {
     if(description)
       snprintf(fDescription, SAFT_DABC_DESCRLEN-2, "%s",description);
   }


   int InfoMessage(char* str, size_t len)
   {
      if(!str) return -1;
      return snprintf(str, len, "saftdabc::Timing_Event - event=0x%lx, param=0x%lx , deadline=0x%lx, executed=0x%lx, flags=0x%lx, overflows=0x%lx, description:%s",
          fEvent, fParam , fDeadline, fExecuted, fFlags, fOverflows, fDescription);

   }

 };

}

#endif
