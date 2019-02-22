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
#ifndef USER_Input
#define USER_Input

#include "dabc/DataIO.h"
#include "dabc/statistic.h"

#include "mbs/MbsTypeDefs.h"

namespace user
{


class Input: public dabc::DataInput
{

public:
  Input (dabc::Url& url);
  virtual ~Input ();

protected:


  virtual bool Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd);

  virtual unsigned Read_Size ();

  virtual unsigned Read_Start (dabc::Buffer& buf);

  virtual unsigned Read_Complete (dabc::Buffer& buf);

  virtual double Read_Timeout ();

  unsigned int GetReadLength ()
   {
     return fReadLength;
   }

  // These are the actual user readout functions to implement: //


   /** Initialize the readout hardware on startup*/
   virtual int User_Init ();

   /** Fill the data at the buffer pointer. size returns filled size in bytes*/
   virtual int User_Readout (uint32_t* pdat, unsigned long& size);

   /** Here any kind of external trigger can be waited for*/
   virtual int User_WaitForTrigger ();

   /** Used to clear any kind of external trigger information**/
   virtual int User_ResetTrigger();

   /** shut down the readout hardware properly at the end*/
   virtual int User_Cleanup();


   /** produce random spectra for the go4 example */
   double Gauss_Rnd(double mean, double sigma);

  /** remember specifications of this input from port config */
  dabc::Url fURL;

  dabc::Ratemeter fErrorRate;

  /** actual payload length of read buffer*/
    unsigned int fReadLength;


    /** Event number since device init*/
    unsigned int fNumEvents;

    /** subevent size for this example*/
    unsigned int fSubeventSize;

    /** For mbsformat: defines subevent subcrate id for case fSingleSubevent=true*/
     unsigned int fSubeventSubcrate;

     /** For mbsformat: defines subevent procid*/
     unsigned int fSubeventProcid;

     /** For mbsformat: defines subevent control*/
     unsigned int fSubeventControl;

     /** rest the trigger before device buffer has been read out.
      * This can speed up things if DAQ is provided with double buffering or external data queues.**/
     bool fEarlyTriggerClear;

     /** flag for optional debug output*/
     bool fDebug;



};
}

#endif
