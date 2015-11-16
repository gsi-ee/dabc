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

#include <math.h>

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
}


base::H1handle stream::DabcProcMgr::MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* options)
{
   std::string xtitle, xlbls;
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
      if (part.find("kind:")==0) { } else
      if (part.find("reuse")==0) { reuse = true; } else
         xtitle = part;
   }

   dabc::Hierarchy h = fTop.GetHChild(name);
   if (!h.null() && reuse && h.GetFieldPtr("bins"))
      return (base::H1handle) h.GetFieldPtr("bins")->GetDoubleArr();

   if (h.null()) h = fTop.CreateHChild(name);
   if (h.null()) return 0;

   h.SetField("_kind","ROOT.TH1D");
   h.SetField("_title", title);
   h.SetField("_dabc_hist", true); // indicate for browser that it is DABC histogram
   h.SetField("_make_request", "DABC.ReqH"); // provide proper request
   h.SetField("_after_request", "DABC.ConvertH"); // convert object into ROOT histogram
   h.SetField("nbins", nbins);
   h.SetField("left", left);
   h.SetField("right", right);
   if (xtitle.length()>0) h.SetField("xtitle", xtitle);
   if (xlbls.length()>0) h.SetField("xlabels", xlbls);

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
   std::string xtitle, ytitle, xlbls, ylbls;
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
      if (part.find("kind:")==0) { } else
      if (part.find("reuse")==0) { reuse = true; } else
      if (xtitle.length()==0) xtitle = part; else ytitle = part;
   }

   dabc::Hierarchy h = fTop.GetHChild(name);
   if (!h.null() && reuse && h.GetFieldPtr("bins"))
      return (base::H1handle) h.GetFieldPtr("bins")->GetDoubleArr();

   if (h.null()) h = fTop.CreateHChild(name);
   if (h.null()) return 0;

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
   if (xtitle.length()>0) h.SetField("xtitle", xtitle);
   if (ytitle.length()>0) h.SetField("ytitle", ytitle);
   if (xlbls.length()>0) h.SetField("xlabels", xlbls);
   if (ylbls.length()>0) h.SetField("ylabels", ylbls);

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

bool stream::DabcProcMgr::ClearHistogram(dabc::Hierarchy& item)
{
   if (!item.HasField("_dabc_hist") || (item.GetFieldPtr("bins")==0)) return false;

   int indx = item.GetField("_kind").AsStr()=="ROOT.TH1D" ? 3 : 6;

   double* arr = item.GetFieldPtr("bins")->GetDoubleArr();
   int len = item.GetFieldPtr("bins")->GetArraySize();
   while (indx<len) arr[indx++]=0.;
   return true;
}

void stream::DabcProcMgr::ClearAllHistograms()
{
   dabc::Iterator iter(fTop);
   while (iter.next()) {

      dabc::Hierarchy item = iter.ref();

      ClearHistogram(item);
   }
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
         ClearAllHistograms();
         res = "true";
      } else
      if (item.IsName("Start")) {
         DOUT0("Call START");
         fWorkingFlag = true;
         res = "true";
      } else
      if (item.IsName("Stop")) {
         DOUT0("Call STOP");
         fWorkingFlag = false;
         res = "true";
      } else {
         res = "false";
      }

   } else {
      std::string kind = item.GetField("_kind").AsStr();
      if ((kind != "ROOT.TH2D") && (kind != "ROOT.TH1D")) return false;

      double* bins = item.GetFieldPtr("bins")->GetDoubleArr();
      if (bins==0) return false;

      name.erase(0,5);

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

   dabc::Buffer raw = dabc::Buffer::CreateBuffer(res.c_str(), res.length(), false, true);
   cmd.SetRawData(raw);

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
   if (fStore.null()) return false;

   dabc::Command cmd("Create");
   cmd.SetStr("fname", storename);
   cmd.SetStr("ftitle", "File with stored stream data");
   cmd.SetStr("tname", "T");
   cmd.SetStr("ttitle", "Tree with stream data");

   if (!fStore.Execute(cmd)) return false;

   // set pointer to inform base class that storage exists
   fTree = (TTree*) cmd.GetPtr("tree_ptr");

   return true;
}

bool stream::DabcProcMgr::CloseStore()
{
   fTree = 0;
   fStore.Execute("Close");
   fStore.Release();
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
   dabc::Command cmd("Fill");
   return fStore.Execute(cmd);
}
