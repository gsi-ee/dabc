// $Id$
// Author: Sergey Linev   22/12/2013

#include "TRootSniffer.h"

#include "TH1.h"
#include "TGraph.h"
#include "TProfile.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TList.h"
#include "TMemFile.h"
#include "TStreamerInfo.h"
#include "TBufferFile.h"
#include "TROOT.h"
#include "TTimer.h"
#include "Property.h"
#include "TFolder.h"
#include "TTree.h"
#include "TBranch.h"
#include "TLeaf.h"
#include "TClass.h"
#include "TDataMember.h"
#include "TDataType.h"
#include "TBaseClass.h"
#include "TObjString.h"
#include "TUrl.h"

#include "TRootSnifferStore.h"

#ifndef HTTP_WITHOUT_ASIMAGE
#include "TImage.h"
#include "TASImage.h"
#endif

#include <cstdlib>

#pragma pack(push, 4)

struct BinDataHeader {
   UInt_t typ;              ///< type of the binary header (1 for the moment)
   UInt_t version;          ///< version of data in binary data
   UInt_t master_version;   ///< version of master object for that binary data
   UInt_t zipped;           ///< length of unzipped data
   UInt_t payload;          ///< length of payload (not including header)

   void reset() {
      typ = 1;
      version = 0;
      master_version = 0;
      zipped = 0;
      payload = 0;
   }

   /** \brief returns true if content of buffer is zipped */
   bool is_zipped() const
     { return (zipped>0) && (payload>0); }

   void* rawdata() const { return (char*) this + sizeof(BinDataHeader); }
};

#pragma pack(pop)



extern "C" void R__zipMultipleAlgorithm(int cxlevel, int *srcsize, char *src, int *tgtsize, char *tgt, int *irep, int compressionAlgorithm);

const char* dabc_prop_kind = "dabc:kind";
const char* dabc_prop_masteritem = "dabc:master";
const char* dabc_prop_more = "dabc:more";
const char* dabc_prop_realname = "dabc:realname"; // real object name
const char* dabc_prop_itemname = "dabc:itemname"; // item name in dabc hierarchy

Long_t gExcludeProperty = G__BIT_ISSTATIC | G__BIT_ISENUM | G__BIT_ISUNION;

// ============================================================================

void TRootSnifferScanRec::SetField(const char* name, const char* value)
{
   if (CanSetFields()) store->SetField(lvl, name, value, num_fields);
   num_fields++;
}

void TRootSnifferScanRec::BeforeNextChild()
{
  if (CanSetFields()) store->BeforeNextChild(lvl, num_childs, num_fields);
  num_childs++;
}

void TRootSnifferScanRec::MakeItemName(const char* objname, TString& _itemname)
{
   std::string nnn = objname;

   size_t pos = nnn.find_last_of("/");
   if (pos!=std::string::npos) nnn = nnn.substr(pos+1);
   if (nnn.empty()) nnn = "item";

   // replace all special symbols which can make problem in http (not in xml)
   while ((pos = nnn.find_first_of("#:&?")) != std::string::npos)
      nnn.replace(pos, 1, "_");

   _itemname = nnn.c_str();
   Int_t cnt = 0;

   while (fItemsNames.FindObject(_itemname.Data())) {
      _itemname.Form("%s_%d", nnn.c_str(), cnt++);
   }

   fItemsNames.Add(new TObjString(_itemname.Data()));
}


void TRootSnifferScanRec::CreateNode(const char* _node_name, const char* _obj_name)
{
   if (!CanSetFields()) return;

   started_node = _node_name;
   TString real_item_name;

   // this is for XML
   if (started_node.First("[]&<>") != kNPOS) {
      real_item_name = started_node;
      MakeItemName("extra_item", started_node); // we generate abstract item just to be safe with syntax
   }

   if (parent) parent->BeforeNextChild();

   if (store) store->CreateNode(lvl, started_node.Data());

   if (real_item_name.Length()>0)
      SetField(dabc_prop_itemname, real_item_name.Data());

   if (_obj_name && (started_node != _obj_name))
      SetField(dabc_prop_realname, _obj_name);
}

