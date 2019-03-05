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

#include "stream/DabcProcMgr.h"

#include "dabc/Buffer.h"
#include "dabc/Iterator.h"
#include "dabc/Factory.h"
#include "dabc/Manager.h"
#include "dabc/BinaryFile.h"
#include "dabc/timing.h"

#include <cstdlib>
#include <math.h>

stream::DabcProcMgr::DabcProcMgr() :
   base::ProcMgr(),
   fTop(),
   fWorkingFlag(true),
   fStore(),
   fStoreInfo("no store created"),
   fSortOrder(true),
   fDefaultFill(3)
{
}

stream::DabcProcMgr::~DabcProcMgr()
{
}

void stream::DabcProcMgr::SetTop(dabc::Hierarchy& top, bool withcmds)
{
   fTop = top;

   if (!withcmds) return;

   dabc::Hierarchy h = fTop.CreateHChild("Control/Clear");
   h.SetField("_kind","Command");
   h.SetField("_title", "Clear all histograms in the server");
   h.SetField("_icon", "dabcsys/plugins/stream/icons/clear.png");
   h.SetField("_fastcmd", "true");
   h.SetField("_numargs", "0");

   h = fTop.CreateHChild("Control/Save");
   h.SetField("_kind","Command");
   h.SetField("_title", "Save all histograms in the dabc.root file");
   h.SetField("_icon", "dabcsys/plugins/stream/icons/save.png");
   h.SetField("_fastcmd", "true");
   h.SetField("_numargs", "0");

   h = fTop.CreateHChild("Control/Start");
   h.SetField("_kind","Command");
   h.SetField("_title", "Start processing of data");
   h.SetField("_icon", "dabcsys/plugins/stream/icons/start.png");
   h.SetField("_fastcmd", "true");
   h.SetField("_numargs", "0");

   h = fTop.CreateHChild("Control/Stop");
   h.SetField("_kind","Command");
   h.SetField("_title", "Stop processing of data");
   h.SetField("_icon", "dabcsys/plugins/stream/icons/stop.png");
   h.SetField("_fastcmd", "true");
   h.SetField("_numargs", "0");


   h = fTop.CreateHChild("Control/RunLog");
   h.SetField(dabc::prop_kind, "log");
   h.EnableHistory(1000);

   h = fTop.CreateHChild("Control/ErrLog");
   h.SetField(dabc::prop_kind, "log");
   h.EnableHistory(1000);
}


void stream::DabcProcMgr::AddRunLog(const char *msg)
{
   dabc::Hierarchy h = fTop.GetHChild("Control/RunLog");
   h.SetField("value", msg);
   h.MarkChangedItems();
}

void stream::DabcProcMgr::AddErrLog(const char *msg)
{
   dabc::Hierarchy h = fTop.GetHChild("Control/ErrLog");
   h.SetField("value", msg);
   h.MarkChangedItems();
}

