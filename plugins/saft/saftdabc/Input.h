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

#ifndef SAFT_INPUT_H
#define SAFT_INPUT_H

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#include "dabc/logging.h"

#include <string>
#include <queue>
#include <cstdint>

#include <iostream>
//#include <iomanip>



#include "saftdabc/Device.h"
#include "saftdabc/Definitions.h"

#ifndef DABC_SAFT_USE_2_0
#include <giomm.h>
#include <glib.h>
#endif


namespace saftdabc
{

//class DeviceRef;


enum EventFormats_t
{
  saft_Format_Raw,
  saft_Format_Mbs,
  saft_Format_Hadaq
};



/** \brief The saftlib timing message input implementation for DABC.
 * Inputs to snoop/latch timestamps can be configured. When a signal occurs, timing message is captured
 * and will be transported in mbs or hadaq subevent container.
 * Alternatively, WR events may be captured by event id and mask
 */

class Input: public dabc::DataInput
{

private:



  /** this contains single queue for all snooped WR events.
   * events are queued until read out by dabc*/
  std::queue<saftdabc::Timing_Event> fTimingEventQueue;


  /** Mutex to protect timingeventqueue*/
  dabc::Mutex fQueueMutex;

  /** flag to avoid invoking transport callback before transport done*/
  bool fWaitingForCallback;


  /** current overflow counter of all configured inputs*/
  uint64_t fOverflowCount;

  /** previous overflow counter of all configured inputs*/
  uint64_t fLastOverflowCount;

protected:

  /** use dabc reference instead of direct pointer here.*/
  DeviceRef fDevice;

  /** worker that is running this data input. Should be the InputTransport assigned*/
  dabc::Reference fTransport;

  /** timeout (in seconds) for readout polling. */
  double fTimeout;

  /** switch to work with time out polling or transport call back mode*/
  bool fUseCallbackMode;

  /** full mbs id number for saft subevent*/
  unsigned fSubeventId;

  /** Event number*/
  long fEventNumber;

  /** verbose mode for timing events*/
  bool fVerbose;

  /** single events for each mbs container*/
  bool fSingleEvents;

  /** */
  EventFormats_t fEventFormat;

  /** contains names of all hardware inputs to be latched*/
  std::vector<std::string> fInput_Names;

//  /** Translation table IO name <> prefix
//   * like in  saft-io-ctl implementation*/
//  std::map<Glib::ustring, guint64> fMap_PrefixName;
//

  /** contains masks of all events to be snooped*/
  std::vector<uint64_t> fSnoop_Masks;

  /** contains event ids of all events to be snooped*/
  std::vector<uint64_t> fSnoop_Ids;

  /* contains offsets of all events to be snooped*/
  std::vector<uint64_t> fSnoop_Offsets;

  /* contains  accept flag bitmask of all events to be snooped:
   * 0x1=late, 0x2=early, 0x4= conflict, 0x8=delayed
   * TODO:*/
  std::vector<uint64_t> fSnoop_Flags;

  /** set up saftlib conditions from the configuration.*/
  bool SetupConditions ();


  void ResetDescriptors ()
  {
    fInput_Names.clear ();
    fSnoop_Masks.clear ();
    fSnoop_Ids.clear ();
    fSnoop_Offsets.clear ();
    fSnoop_Flags.clear ();
  }

  /** clear input queue of timing events */
  void ClearEventQueue ();

  bool Close ();

  /** Fill output buffer with mbs formatted data.*/
  unsigned Write_Mbs (dabc::Buffer& buf);

  /** Fill output buffer with hadaq formatted data.*/
  unsigned Write_Hadaq (dabc::Buffer& buf);

  /** Fill output buffer with raw timing events.*/
  unsigned Write_Raw (dabc::Buffer& buf);

public:
  Input (const saftdabc::DeviceRef &owner);
  virtual ~Input ();

  virtual bool Read_Init (const dabc::WorkerRef &, const dabc::Command &);

  void SetTransportRef(dabc::InputTransport* trans);

  virtual unsigned Read_Size ();
  virtual unsigned Read_Start (dabc::Buffer& buf);
  virtual unsigned Read_Complete (dabc::Buffer& buf);

  virtual double Read_Timeout ()
  {
    return fTimeout;
  }

#ifdef DABC_SAFT_USE_2_0
 /** This is the signalhandler that treats condition events from saftlib*/
    void EventHandler (uint64_t event, uint64_t param, uint64_t deadline, uint64_t executed, uint16_t flags = 0xF);


    /** This is a signalhandler that treats overflow counter events*/
    void OverflowHandler(uint64_t count);

#else
    /** This is the signalhandler that treats condition events from saftlib*/
    void EventHandler (guint64 event, guint64 param, guint64 deadline, guint64 executed, guint16 flags = 0xF);


    /** This is a signalhandler that treats overflow counter events*/
    void OverflowHandler(guint64 count);
#endif


};

}

#endif
