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

#include <algorithm>

dabc::HierarchyStore::HierarchyStore() :
   fBasePath(),
   fIO(nullptr),
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

   if (fIO) {
      delete fIO;
      fIO = nullptr;
   }
}

bool dabc::HierarchyStore::SetBasePath(const std::string &path)
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
   tm.GetNow();

   std::string strdate = tm.OnlyDateAsString();
   std::string strtime = tm.OnlyTimeAsString();

   path.append(strdate);

   if (!fIO) fIO = new FileInterface;

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

bool dabc::HierarchyStore::CheckForNextStore(DateTime& now, double store_period, double flush_period)
{
   // if flags already set, do not touch them here
   if (fDoStore || fDoFlush) return true;

   if (now.null()) now.GetNow();

   // DOUT0("SINCE LAST STORE = %5.1f", now - fLastStoreTm);

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

   if (fDoFlush || fDoStore) {
      // this time will be used as raster when reading files
      // it can be read very efficient - most decoding can be skipped
      h.SetField("storetm", fLastStoreTm);
      h.MarkChangedItems(fLastStoreTm.AsJSDate());
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



dabc::HierarchyReading::HierarchyReading() :
   fBasePath(),
   fIO(nullptr)
{
}

dabc::HierarchyReading::~HierarchyReading()
{
   if (fIO) {
      delete fIO;
      fIO = nullptr;
   }
}

void dabc::HierarchyReading::SetBasePath(const std::string &path)
{
   fBasePath = path;
   if ((fBasePath.length()>0) && (fBasePath[fBasePath.length()-1] != '/')) fBasePath.append("/");
}

std::string dabc::HierarchyReading::MakeFileName(const std::string &fpath, const DateTime& dt)
{
   std::string res = fpath;
   if ((res.length()>0) && (res[res.length()-1] != '/')) res.append("/");

   std::string strdate = dt.OnlyDateAsString();
   std::string strtime = dt.OnlyTimeAsString();

   res += dabc::format("%s/%s.dabc", strdate.c_str(), strtime.c_str());

   return res;
}

bool dabc::HierarchyReading::ScanFiles(const std::string &dirname, const DateTime& onlydate, std::vector<uint64_t>& vect)
{
   std::string mask = dirname + "*.dabc";

   Reference files = fIO->fmatch(mask.c_str(), true);

   for (unsigned n=0;n<files.NumChilds();n++) {
      std::string fname = files.GetChild(n).GetName();
      fname.erase(0, dirname.length());

      size_t pos = fname.find(".dabc");
      if (pos == std::string::npos) {
         EOUT("Wrong time file name %s", fname.c_str());
         continue;
      }
      fname.erase(pos);

      DateTime dt = onlydate;
      if (!dt.SetOnlyTime(fname.c_str())) {
         EOUT("Wrong time file name %s", fname.c_str());
         continue;
      }

      vect.emplace_back(dt.AsJSDate());
   }

   return true;
}


bool dabc::HierarchyReading::ScanTreeDir(dabc::Hierarchy& h, const std::string &dirname)
{
   if (h.null() || dirname.empty()) return false;

   DOUT0("SCAN %s", dirname.c_str());

   std::string mask = dirname + "*";

   Reference dirs = fIO->fmatch(mask.c_str(), false);

   bool isanysubdir(false), isanydatedir(false);

   std::vector<uint64_t> files;

   for (unsigned n=0;n<dirs.NumChilds();n++) {
      std::string fullsubname = dirs.GetChild(n).GetName();

      std::string itemname = fullsubname;
      itemname.erase(0, dirname.length());

      DateTime dt;

      DOUT0("FOUND %s PART %s", fullsubname.c_str(), itemname.c_str());

      if (dt.SetOnlyDate(itemname.c_str())) {
         isanydatedir = true;
         if (!ScanFiles(fullsubname + "/", dt, files)) return false;
      } else {
         isanysubdir = true;
         dabc::Hierarchy subh = h.CreateHChild(itemname);
         if (!ScanTreeDir(subh, fullsubname + "/")) return false;
      }
   }

   dirs.Release();

   if (isanysubdir && isanydatedir) {
      EOUT("DIR %s Cannot combine subdirs with date dirs!!!", dirname.c_str());
      return false;
   }

   if (isanydatedir && (files.size()>0)) {

      std::sort(files.begin(), files.end());

      dabc::DateTime mindt(files.front()), maxdt(files.back());

      DOUT0("DIR: %s mintm: %s maxtm: %s files %lu", dirname.c_str(), mindt.AsJSString().c_str(), maxdt.AsJSString().c_str(), (long unsigned) files.size());

      h.SetField("dabc:path", dirname);
      h.SetField("dabc:mindt", mindt);
      h.SetField("dabc:maxdt" ,maxdt);
      h.SetField("dabc:files", files);
   }


   return true;
}


bool dabc::HierarchyReading::ScanTree()
{
   fTree.Release();
   fTree.Create("TOP");
   if (!fIO) fIO = new FileInterface;
   return ScanTreeDir(fTree, fBasePath);
}

dabc::Buffer dabc::HierarchyReading::ReadBuffer(dabc::BinaryFile& f)
{

   dabc::Buffer buf;

   if (f.eof()) return buf;

   uint64_t size = 0, typ = 0;
   if (!f.ReadBufHeader(&size, &typ)) return buf;

   buf = dabc::Buffer::CreateBuffer(size);
   if (buf.null()) return buf;

   if (!f.ReadBufPayload(buf.SegmentPtr(), size)) { buf.Release(); return buf; }

   buf.SetTotalSize(size);
   buf.SetTypeId(typ);

   return buf;
}

bool dabc::HierarchyReading::ProduceStructure(Hierarchy& tree, const DateTime& from_date, const DateTime& till_date, const std::string &entry, Hierarchy& tgt)
{
   if (tree.null()) return false;

   DOUT0("Produce structure for item %s", tree.ItemName().c_str());

   if (!tree.HasField("dabc:path")) {
      for (unsigned n=0;n<tree.NumChilds();n++) {
         Hierarchy tree_chld = tree.GetChild(n);

         /// we exclude building of the structures which are not interesting for us
         if (!entry.empty())
            if (entry.find(tree_chld.ItemName()) != 0) continue;

         Hierarchy tgt_chld = tgt.CreateHChild(tree_chld.GetName());
         if (!ProduceStructure(tree_chld, from_date, till_date, entry, tgt_chld)) return false;
      }
      return true;
   }

   std::string fpath = tree.Field("dabc:path").AsStr();

   int nfiles = tree.Field("dabc:files").GetArraySize();
   uint64_t* files = tree.Field("dabc:files").GetUIntArr();

   if (!files || (nfiles <= 0)) return false;

   DateTime dt;
   dt.SetJSDate(files[0]);
   if (!from_date.null()) {
      for (int n=0;n<nfiles;n++)
         if (files[n] <= from_date.AsJSDate())
            dt.SetJSDate(files[n]);
   }

   std::string fname = MakeFileName(fpath, dt);

   DOUT0("Opening file %s", fname.c_str());

   dabc::BinaryFile f;
   f.SetIO(fIO);
   if (!f.OpenReading(fname.c_str())) return false;
   dabc::Buffer buf = ReadBuffer(f);
   if (buf.null()) return false;

   if (!tgt.ReadFromBuffer(buf)) return false;

   if (!entry.empty()) {

      Hierarchy sel = tgt.GetTop().FindChild(entry.c_str());

      if (sel.null()) {
         EOUT("Cannot locate entry %s in the storage", entry.c_str());
         return false;
      }

      tgt.DisableReading();    // we ignore all data
      sel.EnableReading(tgt);  // beside selected element
      tgt.EnableReading();     // we enable reading of top - it stores time stamp

      // now we need enable only that entry
      sel.EnableHistory(100);

      while (!f.eof()) {
         buf = ReadBuffer(f);
         if (!buf.null())
            if (!tgt.UpdateFromBuffer(buf, stream_NoDelete)) return false;
      }

   }

   return true;
}

bool dabc::HierarchyReading::GetStrucutre(Hierarchy& tgt, const DateTime& dt)
{
   if (fTree.null()) {
      if (!ScanTree()) return false;
   }

   if (!fIO) return false;

   tgt.Release();
   tgt.Create("TOP");

   return ProduceStructure(fTree, dt, 0, "", tgt);
}

dabc::Hierarchy dabc::HierarchyReading::GetSerie(const std::string &entry, const DateTime& from, const DateTime& till)
{
   dabc::Hierarchy h, res;

   if (fTree.null()) {
      if (!ScanTree()) return res;
   }

   if (!fIO) return res;
   h.Create("TOP");

   if (!ProduceStructure(fTree, from, till, entry, h)) return res;

   res = h.FindChild(entry.c_str());
   if (res.null()) {
      EOUT("Cannot locate entry %s in the storage", entry.c_str());
      return res;
   }

   res.DettachFromParent();

   h.Release(); // rest of hierarchy is not interesting

   return res;
}