base::H1handle stream::DabcProcMgr::MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* options)
{
   std::string xtitle, ytitle, xlbls, fillcolor, drawopt, hmin, hmax;
   bool reuse(false);

   while (options) {
      const char* separ = strchr(options,';');
      std::string part = options;
      if (separ) {
         part.resize(separ-options);
         options = separ+1;
      } else {
         options = nullptr;
      }

      if (part.find("xbin:")==0) { xlbls = part; xlbls.erase(0, 5); } else
      if (part.find("fill:")==0) { fillcolor = part; fillcolor.erase(0,5); } else
      if (part.find("opt:")==0) { drawopt = part; drawopt.erase(0,4); } else
      if (part.find("hmin:")==0) { hmin = part; hmin.erase(0,5); } else
      if (part.find("hmax:")==0) { hmax = part; hmax.erase(0,5); } else
      if (part.find("kind:")==0) { } else
      if (part.find("reuse")==0) { reuse = true; } else
      if (xtitle.empty()) xtitle = part; else ytitle = part;
   }

   dabc::Hierarchy h = fTop.GetHChild(name);
   if (!h.null() && reuse && h.GetFieldPtr("bins"))
      return (base::H1handle) h.GetFieldPtr("bins")->GetDoubleArr();

   if (!h) {
      std::string sname = name;
      auto pos = sname.find_last_of("/");
      if ((pos != std::string::npos) && fSortOrder)
         h = fTop.CreateHChild(sname.substr(0,pos).c_str(), false, true).CreateHChild(sname.substr(pos+1).c_str());
      else
         h = fTop.CreateHChild(name);
   }
   if (!h) return nullptr;

   h.SetField("_kind","ROOT.TH1D");
   h.SetField("_title", title);
   h.SetField("_dabc_hist", true); // indicate for browser that it is DABC histogram
   h.SetField("_make_request", "DABC.ReqH"); // provide proper request
   h.SetField("_after_request", "DABC.ConvertH"); // convert object into ROOT histogram
   h.SetField("nbins", nbins);
   h.SetField("left", left);
   h.SetField("right", right);
   if (!xtitle.empty()) h.SetField("xtitle", xtitle);
   if (!ytitle.empty()) h.SetField("ytitle", ytitle);
   if (xlbls.length()>0) h.SetField("xlabels", xlbls);
   h.SetField("fillcolor", fillcolor.empty() ? fDefaultFill : std::atoi(fillcolor.c_str()));
   if (drawopt.length() > 0) h.SetField("drawopt", drawopt);
   if (!hmin.empty()) h.SetField("hmin", std::atof(hmin.c_str()));
   if (!hmax.empty()) h.SetField("hmax", std::atof(hmax.c_str()));

   std::vector<double> bins;
   bins.resize(nbins+5, 0.);
   bins[0] = nbins;
   bins[1] = left;
   bins[2] = right;
   h.SetField("bins", bins);

   return (base::H1handle) h.GetFieldPtr("bins")->GetDoubleArr();
}

base::H2handle stream::DabcProcMgr::MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options)
{
   std::string xtitle, ytitle, xlbls, ylbls, fillcolor, drawopt, hmin, hmax;
   bool reuse(false);

   while (options!=0) {
      const char* separ = strchr(options,';');
      std::string part = options;
      if (separ!=0) {
         part.resize(separ-options);
         options = separ+1;
      } else {
         options = 0;
      }

      if (part.find("xbin:")==0) { xlbls = part; xlbls.erase(0, 5); } else
      if (part.find("ybin:")==0) { ylbls = part; ylbls.erase(0, 5); } else
      if (part.find("fill:")==0) { fillcolor = part; fillcolor.erase(0,5); } else
      if (part.find("opt:")==0) { drawopt = part; drawopt.erase(0,4); } else
      if (part.find("hmin:")==0) { hmin = part; hmin.erase(0,5); } else
      if (part.find("hmax:")==0) { hmax = part; hmax.erase(0,5); } else
      if (part.find("kind:")==0) { } else
      if (part.find("reuse")==0) { reuse = true; } else
      if (xtitle.empty()) xtitle = part; else ytitle = part;
   }

   dabc::Hierarchy h = fTop.GetHChild(name);
   if (!h.null() && reuse && h.GetFieldPtr("bins"))
      return (base::H1handle) h.GetFieldPtr("bins")->GetDoubleArr();

   if (!h) {
      std::string sname = name;
      auto pos = sname.find_last_of("/");
      if ((pos != std::string::npos) && fSortOrder)
         h = fTop.CreateHChild(sname.substr(0,pos).c_str(), false, true).CreateHChild(sname.substr(pos+1).c_str());
      else
         h = fTop.CreateHChild(name);
   }
   if (!h) return nullptr;

   h.SetField("_kind","ROOT.TH2D");
   h.SetField("_title", title);
   h.SetField("_dabc_hist", true); // indicate for browser that it is DABC histogram
   h.SetField("_make_request", "DABC.ReqH"); // provide proper request
   h.SetField("_after_request", "DABC.ConvertH"); // convert object into ROOT histogram
   h.SetField("nbins1", nbins1);
   h.SetField("left1", left1);
   h.SetField("right1", right1);
   h.SetField("nbins2", nbins2);
   h.SetField("left2", left2);
   h.SetField("right2", right2);
   if (!xtitle.empty()) h.SetField("xtitle", xtitle);
   if (!ytitle.empty()) h.SetField("ytitle", ytitle);
   if (xlbls.length() > 0) h.SetField("xlabels", xlbls);
   if (ylbls.length() > 0) h.SetField("ylabels", ylbls);
   if (!fillcolor.empty()) h.SetField("fillcolor", std::atoi(fillcolor.c_str()));
   h.SetField("drawopt", drawopt.empty() ? std::string("colz") : drawopt);
   if (!hmin.empty()) h.SetField("hmin", std::atof(hmin.c_str()));
   if (!hmax.empty()) h.SetField("hmax", std::atof(hmax.c_str()));

   std::vector<double> bins;
   bins.resize(6+(nbins1+2)*(nbins2+2), 0.);
   bins[0] = nbins1;
   bins[1] = left1;
   bins[2] = right1;
   bins[3] = nbins2;
   bins[4] = left2;
   bins[5] = right2;
   h.SetField("bins", bins);

   return (base::H2handle) h.GetFieldPtr("bins")->GetDoubleArr();
}

