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
#ifndef SAFTDABC_Device
#define SAFTDABC_Device

#include "dabc/Device.h"
#include "dabc/Object.h"
#include "dabc/MemoryPool.h"
#include "dabc/threads.h"


//#include "mbs/MbsTypeDefs.h"

#include <map>

#include <giomm.h>
#include <glib.h>

#include <saftlib/SAFTd.h>
#include <saftlib/TimingReceiver.h>
#include <saftlib/SoftwareActionSink.h>
#include <saftlib/SoftwareCondition.h>

#include <saftlib/Input.h>



// for CommonFunctions:
   #include <time.h>
   #include <sys/time.h>
   #include <stdio.h>
   #include <string.h>
   #include <sstream>
   #include <iostream>
   #include <iomanip>
   #include <inttypes.h>






namespace saftdabc
{

class Input;

class Device: public dabc::Device
{

public:

  Device (const std::string& name, dabc::Command cmd);
  virtual ~Device ();

  /** need to start timeout here*/
  void OnThreadAssigned();

  /** here we may insert some actions to the device cleanup methods*/
  virtual bool DestroyByOwnThread();


  virtual const char* ClassName () const
  {
    return "saftdabc::Device";
  }


  virtual int ExecuteCommand (dabc::Command cmd);

  virtual dabc::Transport* CreateTransport (dabc::Command cmd, const dabc::Reference& port);

  /** clean up all existing saftlib conditions*/
   bool ClearConditions ();

   /** Register input of name to snoop on
    * NewCondition(bool active, guint64 id, guint64 mask, gint64 offset);
    * first argument is pointer to input channel which shall receive timing event*/
   bool RegisterInputCondition (saftdabc::Input* receiver, std::string& ioname);

   /** Register input event per id to snoop on
    * NewCondition(bool active, guint64 id, guint64 mask, gint64 offset);
    * acceptflags contain bitmask for accept flags: 0x1=late, 0x2=early, 0x4= conflict, 0x8=delayed
    * first argument is pointer to input channel which shall receive timing event*/
   bool RegisterEventCondition (saftdabc::Input* receiver, guint64 id, guint64 mask, gint64 offset, unsigned char acceptflags);


   /** Return text description of input belonging to given event id, e.g. "IO2 rising".
    * Will be put into event data for unique identification in data stream*/
   const std::string GetInputDescription(guint64 event);


   /** add numevents to the event  ratemeter. For webgui.*/
   void AddEventStatistics (unsigned numevents);

   /** set info parameter. For webgui.*/
   void SetInfo(const std::string& info, bool forceinfo=true);

protected:

  /** this function keeps glib main loop alive.
   * Runs in main thread of the device*/
  void RunGlibMainLoop();


  virtual void ObjectCleanup ();



  void SetDevInfoParName(const std::string& name)
  {
    fDevInfoName = name;
  }



protected:

    /** name of the saftd attached board name, e.g. baseboard*/
  std::string fBoardName;


  /** Name of info parameter for device messages*/
  std::string fDevInfoName;



  /** the glib main loop*/
   Glib::RefPtr<Glib::MainLoop> fGlibMainloop;

   /** handle for the timing receiver instance*/
   Glib::RefPtr<saftlib::TimingReceiver_Proxy> fTimingReceiver;

   /** Helper vector  to connect conditions */
   std::vector<Glib::RefPtr<saftlib::SoftwareCondition_Proxy> > fConditionProxies;

     /** Helper vector  to connect action sinks */
   std::vector<Glib::RefPtr<saftlib::SoftwareActionSink_Proxy> > fActionSinks;


   /** Mutex to protect list of condition proxies*/
   dabc::Mutex fConditionMutex;


   /** Translation table IO name <> prefix
      * like in  saft-io-ctl implementation*/
    std::map<Glib::ustring, guint64> fMap_PrefixName;




};



   class DeviceRef : public dabc::DeviceRef {

      DABC_REFERENCE(DeviceRef, dabc::DeviceRef, Device)

   public:

   /**cleanup conditions */
  bool ClearConditions () const
  {
    return (null () ? false : GetObject ()->ClearConditions ());
  }

  bool RegisterInputCondition (saftdabc::Input* receiver, std::string& ioname)
  {
    return (null () ? false : GetObject ()->RegisterInputCondition (receiver, ioname));

  }

  bool RegisterEventCondition (saftdabc::Input* receiver, guint64 id, guint64 mask, gint64 offset, unsigned char acceptflags)
  {
    return (null () ? false : GetObject ()->RegisterEventCondition (receiver, id, mask, offset, acceptflags));
  }


  const std::string GetInputDescription(guint64 event)
  {
    return (null() ? std::string("") : GetObject ()->GetInputDescription(event));
  }

  void AddEventStatistics (unsigned numevents)
    {
      if(GetObject()) GetObject ()->AddEventStatistics(numevents);
    }
  void SetInfo(const std::string& info, bool forceinfo=true)
    {
        if(GetObject()) GetObject ()->SetInfo(info, forceinfo);
    }


   };


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////// here CommonFunction declarations stolen from saftlib examples:
// JAM TODO: may this be exported in saftlib?

    // modes for printing to cout
    const guint32 PMODE_NONE     = 0x0;
    const guint32 PMODE_DEC      = 0x1;
    const guint32 PMODE_HEX      = 0x2;
    const guint32 PMODE_VERBOSE  = 0x4;

    // formatting of mask for action sink
    guint64     tr_mask(int i                         // number of bits
                        );

    // formatting of date for output
    std::string tr_formatDate(guint64 time,           // time [ns]
                              guint32 pmode           // mode for printing
                              );

    // formatting of action event ID for output
    std::string tr_formatActionEvent(guint64 id,      // 64bit event ID
                                     guint32 pmode    // mode for printing
                                     );

    // formatting of action param for output; the format depends also on evtNo, except if evtNo == 0xFFFF FFFF
    std::string tr_formatActionParam(guint64 param,    // 64bit parameter
                                     guint32 evtNo,    // evtNo (currently 12 bit) - part of the 64 bit event ID
                                     guint32 pmode     // mode for printing
                                     );

    // formatting of action flags for output
    std::string tr_formatActionFlags(guint16 flags,    // 16bit flags
                                     guint64 delay,    // used in case action was delayed
                                     guint32 pmode     // mode for printing
                                     );






}    // namespace

#endif
