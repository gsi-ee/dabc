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

#include "dabc/Hierarchy.h"

#include <cstring>
#include <cstdlib>

#include "dabc/defines.h"
#include "dabc/threads.h"
#include "dabc/Exception.h"
#include "dabc/Command.h"
#include "dabc/ConfigBase.h"
#include "dabc/ReferencesVector.h"

const char* dabc::prop_version = "_version";
const char* dabc::prop_kind = "_kind";
const char* dabc::prop_realname = "_realname"; // real object name
const char* dabc::prop_masteritem = "_master";
const char* dabc::prop_producer = "_producer";
const char* dabc::prop_error = "_error";
const char* dabc::prop_auth = "_auth";
const char* dabc::prop_hash = "hash";
const char* dabc::prop_history = "_history";
const char* dabc::prop_time = "time";
const char* dabc::prop_more = "_more";
const char* dabc::prop_view = "_view";


uint64_t dabc::HistoryContainer::StoreSize(uint64_t version, int hlimit)
{
   sizestream s;
   Stream(s, version, hlimit);
   return s.size();
}

bool dabc::HistoryContainer::Stream(iostream& s, uint64_t version, int hlimit)
{
   uint64_t pos = s.size();
   uint64_t sz = 0;

   uint32_t storesz = 0, storenum = 0, storevers = 0;

   if (s.is_output()) {
      sz = s.is_real() ? StoreSize(version, hlimit) : 0;
      storesz = sz / 8;

      uint32_t first = 0;
      if ((hlimit>0) && (fArr.Size() > (unsigned) hlimit)) first = fArr.Size() - hlimit;
      storenum = 0;
      bool cross_boundary = false;
      for (unsigned n=first; n<fArr.Size();n++) {
         // we have longer history as requested
         if ((version>0) && (fArr.Item(n).version <= version)) {
            cross_boundary = fArr.Item(n).version < version;
            continue;
         }
         storenum++;
      }

      s.write_uint32(storesz);
      s.write_uint32(storenum | (storevers<<24));

      uint64_t mask = cross_boundary ? 1 : 0;
      s.write_uint64(mask);

      for (uint32_t n=first;n<fArr.Size();n++) {
         if ((version>0) && (fArr.Item(n).version <= version)) continue;
         fArr.Item(n).fields->Stream(s);
      }

   } else {
      s.read_uint32(storesz);
      sz = ((uint64_t) storesz) * 8;
      s.read_uint32(storenum);
      // storevers = storenum >> 24;
      storenum = storenum & 0xffffff;

      uint64_t mask = 0;
      s.read_uint64(mask);
      fCrossBoundary = (mask & 1) != 0;

      if (fArr.Capacity() == 0)
         fArr.Allocate((int)storenum > hlimit ? (int) storenum : hlimit);

      for (uint32_t n=0;n<storenum;n++) {
         RecordFieldsMap* fields = new RecordFieldsMap;
         fields->Stream(s);

         if (fArr.Full()) fArr.PopOnly();
         HistoryItem* h = fArr.PushEmpty();
         h->version = 0; // here is not matter, when applied to other structure will be set
         h->fields = fields;
      }
   }

   return s.verify_size(pos, sz);
}


bool dabc::History::SaveTo(HStore& res)
{
   if (null()) return false;

   // we could use value, restored from binary array
   bool cross_boundary = GetObject()->fCrossBoundary;

   unsigned first = 0;
   if ((res.hlimit()>0) && (GetObject()->fArr.Size() > res.hlimit()))
      first = GetObject()->fArr.Size() - res.hlimit();

   for (unsigned n=first; n < GetObject()->fArr.Size();n++)
      if (GetObject()->fArr.Item(n).version < res.version())
         { cross_boundary = true; break; }

   // if specific version was defined, and we do not have history backward to that version
   // we need to indicate this to the client
   // Client in this case should cleanup its buffers while with gap
   // one cannot correctly reconstruct history backwards
   if (!cross_boundary) res.SetField("history_gap", xmlTrueValue);

   for (unsigned n=first; n < GetObject()->fArr.Size();n++) {
      HistoryItem& item = GetObject()->fArr.Item(n);

      // we have longer history as requested
      if ((res.version()>0) && (item.version <= res.version())) continue;

      res.BeforeNextChild("history");

      res.CreateNode(0);

      item.fields->SaveTo(res);

      res.CloseNode(0);
   }

   res.CloseChilds();

   return true;
}


// ===============================================================================