void stream::DabcProcMgr::SetH1Title(base::H1handle h1, const char* title)
{
}

void stream::DabcProcMgr::SetH2Title(base::H2handle h2, const char* title)
{
}


bool stream::DabcProcMgr::ClearHistogram(dabc::Hierarchy& item)
{
   if (!item.HasField("_dabc_hist") || (item.GetFieldPtr("bins")==0)) return false;

   int indx = item.GetField("_kind").AsStr()=="ROOT.TH1D" ? 3 : 6;

   double* arr = item.GetFieldPtr("bins")->GetDoubleArr();
   int len = item.GetFieldPtr("bins")->GetArraySize();
   while (indx<len) arr[indx++]=0.;
   return true;
}

bool stream::DabcProcMgr::ClearAllHistograms(dabc::Hierarchy &folder)
{
   dabc::Iterator iter(folder);
   bool isany = true;
   while (iter.next()) {
      dabc::Hierarchy item = iter.ref();
      if (ClearHistogram(item)) isany = true;
   }
   return isany;
}

bool stream::DabcProcMgr::SaveAllHistograms(dabc::Hierarchy &folder)
{
   dabc::Buffer buf = folder.SaveToBuffer();

   if (buf.GetTotalSize()==0) return false;

   DOUT0("store hierarchy size %d in temporary h.bin file", buf.GetTotalSize());
   {
      dabc::BinaryFile f;
      system("rm -f h.bin");
      if (f.OpenWriting("h.bin")) {
         if (f.WriteBufHeader(buf.GetTotalSize(), buf.GetTypeId()))
            for (unsigned n=0;n<buf.NumSegments();n++)
               f.WriteBufPayload(buf.SegmentPtr(n), buf.SegmentSize(n));
         f.Close();
      }
   }

   dabc::DateTime dt;
   dt.GetNow();

   std::string args = dabc::format("dabc_root -h h.bin -o dabc-%s-%s.root", dt.OnlyDateAsString("-").c_str(), dt.OnlyTimeAsString("-").c_str());

   DOUT0("Calling: %s", args.c_str());

   int res = system(args.c_str());

   if (res!=0) {
      EOUT("Fail to convert DABC histograms in ROOT file, check h-date-time.bin file");
      args = dabc::format("mv h.bin h-%s-%s.bin", dt.OnlyDateAsString("-").c_str(), dt.OnlyTimeAsString("-").c_str());
      system(args.c_str());
   } else {
      system("rm -f h.bin");
   }

   return res == 0;
}

