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

#include "elderdabc/VisConDabc.h"

#include "dabc/Buffer.h"
#include "dabc/Iterator.h"
#include "dabc/Factory.h"
#include "dabc/Manager.h"
#include "dabc/BinaryFile.h"
#include "dabc/timing.h"

#include <cstdlib>
#include <cmath>

//const struct elderdabc::ObjectHandle elderdabc::NullHandle = {"",nullptr,0};


elderdabc::VisConDabc::VisConDabc() :
   elderpt::viscon::Interface(),
   fSortOrder(true),
   fDefaultFill(3)
{
   fvHistograms1d.clear();
   fvHistograms2d.clear();

}


elderdabc::VisConDabc::~VisConDabc()
{
}

void elderdabc::VisConDabc::SetTop(dabc::Hierarchy& top, bool withcmds)
{
   fTop = top;
   //fTop=top.CreateHChild("Viscon");later
   fHistFolder=top.CreateHChild("Histograms");
   fCondFolder=top.CreateHChild("Conditions");

   // JAM24 - same as in fesa plugin?
   //fCondFolder.SetField("_autoload", "dabcsys/plugins/elder/elder.js");

   if (!withcmds) return;

//   dabc::Hierarchy h = fTop.CreateHChild("Control/Clear");
//   h.SetField("_kind","Command");
//   h.SetField("_title", "Clear all histograms in the server");
//   h.SetField("_icon", "dabcsys/plugins/elder/icons/clear.png");
//   h.SetField("_fastcmd", "true");
//   h.SetField("_numargs", "0");
//
//   h = fTop.CreateHChild("Control/Save");
//   h.SetField("_kind","Command");
//   h.SetField("_title", "Save all histograms in the dabc.root file");
//   h.SetField("_icon", "dabcsys/plugins/elder/icons/save.png");
//   h.SetField("_fastcmd", "true");
//   h.SetField("_numargs", "0");

//   h = fTop.CreateHChild("Control/Start");
//   h.SetField("_kind","Command");
//   h.SetField("_title", "Start processing of data");
//   h.SetField("_icon", "dabcsys/plugins/elder/icons/start.png");
//   h.SetField("_fastcmd", "true");
//   h.SetField("_numargs", "0");
//
//   h = fTop.CreateHChild("Control/Stop");
//   h.SetField("_kind","Command");
//   h.SetField("_title", "Stop processing of data");
//   h.SetField("_icon", "dabcsys/plugins/elder/icons/stop.png");
//   h.SetField("_fastcmd", "true");
//   h.SetField("_numargs", "0");


   dabc::Hierarchy h = fTop.CreateHChild("Control/RunLog");
   h.SetField(dabc::prop_kind, "log");
   h.EnableHistory(1000);

   h = fTop.CreateHChild("Control/ErrLog");
   h.SetField(dabc::prop_kind, "log");
   h.EnableHistory(1000);
}


void elderdabc::VisConDabc::AddRunLog(const char *msg)
{
   dabc::Hierarchy h = fTop.GetHChild("Control/RunLog");
   h.SetField("value", msg);
   h.MarkChangedItems();
   DOUT1("AddRunLog: %s",msg);
}

void elderdabc::VisConDabc::AddErrLog(const char *msg)
{
   dabc::Hierarchy h = fTop.GetHChild("Control/ErrLog");
   h.SetField("value", msg);
   h.MarkChangedItems();
   DOUT1("AddErrLog: %s",msg);
}

//void elderdabc::VisConDabc::PrintLog(const char *msg)
//{
//   //if (fDebug >= 0)
//      DOUT0(msg);
//}

////////////// TODO: implement elder viscon methods below


::elderpt::viscon::Interface::Histogram1DHandle elderdabc::VisConDabc::hist1d_create(const char *name,
                         const char *title,
                         const char *, //axis,
                         int n_bins,
                         double left,
                         double right)
{
   elderdabc::H1handle hist= MakeH1(name, title, n_bins, left, right);
   fvHistograms1d.push_back(hist);
   return fvHistograms1d.size()-1;

return 0;
}

