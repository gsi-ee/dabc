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

#ifndef OLMD_OlmdFile
#define OLMD_OlmdFile



#include <string>
#include <stddef.h>

extern "C" {
   #include "mbsapi/f_evt.h"
   #include "mbsapi/s_filhe.h"
   #include "mbsapi/s_bufhe.h"
}


namespace olmd {

   /** \brief Reading  LMD files with (almost) original  MBS event API, as used in Go4
    * */

   class OlmdFile {
      protected:

         // JAM24: the following just reproduces what is done in go4 TGo4MbsSource class:

         /** Event channel structure used by event source. */
         s_evt_channel *fxInputChannel{nullptr}; //!

         s_ve10_1 *fxEvent{nullptr}; //!

         /** Points to the current gsi buffer structure filled by the event source. */
         s_bufhe *fxBuffer{nullptr}; //!

         /** Reference to header info delivered by source. */
         s_filhe *fxInfoHeader{nullptr}; //!

         /** current position in  target pointer*/
         char* fxCursor{nullptr};

         /** max size  of  target buffer */
         size_t fuMaxSize{0};

         /** size  of current event + header */
         size_t fuReadSize{0};

         /** size  of previously read data in current buffer */
         size_t fuSumSize{0};

         /** end of target buffer*/
         char* fxEndOfBuffer{nullptr};

         /** keep track how  many events are in  current dabc buffer*/
         unsigned long  fxEventsPerBuffer{0};


         bool fbIsOpen{false};

         bool fbHasOldEvent{false};

         /** remember current file name*/
         std::string fxFileName{""};

         /** lmd "tag file" name*/
         std::string fxTagName{""};

         /** Current event index counter */
         unsigned  long fuEventCounter{0};

        /** Indicates if first event should be extracted */
        bool fbFirstEvent{true};

        /** Index of first event to process. */
        unsigned  long fuStartEvent{0};

        /** Index of last event to process. */
        unsigned long fuStopEvent{0};

        /** Number of events to skip from file in between two read events  */
        unsigned long fuEventInterval{0};





      public:

         OlmdFile();

         virtual ~OlmdFile();

         bool IsOpen() {return fbIsOpen;}


         const char* GetFileName(){return fxFileName.c_str();}

         const char *GetTagName() const { return fxTagName.c_str(); }

         bool HasTagFile(){return !fxTagName.empty();}

         void SetTagFile(const char* tag){fxTagName=tag;}

         void SetStartEvent(unsigned long num) {fuStartEvent=num;}

         void SetStopEvent(unsigned long num) {fuStopEvent=num;}

         void SetEventInterval(unsigned long num){ fuEventInterval=num;}


         /** show dabc error message for lmd status */
         void LmdErrorMessage(int status, const char* description);


         bool OpenReading(const char* fname, const char *opt = nullptr);





         void Close();


//
//         /** Reads buffer with several MBS events */
         bool ReadBuffer(void* ptr, uint64_t* sz);


         /** Reads next MBS event */
         bool ReadNextEvent(); //void** ptr, uint64_t* sz);

         /** copy  current mbs event  from  event api bufffer  into  dabc buffer*/
         bool CopyEvent();



   };
}

#endif
