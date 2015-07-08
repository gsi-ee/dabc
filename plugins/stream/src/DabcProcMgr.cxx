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

#include <math.h>

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

bool stream::DabcProcMgr::ExecuteHCommand(dabc::Command cmd)
{
   std::string name = cmd.GetName();
   if (name.find("HCMD_Get")!=0) return false;

   dabc::Hierarchy item = cmd.GetRef("item");
   if (item.null()) return false;

   std::string kind = item.GetField("_kind").AsStr();
   if ((kind != "ROOT.TH2D") && (kind != "ROOT.TH1D")) return false;

   double* bins = item.GetFieldPtr("bins")->GetDoubleArr();
   if (bins==0) return false;

   name.erase(0,5);

   std::string res = "null";

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

   } else {
      return false;
   }



   dabc::Buffer raw = dabc::Buffer::CreateBuffer(res.c_str(), res.length(), false, true);
   cmd.SetRawData(raw);

   return true;
}