void elderdabc::VisConDabc::hist1d_fill(
      ::elderpt::viscon::Interface::Histogram1DHandle h, double value)
{
   if (h < 0 || (size_t) h > fvHistograms1d.size()) return; // error handling required when handle out of bounds?
   elderdabc::H1handle hist = fvHistograms1d[h];
   FillH1(hist, value, 1);
}

void elderdabc::VisConDabc::hist1d_set_bin(
   ::elderpt::viscon::Interface::Histogram1DHandle h, int bin, double value)
{

   if (h < 0 || (size_t) h > fvHistograms1d.size()) return; // error handling required when handle out of bounds?
   elderdabc::H1handle hist = fvHistograms1d[h]; // TODO: error handling when out of bounds?
   SetH1Content(hist, bin, value);
}

::elderpt::viscon::Interface::Histogram2DHandle elderdabc::VisConDabc::hist2d_create(const char *name,
                         const char *title,
                         const char *, //axis1,
                         int n_bins1,
                         double left1,
                         double right1,
                         const char *, //axis2,
                         int n_bins2,
                         double left2,
                         double right2)
{
   elderdabc::H2handle hist= MakeH2(name, title, n_bins1, left1, right1,  n_bins2, left2, right2);
   fvHistograms2d.push_back(hist);
   return fvHistograms2d.size()-1;
}

void elderdabc::VisConDabc::hist2d_fill(::elderpt::viscon::Interface::Histogram2DHandle h, double value1, double value2)
{
   if (h < 0 || (size_t) h > fvHistograms2d.size()) return; // error handling required when handle out of bounds?
   elderdabc::H2handle hist = fvHistograms2d[h];
   FillH2(hist, value1, value2, 1);
}

void elderdabc::VisConDabc::hist2d_set_bin(::elderpt::viscon::Interface::Histogram2DHandle h, int xbin, int ybin, double value)
{
   if (h < 0 || (size_t) h > fvHistograms2d.size()) return; // error handling required when handle out of bounds?
   elderdabc::H2handle hist = fvHistograms2d[h];
   SetH2Content(hist, xbin, ybin, value);
}

           //! @brief Implements the Go4-way of creating a 1d window condition
           //! @param name the name of the condition
           //! @param left left limit of the window
           //! @param right right limit of the window
           //! @param h a handle to a histogram that should be linked to that condition
::elderpt::viscon::Interface::Condition1DHandle elderdabc::VisConDabc::cond1d_create(
                           const char* name,
                          double left,
                          double right,
                          int h
                          )
{
   // first check if we get the histogram name from the handle h:
   std::string histoname="";
   if (h >=0 || (size_t) h < fvHistograms1d.size())
   {
       elderdabc::H1handle hist = fvHistograms1d[h];
       histoname=hist.name;
   }

   elderdabc::C1handle conny= MakeC1(name, left, right, histoname.c_str());
   fvConditions1d.push_back(conny);
    return fvConditions1d.size()-1;

//   return ::elderpt::viscon::Interface::INVALID_HANDLE;
}





//! @brief Implements the Go4-way of creating a 1d window condition
                     //! @param h ???
                     //! @param left left limit of the window
                     //! @param right right limit of the window
                     //! @warning Documented by Pico, h needs documentation
void elderdabc::VisConDabc::cond1d_get(::elderpt::viscon::Interface::Condition1DHandle h, double &left, double &right)
{



   if (h == ::elderpt::viscon::Interface::INVALID_HANDLE)
      return;
   if (h < 0 || (size_t) h > fvConditions1d.size()) return; // error handling required when handle out of bounds?
   elderdabc::C1handle con = fvConditions1d[h];
   GetLimitsC1(con, left, right);
}
                     //! @brief Implements the Go4-way of creating a 1d window condition
           //! @param name the name of the condition
                     //! @param points ???
                     //! @param h a handle to a histogram that should be linked to that condition
                     //! @warning Documented by Pico, points needs documentation