dabc::HierarchyContainer::HierarchyContainer(const std::string &name) :
   dabc::RecordContainer(name, flNoMutex | flIsOwner),
   fNodeVersion(0),
   fNamesVersion(0),
   fChildsVersion(0),
   fAutoTime(false),
   fPermanent(false),
   fNodeChanged(false),
   fNamesChanged(false),
   fChildsChanged(false),
   fDisableDataReading(false),
   fDisableChildsReading(false),
   fDisableReadingAsChild(false),
   fBinData(),
   fHist(),
   fHierarchyMutex(nullptr)
{
   #ifdef DABC_EXTRA_CHECKS
   DebugObject("Hierarchy", this, 1);
   #endif

//   DOUT0("Constructor %s %p", GetName(), this);
}

dabc::HierarchyContainer::~HierarchyContainer()
{
   #ifdef DABC_EXTRA_CHECKS
   DebugObject("Hierarchy", this, -1);
   #endif

//   DOUT0("Destructor %s %p", GetName(), this);

   if (fHierarchyMutex) {
      delete fHierarchyMutex;
      fHierarchyMutex = nullptr;
   }
}


void dabc::HierarchyContainer::CreateHMutex()
{
   if (!fHierarchyMutex) fHierarchyMutex = new Mutex;
}

dabc::HierarchyContainer* dabc::HierarchyContainer::TopParent()
{
   dabc::HierarchyContainer* parent = this;

   while (parent) {
      dabc::HierarchyContainer* top = dynamic_cast<dabc::HierarchyContainer*> (parent->GetParent());
      if (!top) return parent;
      parent = top;
   }

   EOUT("Should never happen");
   return this;
}

uint64_t dabc::HierarchyContainer::StoreSize(unsigned kind, uint64_t version, unsigned hlimit)
{
   sizestream s;
   Stream(s, kind, version, hlimit);
   //DOUT0("HierarchyContainer::StoreSize %s %u", GetName(), (unsigned) s.size());
   return s.size();
}

