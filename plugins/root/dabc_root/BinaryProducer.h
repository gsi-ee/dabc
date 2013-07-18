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


class TDirectory;
class TBufferFile;
class TMemFile;
class TClass;
class TCollection;
class TObject;
class TDabcTimer;


namespace dabc_root {


   /** \brief %RootBinDataContainer provides access to ROOT TBuffer object for DABC
    *
    */

   class RootBinDataContainer : public dabc::BinDataContainer {
      protected:
         TBufferFile* fBuf;

      public:
         RootBinDataContainer(TBufferFile* buf);
         virtual ~RootBinDataContainer();

         virtual void* data() const;
         virtual unsigned length() const;
   };



   /** \brief %BinaryProducer convert ROOT objects to binary data, which can be transformed to browser
    *
    */

   class BinaryProducer : public dabc::Object {
      protected:

         int         fCompression;    ///< compression level, default 5
         TMemFile*   fMemFile;        ///< file used to generate streamer infos
         int         fSinfoSize;      ///< number of entries in streamer infos


         void CreateMemFile();

         dabc::BinData CreateBindData(TBufferFile* sbuf);

      public:
         BinaryProducer(const std::string& name, int compr = 5);
         virtual ~BinaryProducer();

         dabc::BinData GetStreamerInfoBinary();

         int GetStreamerInfoHash() const { return fSinfoSize; }

         dabc::BinData GetBinary(TObject* obj);
   };

}

#endif
