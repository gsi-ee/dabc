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

#ifndef DABC_BinaryFileIO
#define DABC_BinaryFileIO

#ifndef DABC_BinaryFile
#include "dabc/BinaryFile.h"
#endif

#ifndef DABC_DataIO
#include "dabc/DataIO.h"
#endif

namespace dabc {

   /** \brief Binary file input object
    *
    * \ingroup dabc_all_classes
    */

   class BinaryFileInput : public FileInput {
      protected:
         BinaryFile          fFile;

         uint64_t            fCurrentBufSize;
         uint64_t            fCurrentBufType;

         bool OpenNextFile();

         bool CloseFile();


      public:
         BinaryFileInput(const dabc::Url& url);
         virtual ~BinaryFileInput();

         virtual bool Read_Init(const WorkerRef& wrk, const Command& cmd);

         virtual unsigned Read_Size();
         virtual unsigned Read_Complete(Buffer& buf);

  };

   // _________________________________________________________________

   /** \brief Binary file output object
    *
    * \ingroup dabc_all_classes
    */

   class BinaryFileOutput : public FileOutput {
      protected:

         BinaryFile          fFile;

         bool CloseFile();

         bool StartNewFile();

      public:
         BinaryFileOutput(const dabc::Url& url);
         virtual ~BinaryFileOutput();

         virtual bool Write_Init();

         virtual unsigned Write_Buffer(Buffer& buf);
   };

}


#endif