::elderpt::viscon::Interface::Condition2DHandle elderdabc::VisConDabc::cond2d_create(const char *name,
                          const std::vector<double> &points,
                          int h)
{
   int size = points.size();
   if (size < 4)
      return ::elderpt::viscon::Interface::INVALID_HANDLE;
   if (size%2 != 0)
      return ::elderpt::viscon::Interface::INVALID_HANDLE;


     std::string histoname="";
     elderdabc::C2handle connny;
     if (h >=0 || (size_t) h < fvHistograms2d.size())
     {
        elderdabc::H1handle hist = fvHistograms2d[h];
        histoname=hist.name;
     }


     if (size == 4) // create 2d window condition
     {

        elderdabc::C2handle conny= MakeC2Win(name, points[0], points[1], points[2], points[3],histoname.c_str());
        fvConditions2d.push_back(conny);
        return fvConditions2d.size()-1;

     }
     else  // create 2d polygon condition
     {

        elderdabc::C2handle conny= MakeC2Poly(name, points, histoname.c_str());
        fvConditions2d.push_back(conny);
        return fvConditions2d.size()-1;
     }

}



void elderdabc::VisConDabc::cond2d_get(
   ::elderpt::viscon::Interface::Condition2DHandle h,
   std::vector<double> &points)
{
   if (h == ::elderpt::viscon::Interface::INVALID_HANDLE)
      return;

   if (points.size() < 4)
      return;

   if (h < 0 || (size_t) h > fvConditions2d.size())
      return; // error handling required when handle out of bounds?
   elderdabc::C2handle con = fvConditions2d[h];

   if (points.size() == 4) {
      points.resize(4, 0);
      GetLimitsC2Win(con, points[0], points[1], points[2], points[3]);
   } else {
      double *parray;
      int numpoints = 0;
      GetLimitsC2Poly(con, &parray, numpoints);
      points.clear();
      for (int i = 0; i < numpoints; ++i)
         points.push_back(parray[i]);
   }
}







//////////////////////////////////////////// JAM below old from stream interface
elderdabc::H1handle elderdabc::VisConDabc::MakeH1(const char* name, const char* title, int nbins, double left, double right, const char* options)
{
   std::string xtitle, ytitle, xlbls, fillcolor, drawopt, hmin, hmax;
   bool reuse = false, clear_protect = false;
   elderdabc::H1handle ret=elderdabc::NullHandle;
   while (options) {
      const char* separ = strchr(options,';');
      std::string part = options;
      if (separ) {
         part.resize(separ-options);
         options = separ+1;
      } else {
         options = nullptr;
      }

      if (part.find("xbin:") == 0) { xlbls = part; xlbls.erase(0, 5); } else
      if (part.find("fill:") == 0) { fillcolor = part; fillcolor.erase(0,5); } else
      if (part.find("opt:") == 0) { drawopt = part; drawopt.erase(0,4); } else
      if (part.find("hmin:") == 0) { hmin = part; hmin.erase(0,5); } else
      if (part.find("hmax:") == 0) { hmax = part; hmax.erase(0,5); } else
      if (part.find("kind:") == 0) { } else
      if (part.find("reuse") == 0) { reuse = true; } else
      if (part.find("clear_protect") == 0) { clear_protect = true; } else
      if (xtitle.empty()) xtitle = part; else ytitle = part;
   }

   dabc::LockGuard lock(fTop.GetHMutex());

   dabc::Hierarchy h = fHistFolder.GetHChild(name);
   if (!h.null() && reuse && h.GetFieldPtr("bins")){
         ret.data= h.GetFieldPtr("bins")->GetDoubleArr();
         ret.name=name;
         return ret;
   }
   if (!h) {
      std::string sname = name;
      auto pos = sname.find_last_of("/");
      if ((pos != std::string::npos) && fSortOrder)
         h = fHistFolder.CreateHChild(sname.substr(0,pos).c_str(), false, true).CreateHChild(sname.substr(pos+1).c_str());
      else
         h = fHistFolder.CreateHChild(name);
   }
   if (!h) return elderdabc::NullHandle;

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
   h.SetField("fillcolor", fillcolor.empty() ? fDefaultFill : std::stoi(fillcolor));
   if (drawopt.length() > 0) h.SetField("drawopt", drawopt);
   if (!hmin.empty()) h.SetField("hmin", std::stof(hmin));
   if (!hmax.empty()) h.SetField("hmax", std::stof(hmax));
   if (clear_protect) h.SetField("_no_reset", "true");

   std::vector<double> bins;
   bins.resize(nbins+5, 0.);
   bins[0] = nbins;
   bins[1] = left;
   bins[2] = right;
   h.SetField("bins", bins);

   fHistFolder.MarkChangedItems();
   ret.name=name;
   ret.data=h.GetFieldPtr("bins")->GetDoubleArr();
   return ret;
}