bool dabc::HierarchyContainer::Stream(iostream& s, unsigned kind, uint64_t version, unsigned hlimit)
{
   // stream used not only to write or read hierarchy in binary form
   // at the same time it is used to store diff and restore from diff
   // in such case one do not need to recreate everything from the beginning

   uint64_t pos = s.size();
   uint64_t sz = 0;

   uint32_t storesz = 0, storenum = 0, storevers = 0;

   //DOUT0("dabc::HierarchyContainer::Stream %s isout %s isreal %s", GetName(), DBOOL(s.is_output()), DBOOL(s.is_real()));

   if (s.is_output()) {
      bool store_fields = true, store_childs = true, store_history = false, store_diff = false;
      std::string fields_prefix;

      switch (kind) {
         case stream_NamesList: // this is DNS case - only names list
            fields_prefix = "_";
            store_fields = fNamesVersion >= version;
            store_childs = fNamesVersion >= version;
            store_history = false;
            store_diff = store_fields; // we indicate if only selected fields are stored
            break;
         case stream_Value: // only fields are stored
            store_fields = true;
            store_childs = false;
            store_history = !fHist.null() && (hlimit > 0);
            store_diff = false;
            break;
         default: // full stream
            store_fields = fNodeVersion >= version;
            store_childs = fChildsVersion >= version;
            store_history = !fHist.null() && (hlimit > 0);
            break;
      }

      DOUT2("STORE %20s num_childs %u  store %6s  namesv %lu nodevers %lu childsvers %lu", GetName(), NumChilds(), DBOOL(store_childs), (long unsigned)fNamesVersion, (long unsigned)fNodeVersion, (long unsigned) fChildsVersion);

      uint64_t mask = (store_fields ? maskFieldsStored : 0) |
                      (store_childs ? maskChildsStored : 0) |
                      (store_diff ? maskDiffStored : 0) |
                      (store_history ? maskHistory : 0);

      sz = s.is_real() ? StoreSize(kind, version, hlimit) : 0;

      //if (s.is_real()) DOUT0("dabc::HierarchyContainer %s storesize %u", GetName(), (unsigned) sz);

      storesz = sz / 8;
      if (store_childs) storenum = ((uint32_t) NumChilds());
      storenum = storenum | (storevers<<24);

      s.write_uint32(storesz);
      s.write_uint32(storenum);

      s.write_uint64(mask);

      if (store_fields) Fields().Stream(s, fields_prefix);

      if (store_history) fHist()->Stream(s, version, hlimit);

      if (store_childs)
         for (unsigned n=0;n<NumChilds();n++) {
            dabc::HierarchyContainer* child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(n));
            if (!child) continue;
            s.write_str(child->GetName());
            child->Stream(s, kind, version, hlimit);
         }

      //DOUT0("Write childs %u", (unsigned) s.size());

   } else {

      s.read_uint32(storesz);
      sz = ((uint64_t) storesz)*8;
      s.read_uint32(storenum);
      // storevers = storenum >> 24;
      storenum = storenum & 0xffffff;

      uint64_t mask = 0;
      s.read_uint64(mask);

      if (mask & maskFieldsStored) {
         if (fDisableDataReading) {
            if (!s.skip_object()) {
               EOUT("FAIL to skip fields part in the streamer for %s", ItemName().c_str());
            } else {
               DOUT3("SKIP FIELDS in %s", ItemName().c_str());
            }
         } else {
            fNodeChanged = true;
            std::string prefix;
            if (mask & maskDiffStored) { prefix = "_"; fNamesChanged = true; }
            Fields().Stream(s, prefix);
         }
      }

      if (mask & maskHistory) {
         if (fDisableDataReading) {
            if (!s.skip_object())
               EOUT("FAIL to skip history part in the streamer");
            else
               DOUT3("SKIP HISTORY in %s", ItemName().c_str());
         } else {
            if (fHist.null()) fHist.Allocate();
            fHist()->Stream(s, version, hlimit);
         }
      }

      if (mask & maskChildsStored) {
         // when childs cannot be deleted, also order of childs may be broken
         // therefore in that case childs just inserted in arbitrary way
         bool candeletechilds = (kind != stream_NoDelete);

         fChildsChanged = true;
         unsigned tgtcnt = 0;
         // exclude from update permanent items
         if (candeletechilds)
            while ((tgtcnt < NumChilds()) && ((HierarchyContainer*) GetChild(tgtcnt))->fPermanent) tgtcnt++;

         for (unsigned srcnum=0; srcnum<storenum; srcnum++) {
            // first read name of next child
            std::string childname;
            s.read_str(childname);

            if (fDisableChildsReading) {
               if (!s.skip_object())
                  EOUT("FAIL to skip child %s in %s", childname.c_str(), ItemName().c_str());
               else
                  DOUT0("SKIP CHILD %s in %s", childname.c_str(), ItemName().c_str());
               continue;
            }

            dabc::HierarchyContainer* child = nullptr;

            // try to find item with such name in the current list
            unsigned findindx = candeletechilds ? tgtcnt : 0;
            while (findindx<NumChilds()) {
               if (GetChild(findindx)->IsName(childname.c_str())) {
                  child = (HierarchyContainer*) GetChild(findindx);
                  break;
               }
               findindx++;
            }

            if (!child) {
               // if no child with such name found, create and add it in current position
               child = new dabc::HierarchyContainer(childname);

               // TODO: may be tgtcnt should not be used when !candeletechilds is specified
               AddChildAt(child, tgtcnt);
            } else {
               // we need to delete all skipped childs in between
               if (candeletechilds)
                  while (findindx-- > tgtcnt) RemoveChildAt(tgtcnt, true);
            }

            if (child->fDisableReadingAsChild) {
               if (!s.skip_object())
                  EOUT("FAIL to skip item %s completely", child->ItemName().c_str());
               else
                  DOUT3("SKIP item completely %s", child->ItemName().c_str());
            } else {
               child->Stream(s, kind, version, hlimit);
            }

            tgtcnt++;
         }

         // remove remained items at the end
         if (candeletechilds)
            while (tgtcnt < NumChilds()) RemoveChildAt(tgtcnt, true);
      }
   }

   return s.verify_size(pos, sz);
}


bool dabc::HierarchyContainer::SaveTo(HStore& res, bool create_node)
{
   if (create_node)
      res.CreateNode(GetName());

   fFields->SaveTo(res);

   if (res.mask() & storemask_TopVersion) {
      res.SetField(dabc::prop_version, dabc::format("%lu", (long unsigned) fNodeVersion).c_str());
      res.SetMask(res.mask() & ~storemask_TopVersion);
   }

   if ((res.mask() & storemask_Version) != 0) {
      res.SetField("node_ver", dabc::format("%lu", (long unsigned) fNodeVersion).c_str());
      res.SetField("dns_ver", dabc::format("%lu", (long unsigned) fNamesVersion).c_str());
      res.SetField("chld_ver", dabc::format("%lu", (long unsigned) fChildsVersion).c_str());
   }

   if (((res.mask() & storemask_History) != 0) && !fHist.null())
      fHist.SaveTo(res);

   if ((res.mask() & storemask_NoChilds) == 0) {
      for (unsigned n=0;n<NumChilds();n++) {
         dabc::HierarchyContainer* child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(n));
         if (!child) continue;
         res.BeforeNextChild();
         child->SaveTo(res);
      }
   }

   if (create_node)
      res.CloseNode(GetName());

   return true;
}



void dabc::HierarchyContainer::SetModified(bool node, bool hierarchy, bool recursive)
{
   if (node) fNodeChanged = true;
   if (hierarchy) fChildsChanged = true;
   if (recursive)
      for (unsigned n=0;n<NumChilds();n++) {
         dabc::HierarchyContainer* child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(n));
         if (child) child->SetModified(node, hierarchy, recursive);
      }
}


