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

// =================================================================================


// =====================================================================


dabc::HierarchyReading::HierarchyReading() :
   fBasePath(),
   fIO(0)
{
}

dabc::HierarchyReading::~HierarchyReading()
{
   if (fIO!=0) {
      delete fIO;
      fIO = 0;
   }
}

void dabc::HierarchyReading::SetBasePath(const std::string& path)
{
   fBasePath = path;
   if ((fBasePath.length()>0) && (fBasePath[fBasePath.length()-1] != '/')) fBasePath.append("/");
}

bool dabc::HierarchyReading::ScanOnlyTime(const std::string& dirname,  DateTime& tm, bool isminimum)
{
   std::string subdir = dirname;

   if (!tm.null()) {
      char sbuf[100];
      tm.OnlyDateAsString(sbuf, sizeof(sbuf));
      subdir.append(sbuf);
      subdir.append("/");
   }

   std::string mask = subdir + "*.dabc";

   Reference files = fIO->fmatch(mask.c_str(), true);
   DateTime mindt, maxdt;

   for (unsigned n=0;n<files.NumChilds();n++) {
      std::string fname = files.GetChild(n).GetName();
      fname.erase(0, subdir.length());

      size_t pos = fname.find(".dabc");
      if (pos == fname.npos) {
         EOUT("Wrong time file name %s", fname.c_str());
         return false;
      }
      fname.erase(pos);

      DateTime dt = tm;
      if (!dt.SetOnlyTime(fname.c_str())) {
         EOUT("Wrong time file name %s", fname.c_str());
         return false;
      }

      if (mindt.null() || (mindt.AsDouble() > dt.AsDouble())) mindt = dt;
      if (maxdt.null() || (maxdt.AsDouble() < dt.AsDouble())) maxdt = dt;
   }

   if (isminimum && !mindt.null()) tm = mindt;
   if (!isminimum && !maxdt.null()) tm = maxdt;

   return true;

}


bool dabc::HierarchyReading::ScanTreeDir(dabc::Hierarchy& h, const std::string& dirname)
{
   if (h.null() || dirname.empty()) return false;

   DOUT0("SCAN %s", dirname.c_str());

   std::string mask = dirname + "*";

   Reference dirs = fIO->fmatch(mask.c_str(), false);

   DateTime mindt, maxdt;

   bool isanysubdir(false), isanydatedir(false);

   for (unsigned n=0;n<dirs.NumChilds();n++) {
      std::string fullsubname = dirs.GetChild(n).GetName();

      std::string itemname = fullsubname;
      itemname.erase(0, dirname.length());

      DateTime dt;

      DOUT0("FOUND %s PART %s", fullsubname.c_str(), itemname.c_str());

      if (dt.SetOnlyDate(itemname.c_str())) {
         isanydatedir = true;
         if (mindt.null() || (mindt.AsDouble() > dt.AsDouble())) mindt = dt;
         if (maxdt.null() || (maxdt.AsDouble() < dt.AsDouble())) maxdt = dt;
      } else {
         isanysubdir = true;
         dabc::Hierarchy subh = h.CreateChild(itemname.c_str());
         if (!ScanTreeDir(subh, fullsubname + "/")) return false;
      }
   }

   dirs.Release();

   if (isanysubdir && isanydatedir) {
      EOUT("DIR %s Cannot combine subdirs with date dirs!!!", dirname.c_str());
      return false;
   }

   if (isanydatedir) {
      if (!ScanOnlyTime(dirname, mindt, true) ||
          !ScanOnlyTime(dirname, maxdt, false)) return false;

      char buf1[100], buf2[100];
      mindt.AsJSString(buf1, sizeof(buf1));
      maxdt.AsJSString(buf2, sizeof(buf2));

      DOUT0("DIR: %s mintm: %s maxtm: %s", dirname.c_str(), buf1, buf2);

      h.Field("dabc:path").SetStr(dirname);
      h.Field("dabc:mindt").SetDatime(mindt.AsJSDate());
      h.Field("dabc:maxdt").SetDatime(maxdt.AsJSDate());
   }


   return true;
}


bool dabc::HierarchyReading::ScanTree(dabc::Hierarchy& tgt)
{
   if (tgt.null()) tgt.Create("TOP");
   if (fIO==0) fIO = new FileInterface;
   return ScanTreeDir(tgt, fBasePath);
}

