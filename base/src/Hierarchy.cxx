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
#include "dabc/logging.h"
#include "dabc/Exception.h"
#include "dabc/Command.h"


const char* dabc::prop_version = "dabc:version";
const char* dabc::prop_kind = "dabc:kind";
const char* dabc::prop_realname = "dabc:realname";
const char* dabc::prop_masteritem = "dabc:master";
const char* dabc::prop_binary_producer = "dabc:binary_producer";
const char* dabc::prop_content_hash = "dabc:hash";
const char* dabc::prop_history = "dabc:history";
const char* dabc::prop_time = "dabc:time";


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
      sz = ((uint64_t) storesz)*8;
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

// ===============================================================================


dabc::HierarchyContainer::HierarchyContainer(const std::string& name) :
   dabc::RecordContainer(name, flNoMutex | flIsOwner),
   fNodeVersion(0),
   fHierarchyVersion(0),
   fAutoTime(false),
   fPermanent(false),
   fNodeChanged(false),
   fHierarchyChanged(false),
   fDiffNode(false),
   fBinData(),
   fHist()
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

uint64_t dabc::HierarchyContainer::StoreSize(uint64_t version, int hlimit)
{
   sizestream s;
   Stream(s, version, hlimit);
   //DOUT0("HierarchyContainer::StoreSize %s %u", GetName(), (unsigned) s.size());
   return s.size();
}

bool dabc::HierarchyContainer::Stream(iostream& s, uint64_t version, int hlimit)
{
   uint64_t pos = s.size();
   uint64_t sz = 0;

   uint32_t storesz(0), storenum(0), storevers(0);

   //DOUT0("dabc::HierarchyContainer::Stream %s isout %s isreal %s", GetName(), DBOOL(s.is_output()), DBOOL(s.is_real()));

   if (s.is_output()) {
      bool store_fields = fNodeVersion>=version;
      bool store_childs = (fHierarchyVersion >= version) && (hlimit<0);
      bool store_diff = version > 0;
      bool store_history = !fHist.null() && (hlimit>=0);
      uint64_t mask = (store_fields ? maskNodeChanged : 0) |
                      (store_childs ? maskChildsChanged : 0) |
                      (store_diff ? maskDiffStored : 0) |
                      (store_history ? maskHistory : 0);

      sz = s.is_real() ? StoreSize(version, hlimit) : 0;

      //if (s.is_real()) DOUT0("dabc::HierarchyContainer %s storesize %u", GetName(), (unsigned) sz);

      storesz = sz / 8;
      if (store_childs) storenum = ((uint32_t) NumChilds());
      storenum = storenum | (storevers<<24);

      s.write_uint32(storesz);
      s.write_uint32(storenum);

      //DOUT0("Write header size %u", (unsigned) s.size());

      s.write_str(GetName());
      s.write_uint64(mask);

      //DOUT0("Write name %u", (unsigned) s.size());

      if (store_fields) Fields().Stream(s);

      if (store_history) fHist()->Stream(s, version, hlimit);

      if (store_childs)
         for (unsigned n=0;n<NumChilds();n++) {
            dabc::HierarchyContainer* child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(n));
            if (child) child->Stream(s, version);
         }

      //DOUT0("Write childs %u", (unsigned) s.size());

   } else {
      s.read_uint32(storesz);
      sz = ((uint64_t) storesz)*8;
      s.read_uint32(storenum);
      // storevers = storenum >> 24;
      storenum = storenum & 0xffffff;

      std::string name;
      uint64_t mask;

      s.read_str(name);
      s.read_uint64(mask);

      SetName(name.c_str());

      // indicate that it is special node, flag should be treated in update
      if (mask & maskDiffStored)
         fDiffNode = true;

      if (mask & maskNodeChanged) {
         fNodeChanged = true;
         Fields().Stream(s);
      }

      if (mask & maskHistory) {
         if (fHist.null()) fHist.Allocate();
         fHist()->Stream(s, version, hlimit);
      }

      if (mask & maskChildsChanged) {
         fHierarchyChanged = true;
         for (unsigned n=0;n<storenum;n++) {
            dabc::HierarchyContainer* child = new dabc::HierarchyContainer("");
            child->Stream(s,0);
            AddChild(child);
         }
      }
   }

   return s.verify_size(pos, sz);
}


