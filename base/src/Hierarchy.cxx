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

#include <string.h>
#include <stdlib.h>

#include "dabc/Iterator.h"
#include "dabc/timing.h"
#include "dabc/threads.h"
#include "dabc/logging.h"
#include "dabc/Exception.h"
#include "dabc/Command.h"
#include "dabc/ConfigBase.h"
#include "dabc/ReferencesVector.h"


const char* dabc::prop_version = "dabc:version";
const char* dabc::prop_kind = "dabc:kind";
const char* dabc::prop_realname = "dabc:realname"; // real object name
const char* dabc::prop_itemname = "dabc:itemname"; // item name in dabc hierarchy
const char* dabc::prop_masteritem = "dabc:master";
const char* dabc::prop_producer = "dabc:producer";
const char* dabc::prop_error = "dabc:error";
const char* dabc::prop_hash = "hash";
const char* dabc::prop_history = "dabc:history";
const char* dabc::prop_time = "time";
const char* dabc::prop_more = "dabc:more";


// ===============================================================================


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

   uint32_t storesz(0), storenum(0), storevers(0);

   if (s.is_output()) {
      sz = s.is_real() ? StoreSize(version, hlimit) : 0;
      storesz = sz / 8;

      uint32_t first = 0;
      if ((hlimit>0) && (fArr.Size() > (unsigned) hlimit)) first = fArr.Size() - hlimit;
      storenum = 0;
      bool cross_boundary = false;
      for (unsigned n=first; n<fArr.Size();n++) {
         // we have longer history as requested
         if ((version>0) && (fArr.Item(n).version < version)) {
            cross_boundary = true;
            continue;
         }
         storenum++;
      }

      s.write_uint32(storesz);
      s.write_uint32(storenum | (storevers<<24));

      uint64_t mask = cross_boundary ? 1 : 0;
      s.write_uint64(mask);

      for (uint32_t n=first;n<fArr.Size();n++) {
         if ((version>0) && (fArr.Item(n).version < version)) continue;
         fArr.Item(n).fields->Stream(s);
      }

   } else {
      s.read_uint32(storesz);
      sz = ((uint64_t) storesz) * 8;
      s.read_uint32(storenum);
      // storevers = storenum >> 24;
      storenum = storenum & 0xffffff;

      uint64_t mask(0);
      s.read_uint64(mask);
      fCrossBoundary = (mask & 1) != 0;

      if (fArr.Capacity()==0)
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

bool dabc::History::SaveInXmlNode(XMLNodePointer_t topnode, uint64_t version, unsigned hlimit)
{
   if (null() || (topnode==0)) return false;

   // we could use value, restored from binary array
   bool cross_boundary = GetObject()->fCrossBoundary;

//   DOUT0("RequestHistoryAsXml req_version %u", (unsigned) version);

   unsigned first = 0;
   if ((hlimit>0) && (GetObject()->fArr.Size() > hlimit))
      first = GetObject()->fArr.Size() - hlimit;

   for (unsigned n=first; n < GetObject()->fArr.Size();n++) {
      HistoryItem& item = GetObject()->fArr.Item(n);

      // we have longer history as requested
      if (item.version < version) {
         cross_boundary = true;
         // DOUT0("    ignore item %u version %u", n, (unsigned) item.version);
         continue;
      }

      XMLNodePointer_t hnode = Xml::NewChild(topnode, 0, "h", 0);
      item.fields->SaveInXml(hnode);
//      DOUT0("    append item %u version %u value %s", n, (unsigned) item.version, item.fields->Field("value").AsStr().c_str());
   }

   // if specific version was defined, and we do not have history backward to that version
   // we need to indicate this to the client
   // Client in this case should cleanup its buffers while with gap
   // one cannot correctly reconstruct history backwards
   if (!cross_boundary) Xml::NewAttr(topnode, 0, "gap", xmlTrueValue);

   return true;
}


// ===============================================================================


dabc::HierarchyContainer::HierarchyContainer(const std::string& name) :
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
   fHierarchyMutex(0)
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

   if (fHierarchyMutex!=0) {
      delete fHierarchyMutex;
      fHierarchyMutex = 0;
   }
}


void dabc::HierarchyContainer::CreateHMutex()
{
   if (fHierarchyMutex==0) fHierarchyMutex = new Mutex;
}

