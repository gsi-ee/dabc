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

#ifndef MBS_TextInput
#define MBS_TextInput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef DABC_Object
#include "dabc/Object.h"
#endif

#include <fstream>

#include <stdint.h>

namespace mbs {

   /** \brief Text LMD input
    *
    * Provides way to read text files and convert data into MBS event structure  */

   class TextInput : public dabc::FileInput {
      protected:
         std::string         fDataFormat;    // int32_t, uint32_t, float
         int                 fNumData;       // number of data entries in line
         int                 fNumHeaderLines; // number of lines in header
         int                 fCharBufferLength;   // buffer for single string

         std::ifstream       fFile;
         char*               fCharBuffer;
         uint32_t            fEventCounter;
         int                 fFormatId; // 0 - float, 1 - int32_t, 2 - uint32_t
         unsigned            fDataUnitSize; // sizeof of single binary entry

         bool CloseFile();
         bool OpenNextFile();

      public:
         TextInput(const dabc::Url& url);
         virtual ~TextInput();

         virtual bool Read_Init(const dabc::WorkerRef& wrk, const dabc::Command& cmd);

         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(dabc::Buffer& buf);

         virtual unsigned RawDataSize();
         virtual unsigned FillRawData(const char* str, void* rawdata, unsigned maxsize);

   };

}

#endif