elderdabc::H2handle elderdabc::VisConDabc::MakeH2(const char* name, const char* title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char* options)
{
   std::string xtitle, ytitle, xlbls, ylbls, fillcolor, drawopt, hmin, hmax, h2poly;
   bool reuse = false, clear_protect = false;
   elderdabc::H2handle ret=elderdabc::NullHandle;
   while (options != nullptr) {
      const char* separ = strchr(options,';');
      std::string part = options;
      if (separ) {
         part.resize(separ-options);
         options = separ+1;
      } else {
         options = nullptr;
      }

      if (part.find("xbin:") == 0) { xlbls = part; xlbls.erase(0, 5); } else
      if (part.find("ybin:") == 0) { ylbls = part; ylbls.erase(0, 5); } else
      if (part.find("fill:") == 0) { fillcolor = part; fillcolor.erase(0,5); } else
      if (part.find("opt:") == 0) { drawopt = part; drawopt.erase(0,4); } else
      if (part.find("hmin:") == 0) { hmin = part; hmin.erase(0,5); } else
      if (part.find("hmax:") == 0) { hmax = part; hmax.erase(0,5); } else
      if (part.find("kind:") == 0) { } else
      if (part.find("h2poly:") == 0) { h2poly = part; h2poly.erase(0,7); } else
      if (part.find("reuse") == 0) { reuse = true; } else
      if (part.find("clear_protect") == 0) { clear_protect = true; } else
      if (xtitle.empty()) xtitle = part; else ytitle = part;
   }

   dabc::LockGuard lock(fTop.GetHMutex());

   dabc::Hierarchy h = fHistFolder.GetHChild(name);
   if (!h.null() && reuse && h.GetFieldPtr("bins")){
            ret.data= h.GetFieldPtr("bins")->GetDoubleArr();
            ret.name=name;
            return ret;
      }



   if (!h) {
      std::string sname = name;
      auto pos = sname.find_last_of("/");
      if ((pos != std::string::npos) && fSortOrder)
         h = fHistFolder.CreateHChild(sname.substr(0,pos).c_str(), false, true).CreateHChild(sname.substr(pos+1).c_str());
      else
         h = fHistFolder.CreateHChild(name);
   }
   if (!h) return elderdabc::NullHandle;

   h.SetField("_kind", h2poly.empty() ? "ROOT.TH2D" : "ROOT.TH2Poly");
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
   if (!fillcolor.empty()) h.SetField("fillcolor", std::stoi(fillcolor));
   h.SetField("drawopt", drawopt.empty() ? std::string("colz") : drawopt);
   if (!hmin.empty()) h.SetField("hmin", std::stof(hmin));
   if (!hmax.empty()) h.SetField("hmax", std::stof(hmax));
   if (!h2poly.empty()) h.SetField("h2poly", h2poly);
   if (clear_protect) h.SetField("_no_reset", "true");

   std::vector<double> bins;
   bins.resize(6+(nbins1+2)*(nbins2+2), 0.);
   bins[0] = nbins1;
   bins[1] = left1;
   bins[2] = right1;
   bins[3] = nbins2;
   bins[4] = left2;
   bins[5] = right2;
   h.SetField("bins", bins);

   fHistFolder.MarkChangedItems();

   ret.name=name;
   ret.data=h.GetFieldPtr("bins")->GetDoubleArr();
   return ret;
}


void elderdabc::VisConDabc::FillH1(elderdabc::H1handle h1, double x, double weight)
{
   // taken from stream framework, double array is treated like in ROOT histograms (overflow, underflow bins etc.)
   double* arr = h1.data;
   int nbin = (int) arr[0];
   int bin = (int) (nbin * (x - arr[1]) / (arr[2] - arr[1]));
   if (bin<0) arr[3]+=weight; else
   if (bin>=nbin) arr[4+nbin]+=weight; else arr[4+bin]+=weight;
}