dabc::HierarchyContainer* dabc::HierarchyContainer::TopParent()
{
   dabc::HierarchyContainer* parent = this;

   while (parent!=0) {
      dabc::HierarchyContainer* top = dynamic_cast<dabc::HierarchyContainer*> (parent->GetParent());
      if (top==0) return parent;
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

   uint32_t storesz(0), storenum(0), storevers(0);

   //DOUT0("dabc::HierarchyContainer::Stream %s isout %s isreal %s", GetName(), DBOOL(s.is_output()), DBOOL(s.is_real()));

   if (s.is_output()) {
      bool store_fields(true), store_childs(true), store_history(false), store_diff(false);
      std::string fields_prefix;

      switch (kind) {
         case stream_NamesList: // this is DNS case - only names list
            fields_prefix = "dabc:";
            store_fields = fNamesVersion >= version;
            store_childs = fNamesVersion >= version;
            store_history = false;
            store_diff = store_fields; // we indicate if only selected fields are stored
            break;
         case stream_Value: // only fields are stored
            fields_prefix = "";
            store_fields = true;
            store_childs = false;
            store_history = !fHist.null() && (hlimit>0);
            store_diff = false;
            break;
         default: // full stream
            store_fields = fNodeVersion >= version;
            store_childs = fChildsVersion >= version;
            store_history = !fHist.null() && (hlimit>0);
            break;
      }

      DOUT2("STORE %20s num_childs %u  store %6s  namesv %u nodevers %u childsvers %u", GetName(), NumChilds(), DBOOL(store_childs), fNamesVersion, fNodeVersion, fChildsVersion);

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
            if (child==0) continue;
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

      uint64_t mask(0);
      s.read_uint64(mask);

      if (mask & maskFieldsStored) {
         if (fDisableDataReading) {
            if (!s.skip_object())
               EOUT("FAIL to skip fields part in the streamer for %s", ItemName().c_str());
            else
               DOUT3("SKIP FIELDS in %s", ItemName().c_str());
         } else {
            fNodeChanged = true;
            std::string prefix;
            if (mask & maskDiffStored) { prefix = "dabc:"; fNamesChanged = true; }
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

            dabc::HierarchyContainer* child = 0;

            // try to find item with such name in the current list
            unsigned findindx = candeletechilds ? tgtcnt : 0;
            while (findindx<NumChilds()) {
               if (GetChild(findindx)->IsName(childname.c_str())) {
                  child = (HierarchyContainer*) GetChild(findindx);
                  break;
               }
               findindx++;
            }

            if (child == 0) {
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

dabc::XMLNodePointer_t dabc::HierarchyContainer::SaveContainerInXmlNode(XMLNodePointer_t parent, const std::string& altname)
{
   std::string name = GetName();

   XMLNodePointer_t node;

   if (name.find_first_of("[]&<>") != std::string::npos) {
      node = Xml::NewChild(parent, 0, altname.c_str(), 0);
      Xml::NewAttr(node, 0, dabc::prop_itemname, name.c_str());
   } else {
      node = Xml::NewChild(parent, 0, name.c_str(), 0);
   }

   fFields->SaveInXml(node);

   return node;
}


dabc::XMLNodePointer_t dabc::HierarchyContainer::SaveHierarchyInXmlNode(XMLNodePointer_t parentnode, unsigned mask, unsigned chldcnt)
{
   std::string altname = (chldcnt != (unsigned)-1) ? dabc::format("__item%u__",chldcnt) : std::string("topitem");

   XMLNodePointer_t objnode = SaveContainerInXmlNode(parentnode, altname);

   if ((mask & xmlmask_Version) != 0) {
      Xml::NewIntAttr(objnode, "node_ver", fNodeVersion);
      Xml::NewIntAttr(objnode, "dns_ver", fNamesVersion);
      Xml::NewIntAttr(objnode, "chld_ver", fChildsVersion);
   }

   if (((mask & xmlmask_History) != 0) && !fHist.null()) {
      XMLNodePointer_t histnode = Xml::NewChild(objnode, 0, "history", 0);
      fHist.SaveInXmlNode(histnode, 0, 0);
   }

   if ((mask & xmlmask_NoChilds) == 0)
      for (unsigned n=0;n<NumChilds();n++) {
         dabc::HierarchyContainer* child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(n));
         if (child) child->SaveHierarchyInXmlNode(objnode, mask, n);
      }

   return objnode;
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

dabc::Buffer dabc::HierarchyContainer::RequestHistory(uint64_t version, int hlimit)
{
   dabc::Buffer res;

   if ((fHist.Capacity()==0) || (hlimit<0)) return res;

   uint64_t size = StoreSize(version, hlimit);

   res = dabc::Buffer::CreateBuffer(size);

   memstream outs(false, (char*) res.SegmentPtr(), res.SegmentSize());

   if (Stream(outs, version, hlimit)) {

      if (size != outs.size()) { EOUT("Sizes mismatch %lu %lu", (long unsigned) size, (long unsigned) outs.size()); }

      res.SetTotalSize(size);
   }

   return res;
}


std::string dabc::HierarchyContainer::RequestHistoryAsXml(uint64_t version, int limit)
{
//   DOUT0("RequestHistoryAsXml capacity %u", (unsigned) fHist.Capacity());

   if (fHist.Capacity()==0) return "";

   XMLNodePointer_t topnode = Xml::NewChild(0, 0, "gethistory", 0);

   Xml::NewAttr(topnode, 0, "xmlns:dabc", "http://dabc.gsi.de/xhtml");
   Xml::NewAttr(topnode, 0, prop_version, dabc::format("%lu", (long unsigned) GetVersion()).c_str());

   SaveContainerInXmlNode(topnode,"item");

   bool cross_boundary = false;

//   DOUT0("RequestHistoryAsXml req_version %u", (unsigned) version);

   unsigned first = 0;
   if ((limit>0) && (fHist()->fArr.Size() > (unsigned) limit))
      first = fHist()->fArr.Size() - limit;

   for (unsigned n=first; n<fHist()->fArr.Size();n++) {
      HistoryItem& item = fHist()->fArr.Item(n);

      // we have longer history as requested
      if ((version>0) && (item.version < version)) {
         cross_boundary = true;
//         DOUT0("    ignore item %u version %u", n, (unsigned) item.version);
         continue;
      }

      XMLNodePointer_t hnode = Xml::NewChild(topnode, 0, "h", 0);
      item.fields->SaveInXml(hnode);
//      DOUT0("    append item %u version %u value %s", n, (unsigned) item.version, item.fields->Field("value").AsStr().c_str());
   }

   // if specific version was defined, and we do not have history backward to that version
   // we need to indicate this to the client
   // Client in this case should cleanup its buffers while with gap
   // one cannot correctly reconstruct history backwards
   if ((version>0) && !cross_boundary)
      Xml::NewAttr(topnode, 0, "gap", xmlTrueValue);

   std::string res;

   Xml::SaveSingleNode(topnode, &res, 1);
   Xml::FreeNode(topnode);

   return res;
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

   if (cont==0) throw dabc::Exception(ex_Hierarchy, "empty container", ItemName());

   // we do not check names here - top object name can be different
   // if (!IsName(obj->GetName())) throw dabc::Exception(ex_Hierarchy, "mismatch between object and hierarchy itme", ItemName());

   // we do like we manually set/remove all fields, excluding fields starting from 'dabc:'

   Fields().MoveFrom(cont->Fields()); // FIXME: probably have no need to filter fields
   if (Fields().WasChanged()) fNodeChanged = true;

   // now we should check if any childs were changed

   unsigned cnt1(0); // counter over source container
   unsigned cnt2(0); // counter over existing childs

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

      bool findchild(false);
      unsigned findindx(0);

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

      if ((child==0) || !child->IsName(cont_child.GetName())) {
         throw dabc::Exception(ex_Hierarchy, "mismatch between object and hierarchy item", ItemName());
         return false;
      }

      if (child->UpdateHierarchyFrom(cont_child())) fChildsChanged = true;

      cnt1++;
      cnt2++;
   }

   return fChildsChanged || fNodeChanged;
}

dabc::HierarchyContainer* dabc::HierarchyContainer::CreateChildAt(const std::string& name, int indx)
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
   if (fHist.Capacity()==0) return;
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
   if (fHist.Capacity()==0) return;
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
   while (prnt != 0) {

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
      if (Fields().WasChangedWith("dabc:")) fNamesChanged = true;
   }

   if (fNodeChanged) {

      if (fNodeChanged) { fNodeVersion = ver; mask = mask | change_Value; }
      if (fNamesChanged) { fNamesVersion = ver; mask = mask | change_Names; }
      if (fChildsChanged) { fChildsVersion = ver; mask = mask | change_Childs; }

      if (fNodeChanged && fAutoTime) {
         if (tm==0) {
            dabc::DateTime dt;
            if (dt.GetNow()) tm = dt.AsJSDate();
         }
         Fields().Field(prop_time).SetDatime(tm);
      }

      if (CheckIfDoingHistory() && fields_were_chaneged) {
         // we need to be sure that really something changed,
         // otherwise we will produce empty entry
         RecordFieldsMap* prev = fHist()->fPrev;

         if (prev!=0) {
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

   while (prnt!=0) {
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


std::string dabc::Hierarchy::SaveToXml(unsigned mask, const std::string& path)
{
   XMLNodePointer_t topnode = GetObject()->SaveHierarchyInXmlNode(0, mask);

   if ((mask & xmlmask_TopVersion) != 0)
      Xml::NewAttr(topnode, 0, dabc::prop_version, dabc::format("%lu", (long unsigned) GetVersion()).c_str());

   if ((mask & xmlmask_NameSpace) != 0)
      Xml::NewAttr(topnode, 0, "xmlns:dabc", "http://dabc.gsi.de/xhtml");

   std::string res;

   if (topnode) {
      bool compact = (mask & xmlmask_Compact) != 0;
      Xml::SaveSingleNode(topnode, &res, compact ? 0 : 1);
      Xml::FreeNode(topnode);
   }

   if ((mask & xmlmask_TopDabc) != 0) {
      const char* lastslash = "";
      if (!path.empty() && (path[path.length()-1]!='/')) lastslash = "/";

      res = dabc::format("<dabc version=\"2\" xmlns:dabc=\"http://dabc.gsi.de/xhtml\" path=\"%s%s\">\n",
            path.c_str(), lastslash) +
            res +
            std::string("</dabc>\n");
   }

   return res;
}

void dabc::Hierarchy::Create(const std::string& name, bool withmutex)
{
   Release();

   HierarchyContainer* cont = new HierarchyContainer(name);
   if (withmutex) cont->CreateHMutex();
   SetObject(cont);
}

dabc::Hierarchy dabc::Hierarchy::CreateChild(const std::string& name, int indx, bool check_name)
{
   if (null() || name.empty()) return dabc::Hierarchy();

   std::string itemname = name;

   if (check_name) {
      size_t pos = (itemname.find_last_of("/"));
      if (pos!=std::string::npos) itemname = itemname.substr(pos+1);
      if (itemname.empty()) itemname = "item";

      // replace all special symbols which can make problem in http (not in xml)
      while ((pos = itemname.find_first_of("#:&?")) != std::string::npos)
         itemname.replace(pos, 1, "_");

      unsigned cnt = NumChilds();
      std::string basename = itemname;

      // prevent same item name
      while (!FindChild(itemname.c_str()).null()) {
         itemname = dabc::format("%s%d", basename.c_str(), cnt++);
      }
   }

   dabc::Hierarchy res = GetObject()->CreateChildAt(itemname, indx);

   if (check_name && (itemname != name))
      res.SetField(dabc::prop_realname, name);

   return res;
}

dabc::Hierarchy dabc::Hierarchy::FindMaster() const
{
   if (null() || (GetParent()==0) || !HasField(dabc::prop_masteritem)) return dabc::Hierarchy();

   std::string masteritem = GetField(dabc::prop_masteritem).AsStr();

   if (masteritem.empty()) return dabc::Hierarchy();

   return GetParent()->FindChildRef(masteritem.c_str());
}

bool dabc::Hierarchy::IsBinItemChanged(const std::string& itemname, const std::string& hash, uint64_t last_version)
{
   if (null()) return false;

   dabc::Hierarchy item;
   if (itemname.empty()) item = *this; else item = FindChild(itemname.c_str());

   // if item for the object not found, we suppose to always have changed object
   if (item.null()) return true;

   // if there is no hash information provided, we always think that binary data is changed
   if (hash.empty()) return true;

   if (item.Field(dabc::prop_hash).AsStr() != hash) {
      item.SetField(dabc::prop_hash, hash);
      item.MarkChangedItems();
   }

   return (last_version==0) || (last_version<item.GetVersion());
}



bool dabc::Hierarchy::FillBinHeader(const std::string& itemname, const dabc::Buffer& bindata, const std::string& mhash, const std::string& dflt_master_name)
{
   if (null()) return false;

   dabc::Hierarchy item, master;
   if (itemname.empty()) item = *this;
                    else item = FindChild(itemname.c_str());

   if (item.null()) {
      master = FindChild(dflt_master_name.c_str());
      DOUT2("Search default master %s res = %p", dflt_master_name.c_str(), master());
   } else {
      master = item.FindMaster();
   }

   if (!master.null() && !mhash.empty() && (master.Field(dabc::prop_hash).AsStr()!=mhash)) {
      master.SetField(dabc::prop_hash, mhash);
      master.MarkChangedItems();
   }

   if (!bindata.null()) {
      dabc::BinDataHeader* bindatahdr = (dabc::BinDataHeader*) bindata.SegmentPtr();
      bindatahdr->version = item.GetVersion();
      bindatahdr->master_version = master.GetVersion();
   }

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


bool dabc::Hierarchy::RemoveEmptyFolders(const std::string& path)
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
   while (obj!=0) {
      HierarchyContainer* prnt = dynamic_cast<HierarchyContainer*> (obj->GetParent());
      if (prnt == 0) break;
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
         int fCnt;

         bool next()
         {
            if (fSrc.null()) return false;

            if (fCnt<=0) {
               RecordFieldsMap* fields = fSrc()->Fields().Clone();
               SetFieldsMap(fields);
               fCnt=1;
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

