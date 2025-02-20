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

#ifndef ELDER_VisConDabc
#define ELDER_VisConDabc

#include <elderpt.hpp>

#include "dabc/Hierarchy.h"
#include "dabc/Command.h"
#include "dabc/Worker.h"

#include <vector>
#include <string>

namespace elderdabc {

   /** keep reference to hierarchy objects together with unique name */
   struct ObjectHandle
      {
         std::string name;
         double* data;
         int size;
      };


     typedef struct ObjectHandle H1handle;

     typedef struct ObjectHandle H2handle;


     typedef struct ObjectHandle C1handle;
     typedef struct ObjectHandle C2handle;

     static const struct ObjectHandle NullHandle= {"",nullptr,0};


class VisConDabc : public ::elderpt::viscon::Interface
{



      private:

         std::vector<elderdabc::H1handle> fvHistograms1d; // keep pointers to 1d histograms in hierarchy. index in vector used as handle for elder
         std::vector<elderdabc::H2handle> fvHistograms2d; // keep pointers to 2d histograms in hierarchy. index in vector used as handle for elder

         std::vector<elderdabc::C1handle> fvConditions1d; // keep pointers to 1d conditions hierarchy. index in vector used as handle for elder
         std::vector<elderdabc::C2handle> fvConditions2d; // keep pointers to 2d conditions in hierarchy. index in vector used as handle for elder

         // TODO: condition painter in dabc?
//               std::vector<TGo4WinCond*> conditions1d_;
//               std::vector<TGo4WinCond*> conditions2d_;
//               std::vector<TGo4PolyCond*> conditions2d_poly_;

      protected:

         dabc::Hierarchy fTop;
         dabc::Hierarchy fHistFolder;
         dabc::Hierarchy fCondFolder;

         dabc::LocalWorkerRef fStore;
         std::string fStoreInfo;   ///<! last info about storage
         bool fSortOrder{false};   ///<! sorting order
         int  fDefaultFill{0};    ///<! default fill color

         bool ClearHistogram(dabc::Hierarchy& item);

         dabc::Hierarchy FindHistogram(double* handle); // access the actual object in the dabc hierarchy via the data field handle

     //    dabc::Hierarchy FindCondition(double* handle);  // access the actual object in the dabc hierarchy via the data field handle

         bool ClearAllDabcHistograms(dabc::Hierarchy &folder);
         bool SaveAllDabcHistograms(dabc::Hierarchy &folder);

         // JAM 28-02-24: use old implementations from stream with modified handle:



         elderdabc::H1handle MakeH1(const char *name, const char *title, int nbins, double left, double right, const char *xtitle = nullptr); ///<! returns handle to histogram object in hierarchy
         //
         elderdabc::H2handle MakeH2(const char *name, const char *title, int nbins1, double left1, double right1, int nbins2, double left2, double right2, const char *options = nullptr);///<! returns handle to histogram object
         //

         void FillH1(elderdabc::H1handle h1, double x, double weight); // implementation stolen from stream framework
         double GetH1Content(elderdabc::H1handle h1, int bin);
         void  SetH1Content(elderdabc::H1handle h1, int bin, double v);
          void SetH1Title(elderdabc::H1handle h1, const char *title);
         void TagH1Time(elderdabc::H1handle h1);

         void FillH2(elderdabc::H2handle h2, double x, double y, double weight); // implementation stolen from stream framework
         double GetH2Content(elderdabc::H2handle h2, int bin1, int bin2);
         void SetH2Content(elderdabc::H2handle h2, int bin1, int bin2, double v);
         void SetH2Title(elderdabc::H2handle h2, const char *title) ;
         void TagH2Time(elderdabc::H2handle h2);
         //

         elderdabc::C1handle MakeC1(const char *name,
                                 double left,
                                 double right,
                                 const char *histogram_name=nullptr);

         elderdabc::C2handle MakeC2Win(const char *name,
                                         double xmin, double xmax, double ymin, double ymax,
                                         const char *histogram_name=nullptr);


         elderdabc::C2handle MakeC2Poly(const char *name,
                                 const std::vector<double> &points,
                                 const char *histogram_name=nullptr);

