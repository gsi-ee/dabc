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

#ifndef HADAQ_HldFile
#define HADAQ_HldFile



#include <stdint.h>
#include <fstream>
#include <iostream>
//#include <iomanip>
#include <stdint.h>
//using namespace std;

#include "hadaq/HadaqTypeDefs.h"

namespace hadaq {




   class HldFile {
      protected:
         enum EMode { mNone, mWrite, mRead, mError };

         EMode      fMode;
         //void      *fControl;
         uint32_t   fLastError;
         uint64_t   fEventCount;

         /* file name*/
         std::string fName;

         /** file handle. */
         std::fstream* fFile;

         /* run id from timeofday for eventbuilding*/
         RunId fRunNumber;

         /* working buffer*/
         char* fBuffer;

         /* points to current event header in buffer*/
         hadaq::Event* fHadEvent;

         /* actual size of bytes read into buffer most recently*/
         size_t fBufsize;

         size_t ReadFile(char* dest, size_t len);

         size_t WriteFile(char* src, size_t len);

         bool RewindFile(int offset);

         bool OpenFile(const char* fname, bool iswriting, uint32_t buffersize);

      public:
         HldFile();
         virtual ~HldFile();

         uint32_t LastError() const { return fLastError; }

         /** Open file with specified name for writing */
         bool OpenWrite(const char* fname, hadaq::RunId rid=0);

         /** Opened file for reading. Internal buffer required
           * when data read partially and must be kept there. */
         bool OpenRead(const char* fname, uint32_t buffersize = 0x10000);

         bool IsWriteMode() const { return fMode == mWrite; }
         bool IsReadMode() const { return fMode == mRead; }

         void Close();



         /**
          * Read next HadTu element from file into external buffer. File must be opened by method OpenRead(),.
          * If no external buffer is specified, use internal one.
          * Return value points to last written element header
           */
         hadaq::HadTu* ReadElement(char* buffer=0, size_t len=0);

         /** Read next event from file - uses ReadElement on internal buffer. */
         hadaq::Event* ReadEvent();
         /** Read one or several elements to provided user buffer
           * When called, bufsize should has available buffer size,
           * after call contains actual size read.
           * Returns read number of events. */
         unsigned int ReadBuffer(void* buf, uint32_t& bufsize);



         /** Write user buffer to file without reformatting
          * Returns number of written events. */
         unsigned int WriteBuffer(void* buf, uint32_t bufsize);


         /** Write num events to the file, starting with eventheader hdr*/
         bool WriteEvents(hadaq::Event* hdr, unsigned num = 1);

   };

} // end of namespace

#endif