bool dabc::HierarchyContainer::DuplicateHierarchyFrom(HierarchyContainer* cont)
{
   Fields().CopyFrom(cont->Fields(), true);

   // verify if not was changed - do not clear flags, they may be required
   if (!fNodeChanged) fNodeChanged = Fields().WasChanged();

   for (unsigned n=0;n<cont->NumChilds();n++) {
      HierarchyContainer* src_chld = (HierarchyContainer*) cont->GetChild(n);

      HierarchyContainer* child = (HierarchyContainer*) FindChild(src_chld->GetName());
      if (child == 0) {
         child = new HierarchyContainer(src_chld->GetName());
         AddChild(child);
      }

      if (child->DuplicateHierarchyFrom(src_chld)) fChildsChanged = true;
   }

   return fChildsChanged || fNodeChanged;
}


bool dabc::HierarchyContainer::UpdateHierarchyFrom(HierarchyContainer* cont)
{
   fNodeChanged = false;
   fChildsChanged = false;

   if (!cont) throw dabc::Exception(ex_Hierarchy, "empty container", ItemName());

   // we do not check names here - top object name can be different
   // if (!IsName(obj->GetName())) throw dabc::Exception(ex_Hierarchy, "mismatch between object and hierarchy itme", ItemName());

   // we do like manually set/remove all fields, excluding protected fields
   Fields().MoveFrom(cont->Fields());
   if (Fields().WasChanged()) fNodeChanged = true;

   // now we should check if any childs were changed

   unsigned cnt1 = 0; // counter over source container
   unsigned cnt2 = 0; // counter over existing childs

   // skip first permanent childs from update procedure
   while (cnt2 < NumChilds() && ((HierarchyContainer*) GetChild(cnt2))->fPermanent) cnt2++;

   while ((cnt1 < cont->NumChilds()) || (cnt2 < NumChilds())) {
      if (cnt1 >= cont->NumChilds()) {
         RemoveChildAt(cnt2, true);
         fChildsChanged = true;
         continue;
      }

      dabc::Hierarchy cont_child(cont->GetChild(cnt1));
      if (cont_child.null()) {
         EOUT("FAILURE");
         return false;
      }

      bool findchild = false;
      unsigned findindx = 0;

      for (unsigned n=cnt2;n<NumChilds();n++)
         if (GetChild(n)->IsName(cont_child.GetName())) {
            findchild = true;
            findindx = n;
            break;
         }

      if (!findchild) {
         // if child did not found, just take it out form source container and place at proper position

         // remove object and do not cleanup (destroy) it
         cont->RemoveChild(cont_child(), false);

         cont_child()->SetModified(true, true, true);

         AddChildAt(cont_child(), cnt2);

         fChildsChanged = true;
         cnt2++;

         continue;
      }

      while (findindx > cnt2) { RemoveChildAt(cnt2, true); findindx--; fChildsChanged = true; }

      dabc::HierarchyContainer* child = (dabc::HierarchyContainer*) (GetChild(cnt2));

      if (!child || !child->IsName(cont_child.GetName())) {
         throw dabc::Exception(ex_Hierarchy, "mismatch between object and hierarchy item", ItemName());
         return false;
      }

      if (child->UpdateHierarchyFrom(cont_child())) fChildsChanged = true;

      cnt1++;
      cnt2++;
   }

   return fChildsChanged || fNodeChanged;
}

dabc::HierarchyContainer* dabc::HierarchyContainer::CreateChildAt(const std::string &name, int indx)
{
   while ((indx>=0) && (indx<(int) NumChilds())) {
      dabc::HierarchyContainer* child = (dabc::HierarchyContainer*) GetChild(indx);
      if (child->IsName(name.c_str())) return child;
      RemoveChildAt(indx, true);
   }

   dabc::HierarchyContainer* res = new dabc::HierarchyContainer(name);
   AddChild(res);

   //fNamesChanged = true;
   //fChildsChanged = true;

   return res;
}


std::string dabc::HierarchyContainer::ItemName()
{
   std::string res;
   FillFullName(res, 0, true);
   return res;
}


void dabc::HierarchyContainer::BuildObjectsHierarchy(const Reference& top)
{
   if (top.null()) return;

   top()->BuildFieldsMap(&Fields());

   if (top()->IsChildsHidden()) return;

   ReferencesVector chlds;
   if (!top.GetAllChildRef(&chlds)) return;

   for (unsigned n=0;n<chlds.GetSize();n++) {

      if (chlds[n].GetObject()->IsHidden()) continue;

      HierarchyContainer* chld = new HierarchyContainer(chlds[n].GetName());
      AddChild(chld);
      chld->BuildObjectsHierarchy(chlds[n]);
   }
}


