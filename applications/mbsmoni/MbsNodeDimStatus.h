// $Id: MbsNodeDimStatus.h 478 2009-10-29 12:26:09Z goofy $
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
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>
#include "sys_def.h"
#include "sbs_def.h"
#include "ml_def.h"
#include "s_setup.h"
#include "s_set_ml.h"
#include "s_set_mo.h"
#include "s_daqst.h"
#include "dis.hxx"
#include "dimRecords.h"

using namespace std;

class MbsNodeDimStatus{
public:
  MbsNodeDimStatus();
  MbsNodeDimStatus(const char *node, void*, const char *list, const char *setup);
  ~MbsNodeDimStatus();
  void Update(const char *);
  void Reset();
private:
  DimService * AddHisto(const char *name, dabcHistogram **histo, int chan, float min, float max, const char *color, const char *lett, const char *cont);
  DimService * AddRate(const char *name, dabcRate *rate, const char *color, const char *text, int mode, float ul);
  DimService * AddState(const char *name, dabcState *state, int value, const char *color, const char *text);
  DimService * AddInfo(const char *name, dabcInfo *info, int value, const char *color, const char *text);
  DimService * AddValue(const char *name, const char *type, int size, unsigned int *value);
  void SetInfo(dabcInfo *info, int value, const char *color, const char *text);
  void SetState(dabcState *state, int value, const char *color, const char *text);
  int BuildQuality(unsigned int s, unsigned int t, unsigned int v, unsigned int m)
    { return s | (t << 8) | (v << 16) | (m << 24);}

  dabcRate rStreamsFull; DimService *DrStreamsFull; int bStreamsFull; float rMaxStreamsFull;
  dabcRate rFileFilled; DimService *DrFileFilled; int bFileFilled; float rMaxFileFilled;
  dabcRate rEventRate; DimService *DrEventRate; int bEventRate; float rMaxEventRate;
  dabcRate rEventTrend; DimService *DrEventTrend; int bEventTrend; float rMaxEventTrend;
  dabcRate rDataRateKb; DimService *DrDataRateKb; int bDataRateKb; float rMaxDataRateKb;
  dabcRate rDataTrendKb; DimService *DrDataTrendKb; int bDataTrendKb; float rMaxDataTrendKb;
  dabcRate rEvSizeRateB; DimService *DrEvSizeRateB; int bEvSizeRateB; float rMaxEvSizeRateB;
  dabcRate rEvSizeTrendB; DimService *DrEvSizeTrendB; int bEvSizeTrendB; float rMaxEvSizeTrendB;
  dabcRate rStreamRateKb; DimService *DrStreamRateKb; int bStreamRateKb; float rMaxStreamRateKb;
  dabcRate rStreamTrendKb; DimService *DrStreamTrendKb; int bStreamTrendKb; float rMaxStreamTrendKb;
  dabcRate rTriggerRate; DimService *DrTriggerRate; int bTriggerRate; float rMaxTriggerRate;
  dabcRate rTrigger01Rate; DimService *DrTrigger01Rate; int bTrigger01Rate; float rMaxTrigger01Rate;
  dabcRate rTrigger02Rate; DimService *DrTrigger02Rate; int bTrigger02Rate; float rMaxTrigger02Rate;
  dabcRate rTrigger03Rate; DimService *DrTrigger03Rate; int bTrigger03Rate; float rMaxTrigger03Rate;
  dabcRate rTrigger04Rate; DimService *DrTrigger04Rate; int bTrigger04Rate; float rMaxTrigger04Rate;
  dabcRate rTrigger05Rate; DimService *DrTrigger05Rate; int bTrigger05Rate; float rMaxTrigger05Rate;
  dabcRate rTrigger06Rate; DimService *DrTrigger06Rate; int bTrigger06Rate; float rMaxTrigger06Rate;
  dabcRate rTrigger07Rate; DimService *DrTrigger07Rate; int bTrigger07Rate; float rMaxTrigger07Rate;
  dabcRate rTrigger08Rate; DimService *DrTrigger08Rate; int bTrigger08Rate; float rMaxTrigger08Rate;
  dabcRate rTrigger09Rate; DimService *DrTrigger09Rate; int bTrigger09Rate; float rMaxTrigger09Rate;
  dabcRate rTrigger10Rate; DimService *DrTrigger10Rate; int bTrigger10Rate; float rMaxTrigger10Rate;
  dabcRate rTrigger11Rate; DimService *DrTrigger11Rate; int bTrigger11Rate; float rMaxTrigger11Rate;
  dabcRate rTrigger12Rate; DimService *DrTrigger12Rate; int bTrigger12Rate; float rMaxTrigger12Rate;
  dabcRate rTrigger13Rate; DimService *DrTrigger13Rate; int bTrigger13Rate; float rMaxTrigger13Rate;
  dabcRate rTrigger14Rate; DimService *DrTrigger14Rate; int bTrigger14Rate; float rMaxTrigger14Rate;
  dabcRate rTrigger15Rate; DimService *DrTrigger15Rate; int bTrigger15Rate; float rMaxTrigger15Rate;


  dabcHistogram *hTrigCountHis; DimService *DhTrigCountHis; int bTrigCountHis; int chTrigCountHis; float rMinTrigCountHis; float rMaxTrigCountHis;
  dabcHistogram *hTrigRateHis; DimService *DhTrigRateHis; int bTrigRateHis; int chTrigRateHis; float rMinTrigRateHis; float rMaxTrigRateHis;