void TRootSnifferScanRec::CloseNode()
{
   if (store && !started_node.IsNull()) {
      store->CloseNode(lvl, started_node.Data(), num_childs>0);
      started_node = "";
   }
}

void TRootSnifferScanRec::SetRootClass(TClass* cl)
{
   if ((cl==0) || !CanSetFields())  return;

   SetField(dabc_prop_kind, TString::Format("ROOT.%s", cl->GetName()));

   if (TRootSniffer::IsDrawableClass(cl)) {
      // only drawable class can be fetched from the server
      TString master;
      Int_t depth = Depth();
      for (Int_t n=1;n<depth;n++) master.Append("../");
      master.Append("StreamerInfo");
      SetField(dabc_prop_masteritem, master.Data());
   }
}

Bool_t TRootSnifferScanRec::Done()
{
   if (store==0) return kFALSE;

   if ((mask & mask_Search) && store->res) return kTRUE;

   if ((mask & mask_CheckChld) && store->res && (store->res_chld>=0)) return kTRUE;

   return kFALSE;
}

Bool_t TRootSnifferScanRec::SetResult(void* obj, TClass* cl, Int_t chlds)
{
   if (Done()) return kTRUE;

   // only when doing search, result will be propagated
   if ((mask & (mask_Search | mask_CheckChld)) == 0) return kFALSE;

   //DOUT0("Set RESULT obj = %p search path = %s", obj, searchpath ? searchpath : "-null-");

   // only when full search path is scanned
   if (searchpath!=0) return kFALSE;

   if (store==0) return kFALSE;

   store->res = obj;
   store->rescl = cl;
   store->res_chld = chlds;

   return Done();
}

Int_t TRootSnifferScanRec::Depth() const
{
   Int_t cnt = 0;
   const TRootSnifferScanRec* rec = this;
   while (rec->parent) {
      rec = rec->parent;
      cnt++;
   }

   return cnt;
}

Int_t TRootSnifferScanRec::ExtraFolderLevel()
{
   TRootSnifferScanRec* rec = this;
   Int_t cnt = 0;
   while (rec) {
      if (rec->mask & mask_ExtraFolder) return cnt;
      rec = rec->parent;
      cnt++;
   }

   return -1;
}

Bool_t TRootSnifferScanRec::CanExpandItem()
{
   if (mask & (mask_Expand | mask_Search | mask_CheckChld)) return kTRUE;

   if (!has_more) return kFALSE;

   // if parent has expand mask, allow to expand item
   if (parent && (parent->mask & mask_Expand)) return kTRUE;

   return kFALSE;
}


Bool_t TRootSnifferScanRec::GoInside(TRootSnifferScanRec& super, TObject* obj, const char* obj_name)
{
   if (super.Done()) return kFALSE;

   if ((obj!=0) && (obj_name==0)) obj_name = obj->GetName();

   // exclude zero names
   if ((obj_name==0) || (*obj_name==0)) return kFALSE;

   TString obj_item_name;

   super.MakeItemName(obj_name, obj_item_name);

   lvl = super.lvl;
   store = super.store;
   searchpath = super.searchpath;
   mask = super.mask & mask_Actions;
   parent = &super;

   if (mask & mask_Scan) {
      // only when doing scan, increment lvl, used for text formatting
      lvl++;
   } else {
      if (searchpath==0) return kFALSE;

      if (strncmp(searchpath, obj_item_name.Data(), obj_item_name.Length())!=0) return kFALSE;

      const char* separ = searchpath + obj_item_name.Length();

      Bool_t isslash = kFALSE;
      while (*separ=='/') { separ++; isslash = kTRUE; }

      if (*separ==0) {
         searchpath = 0;
         if (mask & mask_Expand) mask = mask_Scan;
      } else {
         if (!isslash) return kFALSE;
         searchpath = separ;
      }
   }

   CreateNode(obj_item_name.Data(), obj ? obj->GetName() : 0);

   return kTRUE;
}