dabc::XMLNodePointer_t dabc::HierarchyContainer::SaveHierarchyInXmlNode(XMLNodePointer_t parentnode)
{
   XMLNodePointer_t objnode = SaveInXmlNode(parentnode);

  for (unsigned n=0;n<NumChilds();n++) {
     dabc::HierarchyContainer* child = dynamic_cast<dabc::HierarchyContainer*> (GetChild(n));
     if (child) child->SaveHierarchyInXmlNode(objnode);
  }

   return objnode;
}


void dabc::HierarchyContainer::SetModified(bool node, bool hierarchy, bool recursive)
{
   if (node) fNodeChanged = true;
   if (hierarchy) fHierarchyChanged = true;
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

   SaveInXmlNode(topnode);

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
      Xml::NewAttr(topnode, 0, "gap", "true");

   std::string res;

   Xml::SaveSingleNode(topnode, &res, 1);
   Xml::FreeNode(topnode);

   return res;
}

bool dabc::HierarchyContainer::UpdateHierarchyFrom(HierarchyContainer* cont)
{
   fNodeChanged = false;
   fHierarchyChanged = false;

   if (cont==0) throw dabc::Exception(ex_Hierarchy, "empty container", ItemName());

   // we do not check names here - top object name can be different
   // if (!IsName(obj->GetName())) throw dabc::Exception(ex_Hierarchy, "mismatch between object and hierarchy itme", ItemName());

   // we do like we manually set/remove all fields, excluding fields starting from 'dabc:'

   if (cont->fDiffNode) {
      if (cont->fNodeChanged) {
         SetFieldsMap(cont->TakeFieldsMap());
         fNodeChanged = true;
      }
      if (!cont->fHierarchyChanged) return fNodeChanged;

   } else {
      Fields().MoveFrom(cont->Fields(), "dabc:"); // FIXME: probably have no need to filter fields
      if (Fields().WasChanged()) fNodeChanged = true;
   }

   // now we should check if any childs were changed

   unsigned cnt1(0); // counter over source container
   unsigned cnt2(0); // counter over existing childs

   // skip first permanent childs from update procedure
   while (cnt2 < NumChilds() && ((HierarchyContainer*) GetChild(cnt2))->fPermanent) cnt2++;

   while ((cnt1 < cont->NumChilds()) || (cnt2 < NumChilds())) {
      if (cnt1 >= cont->NumChilds()) {
         RemoveChildAt(cnt2, true);
         fHierarchyChanged = true;
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

         fHierarchyChanged = true;
         cnt2++;

         continue;
      }

      while (findindx > cnt2) { RemoveChildAt(cnt2, true); findindx--; fHierarchyChanged = true; }

      dabc::HierarchyContainer* child = (dabc::HierarchyContainer*) (GetChild(cnt2));

      if ((child==0) || !child->IsName(cont_child.GetName())) {
         throw dabc::Exception(ex_Hierarchy, "mismatch between object and hierarchy item", ItemName());
         return false;
      }

      if (child->UpdateHierarchyFrom(cont_child())) fHierarchyChanged = true;

      cnt1++;
      cnt2++;
   }

   return fHierarchyChanged || fNodeChanged;
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

   return res;
}


std::string dabc::HierarchyContainer::ItemName()
{
   std::string res;
   FillFullName(res, 0, true);
   return res;
}

void dabc::HierarchyContainer::BuildHierarchy(HierarchyContainer* cont)
{
   cont->Fields().CopyFrom(Fields());

   dabc::RecordContainer::BuildHierarchy(cont);
}

void dabc::HierarchyContainer::AddHistory(RecordFieldsMap* diff)
{
   if (fHist.Capacity()==0) return;
   if (fHist()->fArr.Full()) fHist()->fArr.PopOnly();
   HistoryItem* h = fHist()->fArr.PushEmpty();
   h->version = fNodeVersion;
   h->fields = diff;
}

void dabc::HierarchyContainer::ClearHistoryEntries()
{
   if (fHist.Capacity()==0) return;
   while (fHist()->fArr.Size() > 0) fHist()->fArr.PopOnly();
}


bool dabc::HierarchyContainer::IsNodeChanged(bool withchilds)
{
   if (fNodeChanged || fHierarchyChanged) return true;

   if (Fields().WasChanged()) { fNodeChanged = true; return true; }

   if (withchilds)
      for (unsigned indx=0; indx < NumChilds(); indx++) {
         dabc::HierarchyContainer* child = (dabc::HierarchyContainer*) GetChild(indx);
         if (child && child->IsNodeChanged(withchilds)) return true;
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
         return true;
      }

      prnt = dynamic_cast<HierarchyContainer*> (prnt->GetParent());
   }

   return false;
}