double elderdabc::VisConDabc::GetH1Content(elderdabc::H1handle h1, int bin)
{
   // taken from stream framework, double array is treated like in ROOT histograms (overflow, underflow bins etc.)
   double* arr = h1.data;
   int nbin = (int) arr[0];
   if (bin<0) return arr[3];
   if (bin>=nbin) return arr[4+nbin];
   return arr[4+bin];
}


void  elderdabc::VisConDabc::SetH1Content(elderdabc::H1handle h1, int bin, double v)
{
   // taken from stream framework, double array is treated like in ROOT histograms (overflow, underflow bins etc.)
   double* arr = h1.data;
   int nbin = (int) arr[0];
   if (bin<0) arr[3] = v;
   else if (bin>=nbin) arr[4+nbin] = v;
   else arr[4+bin] = v;
}




void elderdabc::VisConDabc::FillH2(elderdabc::H2handle h2, double x, double y, double weight)
{
   // taken from stream framework, double array is treated like in ROOT histograms (overflow, underflow bins etc.)
   double* arr = h2.data;

   int nbin1 = (int) arr[0];
   int nbin2 = (int) arr[3];

   int bin1 = (int) (nbin1 * (x - arr[1]) / (arr[2] - arr[1]));
   int bin2 = (int) (nbin2 * (y - arr[4]) / (arr[5] - arr[4]));

   if (bin1<0) bin1 = -1; else if (bin1>nbin1) bin1 = nbin1;
   if (bin2<0) bin2 = -1; else if (bin2>nbin2) bin2 = nbin2;

   arr[6 + (bin1+1) + (bin2+1)*(nbin1+2)] += weight;
}

double elderdabc::VisConDabc::GetH2Content(H2handle h2, int bin1, int bin2)
{
   // taken from stream framework, double array is treated like in ROOT histograms (overflow, underflow bins etc.)
   double* arr = h2.data;

   int nbin1 = (int) arr[0];
   int nbin2 = (int) arr[3];

   if (bin1<0) bin1 = -1; else if (bin1>nbin1) bin1 = nbin1;
   if (bin2<0) bin2 = -1; else if (bin2>nbin2) bin2 = nbin2;

   return arr[6 + (bin1+1) + (bin2+1)*(nbin1+2)];
}


void elderdabc::VisConDabc::SetH2Content(H2handle h2, int bin1, int bin2, double v)
{
   // taken from stream framework, double array is treated like in ROOT histograms (overflow, underflow bins etc.)
   double* arr = h2.data;

   int nbin1 = (int) arr[0];
   int nbin2 = (int) arr[3];

   if (bin1<0) bin1 = -1; else if (bin1>nbin1) bin1 = nbin1;
   if (bin2<0) bin2 = -1; else if (bin2>nbin2) bin2 = nbin2;

   arr[6 + (bin1+1) + (bin2+1)*(nbin1+2)] = v;
}



dabc::Hierarchy elderdabc::VisConDabc::FindHistogram(double *handle)
{
//   if (IsBlockHistCreation()) {
//      DOUT0("FindHistogram when blocked due to threaing?\n");
//   }

   if (!handle) return nullptr;

   dabc::Iterator iter(fHistFolder);
   while (iter.next()) {
      dabc::Hierarchy item = iter.ref();
      if (item.HasField("_dabc_hist"))
         if (item.GetFieldPtr("bins")->GetDoubleArr() == handle)
            return item;
   }
   return nullptr;
}

//dabc::Hierarchy elderdabc::VisConDabc::FindCondition(double *handle)
//{
//
//   if (!handle) return nullptr;
//   dabc::Iterator iter(fCondFolder);
//   while (iter.next()) {
//      dabc::Hierarchy item = iter.ref();
//         if (item.GetFieldPtr("limits")->GetDoubleArr() == handle)
//            return item;
//   }
//   return nullptr;
//}
//


void elderdabc::VisConDabc::SetH1Title(elderdabc::H1handle h1, const char *title)
{
   auto item = FindHistogram(h1.data);
   if (!item.null())
      item.SetField("_title", title);
}

void elderdabc::VisConDabc::TagH1Time(elderdabc::H1handle h1)
{
   auto item = FindHistogram(h1.data);
   if (!item.null()) {
      auto now = dabc::DateTime().GetNow();
      item.SetField("_humantime", now.AsString(3, true));
      item.SetField("_time", now.AsUTCSeconds());
   }
}