void dabc::HierarchyContainer::AddHistory(RecordFieldsMap* diff)
{
   if (fHist.Capacity() == 0) return;
   if (fHist()->fArr.Full()) fHist()->fArr.PopOnly();
   HistoryItem* h = fHist()->fArr.PushEmpty();
   h->version = fNodeVersion;
   h->fields = diff;
}

bool dabc::HierarchyContainer::ExtractHistoryStep(RecordFieldsMap* fields, unsigned step)
{
   if (fHist.Capacity()<=step) return false;

   RecordFieldsMap* hfields = fHist()->fArr.Item(fHist.Capacity()-step-1).fields;

   fields->ApplyDiff(*hfields);

   return true;
}


void dabc::HierarchyContainer::ClearHistoryEntries()
{
   if (fHist.Capacity() == 0) return;
   while (fHist()->fArr.Size() > 0) fHist()->fArr.PopOnly();
}


bool dabc::HierarchyContainer::IsNodeChanged(bool withchilds)
{
   if (fNodeChanged || fChildsChanged) return true;

   if (Fields().WasChanged()) { fNodeChanged = true; return true; }

   if (withchilds)
      for (unsigned indx=0; indx < NumChilds(); indx++) {
         dabc::HierarchyContainer* child = (dabc::HierarchyContainer*) GetChild(indx);
         if (child && child->IsNodeChanged(withchilds)) {
            fChildsChanged = true;
            return true;
         }
      }

   return false;
}

bool dabc::HierarchyContainer::CheckIfDoingHistory()
{
   if (!fHist.null()) return fHist.DoHistory();

   HierarchyContainer* prnt = dynamic_cast<HierarchyContainer*> (GetParent());
   while (prnt != nullptr) {

      if (prnt->fHist.DoHistory() && prnt->fHist()->fChildsEnabled) {
         fHist.Allocate(prnt->fHist.Capacity());
         fHist()->fEnabled = true;
         fHist()->fChildsEnabled = true;
         Fields().Field(prop_history).SetUInt(prnt->fHist.Capacity());
         return true;
      }

      prnt = dynamic_cast<HierarchyContainer*> (prnt->GetParent());
   }

   return false;
}

enum ChangeBits {
   change_Value  = 0x1,
   change_Names  = 0x2,
   change_Childs = 0x4
};


void dabc::HierarchyContainer::MarkReading(bool withchilds, bool readvalues, bool readchilds)
{
   if (withchilds)
      for (unsigned indx=0; indx < NumChilds(); indx++) {
         dabc::HierarchyContainer* child = (dabc::HierarchyContainer*) GetChild(indx);
         if (child) child->MarkReading(withchilds, readvalues, readchilds);
      }

   fDisableDataReading = !readvalues;
   fDisableChildsReading = !readchilds;
}


unsigned dabc::HierarchyContainer::MarkVersionIfChanged(uint64_t ver, uint64_t& tm, bool withchilds)
{
   unsigned mask = 0;

   if (withchilds)
      for (unsigned indx=0; indx < NumChilds(); indx++) {
         dabc::HierarchyContainer* child = (dabc::HierarchyContainer*) GetChild(indx);
         if (child) mask = mask | child->MarkVersionIfChanged(ver, tm, withchilds);
      }

   if (mask != 0) fNodeChanged = true;

   if (mask & change_Names) fNamesChanged = true;

   // if any child was changed directly (bit 1)
   if (mask & (change_Value | change_Childs)) fChildsChanged = true;

   bool fields_were_chaneged = Fields().WasChanged();

   if (fields_were_chaneged) {
      fNodeChanged = true;
      if (Fields().WasChangedWith("_")) fNamesChanged = true;
   }

   if (fNodeChanged) {

      if (fNodeChanged) { fNodeVersion = ver; mask = mask | change_Value; }
      if (fNamesChanged) { fNamesVersion = ver; mask = mask | change_Names; }
      if (fChildsChanged) { fChildsVersion = ver; mask = mask | change_Childs; }

      if (fNodeChanged && fAutoTime) {
         if (tm == 0) tm = dabc::DateTime().GetNow().AsJSDate();
         Fields().Field(prop_time).SetDatime(tm);
      }

      if (CheckIfDoingHistory() && fields_were_chaneged) {
         // we need to be sure that really something changed,
         // otherwise we will produce empty entry
         RecordFieldsMap *prev = fHist()->fPrev;

         if (prev) {
            prev->MakeAsDiffTo(Fields());
            AddHistory(prev);
         }

         fHist()->fPrev = Fields().Clone();
      }

      // clear flags only after we produce diff
      Fields().ClearChangeFlags();
      fNodeChanged = false;
      fNamesChanged = false;
      fChildsChanged = false;
   }

   return mask;
}