// ====================================================================


TRootSniffer::TRootSniffer(const char* name, const char* objpath) :
   TNamed(name, "sniffer of root objects"),
   fObjectsPath(objpath),
   fMemFile(0),
   fSinfoSize(0),
   fCompression(5)
{
}

TRootSniffer::~TRootSniffer()
{
   if (fMemFile) { delete fMemFile; fMemFile = 0; }
}

void TRootSniffer::ScanObjectMemebers(TRootSnifferScanRec& rec, TClass* cl, char* ptr, unsigned long int cloffset)
{
   if ((cl==0) || (ptr==0) || rec.Done()) return;

   //DOUT0("SCAN CLASS %s mask %u", cl->GetName(), rec.mask);

   // first of all expand base classes
   TIter cliter(cl->GetListOfBases());
   TObject* obj = 0;
   while ((obj=cliter()) != 0) {
      TBaseClass* baseclass = dynamic_cast<TBaseClass*>(obj);
      if (baseclass==0) continue;
      TClass* bclass = baseclass->GetClassPointer();
      if (bclass==0) continue;

      TRootSnifferScanRec chld;

      if (chld.GoInside(rec, baseclass)) {
         ScanObjectMemebers(chld, bclass, ptr, cloffset + baseclass->GetDelta());
         if (chld.Done()) break;
      }
   }

   //DOUT0("SCAN MEMBERS %s %u mask %u done %s", cl->GetName(), cl->GetListOfDataMembers()->GetSize(), chld.mask, DBOOL(chld.Done()));

   // than expand data members
   TIter iter(cl->GetListOfDataMembers());
   while ((obj=iter()) != 0) {
      TDataMember* member = dynamic_cast<TDataMember*>(obj);
      // exclude enum or static variables
      if ((member==0) || (member->Property() & gExcludeProperty)) continue;

      char* member_ptr = ptr + cloffset + member->GetOffset();
      if (member->IsaPointer()) member_ptr = *((char**) member_ptr);

      TRootSnifferScanRec chld;

      if (chld.GoInside(rec, member)) {
         TClass* mcl = (member->IsBasic() || member->IsSTLContainer())  ? 0 : gROOT->GetClass(member->GetTypeName());

         Int_t coll_offset = mcl ? mcl->GetBaseClassOffset(TCollection::Class()) : -1;

         bool iscollection = (coll_offset>=0);
         if (iscollection) {
            chld.SetField(dabc_prop_more, "true");
            chld.has_more = true;
         }

         if (chld.SetResult(member_ptr, mcl)) break;

         if (IsDrawableClass(mcl))
            chld.SetRootClass(mcl);

         if (chld.CanExpandItem()) {
            if (iscollection) {
               // chld.SetField("#members", "true");
               ScanCollection(chld, (TCollection*) (member_ptr + coll_offset));
            }
         }

         if (chld.SetResult(member_ptr, mcl, chld.num_childs)) break;
      }
   }
}