void elderdabc::VisConDabc::SetH2Title(elderdabc::H2handle h2, const char *title)
{
   auto item = FindHistogram(h2.data);
   if (!item.null())
      item.SetField("_title", title);
}

void elderdabc::VisConDabc::TagH2Time(elderdabc::H2handle h2)
{
   auto item = FindHistogram(h2.data);
   if (!item.null()) {
      auto now = dabc::DateTime().GetNow();
      item.SetField("_humantime", now.AsString(3, true));
      item.SetField("_time", now.AsUTCSeconds());
   }
}


bool elderdabc::VisConDabc::ClearHistogram(dabc::Hierarchy &item)
{
   if (!item.HasField("_dabc_hist") || (item.GetFieldPtr("bins") == nullptr)) return false;

   if (item.HasField("_no_reset")) return true;

   int indx = item.GetField("_kind").AsStr()=="ROOT.TH1D" ? 3 : 6;

   double* arr = item.GetFieldPtr("bins")->GetDoubleArr();
   int len = item.GetFieldPtr("bins")->GetArraySize();
   while (indx<len) arr[indx++]=0.;
   return true;
}



bool elderdabc::VisConDabc::ClearAllDabcHistograms(dabc::Hierarchy &folder)
{
   dabc::Iterator iter(folder);
   bool isany = true;
   while (iter.next()) {
      dabc::Hierarchy item = iter.ref();
      if (ClearHistogram(item)) isany = true;
   }
   return isany;
}



bool elderdabc::VisConDabc::SaveAllDabcHistograms(dabc::Hierarchy &folder)
{
   dabc::Buffer buf = folder.SaveToBuffer();

   if (buf.GetTotalSize() == 0) return false;

   DOUT0("store hierarchy size %d in temporary h.bin file", buf.GetTotalSize());
   {
      dabc::BinaryFile f;
      std::system("rm -f h.bin");
      if (f.OpenWriting("h.bin")) {
         if (f.WriteBufHeader(buf.GetTotalSize(), buf.GetTypeId()))
            for (unsigned n=0;n<buf.NumSegments();n++)
               f.WriteBufPayload(buf.SegmentPtr(n), buf.SegmentSize(n));
         f.Close();
      }
   }

   dabc::DateTime dt;
   dt.GetNow();

   std::string args = dabc::format("dabc_root -h h.bin -o dabc-%s-%s.root", dt.OnlyDateAsString("-", true).c_str(), dt.OnlyTimeAsString("-", true).c_str());

   DOUT0("Calling: %s", args.c_str());

   int res = std::system(args.c_str());

   if (res != 0) {
      EOUT("Fail to convert DABC histograms in ROOT file, check h-date-time.bin file");
      args = dabc::format("mv h.bin h-%s-%s.bin", dt.OnlyDateAsString("-", true).c_str(), dt.OnlyTimeAsString("-", true).c_str());
      std::system(args.c_str());
   } else {
      std::system("rm -f h.bin");
   }

   return res == 0;
}

