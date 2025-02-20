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

#ifndef MBS_LmdFile
#define MBS_LmdFile

#ifndef DABC_BinaryFile
#include "dabc/BinaryFile.h"
#endif

#ifndef MBS_LmdTypeDefs
#include "mbs/LmdTypeDefs.h"
#endif


namespace mbs {

   /** \brief Reading/writing LMD files (new API)
    *
    * New LMD file, exclude index table from previous implementation
    * */

   class LmdFile : public dabc::BasicFile {
      protected:
         FileHeader                  fFileHdr;          //!  file header

      public:

         LmdFile() : dabc::BasicFile(), fFileHdr() {}

         ~LmdFile()
         {
            Close();
         }

         bool OpenReading(const char *fname, const char *opt = nullptr)
         {
            if (isOpened()) return false;

            if (!fname || *fname == 0) {
               fprintf(stderr, "file name not specified\n");
               return false;
            }

            CheckIO();

            fd = io->fopen(fname,  "r", opt);
            if (!fd) {
               fprintf(stderr, "File open failed %s for reading\n", fname);
               return false;
            }

            if (io->fread(&fFileHdr, sizeof(fFileHdr), 1, fd) !=  1) {
               fprintf(stderr, "Failure reading file %s header\n", fname);
               Close();
               return false;
            }

            if (fFileHdr.iEndian != 1) {
               fprintf(stderr, "Wrong endian %u in file %s, expected 1\n", (unsigned) fFileHdr.iEndian, fname);
               Close();
               return false;
            }

            if (fFileHdr.isTypePair(0x65, 0x1) && (fFileHdr.FullSize() == 0xfffffff0) && (fFileHdr.iOffsetSize == 8)) {
               // this is LMD file, created by DABC
               fReadingMode = true;
               // fMbsFormat = false;
               return true;
            }

            /////// JAM24
            /*
            fprintf(stderr, "%s is original LMD file, which is not supported by DABC\n", fname);
            Close();
            return false;
   */
            /////// end JAM24

            if ((fFileHdr.FullSize() > 0x8000) || (fFileHdr.FullSize() < sizeof(fFileHdr))) {
               fprintf(stderr, "File header %u too large in file %s\n", (unsigned) fFileHdr.FullSize(), fname);
               Close();
               return false;
            }

            if (fFileHdr.FullSize() > sizeof(fFileHdr)) {
               // skip dummy bytes from file header
               io->fseek(fd, fFileHdr.FullSize() - sizeof(fFileHdr), true);
            }

            fReadingMode = true;
            // fMbsFormat = true;
            return true;

         }

         bool OpenWriting(const char *fname, const char *opt = nullptr)
         {
            if (isOpened()) return false;

            if (!fname || *fname == 0) {
               fprintf(stderr, "file name not specified\n");
               return false;
            }

            CheckIO();

            fd = io->fopen(fname, "w", opt);
            if (!fd) {
               fprintf(stderr, "File open failed %s for writing\n", fname);
               return false;
            }

            fFileHdr.SetFullSize(0xfffffff0);
            fFileHdr.SetTypePair(0x65, 0x1);
            fFileHdr.iTableOffset = 0;
            fFileHdr.iElements = 0xffffffff;
            fFileHdr.iOffsetSize = 8;  // Offset size, 4 or 8 [bytes]
            fFileHdr.iTimeSpecSec = 0; // compatible with s_bufhe (2*32bit)
            fFileHdr.iTimeSpecNanoSec = 0; // compatible with s_bufhe (2*32bit)

            fFileHdr.iEndian = 1;      // compatible with s_bufhe free[0]
            fFileHdr.iWrittenEndian = 0;// one of LMD__ENDIAN_x
            fFileHdr.iUsedWords = 0;   // total words without header to read for type=100, free[2]
            fFileHdr.iFree3 = 0;       // free[3]

            if (io->fwrite(&fFileHdr, sizeof(fFileHdr),1,fd) != 1) {
               fprintf(stderr, "Failure writing file %s header\n", fname);
               Close();
               return false;
            }

            fReadingMode = false;
            // fMbsFormat = false;
            return true;
         }

         bool Close()
         {
            return CloseBasicFile();
         }

         const FileHeader& hdr() const { return fFileHdr; }

         /** Write buffer or part of buffer
          * User must ensure that content of buffer is corresponds to the lmd header formatting */
         bool WriteBuffer(const void* ptr, uint64_t sz)
         {
            if (!isWriting() || !ptr || (sz == 0)) return false;

            if (io->fwrite(ptr, sz, 1, fd) != 1) {
               fprintf(stderr, "fail to write buffer of size %u to lmd file\n", (unsigned) sz);
               Close();
               return false;
            }

            return true;
         }


         /** Reads buffer with several MBS events */
         bool ReadBuffer(void* ptr, uint64_t* sz, bool onlyevent = false)
         {
            if (isWriting() || !ptr || !sz || (*sz < sizeof(mbs::Header))) return false;

            // if (fMbsFormat) return ReadMbsBuffer(ptr, sz, onlyevent);

            uint64_t maxsz = *sz; *sz = 0;

             printf("start buffer reading maxsz = %u\n", (unsigned) maxsz);

            // any data in LMD should be written with 4-byte wrapping
            size_t readsz = io->fread(ptr, 1, maxsz, fd);

             printf("readsz = %u\n", (unsigned) readsz);

            if (readsz == 0) return false;

            size_t checkedsz = 0;

            mbs::Header* hdr = (mbs::Header*) ptr;

            while (checkedsz < readsz) {
               // special case when event was read not completely
               // or we want to provide only event
               if ((checkedsz + hdr->FullSize() > readsz) || (onlyevent && (checkedsz>0))) {
                  // return pointer to the begin of event
                  io->fseek(fd, -(readsz - checkedsz), true);
                  break;
               }
               checkedsz += hdr->FullSize();
               hdr = (mbs::Header*) ((char*) hdr + hdr->FullSize());
            }

            *sz = checkedsz;

            return checkedsz > 0;
         }

   };
}

#endif
