// MBS DIM status.
#include "MbsNodeDimStatus.h"

//=================================================
MbsNodeDimStatus::MbsNodeDimStatus(){  }
MbsNodeDimStatus::~MbsNodeDimStatus(){  }
MbsNodeDimStatus::MbsNodeDimStatus(char *node, void *daqst, char *list)
{
  DrStreamsFull   =0; bStreamsFull   = 0 ; rMaxStreamsFull   = 100 ;
  DrFileFilled    =0; bFileFilled    = 0 ; rMaxFileFilled    = 100 ;
  DrEventRate     =0; bEventRate     = 1 ; rMaxEventRate     = 10000 ;
  DrEventTrend    =0; bEventTrend    = 1 ; rMaxEventTrend    = 10000 ;
  DrDataRateKb    =0; bDataRateKb    = 1 ; rMaxDataRateKb    = 10000 ;
  DrDataTrendKb   =0; bDataTrendKb   = 1 ; rMaxDataTrendKb   = 10000 ;
  DrEvSizeRateB   =0; bEvSizeRateB   = 0 ; rMaxEvSizeRateB   = 10000 ;
  DrEvSizeTrendB  =0; bEvSizeTrendB  = 0 ; rMaxEvSizeTrendB  = 10000 ;
  DrStreamRateKb  =0; bStreamRateKb  = 0 ; rMaxStreamRateKb  = 10000 ;
  DrStreamTrendKb =0; bStreamTrendKb = 0 ; rMaxStreamTrendKb = 10000 ;
  DrTriggerRate   =0; bTriggerRate   = 1 ; rMaxTriggerRate   = 10000 ;
  DrTrigger01Rate =0; bTrigger01Rate = 0 ; rMaxTrigger01Rate = 10000 ;
  DrTrigger02Rate =0; bTrigger02Rate = 0 ; rMaxTrigger02Rate = 10000 ;
  DrTrigger03Rate =0; bTrigger03Rate = 0 ; rMaxTrigger03Rate = 10000 ;
  DrTrigger04Rate =0; bTrigger04Rate = 0 ; rMaxTrigger04Rate = 10000 ;
  DrTrigger05Rate =0; bTrigger05Rate = 0 ; rMaxTrigger05Rate = 10000 ;
  DrTrigger06Rate =0; bTrigger06Rate = 0 ; rMaxTrigger06Rate = 10000 ;
  DrTrigger07Rate =0; bTrigger07Rate = 0 ; rMaxTrigger07Rate = 10000 ;
  DrTrigger08Rate =0; bTrigger08Rate = 0 ; rMaxTrigger08Rate = 10000 ;
  DrTrigger09Rate =0; bTrigger09Rate = 0 ; rMaxTrigger09Rate = 10000 ;
  DrTrigger10Rate =0; bTrigger10Rate = 0 ; rMaxTrigger10Rate = 10000 ;
  DrTrigger11Rate =0; bTrigger11Rate = 0 ; rMaxTrigger11Rate = 10000 ;
  DrTrigger12Rate =0; bTrigger12Rate = 0 ; rMaxTrigger12Rate = 10000 ;
  DrTrigger13Rate =0; bTrigger13Rate = 0 ; rMaxTrigger13Rate = 10000 ;
  DrTrigger14Rate =0; bTrigger14Rate = 0 ; rMaxTrigger14Rate = 10000 ;
  DrTrigger15Rate =0; bTrigger15Rate = 0 ; rMaxTrigger15Rate = 10000 ;


  DhTrigCountHis =0; bTrigCountHis = 0 ; chTrigCountHis = 16 ; rMinTrigCountHis = 0 ; rMaxTrigCountHis = 15 ;
  DhTrigRateHis  =0; bTrigRateHis  = 0 ; chTrigRateHis  = 16 ; rMinTrigRateHis  = 0 ; rMaxTrigRateHis  = 15 ;

  DsTriggerMode   =0; bTriggerMode   = 1 ;
  DsSpillOn       =0; bSpillOn       = 0 ;
  DsRunMode       =0; bRunMode       = 0 ;
  DsFileOpen      =0; bFileOpen      = 0 ;
  DsBuildingMode  =0; bBuildingMode  = 0 ;
  DsEventBuilding =0; bEventBuilding = 0 ;
  DsRunning       =0; bRunning       = 1 ;

  DeNodeTime  =0;
  DeNodeList =0;
  DeTaskList =0;
  DePerform  =0;
  DeFile     =0;
  DeSetup    =0;
  DeMlSetup  =0;

  DiEvents       =0; bEvents       = 1 ;
  DiBuffers      =0; bBuffers      = 1 ;
  DiMBytes       =0; bMBytes       = 1 ;
  DiBufferSize   =0; bBufferSize   = 1 ;
  DiStreamMbytes =0; bStreamMbytes = 1 ;
  DiFileMbytes   =0; bFileMbytes   = 1 ;
  DiFlushTime    =0; bFlushTime    = 1 ;
  DiStreamScale  =0; bStreamScale  = 1 ;
  DiStreamSync   =0; bStreamSync   = 1 ;
  DiStreamKeep =0; bStreamKeep = 1 ;
  DiUserVal_00 =0; bUserVal_00 = 0 ;
  DiUserVal_01 =0; bUserVal_01 = 0 ;
  DiUserVal_02 =0; bUserVal_02 = 0 ;
  DiUserVal_03 =0; bUserVal_03 = 0 ;
  DiUserVal_04 =0; bUserVal_04 = 0 ;
  DiUserVal_05 =0; bUserVal_05 = 0 ;
  DiUserVal_06 =0; bUserVal_06 = 0 ;
  DiUserVal_07 =0; bUserVal_07 = 0 ;
  DiUserVal_08 =0; bUserVal_08 = 0 ;
  DiUserVal_09 =0; bUserVal_09 = 0 ;
  DiUserVal_10 =0; bUserVal_10 = 0 ;
  DiUserVal_11 =0; bUserVal_11 = 0 ;
  DiUserVal_12 =0; bUserVal_12 = 0 ;
  DiUserVal_13 =0; bUserVal_13 = 0 ;
  DiUserVal_14 =0; bUserVal_14 = 0 ;
  DiUserVal_15 =0; bUserVal_15 = 0 ;

  DiTriggerCount   =0; bTriggerCount   = 0 ;
  DiTrigger01Count =0; bTrigger01Count = 0 ;
  DiTrigger02Count =0; bTrigger02Count = 0 ;
  DiTrigger03Count =0; bTrigger03Count = 0 ;
  DiTrigger04Count =0; bTrigger04Count = 0 ;
  DiTrigger05Count =0; bTrigger05Count = 0 ;
  DiTrigger06Count =0; bTrigger06Count = 0 ;
  DiTrigger07Count =0; bTrigger07Count = 0 ;
  DiTrigger08Count =0; bTrigger08Count = 0 ;
  DiTrigger09Count =0; bTrigger09Count = 0 ;
  DiTrigger10Count =0; bTrigger10Count = 0 ;
  DiTrigger11Count =0; bTrigger11Count = 0 ;
  DiTrigger12Count =0; bTrigger12Count = 0 ;
  DiTrigger13Count =0; bTrigger13Count = 0 ;
  DiTrigger14Count =0; bTrigger14Count = 0 ;
  DiTrigger15Count =0; bTrigger15Count = 0 ;

  DcNodeList =0;
  DcCurNode  =0;
  DcDate     =0;
  DcUser     =0;
  DcRun      =0;
  DcExp      =0;

  char line[128];
  FILE *conf;
  char *pc;
  float value;
  ps_daqst=(s_daqst *)daqst;
  strcpy(cLocalNode,node);
  sprintf(cPrefix,"MBS/%s/Logger/",cLocalNode);
  if(ps_daqst == 0){
    DeNodeList=MD_add_info("NodeList",&eNodeList,1,"White",list);
    DeNodeTime=MD_add_info("NodeTime",&eNodeTime,1,"White","");
    return;
  }
  conf=fopen("dimsetup.txt","r");
  if(conf!=0){
    bEventRate=0;
    bDataRateKb=0;
    bTriggerRate=0;
    while(fgets(line,127,conf)!=NULL){
      if(line[0]!='#'){
        if(strstr(line,cLocalNode)||(strchr(line,'*')!=NULL)){
          pc=strchr(line,':');
          value=10000.;
          if(pc!=NULL)sscanf(pc+1,"%f",&value);
          if(strstr(line,"SpillOn"))bSpillOn=1;
          else if(strstr(line,"TrigCountHis"))  bTrigCountHis=1;
          else if(strstr(line,"TrigRateHis"))   bTrigRateHis=1;
          else if(strstr(line,"TriggerMode"))   bTriggerMode=1;
          else if(strstr(line,"FileOpen"))      bFileOpen=1;
          else if(strstr(line,"BuildingMode"))  bBuildingMode=1;
          else if(strstr(line,"EventBuilding")) bEventBuilding=1;
          else if(strstr(line,"StreamsFull"))  {bStreamsFull=1;rMaxStreamsFull=0.0;}
          else if(strstr(line,"FileFilled"))    bFileFilled=1;
          else if(strstr(line,"EventRate"))    {bEventRate=1;rMaxEventRate=value;}
          else if(strstr(line,"EventTrend"))   {bEventTrend=1;rMaxEventTrend=value;}
          else if(strstr(line,"EvSizeRateB"))  {bEvSizeRateB=1;rMaxEvSizeRateB=value;}
          else if(strstr(line,"EvSizeTrendB")) {bEvSizeTrendB=1;rMaxEvSizeTrendB=value;}
          else if(strstr(line,"DataRateKb"))   {bDataRateKb=1;rMaxDataRateKb=value;}
          else if(strstr(line,"DataTrendKb"))  {bDataTrendKb=1;rMaxDataTrendKb=value;}
          else if(strstr(line,"StreamRateKb")) {bStreamRateKb=1;rMaxStreamRateKb=value;}
          else if(strstr(line,"StreamTrendKb")){bStreamTrendKb=1;rMaxStreamTrendKb=value;}
          else if(strstr(line,"UserVal_00"))bUserVal_00=1;
          else if(strstr(line,"UserVal_01"))bUserVal_01=1;
          else if(strstr(line,"UserVal_02"))bUserVal_02=1;
          else if(strstr(line,"UserVal_03"))bUserVal_03=1;
          else if(strstr(line,"UserVal_04"))bUserVal_04=1;
          else if(strstr(line,"UserVal_05"))bUserVal_05=1;
          else if(strstr(line,"UserVal_06"))bUserVal_06=1;
          else if(strstr(line,"UserVal_07"))bUserVal_07=1;
          else if(strstr(line,"UserVal_08"))bUserVal_08=1;
          else if(strstr(line,"UserVal_09"))bUserVal_09=1;
          else if(strstr(line,"UserVal_10"))bUserVal_10=1;
          else if(strstr(line,"UserVal_11"))bUserVal_11=1;
          else if(strstr(line,"UserVal_12"))bUserVal_12=1;
          else if(strstr(line,"UserVal_13"))bUserVal_13=1;
          else if(strstr(line,"UserVal_14"))bUserVal_14=1;
          else if(strstr(line,"UserVal_15"))bUserVal_15=1;

          else if(strstr(line,"TriggerRate"))  bTriggerRate=1;
          else if(strstr(line,"Trigger01Rate"))bTrigger01Rate=1;
          else if(strstr(line,"Trigger02Rate"))bTrigger02Rate=1;
          else if(strstr(line,"Trigger03Rate"))bTrigger03Rate=1;
          else if(strstr(line,"Trigger04Rate"))bTrigger04Rate=1;
          else if(strstr(line,"Trigger05Rate"))bTrigger05Rate=1;
          else if(strstr(line,"Trigger06Rate"))bTrigger06Rate=1;
          else if(strstr(line,"Trigger07Rate"))bTrigger07Rate=1;
          else if(strstr(line,"Trigger08Rate"))bTrigger08Rate=1;
          else if(strstr(line,"Trigger09Rate"))bTrigger09Rate=1;
          else if(strstr(line,"Trigger10Rate"))bTrigger10Rate=1;
          else if(strstr(line,"Trigger11Rate"))bTrigger11Rate=1;
          else if(strstr(line,"Trigger12Rate"))bTrigger12Rate=1;
          else if(strstr(line,"Trigger13Rate"))bTrigger13Rate=1;
          else if(strstr(line,"Trigger14Rate"))bTrigger14Rate=1;
          else if(strstr(line,"Trigger15Rate"))bTrigger15Rate=1;
          else if(strstr(line,"TriggerCount")) bTriggerCount=1;
          else if(strstr(line,"Trigger01Count"))bTrigger01Count=1;
          else if(strstr(line,"Trigger02Count"))bTrigger02Count=1;
          else if(strstr(line,"Trigger03Count"))bTrigger03Count=1;
          else if(strstr(line,"Trigger04Count"))bTrigger04Count=1;
          else if(strstr(line,"Trigger05Count"))bTrigger05Count=1;
          else if(strstr(line,"Trigger06Count"))bTrigger06Count=1;
          else if(strstr(line,"Trigger07Count"))bTrigger07Count=1;
          else if(strstr(line,"Trigger08Count"))bTrigger08Count=1;
          else if(strstr(line,"Trigger09Count"))bTrigger09Count=1;
          else if(strstr(line,"Trigger10Count"))bTrigger10Count=1;
          else if(strstr(line,"Trigger11Count"))bTrigger11Count=1;
          else if(strstr(line,"Trigger12Count"))bTrigger12Count=1;
          else if(strstr(line,"Trigger13Count"))bTrigger13Count=1;
          else if(strstr(line,"Trigger14Count"))bTrigger14Count=1;
          else if(strstr(line,"Trigger15Count"))bTrigger15Count=1;
        }
      }
    }
    fclose(conf);
  }
  fileOn=bFileOpen+bFileFilled;

  DcUser      =MD_add_value("User","C",sizeof(ps_daqst->c_user),(unsigned int *)&ps_daqst->c_user);
  DcDate      =MD_add_value("Date","C",sizeof(ps_daqst->c_date),(unsigned int *)&ps_daqst->c_date);
  DcRun       =MD_add_value("Run","C",sizeof(ps_daqst->c_exprun),(unsigned int *)&ps_daqst->c_exprun);
  DcExp       =MD_add_value("Experiment","C",sizeof(ps_daqst->c_exper),(unsigned int *)&ps_daqst->c_exper);

  DiEvents      =MD_add_value("Events","I",sizeof(int),&ps_daqst->bl_n_events);
  DiBuffers     =MD_add_value("Buffers","I",sizeof(int),&ps_daqst->bl_n_buffers);
  DiMBytes      =MD_add_value("MBytes","I",sizeof(int),(unsigned int *)&iMBytes);
  DiBufferSize  =MD_add_value("BufferSize","I",sizeof(int),&ps_daqst->l_record_size);
  DiStreamMbytes=MD_add_value("StreamMbytes","I",sizeof(int),(unsigned int *)&iStreamMbytes);
  DiFlushTime   =MD_add_value("FlushTime","I",sizeof(int),&ps_daqst->bl_flush_time);
  DiStreamScale =MD_add_value("StreamScale","I",sizeof(int),&ps_daqst->bl_strsrv_scale);
  DiStreamSync  =MD_add_value("StreamSync","I",sizeof(int),&ps_daqst->bl_strsrv_sync);
  DiStreamKeep  =MD_add_value("StreamKeep","I",sizeof(int),&ps_daqst->bl_strsrv_keep);

  if(bUserVal_00) DiUserVal_00 =MD_add_value("UserVal_00","I",sizeof(int),&ps_daqst->bl_user[0]);
  if(bUserVal_01) DiUserVal_01 =MD_add_value("UserVal_01","I",sizeof(int),&ps_daqst->bl_user[1]);
  if(bUserVal_02) DiUserVal_02 =MD_add_value("UserVal_02","I",sizeof(int),&ps_daqst->bl_user[2]);
  if(bUserVal_03) DiUserVal_03 =MD_add_value("UserVal_03","I",sizeof(int),&ps_daqst->bl_user[3]);
  if(bUserVal_04) DiUserVal_04 =MD_add_value("UserVal_04","I",sizeof(int),&ps_daqst->bl_user[4]);
  if(bUserVal_05) DiUserVal_05 =MD_add_value("UserVal_05","I",sizeof(int),&ps_daqst->bl_user[5]);
  if(bUserVal_06) DiUserVal_06 =MD_add_value("UserVal_06","I",sizeof(int),&ps_daqst->bl_user[6]);
  if(bUserVal_07) DiUserVal_07 =MD_add_value("UserVal_07","I",sizeof(int),&ps_daqst->bl_user[7]);
  if(bUserVal_08) DiUserVal_08 =MD_add_value("UserVal_08","I",sizeof(int),&ps_daqst->bl_user[8]);
  if(bUserVal_09) DiUserVal_09 =MD_add_value("UserVal_09","I",sizeof(int),&ps_daqst->bl_user[9]);
  if(bUserVal_10) DiUserVal_10 =MD_add_value("UserVal_10","I",sizeof(int),&ps_daqst->bl_user[10]);
  if(bUserVal_11) DiUserVal_11 =MD_add_value("UserVal_11","I",sizeof(int),&ps_daqst->bl_user[11]);
  if(bUserVal_12) DiUserVal_12 =MD_add_value("UserVal_12","I",sizeof(int),&ps_daqst->bl_user[12]);
  if(bUserVal_13) DiUserVal_13 =MD_add_value("UserVal_13","I",sizeof(int),&ps_daqst->bl_user[13]);
  if(bUserVal_14) DiUserVal_14 =MD_add_value("UserVal_14","I",sizeof(int),&ps_daqst->bl_user[14]);
  if(bUserVal_15) DiUserVal_15 =MD_add_value("UserVal_15","I",sizeof(int),&ps_daqst->bl_user[15]);

  if(bTriggerCount)   DiTriggerCount   =MD_add_value("TriggerCount","I",sizeof(int),&ps_daqst->bl_n_trig[0]);
  if(bTrigger01Count) DiTrigger01Count =MD_add_value("Trigger01Count","I",sizeof(int),&ps_daqst->bl_n_trig[1]);
  if(bTrigger02Count) DiTrigger02Count =MD_add_value("Trigger02Count","I",sizeof(int),&ps_daqst->bl_n_trig[2]);
  if(bTrigger03Count) DiTrigger03Count =MD_add_value("Trigger03Count","I",sizeof(int),&ps_daqst->bl_n_trig[3]);
  if(bTrigger04Count) DiTrigger04Count =MD_add_value("Trigger04Count","I",sizeof(int),&ps_daqst->bl_n_trig[4]);
  if(bTrigger05Count) DiTrigger05Count =MD_add_value("Trigger05Count","I",sizeof(int),&ps_daqst->bl_n_trig[5]);
  if(bTrigger06Count) DiTrigger06Count =MD_add_value("Trigger06Count","I",sizeof(int),&ps_daqst->bl_n_trig[6]);
  if(bTrigger07Count) DiTrigger07Count =MD_add_value("Trigger07Count","I",sizeof(int),&ps_daqst->bl_n_trig[7]);
  if(bTrigger08Count) DiTrigger08Count =MD_add_value("Trigger08Count","I",sizeof(int),&ps_daqst->bl_n_trig[8]);
  if(bTrigger09Count) DiTrigger09Count =MD_add_value("Trigger09Count","I",sizeof(int),&ps_daqst->bl_n_trig[9]);
  if(bTrigger10Count) DiTrigger10Count =MD_add_value("Trigger10Count","I",sizeof(int),&ps_daqst->bl_n_trig[10]);
  if(bTrigger11Count) DiTrigger11Count =MD_add_value("Trigger11Count","I",sizeof(int),&ps_daqst->bl_n_trig[11]);
  if(bTrigger12Count) DiTrigger12Count =MD_add_value("Trigger12Count","I",sizeof(int),&ps_daqst->bl_n_trig[12]);
  if(bTrigger13Count) DiTrigger13Count =MD_add_value("Trigger13Count","I",sizeof(int),&ps_daqst->bl_n_trig[13]);
  if(bTrigger14Count) DiTrigger14Count =MD_add_value("Trigger14Count","I",sizeof(int),&ps_daqst->bl_n_trig[14]);
  if(bTrigger15Count) DiTrigger15Count =MD_add_value("Trigger15Count","I",sizeof(int),&ps_daqst->bl_n_trig[15]);

  if(bTriggerRate) DrTriggerRate =
    MD_add_rate("TriggerRate",&rTriggerRate,"Blue","T/s",TREND,rMaxTriggerRate);
  if(bTrigger01Rate) DrTrigger01Rate =
    MD_add_rate("Trigger01Rate",&rTrigger01Rate,"Blue","T/s",TREND,rMaxTrigger01Rate);
  if(bTrigger02Rate) DrTrigger02Rate =
    MD_add_rate("Trigger02Rate",&rTrigger02Rate,"Blue","T/s",TREND,rMaxTrigger02Rate);
  if(bTrigger03Rate) DrTrigger03Rate =
    MD_add_rate("Trigger03Rate",&rTrigger03Rate,"Blue","T/s",TREND,rMaxTrigger03Rate);
  if(bTrigger04Rate) DrTrigger04Rate =
    MD_add_rate("Trigger04Rate",&rTrigger04Rate,"Blue","T/s",TREND,rMaxTrigger04Rate);
  if(bTrigger05Rate) DrTrigger05Rate =
    MD_add_rate("Trigger05Rate",&rTrigger05Rate,"Blue","T/s",TREND,rMaxTrigger05Rate);
  if(bTrigger06Rate) DrTrigger06Rate =
    MD_add_rate("Trigger06Rate",&rTrigger06Rate,"Blue","T/s",TREND,rMaxTrigger06Rate);
  if(bTrigger07Rate) DrTrigger07Rate =
    MD_add_rate("Trigger07Rate",&rTrigger07Rate,"Blue","T/s",TREND,rMaxTrigger07Rate);
  if(bTrigger08Rate) DrTrigger08Rate =
    MD_add_rate("Trigger08Rate",&rTrigger08Rate,"Blue","T/s",TREND,rMaxTrigger08Rate);
  if(bTrigger09Rate) DrTrigger09Rate =
    MD_add_rate("Trigger09Rate",&rTrigger09Rate,"Blue","T/s",TREND,rMaxTrigger09Rate);
  if(bTrigger10Rate) DrTrigger10Rate =
    MD_add_rate("Trigger10Rate",&rTrigger10Rate,"Blue","T/s",TREND,rMaxTrigger10Rate);
  if(bTrigger11Rate) DrTrigger11Rate =
    MD_add_rate("Trigger11Rate",&rTrigger11Rate,"Blue","T/s",TREND,rMaxTrigger11Rate);
  if(bTrigger12Rate) DrTrigger12Rate =
    MD_add_rate("Trigger12Rate",&rTrigger12Rate,"Blue","T/s",TREND,rMaxTrigger12Rate);
  if(bTrigger13Rate) DrTrigger13Rate =
    MD_add_rate("Trigger13Rate",&rTrigger13Rate,"Blue","T/s",TREND,rMaxTrigger13Rate);
  if(bTrigger14Rate) DrTrigger14Rate =
    MD_add_rate("Trigger14Rate",&rTrigger14Rate,"Blue","T/s",TREND,rMaxTrigger14Rate);
  if(bTrigger15Rate) DrTrigger15Rate =
    MD_add_rate("Trigger15Rate",&rTrigger15Rate,"Blue","T/s",TREND,rMaxTrigger15Rate);

  if(bFileFilled)    DrFileFilled=
    MD_add_rate("FileFilled",&rFileFilled,"Green","%",BAR,100.);
  if(bEventRate)     DrEventRate=
    MD_add_rate("EventRate",&rEventRate,"Red","Ev/s",ARC,rMaxEventRate);
  if(bEventTrend)    DrEventTrend=
    MD_add_rate("EventTrend",&rEventTrend,"Red","Ev/s",TREND,rMaxEventTrend);
  if(bEvSizeRateB)   DrEvSizeRateB=
    MD_add_rate("EvSizeRateB",&rEvSizeRateB,"Red","B/ev",BAR,rMaxEvSizeRateB);
  if(bEvSizeTrendB)  DrEvSizeTrendB=
    MD_add_rate("EvSizeTrendB",&rEvSizeTrendB,"Red","B/ev",TREND,rMaxEvSizeTrendB);
  if(bDataRateKb)    DrDataRateKb=
    MD_add_rate("DataRateKb",&rDataRateKb,"Red","KB/s",ARC,rMaxDataRateKb);
  if(bDataTrendKb)   DrDataTrendKb=
    MD_add_rate("DataTrendKb",&rDataTrendKb,"Red","KB/s",TREND,rMaxDataTrendKb);
  if(bStreamRateKb)  DrStreamRateKb=
    MD_add_rate("StreamRateKb",&rStreamRateKb,"Red","KB/s",ARC,rMaxStreamRateKb);
  if(bStreamTrendKb) DrStreamTrendKb=
    MD_add_rate("StreamTrendKb",&rStreamTrendKb,"Red","KB/s",TREND,rMaxStreamTrendKb);
  if(bStreamsFull)   DrStreamsFull=
    MD_add_rate("StreamsFull",&rStreamsFull,"Green","n",BAR,0.0);

  // severity of states must not be -1 because that is the NOLINK value.
  // Negative value (-2) means suppress severity

  DsRunning   =MD_add_state("Acquisition",&sRunning,-2,"Gray","Stopped");
  if(bRunMode) DsRunMode=MD_add_state("RunMode",&sRunMode,-2,"Gray","MBS standalone");
  if(bBuildingMode) DsBuildingMode=
    MD_add_state("BuildingMode",&sBuildingMode,-2,"Gray","Immediate");
  if(bEventBuilding) DsEventBuilding=
    MD_add_state("EventBuilding",&sEventBuilding,-2,"Gray","Working");
  if(bSpillOn) DsSpillOn=
    MD_add_state("SpillOn",&sSpillOn,-2,"Gray","Off");
  if(bTriggerMode) DsTriggerMode=
    MD_add_state("TriggerMode",&sTriggerMode,-2,"Gray","Slave");
  if(bFileOpen) DsFileOpen=
    MD_add_state("FileOpen",&sFileOpen,-2,"Gray","No file");

  DeTaskList=MD_add_info("TaskList",&eTaskList,1,"White","");
  if(fileOn) DeFile    =MD_add_info("FileName",&eFile,1,"Gray","No file");
  DeSetup   =MD_add_info("SetupFiles",&eSetup,1,"Gray","Setup not loaded");
  DePerform =MD_add_info("Perform",&ePerform,-2,"Gray","Events: 0, KBytes: 0, E/s: 0, MB/s: 0.");
  if(bTrigCountHis)DhTrigCountHis=
    MD_add_histo("TrigCountHis",&hTrigCountHis,chTrigCountHis,
        rMinTrigCountHis,rMaxTrigCountHis,"Red","Trigger#","Counts");
  if(bTrigRateHis)DhTrigRateHis=
    MD_add_histo("TrigRateHis",&hTrigRateHis,chTrigRateHis,
        rMinTrigRateHis,rMaxTrigRateHis,"Red","Trigger#","Counts");
}

