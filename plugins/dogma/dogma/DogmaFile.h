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

#ifndef DOGMA_DogmaFile
#define DOGMA_DogmaFile

#ifndef DABC_BinaryFile
#include "dabc/BinaryFile.h"
#endif

#ifndef DOGMA_defines
#include "dogma/defines.h"
#endif

namespace dogma {

   /** \brief DOGMA file implementation */

   class DogmaFile : public dabc::BasicFile {
      protected:
         bool           fEOF{true};         //! flag indicate that end-of-file was reached

      public:
         DogmaFile() {}
         ~DogmaFile() { Close(); }

         /** Open file with specified name for writing */
         bool OpenWrite(const char *fname, const char *opt = nullptr);

         /** Opened file for reading. Internal buffer required
           * when data read partially and must be kept there. */
         bool OpenRead(const char *fname, const char *opt = nullptr);

         /** Close file */
         void Close();

         /** When file open for reading, method returns true when file end was achieved */
         bool eof() const { return fEOF; }

         /** Read one or several elements to provided user buffer
           * When called, bufsize should has available buffer size,
           * after call contains actual size read.
           * If /param onlyevent=true, the only hadaq element will be read.
           * Returns true if any data were successfully read. */
         bool ReadBuffer(void* ptr, uint32_t* bufsize, bool onlyevent = false);

         /** Write user buffer to file without reformatting
          * User must be aware about correct formatting of data.
          * Returns true if data was written.*/
         bool WriteBuffer(void* buf, uint32_t bufsize);
   };

} // end of namespace

#endif
