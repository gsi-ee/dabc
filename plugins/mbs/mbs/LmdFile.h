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

#ifndef MBS_LmdFile
#define MBS_LmdFile

#ifndef MBS_LmdTypeDefs
#include "LmdTypeDefs.h"
#endif

#ifndef MBS_MbsTypeDefs
#include "MbsTypeDefs.h"
#endif

namespace mbs {

   class LmdFile {
      protected:
         enum EMode { mNone, mWrite, mRead, mError };

         EMode      fMode;
         void      *fControl;
         uint32_t   fLastError;

      public:
         LmdFile();
         virtual ~LmdFile();

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
          * Each element must contain mbs::Header with correctly set size
          */
         bool WriteElements(mbs::Header* hdr, unsigned num = 1);

         /** Read next element from file. File must be opened by method OpenRead(),
           * Data will be copied first in internal buffer and than provided to user.
           */
         mbs::Header* ReadElement();

         /** Read one or several elements to provided user buffer
           * When called, bufsize should has available buffer size,
           * after call contains actual size read.
           * Returns read number of events. */
         unsigned int ReadBuffer(void* buf, uint32_t& bufsize);

         /** Write one or several events to the file.
          * Same as WriteElements */
         bool WriteEvents(EventHeader* hdr, unsigned num = 1);

         /** Read next event from file - same as ReadElement. */
         mbs::EventHeader* ReadEvent();
   };

} // end of namespace

#endif