void TRootSniffer::ScanObject(TRootSnifferScanRec& rec, TObject* obj)
{
   if (obj==0) return;

   //if (rec.mask & mask_Expand)
   //   printf("EXPAND OBJECT %s can expand %s mask %u\n", obj->GetName(), DBOOL(rec.CanExpandItem()), rec.mask);

   // mark object as expandable for direct child of extra folder
   // or when non drawable object is scanned

   if (rec.SetResult(obj, obj->IsA())) return;

   int isextra = rec.ExtraFolderLevel();

   if ((isextra==1) || ((isextra>1) && !IsDrawableClass(obj->IsA()))) {
     rec.SetField(dabc_prop_more, "true");
     rec.has_more = kTRUE;
   }

   rec.SetRootClass(obj->IsA());

   if (obj->InheritsFrom(TFolder::Class())) {
      // starting from special folder, we automatically scan members

      TFolder* fold = (TFolder*) obj;
      if (fold->TestBit(BIT(19))) rec.mask = rec.mask | mask_ExtraFolder;

      ScanCollection(rec, fold->GetListOfFolders());
   } else
   if (obj->InheritsFrom(TDirectory::Class())) {
      ScanCollection(rec, ((TDirectory*) obj)->GetList());
   } else
   if (obj->InheritsFrom(TTree::Class())) {
      ScanCollection(rec, ((TTree*) obj)->GetListOfLeaves());
   } else
   if (obj->InheritsFrom(TBranch::Class())) {
      ScanCollection(rec, ((TBranch*) obj)->GetListOfLeaves());
   } else
   if (rec.CanExpandItem()) {
      ScanObjectMemebers(rec, obj->IsA(), (char*) obj, 0);
   }

   // here we should know how many childs are accumulated
   rec.SetResult(obj, obj->IsA(), rec.num_childs);
}

void TRootSniffer::ScanCollection(TRootSnifferScanRec& rec, TCollection* lst, const char* foldername, Bool_t extra)
{
   if ((lst==0) || (lst->GetSize()==0)) return;

   TRootSnifferScanRec folderrec;
   if (foldername) {
      if (!folderrec.GoInside(rec, 0, foldername)) return;
      if (extra) folderrec.mask = folderrec.mask | mask_ExtraFolder;
   }

   {
      TRootSnifferScanRec& master = foldername ? folderrec : rec;

      TIter iter(lst);
      TObject* obj(0);

      // DOUT0("SEARCH OBJECT with name %s ", itemname.c_str());

      while ((obj = iter()) != 0) {

         TRootSnifferScanRec chld;

         if (chld.GoInside(master, obj)) {
            ScanObject(chld, obj);
            if (chld.Done()) break;
         }

      }
   }
}


void TRootSniffer::ScanRoot(TRootSnifferScanRec& rec)
{
   rec.SetField(dabc_prop_kind, "ROOT.Session");

   {
      TRootSnifferScanRec chld;
      if (chld.GoInside(rec, 0, "StreamerInfo"))
         chld.SetField(dabc_prop_kind, "ROOT.TList");
   }

   TFolder* topf = dynamic_cast<TFolder*> (gROOT->FindObject(TString::Format("//root/%s",fObjectsPath.Data())));

   ScanCollection(rec, gROOT->GetList());

   ScanCollection(rec, gROOT->GetListOfCanvases(), "Canvases");

   ScanCollection(rec, gROOT->GetListOfFiles(), "Files");

   ScanCollection(rec, topf ? topf->GetListOfFolders() : 0, "Objects");
}

Bool_t TRootSniffer::IsDrawableClass(TClass* cl)
{
   if (cl==0) return kFALSE;
   if (cl->InheritsFrom(TH1::Class())) return kTRUE;
   if (cl->InheritsFrom(TGraph::Class())) return kTRUE;
   if (cl->InheritsFrom(TCanvas::Class())) return kTRUE;
   if (cl->InheritsFrom(TProfile::Class())) return kTRUE;
   return kFALSE;
}


Bool_t TRootSniffer::IsBrowsableClass(TClass* cl)
{
   if (cl==0) return kFALSE;

   if (cl->InheritsFrom(TTree::Class())) return kTRUE;
   if (cl->InheritsFrom(TBranch::Class())) return kTRUE;
   if (cl->InheritsFrom(TLeaf::Class())) return kTRUE;
   if (cl->InheritsFrom(TFolder::Class())) return kTRUE;

   return kFALSE;
}