bool stream::DabcProcMgr::ExecuteHCommand(dabc::Command cmd)
{
   std::string name = cmd.GetName();

   if ((name.find("HCMD_")!=0) && (name!="ROOTCMD")) return false;

   dabc::Hierarchy item = cmd.GetRef("item");
   if (item.null()) return false;
   std::string res = "null";

   if (name == "ROOTCMD") {
      if (item.IsName("Clear")) {
         DOUT0("Call CLEAR");
         ClearAllHistograms(fTop);
         res = "true";
      } else if (item.IsName("Save")) {
         DOUT0("Call SAVE");
         SaveAllHistograms(fTop);
         res = "true";
      } else if (item.IsName("Start")) {
         DOUT0("Call START");
         fWorkingFlag = true;
         res = "true";
      } else if (item.IsName("Stop")) {
         DOUT0("Call STOP");
         fWorkingFlag = false;
         res = "true";
      } else {
         res = "false";
      }

   } else if (name == "HCMD_ClearHistos") {

      res = ClearAllHistograms(item) ? "true" : "false";

   } else {
      std::string kind = item.GetField("_kind").AsStr();
      if ((kind != "ROOT.TH2D") && (kind != "ROOT.TH1D")) return false;

      double* bins = item.GetFieldPtr("bins")->GetDoubleArr();
      if (bins==0) return false;

      name.erase(0,5); // remove HCMD_ prefix

      if ((name == "GetMean") || (name=="GetRMS") || (name=="GetEntries")) {
         if (kind != "ROOT.TH1D") return false;
         int nbins = item.GetField("nbins").AsInt();
         double left = item.GetField("left").AsDouble();
         double right = item.GetField("right").AsDouble();

         double sum0(0), sum1(0), sum2(0);

         for (int n=0;n<nbins;n++) {
            double x = left + (right-left)/nbins*(n+0.5);
            sum0 += bins[n+4];
            sum1 += x*bins[n+4];
            sum2 += x*x*bins[n+4];
         }
         double mean(0), rms(0);
         if (sum0>0) {
            mean = sum1/sum0;
            rms = sqrt(sum2/sum0 - mean*mean);
         }
         if (name == "GetEntries") res = dabc::format("%14.7g",sum0);
         else if (name == "GetMean") res = dabc::format("%8.6g",mean);
         else res = dabc::format("%8.6g",rms);

      } else
      if (name=="Clear") {
         res = ClearHistogram(item) ? "true" : "false";
      } else {
         return false;
      }
   }

   cmd.SetStrRawData(res);

   return true;
}

typedef void StreamCallFunc(void*);

bool stream::DabcProcMgr::CallFunc(const char* funcname, void* arg)
{
   if (funcname==0) return false;

   void* symbol = dabc::Factory::FindSymbol(funcname);
   if (symbol == 0) return false;

   StreamCallFunc* func = (StreamCallFunc*) symbol;

   func(arg);

   return true;
}


bool stream::DabcProcMgr::CreateStore(const char* storename)
{
   fStore = dabc::mgr.CreateObject("root::TreeStore","stream_store");
   if (fStore.null()) {
      fStoreInfo = "Fail to create root::TreeStore, check libDabcRoot plugin";
      return false;
   }

   dabc::Command cmd("Create");
   cmd.SetStr("fname", storename);
   cmd.SetStr("ftitle", "File with stored stream data");
   cmd.SetStr("tname", "T");
   cmd.SetStr("ttitle", "Tree with stream data");

   if (!fStore.Execute(cmd)) {
      fStoreInfo = dabc::format("Fail to create ROOT file %s", storename);
      return false;
   }

   // set pointer to inform base class that storage exists
   fTree = (TTree*) cmd.GetPtr("tree_ptr");

   fStoreInfo = dabc::format("Create ROOT file %s", storename);

   return true;
}

bool stream::DabcProcMgr::CloseStore()
{
   fTree = 0;
   fStore.Execute("Close");
   fStore.Release();
   fStoreInfo = "ROOT store closed";
   return true;
}

bool stream::DabcProcMgr::CreateBranch(const char* name, const char* class_name, void** obj)
{
   DOUT3("Create Branch1 %s", name);

   dabc::Command cmd("CreateBranch");
   cmd.SetStr("name", name);
   cmd.SetStr("class_name", class_name);
   cmd.SetPtr("obj", (void*) obj);
   return fStore.Execute(cmd);
}

bool stream::DabcProcMgr::CreateBranch(const char* name, void* member, const char* kind)
{
   DOUT3("Create Branch2 %s", name);

   dabc::Command cmd("CreateBranch");
   cmd.SetStr("name", name);
   cmd.SetPtr("member", member);
   cmd.SetStr("kind", kind);
   return fStore.Execute(cmd);
}

bool stream::DabcProcMgr::StoreEvent()
{
   if (fStore.null()) return false;

   dabc::Command cmd("Fill");
   if (!fStore.Execute(cmd)) return false;

   fStoreInfo = cmd.GetStr("StoreInfo");

   return true;
}
