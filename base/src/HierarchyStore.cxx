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

#include "dabc/HierarchyStore.h"

#include "dabc/logging.h"

dabc::HierarchyStore::HierarchyStore() :
   fBasePath(),
   fIO(0),
   fFile(),
   fLastStoreTm(),
   fLastFlushTm(),
   fDoStore(false),
   fDoFlush(false),
   fLastVersion(0),
   fStoreBuf(),
   fFlushBuf()
{
}

dabc::HierarchyStore::~HierarchyStore()
{
   CloseFile();

   if (fIO!=0) {
      delete fIO;
      fIO = 0;
   }
}

bool dabc::HierarchyStore::SetBasePath(const std::string& path)
{
   if (fFile.isOpened()) {
      EOUT("Cannot change base path when current file is opened");
      return false;
   }

   fBasePath = path;
   return true;
}

bool dabc::HierarchyStore::StartFile(dabc::Buffer buf)
{
   if (!CloseFile()) return false;

   std::string path = fBasePath;
   if ((path.length()>0) && (path[path.length()-1]!='/')) path.append("/");

   dabc::DateTime tm;
   if (!tm.GetNow()) return false;

   char strdate[50], strtime[50];

   tm.OnlyDateAsString(strdate, sizeof(strdate));
   tm.OnlyTimeAsString(strtime, sizeof(strtime));

   path.append(strdate);

   if (fIO==0) fIO = new FileInterface;

   if (!fIO->mkdir(path.c_str())) {
      EOUT("Cannot create path %s for hierarchy storage", path.c_str());
      return false;
   }

   DOUT0("HierarchyStore:: CREATE %s", path.c_str());

   path.append("/");
   path.append(strtime);
   path.append(".dabc");

   fFile.SetIO(fIO);
   if (!fFile.OpenWriting(path.c_str())) {
      EOUT("Cannot open file %s for hierarchy storage", path.c_str());
      return false;
   }

   return WriteDiff(buf);
}

bool dabc::HierarchyStore::WriteDiff(dabc::Buffer buf)
{
   if (!fFile.isWriting()) return false;

   BufferSize_t fullsz = buf.GetTotalSize();

   DOUT0("HierarchyStore::  WRITE %u", (unsigned) buf.GetTotalSize());

   if (!fFile.WriteBufHeader(fullsz, buf.GetTypeId())) {
      EOUT("Cannot write buffer header");
      return false;
   }

   for (unsigned n=0;n<buf.NumSegments();n++)
      if (!fFile.WriteBufPayload(buf.SegmentPtr(n), buf.SegmentSize(n))) {
         EOUT("Cannot write buffer segment");
         return false;
      }

   return true;
}

bool dabc::HierarchyStore::CloseFile()
{
   if (!fFile.isOpened()) return true;

   fFile.Close();

   return true;
}

bool dabc::HierarchyStore::CheckForNextStore(TimeStamp& now, double store_period, double flush_period)
{
   // if flags already set, do not touch them here
   if (fDoStore || fDoFlush) return true;

   if (now.null()) now.GetNow();

   // in the very first call trigger only write of base structure
   if (fLastStoreTm.null()) {
      fLastStoreTm = now;
      fLastFlushTm = now;
      fDoStore = false;
      fDoFlush = true;
   } else
   if (now - fLastStoreTm > store_period) {
      fLastStoreTm = now;
      fDoStore = true;
      if (now - fLastFlushTm >= flush_period) {
         fDoFlush = true;
         fLastFlushTm = now;
      }
   }

   return fDoStore || fDoFlush;
}

bool dabc::HierarchyStore::ExtractData(dabc::Hierarchy& h)
{
   if (fDoStore) {
      // we record diff to previous version, including all history entries

      fStoreBuf = h.SaveToBuffer(dabc::stream_Full, fLastVersion, 1000000000);

      DOUT0("HierarchyStore::  EXTRACT prev %u now %u chlds %u size %u",
                   (unsigned) fLastVersion, (unsigned) h.GetVersion(),
                   (unsigned) h()->GetChildsVersion(), (unsigned) fStoreBuf.GetTotalSize());

      fLastVersion = h.GetVersion();
   }

   if (fDoFlush) {
      // we record complete hierarchy without any history entry
      fFlushBuf = h.SaveToBuffer(dabc::stream_Full, 0, 0);
      fLastVersion = h.GetVersion();
   }

   return true;
}

bool dabc::HierarchyStore::WriteExtractedData()
{
   if (fDoStore) {
      if (!fStoreBuf.null()) WriteDiff(fStoreBuf);
      fStoreBuf.Release();
      fDoStore = false;
   }

   if (fDoFlush) {

      if (!fFlushBuf.null()) StartFile(fFlushBuf);

      fFlushBuf.Release();
      fDoFlush = false;
   }

   return true;
}