  dabcState sTriggerMode; DimService *DsTriggerMode; int bTriggerMode;
  dabcState sSpillOn; DimService *DsSpillOn; int bSpillOn;
  dabcState sRunMode; DimService *DsRunMode; int bRunMode;
  dabcState sFileOpen; DimService *DsFileOpen; int bFileOpen;
  dabcState sBuildingMode; DimService *DsBuildingMode; int bBuildingMode;
  dabcState sEventBuilding; DimService *DsEventBuilding; int bEventBuilding;
  dabcState sRunning; DimService *DsRunning; int bRunning;
  dabcState sRfioServer; DimService *DsRfioServer; int bRfioServer;

  dabcInfo eNodeTime; DimService *DeNodeTime;
  dabcInfo eNodeList; DimService *DeNodeList;
  dabcInfo eTaskList; DimService *DeTaskList;
  dabcInfo ePerform; DimService *DePerform;
  dabcInfo eFile; DimService *DeFile;
  dabcInfo eSetup; DimService *DeSetup;
  dabcInfo eMlSetup; DimService *DeMlSetup;

  int iEvents; DimService *DiEvents; int bEvents;
  int iBuffers; DimService *DiBuffers; int bBuffers;
  int iMBytes; DimService *DiMBytes; int bMBytes;
  int iBufferSize; DimService *DiBufferSize; int bBufferSize;
  int iStreamMbytes; DimService *DiStreamMbytes; int bStreamMbytes;
  int iFileMbytes; DimService *DiFileMbytes; int bFileMbytes;
  int iFlushTime; DimService *DiFlushTime; int bFlushTime;
  int iStreamScale; DimService *DiStreamScale; int bStreamScale;
  int iStreamSync; DimService *DiStreamSync; int bStreamSync;
  int iStreamKeep; DimService *DiStreamKeep; int bStreamKeep;
  int iNofStreams; DimService *DiNofStreams; int bNofStreams;
  int iStreamBufs; DimService *DiStreamBufs; int bStreamBufs;
  int iUserVal_00; DimService *DiUserVal_00; int bUserVal_00;
  int iUserVal_01; DimService *DiUserVal_01; int bUserVal_01;
  int iUserVal_02; DimService *DiUserVal_02; int bUserVal_02;
  int iUserVal_03; DimService *DiUserVal_03; int bUserVal_03;
  int iUserVal_04; DimService *DiUserVal_04; int bUserVal_04;
  int iUserVal_05; DimService *DiUserVal_05; int bUserVal_05;
  int iUserVal_06; DimService *DiUserVal_06; int bUserVal_06;
  int iUserVal_07; DimService *DiUserVal_07; int bUserVal_07;
  int iUserVal_08; DimService *DiUserVal_08; int bUserVal_08;
  int iUserVal_09; DimService *DiUserVal_09; int bUserVal_09;
  int iUserVal_10; DimService *DiUserVal_10; int bUserVal_10;
  int iUserVal_11; DimService *DiUserVal_11; int bUserVal_11;
  int iUserVal_12; DimService *DiUserVal_12; int bUserVal_12;
  int iUserVal_13; DimService *DiUserVal_13; int bUserVal_13;
  int iUserVal_14; DimService *DiUserVal_14; int bUserVal_14;
  int iUserVal_15; DimService *DiUserVal_15; int bUserVal_15;

  int iTriggerCount; DimService *DiTriggerCount; int bTriggerCount;
  int iTrigger01Count; DimService *DiTrigger01Count; int bTrigger01Count;
  int iTrigger02Count; DimService *DiTrigger02Count; int bTrigger02Count;
  int iTrigger03Count; DimService *DiTrigger03Count; int bTrigger03Count;
  int iTrigger04Count; DimService *DiTrigger04Count; int bTrigger04Count;
  int iTrigger05Count; DimService *DiTrigger05Count; int bTrigger05Count;
  int iTrigger06Count; DimService *DiTrigger06Count; int bTrigger06Count;
  int iTrigger07Count; DimService *DiTrigger07Count; int bTrigger07Count;
  int iTrigger08Count; DimService *DiTrigger08Count; int bTrigger08Count;
  int iTrigger09Count; DimService *DiTrigger09Count; int bTrigger09Count;
  int iTrigger10Count; DimService *DiTrigger10Count; int bTrigger10Count;
  int iTrigger11Count; DimService *DiTrigger11Count; int bTrigger11Count;
  int iTrigger12Count; DimService *DiTrigger12Count; int bTrigger12Count;
  int iTrigger13Count; DimService *DiTrigger13Count; int bTrigger13Count;
  int iTrigger14Count; DimService *DiTrigger14Count; int bTrigger14Count;
  int iTrigger15Count; DimService *DiTrigger15Count; int bTrigger15Count;

  char cNodeList[512]; DimService *DcNodeList;
  char cCurNode[64]; DimService *DcCurNode;
  char cDate[64]; DimService *DcDate;
  char cUser[64]; DimService *DcUser;
  char cRun[64]; DimService *DcRun;
  char cExp[64]; DimService *DcExp;

  char cMbsNode[64];
  char cPrefix[64];
  char cServerName[64];
  char cSetupFile[64];
  int fileOn;
  s_daqst *ps_daqst;
};
