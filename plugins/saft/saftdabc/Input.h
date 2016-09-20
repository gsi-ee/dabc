// $Id: Input.h 2216 2014-04-03 13:59:28Z linev $

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
#include <stdint.h>

#include <iostream>
//#include <iomanip>
#include <giomm.h>
#include <glib.h>

#include "saftdabc/Device.h"
#include "saftdabc/Definitions.h"

namespace saftdabc
{

//class DeviceRef;




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

//  /** clean up existing saftlib conditions*/
//  bool ClearConditions ();
//
//  /** Register input of name to snoop on
//   * NewCondition(bool active, guint64 id, guint64 mask, gint64 offset);*/
//  bool RegisterInputCondition (std::string ioname);
//
//  /** Register input event per id to snoop on
//   /** NewCondition(bool active, guint64 id, guint64 mask, gint64 offset);
//   * acceptflags contain bitmask for accept flags: 0x1=late, 0x2=early, 0x4= conflict, 0x8=delayed
//   * */
//  bool RegisterEventCondition (guint64 id, guint64 mask, gint64 offset, unsigned char acceptflags);


  void ResetDescriptors ()
  {
    fInput_Names.clear ();
    fSnoop_Masks.clear ();
    fSnoop_Ids.clear ();
    fSnoop_Offsets.clear ();
    fSnoop_Flags.clear ();
  }

  void ClearEventQueue ()
  {
    //single queue for all WR events:
    while(!fTimingEventQueue.empty()) fTimingEventQueue.pop();
  }

  bool Close ();




public:
  Input (const saftdabc::DeviceRef &owner);
  virtual ~Input ();

  virtual bool Read_Init (const dabc::WorkerRef& wrk, const dabc::Command& cmd);

  void SetTransportRef(dabc::InputTransport* trans);

  virtual unsigned Read_Size ();
  virtual unsigned Read_Start (dabc::Buffer& buf);
  virtual unsigned Read_Complete (dabc::Buffer& buf);

  virtual double Read_Timeout ()
  {
    return fTimeout;
  }


  /* This is the signalhandler that treats condition events from saftlib
     TODO: link to condition by
     sigc::slot<void, int> sl = sigc::mem_fun(my_foo, &foo::bar);*/
    void EventHandler (guint64 event, guint64 param, guint64 deadline, guint64 executed, guint16 flags = 0xF);



};

}

#endif