void dabc::HierarchyContainer::MarkVersionIfChanged(uint64_t ver, double& tm, bool withchilds, bool force)
{
   if (force || IsNodeChanged(false)) {

      if (force || fNodeChanged) fNodeVersion = ver;
      if (force || fHierarchyChanged) fHierarchyVersion = ver;

      if (fNodeChanged && fAutoTime) {
         if (tm<=0) {
            dabc::DateTime dt;
            if (dt.GetNow()) tm = dt.AsDouble();
         }
         Fields().Field(prop_time).SetStr(dabc::format("%5.3f",tm));
      }

      if (CheckIfDoingHistory()) {
         RecordFieldsMap* prev = fHist()->fPrev;
         if (prev!=0) {
            prev->MakeAsDiffTo(Fields());
            AddHistory(prev);
         }

         fHist()->fPrev = Fields().Clone();
      }

      Fields().ClearChangeFlags();
      fNodeChanged = false;
      fHierarchyChanged = false;
   }

   if (withchilds)
      for (unsigned indx=0; indx < NumChilds(); indx++) {
         dabc::HierarchyContainer* child = (dabc::HierarchyContainer*) GetChild(indx);
         if (child) child->MarkVersionIfChanged(ver, tm, withchilds, force);
      }
}


void dabc::HierarchyContainer::MarkChangedItems(bool withchilds, double tm)
{
   if (!IsNodeChanged(withchilds)) return;

   HierarchyContainer* top = this, *prnt = this;

   while (prnt!=0) {
     top = prnt;
     prnt = dynamic_cast<HierarchyContainer*> (prnt->GetParent());
   }

   uint64_t next_ver = top->GetVersion() + 1;

   // mark node and childs (if specified)
   MarkVersionIfChanged(next_ver, tm, withchilds, false);

   // mark changes in parent
   prnt = this;
   while (prnt != 0) {
      prnt = dynamic_cast<HierarchyContainer*> (prnt->GetParent());
      if (prnt) prnt->MarkVersionIfChanged(next_ver, tm, false, true);
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


void dabc::Hierarchy::Build(const std::string& topname, Reference top)
{
   if (null() && !topname.empty()) Create(topname);

   if (!top.null() && !null())
      top()->BuildHierarchy(GetObject());
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


bool dabc::Hierarchy::UpdateHierarchy(Reference top)
{
   if (null()) {
      Build(top.GetName(), top);
      return true;
   }

   if (top.null()) {
      EOUT("Objects structure must be provided");
      return false;
   }

   dabc::Hierarchy src;

   src.Build(top.GetName(), top);

   return Update(src);
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
         Field(prop_history).SetUInt(length);
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


dabc::Buffer dabc::Hierarchy::SaveToBuffer(uint64_t version)
{
   if (null()) return dabc::Buffer();

   uint64_t size = GetObject()->StoreSize(version);

   dabc::Buffer res = dabc::Buffer::CreateBuffer(size);

   memstream outs(false, (char*) res.SegmentPtr(), res.SegmentSize());

   if (GetObject()->Stream(outs, version)) {

      if (size != outs.size()) { EOUT("Sizes mismatch %lu %lu", (long unsigned) size, (long unsigned) outs.size()); }

      res.SetTotalSize(size);
   }

   return res;
}

bool dabc::Hierarchy::ReadFromBuffer(const dabc::Buffer& buf)
{
   if (buf.null()) return false;

   memstream inps(true, (char*) buf.SegmentPtr(), buf.SegmentSize());

   dabc::HierarchyContainer* cont = new dabc::HierarchyContainer("");

   if (!cont->Stream(inps)) {
      EOUT("Cannot reconstruct hierarchy from the binary data!");
      delete cont;
      return false;
   }

   SetObject(cont);

   return true;
}

bool dabc::Hierarchy::UpdateFromBuffer(const dabc::Buffer& buf)
{
   if (null()) return ReadFromBuffer(buf);

   dabc::Hierarchy src;

   if (!src.ReadFromBuffer(buf)) return false;

   if (!GetObject()->UpdateHierarchyFrom(src.GetObject())) return false;

   uint64_t next_ver = GetVersion() + 1;

   double tm = 0;

   // mark node and its childs
   GetObject()->MarkVersionIfChanged(next_ver, tm, true, false);

   return true;
}




std::string dabc::Hierarchy::SaveToXml(bool compact)
{
   XMLNodePointer_t topnode = GetObject()->SaveHierarchyInXmlNode(0);

   Xml::NewAttr(topnode, 0, dabc::prop_version, dabc::format("%lu", (long unsigned) GetVersion()).c_str());

   std::string res;

   if (topnode) {
      Xml::SaveSingleNode(topnode, &res, compact ? 0 : 1);
      Xml::FreeNode(topnode);
   }

   return res;
}


void dabc::Hierarchy::Create(const std::string& name)
{
   Release();
   SetObject(new HierarchyContainer(name));
}

dabc::Hierarchy dabc::Hierarchy::CreateChild(const std::string& name, int indx)
{
   if (null() || name.empty()) return dabc::Hierarchy();

   return GetObject()->CreateChildAt(name, indx);

   return FindChild(name.c_str());
}

dabc::Hierarchy dabc::Hierarchy::FindMaster()
{
   if (null() || (GetParent()==0) || !HasField(dabc::prop_masteritem)) return dabc::Hierarchy();

   std::string masteritem = Field(dabc::prop_masteritem).AsStdStr();

   if (masteritem.empty()) return dabc::Hierarchy();

   return GetParent()->FindChildRef(masteritem.c_str());
}


dabc::Buffer dabc::Hierarchy::GetBinaryData(uint64_t query_version)
{
   if (null()) return 0;

   dabc::Hierarchy master = FindMaster();

   // take data under lock that we are sure - nothing change for me
   uint64_t item_version = GetVersion();
   uint64_t master_version(0);

   if (!master.null()) {
      master_version = master.GetVersion();
   }

   Buffer& bindata = GetObject()->bindata();

   dabc::BinDataHeader* bindatahdr = 0;

   if (!bindata.null())
      bindatahdr = (dabc::BinDataHeader*) bindata.SegmentPtr();

   bool can_reply = (bindatahdr!=0) && (item_version==bindatahdr->version) && (query_version<=item_version);

   if (!can_reply) return 0;

   if ((query_version>0) && (query_version==bindatahdr->version)) {
      dabc::BinDataHeader dummyhdr;
      dummyhdr.reset();
      dummyhdr.version = bindatahdr->version;
      dummyhdr.master_version = master_version;

      // create temporary buffer with header only indicating that version is not changed
      return dabc::Buffer::CreateBuffer(&dummyhdr, sizeof(dummyhdr), false, true);
   }

   bindatahdr->master_version = master_version;
   return bindata;
}


dabc::Command dabc::Hierarchy::ProduceBinaryRequest()
{
   if (null()) return 0;

   std::string request_name;
   std::string producer_name = FindBinaryProducer(request_name);

   if (request_name.empty()) return 0;

   dabc::Command cmd("GetBinary");
   cmd.SetReceiver(producer_name);
   cmd.SetStr("Item", request_name);

   return cmd;
}

dabc::Buffer dabc::Hierarchy::ApplyBinaryRequest(Command cmd)
{
   if (null() || (cmd.GetResult() != cmd_true)) return 0;

   Buffer bindata = cmd.GetRawData();
   if (bindata.null()) return 0;

   uint64_t item_version = GetVersion();

   dabc::BinDataHeader* bindatahdr = (dabc::BinDataHeader*) bindata.SegmentPtr();
   bindatahdr->version = item_version;

   GetObject()->bindata() = bindata;

   dabc::Hierarchy master = FindMaster();

   uint64_t master_version = master.GetVersion();

   //DOUT0("BINARY REQUEST AFTER MASTER VERSION %u", (unsigned) master_version);

   // master hash can let us know if something changed in the master
   std::string masterhash = cmd.GetStdStr("MasterHash");

   if (!master.null() && !masterhash.empty() && (master.Field(dabc::prop_content_hash).AsStdStr()!=masterhash)) {

      master()->fNodeChanged = true;

      master()->MarkChangedItems();

      // DOUT0("MASTER VERSION WAS %u NOW %u", (unsigned) master_version, (unsigned) master.GetVersion());

      master_version = master.GetVersion();
      master.Field(dabc::prop_content_hash).SetStr(masterhash);
   }

   bindatahdr->master_version = master_version;

   return bindata;
}


std::string dabc::Hierarchy::FindBinaryProducer(std::string& request_name)
{
   dabc::Hierarchy parent = *this;
   std::string producer_name;

   // we need to find parent on the most-top position with binary_producer property
   while (!parent.null()) {
      if (parent.HasField(dabc::prop_binary_producer)) {
         producer_name = parent.Field(dabc::prop_binary_producer).AsStdStr();
         request_name = RelativeName(parent);
      }
      parent = parent.GetParentRef();
   }

   return producer_name;
}


dabc::Command dabc::Hierarchy::ProduceHistoryRequest()
{
   // STEP1. Create request which can be send to remote

   if (null()) return 0;

   int history_size = Field(dabc::prop_history).AsInt(0);

   if (history_size<=0) return 0;

   std::string producer_name, request_name;

   producer_name = FindBinaryProducer(request_name);

   if (producer_name.empty() && request_name.empty()) return 0;

   if (GetObject()->fHist.null()) {
//      DOUT0("Allocate history %u for the object %p", history_size, GetObject());
      GetObject()->fHist.Allocate(history_size);
   }

   dabc::Command cmd("GetBinary");
   cmd.SetStr("Item", request_name);
   cmd.SetBool("history", true); // indicate that we are requesting history
   cmd.SetInt("limit", history_size);
   cmd.SetUInt("version", GetObject()->fHist()->fRemoteReqVersion);
   cmd.SetReceiver(producer_name);

   return cmd;
}

dabc::Buffer dabc::Hierarchy::ExecuteHistoryRequest(Command cmd)
{
   // STEP2. Process request in the structure, which really records history

   if (null()) return 0;

   unsigned ver = cmd.GetUInt("version");
   int limit = cmd.GetInt("limit");

   dabc::Buffer buf = GetObject()->RequestHistory(ver, limit);

   if (!buf.null()) cmd.SetUInt("version", GetVersion());

   return buf;
}


bool dabc::Hierarchy::ApplyHistoryRequest(Command cmd)
{
   // STEP3. Unfold reply from remote and reproduce history recording here

   if (cmd.GetResult() != cmd_true) {
      EOUT("Command executed not correctly");
      return false;
   }

   if (null() || GetObject()->fHist.null()) {
      EOUT("History object missing");
      return false;
   }

//   DOUT0("STEP3 - analyze reply");

   dabc::Buffer buf = cmd.GetRawData();

   if (buf.null()) { EOUT("No raw data in history reply"); return false; }

   dabc::Hierarchy hreq;
   if (!hreq.ReadFromBuffer(buf)) {
      EOUT("Fail read hrequest from binary buffer");
      return false;
   }

   RecordFieldsMap& src_fields = hreq.GetObject()->Fields();

   RecordFieldsMap& tgt_fields = GetObject()->Fields();

   tgt_fields.MoveFrom(src_fields);

   // this is all about history
   // we are adding history with previous number while
   if (!hreq()->fHist.null())  {
      RecordFieldsMap* entry = 0;

//      DOUT0("Object %p capacity %u", GetObject(), GetObject()->fHist.Capacity());

      // if request was not able to deliver all items up to specified version,
      // we need to remove all previous items, while history will not be correct
      if (!hreq()->fHist()->fCrossBoundary)
         GetObject()->ClearHistoryEntries();

//     DOUT0("SRC num entries %u TGT numentries %u TGT capacity %u",
//        hreq()->fHist.Size(), GetObject()->fHist.Size(), GetObject()->fHist.Capacity());

      while ((entry = hreq()->fHist()->TakeNext())!=0)
         GetObject()->AddHistory(entry);
   }

   GetObject()->fNodeChanged = true;
   GetObject()->MarkChangedItems();

   // remember when reply comes back
   GetObject()->fHist()->fRemoteReqVersion = cmd.GetUInt("version");
   GetObject()->fHist()->fLocalReqVersion = GetVersion();

   return true;
}


dabc::Buffer dabc::Hierarchy::GetLocalImage(uint64_t version)
{
   if (null()) return 0;
   if (Field(prop_kind).AsStdStr()!="image.png") return 0;

   return GetObject()->bindata();
}

dabc::Command dabc::Hierarchy::ProduceImageRequest()
{
   std::string producer_name, request_name;

   producer_name = FindBinaryProducer(request_name);

   if (producer_name.empty() && request_name.empty()) return 0;

   dabc::Command cmd("GetBinary");
   cmd.SetStr("Item", request_name);
   cmd.SetBool("image", true); // indicate that we are requesting image
   cmd.SetReceiver(producer_name);

   return cmd;
}

bool dabc::Hierarchy::ApplyImageRequest(Command cmd)
{
   if (null() || (cmd.GetResult() != cmd_true)) return false;

   Buffer bindata = cmd.GetRawData();
   if (bindata.null()) return false;

   GetObject()->fNodeChanged = true;
   GetObject()->MarkChangedItems();
   GetObject()->bindata() = bindata;

   return true;
}

