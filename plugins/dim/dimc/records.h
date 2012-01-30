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

#ifndef DIMC_records
#define DIMC_records

#define _DABC_HISTOGRAM_RECORD_FORMAT_    "L:1;F:1;F:1;C:32;C:32;C:16;L"
#define _DABC_STATUS_RECORD_FORMAT_       "L:1;C:16;C:16"
#define _DABC_INFO_RECORD_FORMAT_         "L:1;C:16;C:128"
#define _DABC_RATE_RECORD_FORMAT_         "F:1;L:1;F:1;F:1;F:1;F:1;C:16;C:16;C:16"

#define _DABC_COLOR_RED_        "Red"
#define _DABC_COLOR_GREEN_      "Green"
#define _DABC_COLOR_BLUE_       "Blue"
#define _DABC_COLOR_CYAN_       "Cyan"
#define _DABC_COLOR_YELLOW_     "Yellow"
#define _DABC_COLOR_MAGENTA_    "Magenta"


#ifdef __cplusplus /* compatibility mode for pure C includes */

namespace dimc {

#endif

   typedef struct{
      int channels;  /* channels of data */
      float xlow;
      float xhigh;
      char xlett[32];
      char cont[32];
      char color[16]; /* (color name: White, Red, Green, Blue, Cyan, Yellow, Magenta) */
      int data; /* first data word. */
     /* DIM record must be allocated by malloc, including this header and the data field. */
   } HistogramRec;

   typedef struct{
      int severity; /* (0=success, 1=warning, 2=error, 3=fatal)*/
      char color[16]; /* (color name: Red, Green, Blue, Cyan, Yellow, Magenta)*/
      char status[16]; /* status name */
   } StatusRec;


   typedef struct{
      int verbose; /* (0=Plain text, 1=Node:text) */
      char color[16]; /* (Red, Green, Blue, Cyan, Magenta, Yellow) */
      char info[128]; /* info message */
   } InfoRec;


   typedef struct {
      float value; 
      int displaymode; /* one of the DISPLAY_x */
      float lower; /* limit. If limits are equal, use autoscale */
      float upper; /* limit */
      float alarmlower; /* alarm */
      float alarmupper; /* alarm */
      char color[16]; /* (color name: Red, Green, Blue, Cyan, Yellow, Magenta) */
      char alarmcolor[16]; /* (color name: Red, Green, Blue, Cyan, Yellow, Magenta) */
      char units[16];
   } RateRec;

   enum RateDisplayMode { 
       DISPLAY_ARC = 0,
       DISPLAY_BAR = 1,
       DISPLAY_TREND = 2,
       DISPLAY_STAT = 3
   };

#ifdef __cplusplus

   extern const char* HistogramRecDesc;
   extern const char* StatusRecDesc;
   extern const char* InfoRecDesc;
   extern const char* RateRecDesc;

   extern const char* col_Red;
   extern const char* col_Green;
   extern const char* col_Blue;
   extern const char* col_Cyan;
   extern const char* col_Yellow;
   extern const char* col_Magenta;

} /* namespace */
#endif /* c++ */


#endif
