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

#ifndef MBS_TextInput
#define MBS_TextInput

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

#ifndef DABC_Folder
#include "dabc/Folder.h"
#endif

#include <fstream>

namespace mbs {

   class TextInput : public dabc::DataInput {
      public:
         TextInput(const char* fname = 0, uint32_t bufsize = 0x10000);
         virtual ~TextInput();

         virtual bool Read_Init(dabc::Command* cmd = 0, dabc::WorkingProcessor* port = 0);

         bool Init();

         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(dabc::Buffer* buf);

      protected:
         bool CloseFile();

         bool OpenNextFile();

         std::string         fFileName;      // as specified in configuration
         uint32_t            fBufferSize;    // buffer size
         std::string         fDataFormat;    // int32_t, uint32_t, float
         int                 fNumData;       // number of data entries in line
         int                 fNumHeaderLines; // number of lines in header
         int                 fCharBufferLength;   // buffer for single string

         dabc::Folder*       fFilesList;

         std::string         fCurrentFileName;

         std::ifstream       fFile;
         char*               fCharBuffer;
         uint32_t            fEventCounter;
         int                 fFormatId; // 0 - float, 1 - int32_t, 2 - uint32_t
         unsigned            fDataUnitSize; // sizeof of single binary entry
   };

}

#endif
