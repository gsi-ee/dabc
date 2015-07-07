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