uint64_t dabc::HierarchyContainer::GetNextVersion() const
{
   const HierarchyContainer* top = this, *prnt = this;

   while (prnt) {
     top = prnt;
     prnt = dynamic_cast<const HierarchyContainer*> (prnt->GetParent());
   }

   return top->GetVersion() + 1;
}


void dabc::HierarchyContainer::MarkChangedItems(uint64_t tm)
{
   uint64_t next_ver = GetNextVersion();

   // mark node and childs
   unsigned mask = MarkVersionIfChanged(next_ver, tm, true);

   if (mask == 0) return;

   // mark changes in parent
   HierarchyContainer* prnt = dynamic_cast<HierarchyContainer*> (GetParent());
   while (prnt != 0) {
      prnt->fNodeChanged = true;
      if (mask & change_Names) prnt->fNamesChanged = true;
      if (mask & (change_Value | change_Childs)) prnt->fChildsChanged = true;

      prnt->MarkVersionIfChanged(next_ver, tm, false);
      prnt = dynamic_cast<HierarchyContainer*> (prnt->GetParent());
   }
}

void dabc::HierarchyContainer::EnableTimeRecording(bool withchilds)
{
   fAutoTime = true;
   if (withchilds)
      for (unsigned indx=0; indx < NumChilds(); indx++) {
         dabc::HierarchyContainer* child = (dabc::HierarchyContainer*) GetChild(indx);
         if (child) child->EnableTimeRecording(withchilds);
      }
}

// __________________________________________________-


void dabc::Hierarchy::BuildNew(Reference top)
{
   if (null()) Create("Objects");
   GetObject()->BuildObjectsHierarchy(top);
}


bool dabc::Hierarchy::Update(dabc::Hierarchy& src)
{
   if (src.null()) {
      // now release will work - any hierarchy will be deleted once no any other refs exists
      Release();
      return true;
   }

   if (null()) {
      EOUT("Hierarchy structure MUST exist at this moment");
      Create("DABC");
   }

   if (GetObject()->UpdateHierarchyFrom(src()))
      GetObject()->MarkChangedItems();

   return true;
}

bool dabc::Hierarchy::Duplicate(const Hierarchy& src)
{
   if (src.null()) return true;

   if (null()) {
      EOUT("Hierarchy structure MUST exist at this moment");
      Create("DABC");
   }

   if (GetObject()->DuplicateHierarchyFrom(src()))
      GetObject()->MarkChangedItems();

   return true;
}


void dabc::Hierarchy::EnableHistory(unsigned length, bool withchilds)
{
   if (null()) return;

   if (length>0) GetObject()->EnableTimeRecording(withchilds);

   if (GetObject()->fHist.Capacity() != length) {
      if (length>0) {
         GetObject()->fHist.Allocate(length);
         GetObject()->fHist()->fEnabled = true;
         GetObject()->fHist()->fChildsEnabled = withchilds;
         SetField(prop_history, length);
      } else {
         GetObject()->fHist.Release();
         RemoveField(prop_history);
      }
   }
}


bool dabc::Hierarchy::HasLocalHistory() const
{
   return null() ? false : GetObject()->fHist.DoHistory();
}


bool dabc::Hierarchy::HasActualRemoteHistory() const
{
   if (null() || (GetObject()->fHist.Capacity() == 0)) return false;

   // we have actual copy of remote history when request was done after last hierarchy update
   return ((GetObject()->fHist()->fRemoteReqVersion > 0) &&
           (GetObject()->fHist()->fLocalReqVersion == GetObject()->GetVersion()));
}


dabc::Buffer dabc::Hierarchy::SaveToBuffer(unsigned kind, uint64_t version, unsigned hlimit)
{
   if (null()) return dabc::Buffer();

   uint64_t size = GetObject()->StoreSize(kind, version, hlimit);

   dabc::Buffer res = dabc::Buffer::CreateBuffer(size);
   if (res.null()) return res;

   memstream outs(false, (char*) res.SegmentPtr(), res.SegmentSize());

   if (GetObject()->Stream(outs, kind, version, hlimit)) {

      if (size != outs.size()) { EOUT("Sizes mismatch %lu %lu", (long unsigned) size, (long unsigned) outs.size()); }

      res.SetTotalSize(size);
   } else {
      res.Release();
   }

   return res;
}

bool dabc::Hierarchy::ReadFromBuffer(const dabc::Buffer& buf)
{
   if (buf.null()) return false;

   memstream inps(true, (char*) buf.SegmentPtr(), buf.SegmentSize());

   if (null()) Create("RAW");

   if (!GetObject()->Stream(inps)) {
      EOUT("Cannot reconstruct hierarchy from the binary data!");
      return false;
   }

   return true;
}

