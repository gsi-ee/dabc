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

#ifndef DABC_ROOT_BainryProducer
#define DABC_ROOT_BainryProducer

#ifndef DABC_Object
#include "dabc/Object.h"
#endif

#ifndef DABC_Hierarchy
#include "dabc/Hierarchy.h"
#endif

class TBufferFile;
class TMemFile;
class TObject;

namespace dabc_root {

   /** \brief %BinaryProducer convert ROOT objects to binary data, which can be transformed to browser
    *
    */

   class BinaryProducer : public dabc::Object {
      protected:

         int         fCompression;    ///< compression level, default 5
         TMemFile*   fMemFile;        ///< file used to generate streamer infos
         int         fSinfoSize;      ///< number of entries in streamer infos

         void CreateMemFile();

         dabc::Buffer CreateBindData(TBufferFile* sbuf);

         dabc::Buffer CreateImage(TObject* obj);

      public:
         BinaryProducer(const std::string& name, int compr = 5);
         virtual ~BinaryProducer();

         dabc::Buffer GetStreamerInfoBinary();

         std::string GetStreamerInfoHash() const;

         dabc::Buffer GetBinary(TObject* obj, bool asimage = false);
   };

}

#endif
