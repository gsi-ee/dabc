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

#ifndef RFIO_FileInterface
#define RFIO_FileInterface

#ifndef DABC_BinaryFile
#include "dabc/BinaryFile.h"
#endif

namespace rfio {

   class FileInterface : public dabc::FileInterface {
      protected:
         void*  fRemote{nullptr};      //! connection to datamover, done once when any special argument is appearing
         int    fOpenedCounter{0};     //! counter of opened files via special connection to datamover
         int    fDataMoverIndx{0};    //! obtained data mover index
         char   fDataMoverName[64];   //! obtained data mover name

      public:

         FileInterface();

         virtual ~FileInterface();

         Handle fopen(const char *fname, const char *mode, const char *opt = nullptr) override;

         void fclose(Handle f) override;

         size_t fwrite(const void* ptr, size_t sz, size_t nmemb, Handle f) override;

         size_t fread(void* ptr, size_t sz, size_t nmemb, Handle f) override;

         bool feof(Handle f) override;

         bool fflush(Handle f) override;

         bool fseek(Handle f, long int offset, bool realtive = true) override;

         dabc::Object *fmatch(const char *fmask, bool select_files = true) override;

         int GetFileIntPar(Handle h, const char *parname) override;

         bool GetFileStrPar(Handle h, const char *parname, char* sbuf, int sbuflen) override;

   };

}

#endif