//bool elderdabc::VisConDabc::ExecuteHCommand(dabc::Command cmd)
//{
//   std::string name = cmd.GetName();
//
//   if ((name.find("HCMD_") != 0) && (name != "ROOTCMD")) return false;
//
//   dabc::Hierarchy item = cmd.GetRef("item");
//   if (item.null()) return false;
//   std::string res = "null";
//
//   if (name == "ROOTCMD") {
//      if (item.IsName("Clear")) {
//         DOUT0("Call CLEAR");
//         ClearAllDabcHistograms(fHistFolder);
//         res = "true";
//      } else if (item.IsName("Save")) {
//         DOUT0("Call SAVE");
//         SaveAllHistograms(fHistFolder);
//         res = "true";
/////////////////////////////////////////
////      } else if (item.IsName("Start")) {
////         DOUT0("Call START");
////         //fWorkingFlag = true;
////         res = "true";
////      } else if (item.IsName("Stop")) {
////         DOUT0("Call STOP");
////         //fWorkingFlag = false;
////         res = "true";
/////////////////////////////////////////
//      }else {
//         res = "false";
//      }
//
//   }
////   else if (name == "HCMD_ClearHistos") {
////
////      res = ClearAllDabcHistograms(item) ? "true" : "false";
////
////   } else {
////      std::string kind = item.GetField("_kind").AsStr();
////      if ((kind != "ROOT.TH2D") && (kind != "ROOT.TH1D")) return false;
////
////      double* bins = item.GetFieldPtr("bins")->GetDoubleArr();
////      if (!bins) return false;
////
////      name.erase(0,5); // remove HCMD_ prefix
////
////      if ((name == "GetMean") || (name=="GetRMS") || (name=="GetEntries")) {
////         if (kind != "ROOT.TH1D") return false;
////         int nbins = item.GetField("nbins").AsInt();
////         double left = item.GetField("left").AsDouble();
////         double right = item.GetField("right").AsDouble();
////
////         double sum0 = 0, sum1 = 0, sum2 = 0;
////
////         for (int n=0;n<nbins;n++) {
////            double x = left + (right-left)/nbins*(n+0.5);
////            sum0 += bins[n+4];
////            sum1 += x*bins[n+4];
////            sum2 += x*x*bins[n+4];
////         }
////         double mean = 0, rms = 0;
////         if (sum0>0) {
////            mean = sum1/sum0;
////            rms = sqrt(sum2/sum0 - mean*mean);
////         }
////         if (name == "GetEntries") res = dabc::format("%14.7g",sum0);
////         else if (name == "GetMean") res = dabc::format("%8.6g",mean);
////         else res = dabc::format("%8.6g",rms);
////
////      } else
////      if (name=="Clear") {
////         res = ClearHistogram(item) ? "true" : "false";
////      } else {
////         return false;
////      }
////   }
//
//   cmd.SetStrRawData(res);
//   return res=="true"? true: false;
//}

//typedef void StreamCallFunc(void*);




elderdabc::C1handle elderdabc::VisConDabc::MakeC1(const char *name, double left,
      double right, const char *histogram_name)
{
   dabc::LockGuard lock(fTop.GetHMutex());
   dabc::Hierarchy h = fCondFolder.GetHChild(name);
   elderdabc::C1handle ret=elderdabc::NullHandle;
   if (!h.null() && h.GetFieldPtr("limits")){
            ret.data= h.GetFieldPtr("limits")->GetDoubleArr();
            ret.name=name;
            ret.size=h.GetField("numpoints").AsInt(2);
            return ret;
      }


   if (!h) {
      std::string sname = name;
      auto pos = sname.find_last_of("/");
      if ((pos != std::string::npos) && fSortOrder)
         h = fCondFolder.CreateHChild(sname.substr(0,pos).c_str(), false, true).CreateHChild(sname.substr(pos+1).c_str());
      else
         h = fCondFolder.CreateHChild(name);
   }
   if (!h) return elderdabc::NullHandle;

   h.SetField("_kind","ELDER.WINCON1");
   h.SetField("_can_draw", true);
   h.SetField("_title", name);
   //h.SetField("_dabc_hist", true); // indicate for browser that it is DABC histogram
//   h.SetField("_make_request", "DABC.ReqH"); // provide proper request
//   h.SetField("_after_request", "ELDER.ConvertC"); // convert object into elder condition display, added in elder.js
   h.SetField("_autoload", "dabcsys/plugins/elder/htm/elder.js");
   if(histogram_name)
      h.SetField("linked_histogram",histogram_name);
   else
      h.SetField("linked_histogram", "");

   std::vector<double> limits;
   limits.resize(2, 0.);
   limits[0] = left;
   limits[1] = right;
   h.SetField("limits", limits);
   h.SetField("numpoints",2);
   fCondFolder.MarkChangedItems();
   ret.name=name;
   ret.data=h.GetFieldPtr("limits")->GetDoubleArr();
   ret.size=2;
   return ret;
}