void TRootSniffer::ScanHierarchy(const char* path, TRootSnifferStore* store)
{
   TRootSnifferScanRec rec;
   rec.searchpath = path;
   if (rec.searchpath) {
      if (*rec.searchpath == '/') rec.searchpath++;
      if (*rec.searchpath==0) rec.searchpath=0;
   }

   // if path non-empty, we should find item first and than start scanning
   rec.mask = rec.searchpath==0 ? mask_Scan : mask_Expand;

   rec.store = store;

   rec.CreateNode("ROOT");

   ScanRoot(rec);

   rec.CloseNode();
}

void TRootSniffer::ScanXml(const char* path, TString& xml)
{
   TRootSnifferStoreXml store(xml);

   ScanHierarchy(path, &store);
}

void TRootSniffer::ScanJson(const char* path, TString& json)
{
   TRootSnifferStoreJson store(json);

   ScanHierarchy(path, &store);
}


void* TRootSniffer::FindInHierarchy(const char* path, TClass** cl, Int_t* chld)
{
   TRootSnifferStore store;

   TRootSnifferScanRec rec;
   rec.searchpath = path;
   rec.mask = (chld!=0) ? mask_CheckChld : mask_Search;
   if (*rec.searchpath == '/') rec.searchpath++;
   rec.store = &store;

   ScanRoot(rec);

   if (cl) *cl = store.rescl;
   if (chld) *chld = store.res_chld;
   return store.res;
}

TObject* TRootSniffer::FindTObjectInHierarchy(const char* path)
{
   TClass* cl(0);

   TObject* obj = (TObject*) FindInHierarchy(path, &cl);

   return (cl!=0) && (cl->GetBaseClassOffset(TObject::Class())==0) ? obj : 0;
}


TString TRootSniffer::GetObjectHash(TObject* obj)
{
   if ((obj!=0) && (obj->InheritsFrom(TH1::Class())))
      return TString::Format("%g", ((TH1*)obj)->GetEntries());

   return "";
}

TString TRootSniffer::GetStreamerInfoHash()
{
   return TString::Format("%d", fSinfoSize);
}


Bool_t TRootSniffer::CanDrawItem(const char* path)
{
   // method verifies if object can be drawn

   TClass *obj_cl(0);
   void *res = FindInHierarchy(path, &obj_cl);
   return (res!=0) && IsDrawableClass(obj_cl);
}

Bool_t TRootSniffer::CanExploreItem(const char* path)
{
   // method returns true when object has childs or
   // one could try to expand item

   TClass *obj_cl(0);
   Int_t obj_chld(-1);
   void *res = FindInHierarchy(path, &obj_cl, &obj_chld);
   return (res!=0) && (obj_chld > 0);
}



void TRootSniffer::CreateMemFile()
{
   if (fMemFile!=0) return;

   TDirectory* olddir = gDirectory; gDirectory = 0;
   TFile* oldfile = gFile; gFile = 0;

   fMemFile = new TMemFile("dummy.file", "RECREATE");
   gROOT->GetListOfFiles()->Remove(fMemFile);

   TH1F* d = new TH1F("d","d", 10, 0, 10);
   fMemFile->WriteObject(d, "h1");
   delete d;

   TGraph* gr = new TGraph(10);
   gr->SetName("abc");
   //      // gr->SetDrawOptions("AC*");
   fMemFile->WriteObject(gr, "gr1");
   delete gr;

   fMemFile->WriteStreamerInfo();

   // make primary list of streamer infos
   TList* l = new TList();

   l->Add(gROOT->GetListOfStreamerInfo()->FindObject("TGraph"));
   l->Add(gROOT->GetListOfStreamerInfo()->FindObject("TH1F"));
   l->Add(gROOT->GetListOfStreamerInfo()->FindObject("TH1"));
   l->Add(gROOT->GetListOfStreamerInfo()->FindObject("TNamed"));
   l->Add(gROOT->GetListOfStreamerInfo()->FindObject("TObject"));

   fMemFile->WriteObject(l, "ll");
   delete l;

   fMemFile->WriteStreamerInfo();

   l = fMemFile->GetStreamerInfoList();
   // l->Print("*");
   fSinfoSize = l->GetSize();
   delete l;

   gDirectory = olddir;
   gFile = oldfile;
}

