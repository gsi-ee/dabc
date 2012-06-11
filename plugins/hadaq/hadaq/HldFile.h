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


#define HLD__SUCCESS        0
#define HLD__FAILURE        1
#define HLD__CLOSE_ERR      3
#define HLD__NOFILE      2
#define HLD__NOHLDFILE   4
#define HLD__EOFILE      5
//#define PUTHLD__FILE_EXIST  101
//#define PUTHLD__TOOBIG      102
//#define PUTHLD__OPEN_ERR    103
//#define PUTHLD__EXCEED      104

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
         std::ifstream* fFile;



         /* working buffer*/
         char* fBuffer;

         /* points to current event header in buffer*/
         hadaq::Event* fHadEvent; //!

         /* actual size of bytes read into buffer most recently*/
         std::streamsize fBufsize; //!


         std::streamsize ReadFile(char* dest, size_t len);

         bool RewindFile(int offset);


      public:
         HldFile();
         virtual ~HldFile();

         uint32_t LastError() const { return fLastError; }

         /** Open file with specified name for writing */
         bool OpenWrite(const char* fname, uint32_t buffersize = 0x10000);

         /** Opened file for reading. Internal buffer required
           * when data read partially and must be kept there. */
         bool OpenRead(const char* fname, uint32_t buffersize = 0x10000);

         bool IsWriteMode() const { return fMode == mWrite; }
         bool IsReadMode() const { return fMode == mRead; }

         void Close();

         /** Write one or several elements into the file.
          * Each element must contain hadaq::Header with correctly set size
          */
         bool WriteElements(hadaq::Event* hdr, unsigned num = 1);

         /**
          * Read next HadTu element from file into external buffer. File must be opened by method OpenRead(),.
          * If no external buffer is specified, use internal one.
          * Return value points to last written element header
           */
         hadaq::HadTu* ReadElement(char* buffer=0, size_t len=0);

         /** Read one or several elements to provided user buffer
           * When called, bufsize should has available buffer size,
           * after call contains actual size read.
           * Returns read number of events. */
         unsigned int ReadBuffer(void* buf, uint32_t& bufsize);

         /** Write one or several events to the file.
          * Same as WriteElements */
         bool WriteEvents(hadaq::Event* hdr, unsigned num = 1);

         /** Read next event from file - uses ReadElement on internal buffer. */
         hadaq::Event* ReadEvent();
   };

} // end of namespace

#endif
