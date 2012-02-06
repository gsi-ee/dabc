// $Id$
//-----------------------------------------------------------------------
//       The GSI Online Offline Object Oriented (Go4) Project
//         Experiment Data Processing at EE department, GSI
//-----------------------------------------------------------------------
// Copyright (C) 2000- GSI Helmholtzzentrum für Schwerionenforschung GmbH
//                     Planckstr. 1, 64291 Darmstadt, Germany
// Contact:            http://go4.gsi.de
//-----------------------------------------------------------------------
// This software can be used under the license agreements as stated
// in Go4License.txt file which is part of the distribution.
//-----------------------------------------------------------------------
#ifndef DABCRECORDS
#define DABCRECORDS

enum recordtype {
  ATOMIC      = 0,
  GENERIC     = 1,
  STATE       = 2,
  RATE        = 3,
  HISTOGRAM   = 4,
  MODULE      = 5,
  PORT        = 6,
  DEVICE      = 7,
  QUEUE       = 8,
  COMMANDDESC = 9,
  INFO        = 10
};
enum recordstate{
  NOTSPEC     = 0,
  UPTODATE    = 1,
  UNSOLICITED = 2,
  OBSOLETE    = 3,
  INVALID     = 4,
  UNDEFINED   = 5
};
enum recordmode {
  NOMODE    = 0 ,
  ANYMODE   = 1
};
enum ratemode {
  ARC   = 0,
  BAR   = 1,
  TREND = 2,
  STAT  = 3
};
enum visibility {
  HIDDEN    =  0,
  VISIBLE   =  1,
  MONITOR   =  2,
  CHANGABLE =  4,
  IMPORTANT =  8,
  LOGGING   = 16
};
static const char * HISTOGRAMDESC="L:1;F:1;F:1;C:32;C:32;C:16;L";
typedef struct{
  int channels;  // channels of data
  float xlow;
  float xhigh;
  char xlett[32];
  char cont[32];
  char color[16];
  int data; // first data word
} dabcHistogram;
static const char * STATEDESC="L:1;C:16;C:16";
typedef struct{
  int severity; // (0=success, 1=warning, 2=error, 3=fatal)
  char color[16]; // (color name)
  char status[16]; // status name
} dabcState;
static const char * INFODESC="L:1;C:16;C:128";
typedef struct{
  int mode; // (0=plain text, 1=node plus text)
  char color[16]; // (color name)
  char text[128]; // status name
} dabcInfo;
static const char * RATEDESC="F:1;L:1;F:1;F:1;F:1;F:1;C:16;C:16;C:16";
typedef struct{
  float value;
  int displaymode;
  float lower; // limit
  float upper; // limit
  float alarmlower; // alarm
  float alarmupper; // alarm
  char color[16];
  char alarmcolor[16];
  char units[16];
} dabcRate;
#endif