Bool_t TRootSniffer::ProduceBinary(const char* path, const char* options, void* &ptr, Long_t& length)
{
   if ((path==0) || (*path==0)) return kFALSE;

   if (*path=='/') path++;

   TBufferFile* sbuf = 0;

   // Info("ProduceBinary","Request %s", path);

   if (strcmp(path,"StreamerInfo")==0) {
      CreateMemFile();

      TDirectory* olddir = gDirectory; gDirectory = 0;
      TFile* oldfile = gFile; gFile = 0;

      fMemFile->WriteStreamerInfo();
      TList* l = fMemFile->GetStreamerInfoList();
      //l->Print("*");

      fSinfoSize = l->GetSize();

      // TODO: one could reuse memory from dabc::MemoryPool here
      //       now keep as it is and copy data at least once
      sbuf = new TBufferFile(TBuffer::kWrite, 100000);
      sbuf->SetParent(fMemFile);
      sbuf->MapObject(l);
      l->Streamer(*sbuf);
      delete l;

      gDirectory = olddir;
      gFile = oldfile;
   } else {

      TClass* obj_cl(0);
      void* obj_ptr = FindInHierarchy(path, &obj_cl);
      if ((obj_ptr==0) || (obj_cl==0)) return kFALSE;

      CreateMemFile();

      TDirectory* olddir = gDirectory; gDirectory = 0;
      TFile* oldfile = gFile; gFile = 0;

      TList* l1 = fMemFile->GetStreamerInfoList();

      if (obj_cl->GetBaseClassOffset(TObject::Class())==0) {
         TObject* obj = (TObject*) obj_ptr;

         sbuf = new TBufferFile(TBuffer::kWrite, 100000);
         sbuf->SetParent(fMemFile);
         sbuf->MapObject(obj);
         obj->Streamer(*sbuf);
      } else {
         Info("ProduceBinary", "Non TObject class not yet supported");
         delete sbuf;
         sbuf = 0;
      }

      bool believe_not_changed = false;

      if ((fMemFile->GetClassIndex()==0) || (fMemFile->GetClassIndex()->fArray[0] == 0)) {
         believe_not_changed = true;
      }

      fMemFile->WriteStreamerInfo();
      TList* l2 = fMemFile->GetStreamerInfoList();

      if (believe_not_changed && (l1->GetSize() != l2->GetSize())) {
         Error("ProduceBinary", "StreamerInfo changed when we were expecting no changes!!!!!!!!!");
         delete sbuf; sbuf = 0;
      }

      fSinfoSize = l2->GetSize();

      delete l1; delete l2;

      gDirectory = olddir;
      gFile = oldfile;
   }

   if (!CreateBindData(sbuf,ptr,length)) return kFALSE;

   BinDataHeader* hdr = (BinDataHeader*) ptr;

   if (strcmp(path,"StreamerInfo")==0) {
      hdr->version = fSinfoSize;
      hdr->master_version = 0;
   } else {
      hdr->version = 0;
      hdr->master_version = fSinfoSize;
   }

   return kTRUE;
}

