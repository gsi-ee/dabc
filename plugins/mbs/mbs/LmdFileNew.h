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

#ifndef MBS_LmdFileNew
#define MBS_LmdFileNew

#ifndef DABC_BinaryFile
#include "dabc/BinaryFile.h"
#endif

#ifndef MBS_LmdTypeDefs
#include "mbs/LmdTypeDefs.h"
#endif


namespace mbs {

   // implement basic POSIX interface, can be extended in the future

   class LmdFileNew {
      protected:
         dabc::FileInterface*        io;                //!  interface to the file system
         bool                        iowoner;           //!  if true, io object owned by file
         dabc::FileInterface::Handle fd;                //!  file descriptor
         bool                        fReadingMode;      //!  reading/writing mode
         FileHeader                  fFileHdr;          //!  file header

      public:

         LmdFileNew() :
            io(0),
            iowoner(false),
            fd(0),
            fReadingMode(false)
         {
         }

         void SetIO(dabc::FileInterface* _io, bool _ioowner = false)
         {
            if (iowoner && io) delete io;
            io = _io;
            iowoner = _ioowner;
         }

         ~LmdFileNew()
         {
            Close();
            if (iowoner && io) delete io;
            io = 0; iowoner = false;
         }

         // returns true if file is opened
         inline bool isOpened() const { return (io!=0) && (fd!=0); }

         inline bool isReading() const { return isOpened() && fReadingMode; }

         inline bool isWriting() const { return isOpened() && !fReadingMode; }

         bool eof() const { return isReading() ? io->feof(fd) : true; }

         bool OpenReading(const char* fname)
         {
            if (isOpened()) return false;

            if (io==0) { io = new dabc::FileInterface; iowoner = true; }

            if (fname==0 || *fname==0) {
               fprintf(stderr, "file name not specified\n");
               return false;
            }
            fd = io->fopen(fname,  "r");
            if (fd==0) {
               fprintf(stderr, "File open failed %s for reading\n", fname);
               return false;
            }

            if (io->fread(&fFileHdr, sizeof(fFileHdr), 1, fd) !=  1) {
               fprintf(stderr, "Failure reading file %s header", fname);
               Close();
               return false;
            }

            fReadingMode = true;
            return true;
         }

         bool OpenWriting(const char* fname)
         {
            if (isOpened()) return false;

            if (io==0) { io = new dabc::FileInterface; iowoner = true; }

            if (fname==0 || *fname==0) {
               fprintf(stderr, "file name not specified\n");
               return false;
            }
            fd = io->fopen(fname, "w");
            if (fd==0) {
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

            if (io->fwrite(&fFileHdr, sizeof(fFileHdr),1,fd)!=1) {
               fprintf(stderr, "Failure writing file %s header", fname);
               Close();
               return false;
            }

            fReadingMode = false;
            return true;
         }

         bool Close()
         {
            if (fd && io) io->fclose(fd);
            fd = 0;
            return true;
         }

         const FileHeader& hdr() const { return fFileHdr; }

         /** Write buffer or part of buffer
          * User must ensure that content of buffer is corresponds to the lmd header formatting */
         bool WriteBuffer(const void* ptr, uint64_t sz)
         {
            if (!isWriting() || (ptr==0) || (sz==0)) return false;

            if (io->fwrite(ptr, sz, 1, fd) != 1) {
               fprintf(stderr, "fail to write buffer of size %u to lmd file\n", (unsigned) sz);
               Close();
               return false;
            }

            return true;
         }

         bool ReadBuffer(void* ptr, uint64_t* sz, bool onlyevent = false)
         {
            if (isWriting() || (ptr==0) || (sz==0) || (*sz == 0)) return false;

            uint64_t maxsz = *sz; *sz = 0;

            // any data in LMD should be written with 4-byte wrapping
            size_t readsz = io->fread(ptr, 1, maxsz, fd);

            if (readsz==0) return false;

            size_t checkedsz = 0;

            mbs::Header* hdr = (mbs::Header*) ptr;

            while (checkedsz < readsz) {
               // special case when event was read not completely
               // or we want to provide only event
               if ((checkedsz + hdr->FullSize() > readsz) || (onlyevent && (checkedsz>0))) {
                  // return event to the begin of event
                  io->fseek(fd, -(readsz - checkedsz), true);
                  break;
               }
               checkedsz += hdr->FullSize();
               hdr = (mbs::Header*) ((char*) hdr + hdr->FullSize());
            }

            *sz = checkedsz;

            return checkedsz>0;
         }

   };
}

#endif