bool dabc::Hierarchy::UpdateFromBuffer(const dabc::Buffer& buf, HierarchyStreamKind kind)
{
   if (buf.null()) return false;

   if (null()) {
      EOUT("Hierarchy must be created before it can be updated");
      Create("TOP");
   }

   memstream inps(true, (char*) buf.SegmentPtr(), buf.SegmentSize());

   if (!GetObject()->Stream(inps, kind)) {
      EOUT("Cannot reconstruct hierarchy from the binary data!");
      return false;
   }

   GetObject()->MarkChangedItems();

   return true;
}


void dabc::Hierarchy::Create(const std::string &name, bool withmutex)
{
   Release();

   HierarchyContainer* cont = new HierarchyContainer(name);
   if (withmutex) cont->CreateHMutex();
   SetObject(cont);
}


dabc::Hierarchy dabc::Hierarchy::GetHChild(const std::string &name, bool allowslahes, bool force, bool sortorder)
{
   // DOUT0("GetHChild main:%p name:%s force %s", this, name.c_str(), DBOOL(force));

   // if name empty, return null
   if (name.empty()) return dabc::Hierarchy();

   size_t pos = name.rfind("/");
   if ((pos != std::string::npos) && !allowslahes) {
      if (pos == 0) return GetHChild(name.substr(1), allowslahes, force, sortorder);
      if (pos == name.length()-1) return GetHChild(name.substr(0, name.length()-1), allowslahes, force, sortorder);
      return GetHChild(name.substr(0, pos), allowslahes, force, sortorder).GetHChild(name.substr(pos+1), allowslahes, force, sortorder);
   }

   std::string itemname = name;
   if (pos != std::string::npos) itemname = itemname.substr(pos+1);
   if (itemname.empty()) itemname = "item";

   // replace all special symbols which can make problem in http (not in xml)
   while ((pos = itemname.find_first_of("#:&?")) != std::string::npos)
      itemname.replace(pos, 1, "_");

   if (itemname == name) {
      // simple case - no any special symbols
      dabc::Hierarchy res = FindChild(name.c_str());
      if (!res.null() && !res.HasField(dabc::prop_realname)) {
         return res;
      } else if (res.null() && !force) {
         return res;
      }
   }

   int first_big = -1;

   for (unsigned n=0;n<NumChilds();n++) {
      dabc::Hierarchy res = GetChild(n);

      std::string realname = res.GetName();
      if (res.HasField(dabc::prop_realname))
         realname = res.GetField(dabc::prop_realname).AsStr();

      if (realname == name) return res;

      if (sortorder && (first_big < 0) && (realname > name))
         first_big = (int) n;
   }

   if (!force) return dabc::Hierarchy();

   if (itemname != name) {
      unsigned cnt = NumChilds();
      std::string basename = itemname;

      // prevent same item name
      while (!FindChild(itemname.c_str()).null())
         itemname = basename + std::to_string(cnt++);
   }

   dabc::Hierarchy res = new dabc::HierarchyContainer(itemname);

   if (!sortorder || (first_big < 0))
      GetObject()->AddChild(res.GetObject());
   else
      GetObject()->AddChildAt(res.GetObject(), first_big);

   if (itemname != name)
      res.SetField(dabc::prop_realname, name);

   //fNamesChanged = true;
   //fChildsChanged = true;

   return res;
}

bool dabc::Hierarchy::RemoveHChild(const std::string &path, bool allowslahes)
{
   dabc::Hierarchy h = GetHChild(path, allowslahes);
   if (h.null()) return false;

   dabc::Hierarchy prnt = h.GetParentRef();
   h.Destroy();
   while (!prnt.null() && (prnt() != GetObject())) {
      h << prnt;
      prnt = h.GetParentRef();
      if (h.NumChilds() != 0) break;
      h.Destroy(); // delete as long no any other involved
   }
   return true;
}


dabc::Hierarchy dabc::Hierarchy::FindMaster() const
{
   if (null() || !GetParent() || !HasField(dabc::prop_masteritem)) return dabc::Hierarchy();

   std::string masteritem = GetField(dabc::prop_masteritem).AsStr();

   if (masteritem.empty()) return dabc::Hierarchy();

   return GetParent()->FindChildRef(masteritem.c_str());
}


bool dabc::Hierarchy::IsBinItemChanged(const std::string &itemname, uint64_t hash, uint64_t last_version)
{
   if (null()) return false;

   dabc::Hierarchy item;
   if (itemname.empty()) item = *this; else item = FindChild(itemname.c_str());

   // if item for the object not found, we suppose to always have changed object
   if (item.null()) return true;

   // if there is no hash information provided, we always think that binary data is changed
   if (hash == 0) return true;

   if (item.Field(dabc::prop_hash).AsUInt() != hash) {
      item.SetField(dabc::prop_hash, hash);
      item.SetFieldProtected(dabc::prop_hash, true);
      item.MarkChangedItems();
   }

   return (last_version == 0) || (last_version < item.GetVersion());
}