Bool_t TRootSniffer::ProduceImage(Int_t kind, const char* path, const char* options, void* &ptr, Long_t& length)
{
   // Method to produce image from specified object
   //
   // Parameters:
   //    kind - image kind TImage::kPng, TImage::kJpeg, TImage::kGif
   //    path - path to object
   //    options - extra options
   //
   // By default, image 300x200 is produced
   // In options string one could provide following options:
   //    w - image width
   //    h - image height
   //    opt - draw options
   //  For instance:
   //     http://localhost:8080/Files/hsimple.root/hpx/get.png?w=500&h=500&opt=lego1
   //
   //  Return is memory with produced image
   //  Memory must be released by user with free(ptr) call
   //
   // Method only works when ROOT compiled with TASImage

   ptr = 0;
   length = 0;

#ifndef HTTP_WITHOUT_ASIMAGE
   if ((path==0) || (*path==0)) return kFALSE;
   if (*path=='/') path++;

   TClass* obj_cl(0);
   void* obj_ptr = FindInHierarchy(path, &obj_cl);
   if ((obj_ptr==0) || (obj_cl==0)) return kFALSE;

   if (obj_cl->GetBaseClassOffset(TObject::Class())!=0) {
      Error("TRootSniffer", "Only derived from TObject classes can be drawn");
      return kFALSE;
   }

   TObject* obj = (TObject*) obj_ptr;

   TImage* img = TImage::Create();
   if (img==0) return kFALSE;

   if (obj->InheritsFrom(TPad::Class())) {

      Info("TRootSniffer", "Crate IMAGE directly from pad");
      img->FromPad((TPad*) obj);
   } else
   if (IsDrawableClass(obj->IsA())) {

      Info("TRootSniffer", "Crate IMAGE from object %s", obj->GetName());

      Int_t width(300), height(200);
      TString drawopt = "";

      if ((options!=0) && (*options!=0)) {
         TUrl url;
         url.SetOptions(options);
         url.ParseOptions();
         Int_t w = url.GetIntValueFromOptions("w");
         if (w>10) width = w;
         Int_t h = url.GetIntValueFromOptions("h");
         if (h>10) height = h;
         const char* opt = url.GetValueFromOptions("opt");
         if (opt!=0) drawopt = opt;
      }

      Bool_t isbatch = gROOT->IsBatch();
      TVirtualPad* save_gPad = gPad;

      if (!isbatch) gROOT->SetBatch(kTRUE);

      TCanvas* c1 = new TCanvas("__online_draw_canvas__","title", width, height);
      obj->Draw(drawopt.Data());
      img->FromPad(c1);
      delete c1;

      if (!isbatch) gROOT->SetBatch(kFALSE);
      gPad = save_gPad;

   } else {
      delete img;
      return kFALSE;
   }


   TASImage* tasImage = new TASImage();
   tasImage->Append(img);

   char* png_buffer(0);
   int size(0);

   tasImage->GetImageBuffer(&png_buffer, &size, (TImage::EImageFileTypes) kind);

   if ((png_buffer!=0) && (size>0)) {
      ptr = malloc(size);
      length = size;
      memcpy(ptr, png_buffer, length);
   }

   delete [] png_buffer;
   delete tasImage;

#endif

   return ptr!=0;
}

Bool_t TRootSniffer::CreateBindData(TBufferFile* sbuf, void* &ptr, Long_t& length)
{
   if (sbuf==0) return kFALSE;

   bool with_zip = fCompression > 0;

   const Int_t kMAXBUF = 0xffffff;
   Int_t noutot = 0;
   Int_t fObjlen = sbuf->Length();
   Int_t nbuffers = 1 + (fObjlen - 1)/kMAXBUF;
   Int_t buflen = TMath::Max(512,fObjlen + 9*nbuffers + 28);
   if (buflen<fObjlen) buflen = fObjlen;

   ptr = malloc(sizeof(BinDataHeader)+buflen);

   if (ptr == 0) {
      Error("CreateBinData", "Cannot create buffer of size %u", (unsigned) (sizeof(BinDataHeader)+buflen));
      delete sbuf;
      return kFALSE;
   }

   BinDataHeader* hdr = (BinDataHeader*) ptr;
   hdr->reset();

   char* fBuffer = ((char*) ptr) + sizeof(BinDataHeader);

   if (with_zip) {
      Int_t cxAlgorithm = 0;

      char *objbuf = sbuf->Buffer();
      char *bufcur = fBuffer; // start writing from beginning

      Int_t nzip   = 0;
      Int_t bufmax = 0;
      Int_t nout = 0;

      for (Int_t i = 0; i < nbuffers; ++i) {
         if (i == nbuffers - 1) bufmax = fObjlen - nzip;
         else               bufmax = kMAXBUF;
         R__zipMultipleAlgorithm(fCompression, &bufmax, objbuf, &bufmax, bufcur, &nout, cxAlgorithm);
         if (nout == 0 || nout >= fObjlen) { //this happens when the buffer cannot be compressed
            Error("CreateBindData", "Fail to zip buffer");
            noutot = 0;
            with_zip = false;
            break;
         }
         bufcur += nout;
         noutot += nout;
         objbuf += kMAXBUF;
         nzip   += kMAXBUF;
      }
   }

   if (with_zip) {
      hdr->zipped = (UInt_t) fObjlen;
      hdr->payload = (UInt_t) noutot;
   } else {
      memcpy(fBuffer, sbuf->Buffer(), fObjlen);
      hdr->zipped = 0;
      hdr->payload = (UInt_t) fObjlen;
   }

   length = sizeof(BinDataHeader) + hdr->payload;

   delete sbuf;

   return kTRUE;
}