//=================================================
int MbsNodeDimStatus::MD_start(char *name)
{
  sprintf(cServerName,"%s:%s",cLocalNode,name);
  dis_start_serving(cServerName);
  return 0;
}
//=================================================
void MbsNodeDimStatus::MD_update(char *times)
{
  int i, *pi;
  char *pc;
  char full[128];
  char name[32];
  char color[16];
  if(ps_daqst == 0){
    MD_set_info(&eNodeTime,1,"White",times);
    DeNodeTime->updateService();
    return;
  }
  rEvSizeRateB.value=0.;
  if(ps_daqst->bh_acqui_running){
    rEventRate.value=(REAL4)ps_daqst->bl_r_events;
    rDataRateKb.value=(REAL4)ps_daqst->bl_r_kbyte;
    if(rEventRate.value > 0.0) rEvSizeRateB.value=1024.*rDataRateKb.value/rEventRate.value;
    rStreamRateKb.value=(REAL4)ps_daqst->bl_r_strserv_kbytes;
  }else{
    for(i=0;i<16;i++) ps_daqst->bl_r_trig[i]=0;
    rEventRate.value=0.;
    ps_daqst->bl_r_events=0;
    rDataRateKb.value=0.;
    ps_daqst->bl_r_kbyte=0;
    rStreamRateKb.value=0.;
    ps_daqst->bl_r_strserv_kbytes=0;
  }
  rStreamsFull.value=(REAL4)(ps_daqst->bl_no_streams-ps_daqst->l_free_streams);
  if(DrStreamsFull)DrStreamsFull->updateService();
  if(DrEventRate)DrEventRate->updateService();
  if(DrEvSizeRateB)DrEvSizeRateB->updateService();
  if(DrDataRateKb)DrDataRateKb->updateService();
  if(ps_daqst->bh_running[SYS__stream_serv]&&DrStreamRateKb)DrStreamRateKb->updateService();
  rEventTrend.value=rEventRate.value;
  rEvSizeTrendB.value=rEvSizeRateB.value;
  rDataTrendKb.value=rDataRateKb.value;
  rStreamTrendKb.value=rStreamRateKb.value;
  if(DrEventTrend)DrEventTrend->updateService();
  if(DrEvSizeTrendB)DrEvSizeTrendB->updateService();
  if(DrDataTrendKb)DrDataTrendKb->updateService();
  if(ps_daqst->bh_running[SYS__stream_serv]&&DrStreamTrendKb)DrStreamTrendKb->updateService();

  // if transport is not there, daq cannot be running
  if((ps_daqst->bh_running[SYS__transport]==0)&&(ps_daqst->bh_running[SYS__ds]==0)){
    ps_daqst->bh_acqui_running=0;
    ps_daqst->bl_trans_connected=0;
  }
  if(ps_daqst->bl_dabc_enabled == 1){
    if(ps_daqst->bl_trans_connected == 1)MD_set_state(&sRunMode,-2, "Green","DABC connected");
    else                                 MD_set_state(&sRunMode,-2, "Cyan","MBS to DABC");
  }else{
    if(ps_daqst->bl_trans_connected == 1)MD_set_state(&sRunMode,-2, "Green","Transport client");
    else                                 MD_set_state(&sRunMode,-2, "Gray", "MBS standalone");
  }

  // severity of states must not be -1 because that is the NOLINK value.
  // Negative value (-2) means suppress severity
  if(ps_daqst->bh_acqui_running)
    MD_set_state(&sRunning,-2, "Green","Running");
  else MD_set_state(&sRunning,-2, "Red",  "Stopped");

  if(ps_daqst->bl_delayed_eb_ena)
    MD_set_state(&sBuildingMode,-2, "Blue","Delayed");
  else MD_set_state(&sBuildingMode,-2, "Green",  "Immediate");

  if(ps_daqst->bl_event_build_on)
    MD_set_state(&sEventBuilding,-2, "Green","Working");
  else MD_set_state(&sEventBuilding,-2, "Blue",  "Suspended");

  if(ps_daqst->bl_spill_on > 0)
    MD_set_state(&sSpillOn,-2, "Green","Spill ON");
  else MD_set_state(&sSpillOn,-2, "Red",  "Spill OFF");

  if(ps_daqst->bh_trig_master > 0)
    MD_set_state(&sTriggerMode,-2, "Green","Master");
  else MD_set_state(&sTriggerMode,-2, "Gray",  "Slave");

  strcpy(full,"Loaded setup: ");
  if(strlen(ps_daqst->c_setup_name)){
    strcat(full,ps_daqst->c_setup_name);
    MD_set_info(&eSetup,1, "Green",full);
  }
  if(strlen(ps_daqst->c_ml_setup_name)) {
    strcat(full,", multi: ");
    strcat(full,ps_daqst->c_ml_setup_name);
    MD_set_info(&eSetup,1, "Green",full);
  }
  DeSetup->updateService();
  memset(full,0,128);
  strcpy(color,"White");
  for(i=0;i<(int)ps_daqst->l_procs_run;i++){
    if(ps_daqst->l_pid[i]>0){
      pc=strstr(ps_daqst->c_pname[i],"m_");
      pc += 2;
      strcpy(name,pc);
      name[0] = name[0]-32; // First upper case
      strcat(full,name);
      strcat(full," ");
    } else strcpy(color,"Red");}
  MD_set_info(&eTaskList,1,color,full);

  sprintf(full,"Events: %10d, MBytes: %6d, E/s: %6d, MB/s: %6.2f",
      ps_daqst->bl_n_events,ps_daqst->bl_n_kbyte/1000,
      ps_daqst->bl_r_events,(float)ps_daqst->bl_r_kbyte/1000.);
  MD_set_info(&ePerform,1,"Yellow",full);

  if(fileOn){
    if(strlen(ps_daqst->c_file_name)!=0){
      rFileFilled.value=ps_daqst->bl_n_kbyte_file/10/ps_daqst->l_file_size;
      sprintf(full,"%s size: %d MB, filled: %d MB %4.0f%%",
          ps_daqst->c_file_name,
          ps_daqst->l_file_size,
          ps_daqst->bl_n_kbyte_file/1000,
          rFileFilled.value);
      if(ps_daqst->l_open_file){
        strcat(full," open");
        MD_set_info(&eFile,1,"Green",full);
        MD_set_state(&sFileOpen,0, "Green","File open");
      }
      else {
        strcat(full," closed");
        MD_set_info(&eFile,1,"Red",full);
        MD_set_state(&sFileOpen,1, "Red","File closed");
      }
    }}

  if(DeFile) DeFile->updateService();
  if(DrFileFilled)DrFileFilled->updateService();
  if(DsFileOpen)DsFileOpen->updateService();
  if(DsSpillOn)DsSpillOn->updateService();
  if(DsRunMode)DsRunMode->updateService();
  if(DsBuildingMode)DsBuildingMode->updateService();
  if(DsEventBuilding)DsEventBuilding->updateService();
  if(DsTriggerMode) DsTriggerMode->updateService();
  DsRunning->updateService();
  DePerform->updateService();
  DeTaskList->updateService();

  iMBytes=ps_daqst->bl_n_kbyte/1000;
  iStreamMbytes=ps_daqst->bl_n_strserv_kbytes/1000;
  iFileMbytes=ps_daqst->bl_n_kbyte_file/1000;
  DiEvents->updateService();
  DiBuffers->updateService();
  DiMBytes->updateService();
  DiBufferSize->updateService();
  DiStreamMbytes->updateService();
  if(DiFileMbytes)DiFileMbytes->updateService();
  DiFlushTime->updateService();
  DiStreamScale->updateService();
  DiStreamSync->updateService();
  DiStreamKeep->updateService();

  DcDate->updateService();
  DcUser->updateService();
  DcRun->updateService();
  DcExp->updateService();
  if(bUserVal_00) DiUserVal_00->updateService();
  if(bUserVal_01) DiUserVal_01->updateService();
  if(bUserVal_02) DiUserVal_02->updateService();
  if(bUserVal_03) DiUserVal_03->updateService();
  if(bUserVal_04) DiUserVal_04->updateService();
  if(bUserVal_05) DiUserVal_05->updateService();
  if(bUserVal_06) DiUserVal_06->updateService();
  if(bUserVal_07) DiUserVal_07->updateService();
  if(bUserVal_08) DiUserVal_08->updateService();
  if(bUserVal_09) DiUserVal_09->updateService();
  if(bUserVal_10) DiUserVal_10->updateService();
  if(bUserVal_11) DiUserVal_11->updateService();
  if(bUserVal_12) DiUserVal_12->updateService();
  if(bUserVal_13) DiUserVal_13->updateService();
  if(bUserVal_14) DiUserVal_14->updateService();
  if(bUserVal_15) DiUserVal_15->updateService();

  if(bTriggerCount)   DiTriggerCount->updateService();
  if(bTrigger01Count) DiTrigger01Count->updateService();
  if(bTrigger02Count) DiTrigger02Count->updateService();
  if(bTrigger03Count) DiTrigger03Count->updateService();
  if(bTrigger04Count) DiTrigger04Count->updateService();
  if(bTrigger05Count) DiTrigger05Count->updateService();
  if(bTrigger06Count) DiTrigger06Count->updateService();
  if(bTrigger07Count) DiTrigger07Count->updateService();
  if(bTrigger08Count) DiTrigger08Count->updateService();
  if(bTrigger09Count) DiTrigger09Count->updateService();
  if(bTrigger10Count) DiTrigger10Count->updateService();
  if(bTrigger11Count) DiTrigger11Count->updateService();
  if(bTrigger12Count) DiTrigger12Count->updateService();
  if(bTrigger13Count) DiTrigger13Count->updateService();
  if(bTrigger14Count) DiTrigger14Count->updateService();
  if(bTrigger15Count) DiTrigger15Count->updateService();
  if(bTrigCountHis){
    pi=(int *)&(hTrigCountHis->data);
    for (i=0; i<16;i++) *pi++ = ps_daqst->bl_n_trig[i];
    DhTrigCountHis->updateService();
  }
  if(bTrigRateHis){
    pi=(int *)&(hTrigRateHis->data);
    for (i=0; i<16;i++) *pi++ = ps_daqst->bl_r_trig[i];
    DhTrigRateHis->updateService();
  }

  if(bTriggerRate)  {rTriggerRate.value=(REAL4)ps_daqst->bl_r_trig[0];    DrTriggerRate->updateService();}
  if(bTrigger01Rate){rTrigger01Rate.value=(REAL4)ps_daqst->bl_r_trig[1];  DrTrigger01Rate->updateService();}
  if(bTrigger02Rate){rTrigger02Rate.value=(REAL4)ps_daqst->bl_r_trig[2];  DrTrigger02Rate->updateService();}
  if(bTrigger03Rate){rTrigger03Rate.value=(REAL4)ps_daqst->bl_r_trig[3];  DrTrigger03Rate->updateService();}
  if(bTrigger04Rate){rTrigger04Rate.value=(REAL4)ps_daqst->bl_r_trig[4];  DrTrigger04Rate->updateService();}
  if(bTrigger05Rate){rTrigger05Rate.value=(REAL4)ps_daqst->bl_r_trig[5];  DrTrigger05Rate->updateService();}
  if(bTrigger06Rate){rTrigger06Rate.value=(REAL4)ps_daqst->bl_r_trig[6];  DrTrigger06Rate->updateService();}
  if(bTrigger07Rate){rTrigger07Rate.value=(REAL4)ps_daqst->bl_r_trig[7];  DrTrigger07Rate->updateService();}
  if(bTrigger08Rate){rTrigger08Rate.value=(REAL4)ps_daqst->bl_r_trig[8];  DrTrigger08Rate->updateService();}
  if(bTrigger09Rate){rTrigger09Rate.value=(REAL4)ps_daqst->bl_r_trig[9];  DrTrigger09Rate->updateService();}
  if(bTrigger10Rate){rTrigger10Rate.value=(REAL4)ps_daqst->bl_r_trig[10]; DrTrigger10Rate->updateService();}
  if(bTrigger11Rate){rTrigger11Rate.value=(REAL4)ps_daqst->bl_r_trig[11]; DrTrigger11Rate->updateService();}
  if(bTrigger12Rate){rTrigger12Rate.value=(REAL4)ps_daqst->bl_r_trig[12]; DrTrigger12Rate->updateService();}
  if(bTrigger13Rate){rTrigger13Rate.value=(REAL4)ps_daqst->bl_r_trig[13]; DrTrigger13Rate->updateService();}
  if(bTrigger14Rate){rTrigger14Rate.value=(REAL4)ps_daqst->bl_r_trig[14]; DrTrigger14Rate->updateService();}
  if(bTrigger15Rate){rTrigger15Rate.value=(REAL4)ps_daqst->bl_r_trig[15]; DrTrigger15Rate->updateService();}
}
//=================================================
DimService * MbsNodeDimStatus::MD_add_rate(char *name, dabcRate *rate, char *color, char *text, int mode, float ul)
{
  DimService * service=0;
  char full[128];
  memset(rate,0,sizeof(dabcRate));
  strcpy(rate->color,color);
  strcpy(rate->units,text);
  rate->displaymode=mode;
  rate->upper=ul;
  sprintf(full,"%s%s",cPrefix,name);
  service=new DimService(full,RATEDESC,rate,sizeof(dabcRate));
  service->setQuality(buildQuality(UPTODATE,RATE,MONITOR,NOMODE));
  return service;
}
//=================================================
DimService * MbsNodeDimStatus::MD_add_histo(char *name, dabcHistogram **histo, int chan, float min, float max, char *color, char *lett, char *cont)
{
  DimService * service=0;
  char full[128];
  int size;
  dabcHistogram *his;
  size=sizeof(dabcHistogram)+(chan-1)*4;
  his=(dabcHistogram *)malloc(size);
  memset(his,0,size);
  strcpy(his->color,color);
  strcpy(his->xlett,lett);
  strcpy(his->cont,cont);
  his->xlow=min;
  his->xhigh=max;
  his->channels=chan;
  sprintf(full,"%s%s",cPrefix,name);
  service=new DimService(full,HISTOGRAMDESC,his,size);
  service->setQuality(buildQuality(UPTODATE,HISTOGRAM,MONITOR,NOMODE));
  *histo=his;
  return service;
}
//=================================================
DimService * MbsNodeDimStatus::MD_add_state(char *name, dabcState *state, int value, char *color, char *text)
{
  DimService * service=0;
  char full[128];
  sprintf(cPrefix,"MBS/%s/Logger/",cLocalNode);
  memset(state,0,sizeof(dabcState));
  sprintf(full,"MBS/%s/%s/State",cLocalNode,name);
  MD_set_state(state,value,color,text);
  service=new DimService(full,STATEDESC,state,sizeof(dabcState));
  service->setQuality(buildQuality(UPTODATE,STATE,MONITOR,NOMODE));
  return service;
}
//=================================================
DimService * MbsNodeDimStatus::MD_add_info(char *name, dabcInfo *info, int value, char *color, char *text)
{
  DimService * service=0;
  char full[128];
  memset(info,0,sizeof(dabcInfo));
  sprintf(full,"%s%s",cPrefix,name);
  MD_set_info(info,value,color,text);
  service=new DimService(full,INFODESC,info,sizeof(dabcInfo));
  service->setQuality(buildQuality(UPTODATE,INFO,MONITOR,NOMODE));
  return service;
}
//=================================================
DimService * MbsNodeDimStatus::MD_add_value(char *name, char *type, int size, unsigned int *value)
{
  DimService * service=0;
  char full[128];
  sprintf(full,"%s%s",cPrefix,name);
  service=new DimService(full,type,value,size);
  service->setQuality(buildQuality(UPTODATE,ATOMIC,VISIBLE,NOMODE));
  return service;
}
//=================================================
void MbsNodeDimStatus::MD_set_state(dabcState *state, int value, char *color, char *text)
{
  state->severity=value;
  memset(state->color,0,16);
  strcpy(state->color,color);
  memset(state->status,0,16);
  strncpy(state->status,text,15);
}
//=================================================
void MbsNodeDimStatus::MD_set_info(dabcInfo *info, int value, char *color, char *text)
  {
    info->mode=value;
    memset(info->color,0,16);
    strcpy(info->color,color);
    memset(info->text,0,128);
    strncpy(info->text,text,127);
  }