elderdabc::C2handle elderdabc::VisConDabc::MakeC2Win(const char *name,
      double xmin, double xmax, double ymin, double ymax,
      const char *histogram_name)
{

   dabc::LockGuard lock(fTop.GetHMutex());
   dabc::Hierarchy h = fCondFolder.GetHChild(name);
   elderdabc::C2handle ret=elderdabc::NullHandle;
   if (!h.null() && h.GetFieldPtr("limits")){
      ret.data= h.GetFieldPtr("limits")->GetDoubleArr();
      ret.name=name;
      ret.size=h.GetField("numpoints").AsInt(4);
      return ret;
   }
   if (!h) {
         std::string sname = name;
         auto pos = sname.find_last_of("/");
         if ((pos != std::string::npos) && fSortOrder)
            h = fCondFolder.CreateHChild(sname.substr(0,pos).c_str(), false, true).CreateHChild(sname.substr(pos+1).c_str());
         else
            h = fCondFolder.CreateHChild(name);
      }
      if (!h) return elderdabc::NullHandle;

      h.SetField("_kind","ELDER.WINCON2");
      h.SetField("_can_draw", true);
      h.SetField("_title", name);
      //h.SetField("_dabc_hist", true); // indicate for browser that it is DABC histogram
//      h.SetField("_make_request", "DABC.ReqH"); // provide proper request
//      h.SetField("_after_request", "ELDER.ConvertC");  // convert object into elder condition display, added in elder.js
      h.SetField("_autoload", "dabcsys/plugins/elder/htm/elder.js");
      if(histogram_name)
         h.SetField("linked_histogram",histogram_name);
      else
         h.SetField("linked_histogram", "");

      std::vector<double> limits;
      limits.resize(4, 0.);
      limits[0] = xmin;
      limits[1] = xmax;
      limits[2] = ymin;
      limits[3] = ymax;
      h.SetField("limits", limits);
      h.SetField("numpoints",4);
      fCondFolder.MarkChangedItems();
      ret.name=name;
      ret.data=h.GetFieldPtr("limits")->GetDoubleArr();
      ret.size=4;
      return ret;
}

elderdabc::C2handle elderdabc::VisConDabc::MakeC2Poly(const char *name,
      const std::vector<double> &points, const char *histogram_name)
{

   dabc::LockGuard lock(fTop.GetHMutex());
   dabc::Hierarchy h = fCondFolder.GetHChild(name);
   elderdabc::C2handle ret=elderdabc::NullHandle;
   if (!h.null() && h.GetFieldPtr("limits")){
         ret.data= h.GetFieldPtr("limits")->GetDoubleArr();
         ret.name=name;
         ret.size= h.GetField("numpoints").AsInt(0);
         return ret;
      }
      if (!h) {
         std::string sname = name;
         auto pos = sname.find_last_of("/");
         if ((pos != std::string::npos) && fSortOrder)
            h = fCondFolder.CreateHChild(sname.substr(0,pos).c_str(), false, true).CreateHChild(sname.substr(pos+1).c_str());
         else
            h = fCondFolder.CreateHChild(name);
      }
      if (!h) return elderdabc::NullHandle;

      h.SetField("_kind","ELDER.POLYCON");
      h.SetField("_can_draw", true);
      h.SetField("_title", name);
      //h.SetField("_dabc_hist", true); // indicate for browser that it is DABC histogram
//      h.SetField("_make_request", "DABC.ReqH"); // provide proper request
//      h.SetField("_after_request", "ELDER.ConvertC");  // convert object into elder condition display, added in elder.js
      h.SetField("_autoload", "dabcsys/plugins/elder/htm/elder.js");
      if(histogram_name)
         h.SetField("linked_histogram",histogram_name);
      else
         h.SetField("linked_histogram", "");

      h.SetField("limits", points);
      h.SetField("numpoints",points.size());
      fCondFolder.MarkChangedItems();
      ret.name=name;
      ret.data=h.GetFieldPtr("limits")->GetDoubleArr();
      ret.size=points.size();
      return ret;
}

void elderdabc::VisConDabc::GetLimitsC1(elderdabc::C1handle h, double &left,
      double &right)
{
   double* arr = h.data;
   left=arr[0];
   right=arr[1];
}

void elderdabc::VisConDabc::GetLimitsC2Poly(elderdabc::C2handle h,
      double **points, int &numpoints)
{
   *points = h.data;
   numpoints=h.size;
}


void elderdabc::VisConDabc::GetLimitsC2Win(elderdabc::C2handle h, double &xmin,
      double &xmax, double &ymin, double &ymax)
{
   double* arr = h.data;
   xmin=arr[0];
   xmax=arr[1];
   ymin=arr[2];
   ymax=arr[3];
}