Bool_t TRootSniffer::Produce(const char* kind, const char* path, const char* options, void* &ptr, Long_t& length)
{
   if ((kind==0) || (*kind==0) || (strcmp(kind,"bin")==0))
      return ProduceBinary(path, options, ptr, length);

   if (strcmp(kind,"png")==0)
      return ProduceImage(TImage::kPng, path, options, ptr, length);

   if (strcmp(kind,"jpeg")==0)
      return ProduceImage(TImage::kJpeg, path, options, ptr, length);

   return kFALSE;
}


Bool_t TRootSniffer::RegisterObject(const char* subfolder, TObject* obj)
{
   if (obj==0) return kFALSE;

   TFolder* topf = gROOT->GetRootFolder();

   if (topf==0) {
      Error("RegisterObject","Not found top ROOT folder!!!");
      return kFALSE;
   }

   TFolder* topdabcfold = dynamic_cast<TFolder*> (topf->FindObject(fObjectsPath.Data()));
   if (topdabcfold==0) {
      topdabcfold = topf->AddFolder(fObjectsPath.Data(), "Top online folder");
      topdabcfold->SetOwner(kFALSE);
   }

   TFolder* dabcfold = topdabcfold;

   if ((subfolder!=0) && (strlen(subfolder)>0)) {

      TObjArray* arr = TString(subfolder).Tokenize("/");
      for (Int_t i=0; i <= (arr ? arr->GetLast() : -1); i++) {

         const char* subname = arr->At(i)->GetName();
         if (strlen(subname)==0) continue;

         TFolder* fold = dynamic_cast<TFolder*> (dabcfold->FindObject(subname));
         if (fold==0) {
            fold = dabcfold->AddFolder(subname, "sub-folder");
            fold->SetOwner(kFALSE);

            if ((dabcfold == topdabcfold) && (strcmp(subname, "extra")==0))
               fold->SetBit(BIT(19), kTRUE);
         }
         dabcfold = fold;
      }

   }

   // If object will be destroyed, it must be removed from the folders automatically
   obj->SetBit(kMustCleanup);

   dabcfold->Add(obj);

   // register folder for cleanup
   if (!gROOT->GetListOfCleanups()->FindObject(dabcfold))
      gROOT->GetListOfCleanups()->Add(dabcfold);

   return kTRUE;
}

Bool_t TRootSniffer::UnregisterObject(TObject* obj)
{
   if (obj==0) return kTRUE;

   TFolder* topf = gROOT->GetRootFolder();

   if (topf==0) {
      Error("UnregisterObject","Not found top ROOT folder!!!");
      return kFALSE;
   }

   TFolder* dabcfold = dynamic_cast<TFolder*> (topf->FindObject(fObjectsPath.Data()));

   if (dabcfold) dabcfold->RecursiveRemove(obj);

   return kTRUE;
}