         void GetLimitsC1(elderdabc::C1handle h, double &left, double &right);

         void GetLimitsC2Poly(elderdabc::C2handle h, double** points, int &numpoints);

         void GetLimitsC2Win(elderdabc::C2handle h,  double &xmin, double &xmax, double &ymin, double &ymax);


      public:
         VisConDabc();
         virtual ~VisConDabc();

         void SetTop(dabc::Hierarchy& top, bool withcmds = false);

         void SetDefaultFill(int fillcol = 3) { fDefaultFill = fillcol; }



         //! @brief Implements the Go4-way of creating a 1d histogram.
            //! @param name The name of the histogram.
            //! @param title The title of the histogram.
            //! @param axis  The x-axis name of the histogram. Will be ignored
            //!              in this particular implementation.
            //! @param n_bins  The number of bins of the histogram.
            //! @param left    The value of the leftmost channel of the histogram.
            //! @param right   The value of the rightmost channel of the histogram.
            //! @return a handle that can be used to fill the histogram by
            //!         using the function hist1d_fill.
         ::elderpt::viscon::Interface::Histogram1DHandle hist1d_create(const char *name,
                          const char *title,
                          const char *axis,
                          int n_bins,
                          double left,
                          double right) override;

            //! @brief Implements the Go4-way of filling a 1d histogram.
            //! @param h a handle to the histogram that was created using hist1d_create
            //! @param value the value that is to be filled into the histogram
            void hist1d_fill(::elderpt::viscon::Interface::Histogram1DHandle h, double value) override;

            void hist1d_set_bin(::elderpt::viscon::Interface::Histogram1DHandle h, int bin, double value) override;

            ::elderpt::viscon::Interface::Histogram2DHandle hist2d_create(const char *name,
                          const char *title,
                          const char *axis1,
                          int n_bins1,
                          double left1,
                          double right1,
                          const char *axis2,
                          int n_bins2,
                          double left2,
                          double right2) override;

            void hist2d_fill(::elderpt::viscon::Interface::Histogram2DHandle h, double value1, double value2) override ;

            void hist2d_set_bin(::elderpt::viscon::Interface::Histogram2DHandle h, int xbin, int ybin, double value) override;

            //! @brief Implements the Go4-way of creating a 1d window condition
            //! @param name the name of the condition
            //! @param left left limit of the window
            //! @param right right limit of the window
            //! @param h a handle to a histogram that should be linked to that condition
            ::elderpt::viscon::Interface::Condition1DHandle cond1d_create(const char *name,
                           double left,
                           double right,
                           ::elderpt::viscon::Interface::Histogram1DHandle h) override;

             //! @brief Implements the Go4-way of creating a 1d window condition
             //! @param h ???
             //! @param left left limit of the window
             //! @param right right limit of the window
             //! @warning Documented by Pico, h needs documentation
             void cond1d_get(::elderpt::viscon::Interface::Condition1DHandle h, double &left, double &right) override ;

             //! @brief Implements the Go4-way of creating a 2d window condition
            //! @param name the name of the condition
             //! @param points ???
             //! @param h a handle to a histogram that should be linked to that condition
             //! @warning Documented by Pico, points needs documentation
             ::elderpt::viscon::Interface::Condition2DHandle cond2d_create(const char *name,
                           const std::vector<double> &points,
                           elderpt::viscon::Interface::Histogram1DHandle h);

               void cond2d_get(::elderpt::viscon::Interface::Condition2DHandle h, std::vector<double> &points) override;







        void SetSortedOrder(bool on = true)   { fSortOrder = on; }
        bool IsSortedOrder()  { return fSortOrder; }

         void AddRunLog(const char *msg);// override;
         void AddErrLog(const char *msg);// override;

 //        bool ExecuteHCommand(dabc::Command cmd);


         bool SaveAllHistograms() { return SaveAllDabcHistograms(fTop); }

         bool ClearAllHistograms() { return ClearAllDabcHistograms(fTop); }

         std::string GetStoreInfo() const { return fStoreInfo; }
   };

}

#endif