bool dabc::Hierarchy::FillBinHeader(const std::string &itemname, dabc::Command& cmd, uint64_t mhash, const std::string &dflt_master_name)
{
   if (null()) return false;

   dabc::Hierarchy item, master;
   if (itemname.empty()) item = *this;
                    else item = FindChild(itemname.c_str());

   if (item.null() && !dflt_master_name.empty()) {
      master = FindChild(dflt_master_name.c_str());
      DOUT2("Found default master %s res = %p", dflt_master_name.c_str(), master());
   } else {
      master = item.FindMaster();
   }

   if (!master.null() && (mhash != 0) && (master.GetField(dabc::prop_hash).AsUInt()!=mhash)) {
      master.SetField(dabc::prop_hash, mhash);
      master.SetFieldProtected(dabc::prop_hash, true);
      master.MarkChangedItems();
   }

   if (!item.null()) cmd.SetUInt("BVersion", item.GetVersion());
   if (!master.null()) cmd.SetUInt("MVersion", master.GetVersion());

   return true;
}


std::string dabc::Hierarchy::FindBinaryProducer(std::string& request_name, bool topmost)
{
   dabc::Hierarchy parent = *this;
   std::string producer_name;

   // we need to find parent on the most-top position with binary_producer property
   while (!parent.null()) {
      if (parent.HasField(dabc::prop_producer)) {
         producer_name = parent.Field(dabc::prop_producer).AsStr();
         request_name = RelativeName(parent);
         if (!topmost) break;
      }
      parent = parent.GetParentRef();
   }

   return producer_name;
}


bool dabc::Hierarchy::RemoveEmptyFolders(const std::string &path)
{
   dabc::Hierarchy h = GetFolder(path);
   if (h.null()) return false;

   dabc::Hierarchy prnt = h.GetParentRef();
   h.Destroy();
   while (!prnt.null() && (prnt() != GetObject())) {
      h << prnt;
      prnt = h.GetParentRef();
      if (h.NumChilds() != 0) break;
      h.Destroy(); // delete as long no any other involved
   }
   return true;
}

std::string dabc::Hierarchy::ItemName() const
{
   if (null()) return std::string();
   return GetObject()->ItemName();
}

dabc::Hierarchy dabc::Hierarchy::GetTop() const
{
   HierarchyContainer* obj = GetObject();
   while (obj) {
      HierarchyContainer* prnt = dynamic_cast<HierarchyContainer*> (obj->GetParent());
      if (!prnt) break;
      obj = prnt;
   }
   return dabc::Hierarchy(obj);
}

bool dabc::Hierarchy::DettachFromParent()
{
   Object* prnt = GetParent();

   return prnt ?  prnt->RemoveChild(GetObject(), false) : true;
}

void dabc::Hierarchy::DisableReading(bool withchlds)
{
   if (!null()) GetObject()->MarkReading(withchlds, false, false);
}

void dabc::Hierarchy::DisableReadingAsChild()
{
   if (!null()) GetObject()->fDisableReadingAsChild = true;
}

void dabc::Hierarchy::EnableReading(const Hierarchy& upto)
{
   if (null()) return;
   GetObject()->MarkReading(false, true, true);

   if (upto.null()) return;

   dabc::Hierarchy prnt = GetParentRef();

   while (!prnt.null()) {
      prnt()->MarkReading(false, false, true);
      if (prnt == upto) break;
      prnt = prnt.GetParentRef();
   }
}


// =====================================================

namespace dabc {
   class HistoryIterContainer : public RecordContainer {
      friend class HistoryIter;

      protected:
         Hierarchy fSrc;
         int fCnt{0};

         bool next()
         {
            if (fSrc.null()) return false;

            if (fCnt <= 0) {
               RecordFieldsMap* fields = fSrc()->Fields().Clone();
               SetFieldsMap(fields);
               fCnt = 1;
               return true;
            }

            if (fSrc()->ExtractHistoryStep(&Fields(), fCnt-1)) {
               fCnt++;
               return true;
            }

            SetFieldsMap(new RecordFieldsMap);
            fCnt = 0;

            return false;
         }

      public:

         HistoryIterContainer(Hierarchy& src) :
            RecordContainer(src.GetName()),
            fSrc(src),
            fCnt(0)
         {
         }

   };
}


bool dabc::HistoryIter::next()
{
   HistoryIterContainer* cont = dynamic_cast<HistoryIterContainer*> (GetObject());

   return cont ? cont->next() : false;
}

dabc::HistoryIter dabc::Hierarchy::MakeHistoryIter()
{
   dabc::HistoryIter res;

   if (!null())
      res = new HistoryIterContainer(*this);

   return res;
}

