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

#include "olmd/OlmdFile.h"

#include "dabc/logging.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>



olmd::OlmdFile::OlmdFile(): fbIsOpen(false)
{
   fxInputChannel = f_evt_control();
}

olmd::OlmdFile::~OlmdFile()
{
   Close();
   if(fxInputChannel) free(fxInputChannel);
}



bool olmd::OlmdFile::OpenReading(const char *fname, const char*)
{
   Close();
   void *headptr=&fxInfoHeader; // some new compilers may warn if we directly dereference member
   char *tagfile = nullptr;
   if(HasTagFile()) tagfile = (char*) GetTagName();
   int status = f_evt_get_tagopen(fxInputChannel,
         tagfile,
         (char*)  fname,
         (char**) headptr,
            0);

   if(status !=GETEVT__SUCCESS) {
      LmdErrorMessage(status, dabc::format("Open file %s for reading failed!",fname).c_str());
      fxFileName="";
      return false;

   } else {
      fbIsOpen=true;
      fbFirstEvent=true;
      DOUT0("Opened for reading - Original LMD File  %s ", fname);
      fxFileName=fname;
      return true;
   }
   return true;
}

void olmd::OlmdFile::Close()
{
   int status=0;
   if(!fbIsOpen) return;
   status = f_evt_get_tagclose(fxInputChannel);
   if(status == GETEVT__SUCCESS)
   {
      fbIsOpen = false;
   }
   else
   {
     LmdErrorMessage(status, dabc::format("Close file %s failed!",GetFileName()).c_str());
   }
   fbIsOpen=false;
}



bool olmd::OlmdFile::ReadNextEvent()
{
   int status=0, skip=0;
   void* evtptr=&fxEvent;
   // below is  almost the same mechanics as in  good old go4 MbsFile (JAM24)
     // test if we had reached the last event:
     if(fuStopEvent != 0 && fuEventCounter >= fuStopEvent)
        {
           status= GETEVT__NOMORE;
        }
     else
        {
        if(fbFirstEvent)
           {
           if(fuStartEvent>0)
              {
                 // we want to work with "real" event number for first event
                 skip = fuStartEvent-1;
                 if(skip) fuEventCounter++; // need to correct for init value of event counter below!
              }
           else
              {
                 skip = 0;
              }
           fbFirstEvent = false;
        }
       else
          {
           skip =  fuEventInterval;
          }
        status = f_evt_get_tagnext(fxInputChannel, skip, (int**) evtptr);
        (skip ? fuEventCounter+=skip : fuEventCounter++);
     } //if(fuStopEvent != 0

     if(status != 0)
        {
            if (status == GETEVT__NOMORE)
               {
                  DOUT0("No more data from file %s.",GetFileName());
                  Close();
               }
            else
               {
                  LmdErrorMessage(status,
                              dabc::format("on reading file %s , tag:%s, start:%ld stop: %ld interval:%ld, current event:%ld!",
                                    GetFileName(), GetTagName(), fuStartEvent, fuStopEvent, fuEventInterval, fuEventCounter).c_str());
               }
            return false;
        }
   return true;
}


//bool olmd::OlmdFile::ReadBuffer(void* ptr, uint64_t* sz, bool onlyevent)

//size_t olmd::OlmdFile::CopyEvent(char** ptr, char* terminus)
bool  olmd::OlmdFile::CopyEvent()
{
//   size_t readsize=0;
//   char* cursor=*ptr;
   fuReadSize=(fxEvent->l_dlen -4) * sizeof(short) + sizeof(s_ve10_1);
   if(fxCursor + fuReadSize < fxEndOfBuffer)
      {
              memcpy(fxCursor, fxEvent, fuReadSize);
              fxCursor+=fuReadSize;
              fuSumSize+=fuReadSize;
              fxEventsPerBuffer++;
      }
   else
      {
             DOUT5("OlmdFile::CopyEvent: not enough space in target buffer (remaining size %ld) for event (size:%ld) after filling %ld events",
                      (size_t) (fxEndOfBuffer - fxCursor), fuReadSize, fxEventsPerBuffer);
             fbHasOldEvent=true;
             return false;
      }

    return true;
}



bool olmd::OlmdFile::ReadBuffer(void* ptr, uint64_t* sz)
{
   if(!IsOpen()) return false;
   fxCursor=reinterpret_cast<char*> (ptr);
   fuMaxSize = *sz; // keep total length of  dabc target buffer
   fxEndOfBuffer= fxCursor+ fuMaxSize;
   *sz = 0;
   fuSumSize=0;
   //fuReadSize=0; // set by CopyEvent will  contain length of next event (header+payload)
   fxEventsPerBuffer=0;
   // first copy  previous event to buffer  if we have  one:
   if(fbHasOldEvent)
   {
      fbHasOldEvent=false;
      if(!CopyEvent())
         {
            EOUT("OlmdFile::ReadBuffer: not enough space in target buffer (size %ld) for old  event (size:%ld). never  come here?.",fuMaxSize, fuReadSize);
            fbHasOldEvent=false;// prevent infinite loop
            return  false;
         }
   } // has old event

   // now get new events from file until buffer is full:
  do
   {
     if(!ReadNextEvent())
        return false;
   }
  while (CopyEvent());
  *sz=fuSumSize;
  return true;
}


void olmd::OlmdFile::LmdErrorMessage(int status, const char* description)
{
   char msg[128];
   f_evt_error(status,msg,1); // provide text message for later output
   EOUT("%s :%s", msg, description);
}
