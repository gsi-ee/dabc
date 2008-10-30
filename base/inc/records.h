#ifndef DABC_records
#define DABC_records

namespace dabc {
    
   typedef struct{
      int channels;  // channels of data
      float xlow;
      float xhigh;
      char xlett[32];
      char cont[32];
      char color[16]; // (color name: White, Red, Green, Blue, Cyan, Yellow, Magenta)
      int data; // first data word. 
     // DIM record must be allocated by malloc, including this header and the data field.
   } HistogramRec;

   extern const char* HistogramRecDesc;

   typedef struct{
      int severity; // (0=success, 1=warning, 2=error, 3=fatal)
      char color[16]; // (color name: Red, Green, Blue, Cyan, Yellow, Magenta)
      char status[16]; // status name
   } StatusRec;
   
   extern const char* StatusRecDesc;


    typedef struct{
    int verbose; //(0=Plain text, 1=Node:text)
    char color[16]; //(Red, Green, Blue, Cyan, Magenta, Yellow)
    char info[128]; // info message
   } InfoRec;
   
   extern const char* InfoRecDesc;


   typedef struct{
      float value; 
      int displaymode; // one of the DISPLAY_x
      float lower; // limit. If limits are equal, use autoscale
      float upper; // limit
      float alarmlower; // alarm
      float alarmupper; // alarm
      char color[16]; // (color name: Red, Green, Blue, Cyan, Yellow, Magenta)
      char alarmcolor[16]; // (color name: Red, Green, Blue, Cyan, Yellow, Magenta)
      char units[16];
   } RateRec;

   enum RateDisplayMode { 
        DISPLAY_ARC = 0,
        DISPLAY_BAR = 1,
        DISPLAY_TREND = 2,
        DISPLAY_STAT = 3 };
        
   extern const char* RateRecDesc;

   extern const char* col_Red;
   extern const char* col_Green;
   extern const char* col_Blue;
   extern const char* col_Cyan;
   extern const char* col_Yellow;
   extern const char* col_Magenta;

}

#endif
