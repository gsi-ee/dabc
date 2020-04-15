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

#include "mbs/Monitor.h"

//#include <cstdlib>
//#include <cstring>
//#include <cstdio>
#include <cmath>

//#include "dabc/SocketThread.h"
//#include "dabc/Pointer.h"
#include "dabc/Publisher.h"

#define SYS__read_meb         0
#define SYS__collector        1
#define SYS__transport        2
#define SYS__event_serv       3
#define SYS__msg_log          4
#define SYS__dispatch         5
#define SYS__util             6
#define SYS__sbs_mon          7
#define SYS__read_cam_slav    8
#define SYS__esone_serv       9
#define SYS__stream_serv     10
#define SYS__histogram       11
#define SYS__prompt          12
#define SYS__daq_rate        13
#define SYS__smi             14
#define SYS__ds              15
#define SYS__dr              16
#define SYS__ar              17
#define SYS__rirec           18
#define SYS__to              19
#define SYS__vme_serv        20

#define VERSION__SETUP  1
#define VERSION__SET_ML 1
#define VERSION__SET_MO 1


mbs::DaqStatusAddon::DaqStatusAddon(int fd) :
   dabc::SocketIOAddon(fd),
   fState(ioInit),
   fStatus(),
   fSwapping(false),
   fSendCmd(0)
{
   SetDeleteWorkerOnClose(false);
}


void mbs::DaqStatusAddon::OnThreadAssigned()
{
   // DOUT0("mbs::DaqStatusAddon::OnThreadAssigned socket %d", fSocket);

   dabc::SocketIOAddon::OnThreadAssigned();

   if (fState == ioDone) fState = ioInit;

   if (fState != ioInit) {
      EOUT("Get thread assigned in not-init state - check why");
      exit(234);
   }

   fState = ioRecvHeader;

   memset(&fStatus, 0, sizeof(mbs::DaqStatus));
   StartRecv(&fStatus, 28);

   fSendCmd = 1;
   StartSend(&fSendCmd, sizeof(fSendCmd));

   // check that status data are received in reasonable time
   ActivateTimeout(5.);
}

double mbs::DaqStatusAddon::ProcessTimeout(double last_diff)
{
   EOUT("Status timeout - delete addon");
   DeleteAddonItself();
   EOUT("Status timeout - delete addon called");
   return -1;
}

void mbs::DaqStatusAddon::OnRecvCompleted()
{
   if (fState == ioRecvHeader) {

      if(fStatus.l_endian != 1) fSwapping = true;
      if(fSwapping) mbs::SwapData(&fStatus, 28);

      fState = ioRecvData;

      if ((fStatus.l_version != 51) && (fStatus.l_version != 62) && (fStatus.l_version != 63))  {
         EOUT("Unsupported status version %u", (unsigned) fStatus.l_version);
         DeleteAddonItself();
         return;
      }

      StartRecv(&fStatus.bh_daqst_initalized, (fStatus.l_fix_lw-7)*4);
      return;
   }

   if (fState == ioRecvData) {

      if (fSwapping)
         mbs::SwapData(&fStatus.bh_daqst_initalized, (fStatus.l_fix_lw-7)*4 - (19 * fStatus.l_sbs__str_len_64));

      fState = ioDone;

      SubmitWorkerCmd(dabc::Command("ProcessDaqStatus"));
      return;
   }

   EOUT("Wrong state when recv data");
   DeleteAddonItself();
}

// =========================================================================

mbs::Monitor::Monitor(const std::string &name, dabc::Command cmd) :
   mbs::MonitorSlowControl(name, "Mbs", cmd),
   fHierarchy(),
   fCounter(0),
   fMbsNode(),
   fPeriod(1.),
   fRateInterval(1.),
   fHistory(100),
   fStatPort(0),
   fLoggerPort(0),
   fCmdPort(0),
   fStatus(),
   fStatStamp(),
   fPrintf(false)
{
   fMbsNode = Cfg("node", cmd).AsStr();
   fPeriod = Cfg("period", cmd).AsDouble(1.);
   fRateInterval = Cfg("rateinterval", cmd).AsDouble(1.);
   fHistory = Cfg("history", cmd).AsInt(200);
   bool publish = Cfg("publish", cmd).AsBool(true);
   fPrintf = Cfg("printf", cmd).AsBool(false);

   fFileStateName = "MbsFileOpen";
   fAcqStateName = "MbsAcqRunning";
   fSetupStateName = "MbsSetupLoaded";
   fRateIntervalName = "MbsRateInterval";
   fHistoryName = "MbsHistoryDepth";

   if (Cfg("stat", cmd).AsStr() == "true")
      fStatPort = 6008;
   else
      fStatPort = Cfg("stat", cmd).AsInt(0);

   if (Cfg("logger", cmd).AsStr() == "true")
      fLoggerPort = 6007;
   else
      fLoggerPort = Cfg("logger", cmd).AsInt(0);

   if (Cfg("cmd", cmd).AsStr() == "true")
      fCmdPort = 6019;
   else
      fCmdPort = Cfg("cmd", cmd).AsInt(0);

   fHierarchy.Create("MBS");

   // this is just emulation, later one need list of real variables
   dabc::Hierarchy item = fHierarchy.CreateHChild("DataRate");
   item.SetField(dabc::prop_kind, "rate");
   if (fHistory>1) item.EnableHistory(fHistory);

   item = fHierarchy.CreateHChild("EventRate");
   item.SetField(dabc::prop_kind, "rate");
   if (fHistory>1) item.EnableHistory(fHistory);

   item = fHierarchy.CreateHChild("ServerRate");
   item.SetField(dabc::prop_kind, "rate");
   if (fHistory>1) item.EnableHistory(fHistory);

   item = fHierarchy.CreateHChild("FileRate");
   item.SetField(dabc::prop_kind, "rate");
   if (fHistory>1) item.EnableHistory(fHistory);

   item = fHierarchy.CreateHChild("logger");
   item.SetField(dabc::prop_kind, "log");
   if (fHistory>1) item.EnableHistory(fHistory);

   item = fHierarchy.CreateHChild("rate_log");
   item.SetField(dabc::prop_kind, "log");
   if (fHistory>1) item.EnableHistory(fHistory);

   item = fHierarchy.CreateHChild("rash_log");
   item.SetField(dabc::prop_kind, "log");
   if (fHistory>1) item.EnableHistory(fHistory);

   item = fHierarchy.CreateHChild("rast_log");
   item.SetField(dabc::prop_kind, "log");
   if (fHistory>1) item.EnableHistory(fHistory);

   item = fHierarchy.CreateHChild("ratf_log");
   item.SetField(dabc::prop_kind, "log");
   if (fHistory>1) item.EnableHistory(fHistory);

   item = fHierarchy.CreateHChild(fFileStateName);
   item.SetField(dabc::prop_kind, "log");
   if (fHistory>1) item.EnableHistory(fHistory);

   item = fHierarchy.CreateHChild(fAcqStateName);
   item.SetField(dabc::prop_kind, "log");
   if (fHistory>1) item.EnableHistory(fHistory);

   item = fHierarchy.CreateHChild(fSetupStateName);
   item.SetField(dabc::prop_kind, "log");
   if (fHistory>1) item.EnableHistory(fHistory);

   item = fHierarchy.CreateHChild(fRateIntervalName);
   item.SetField(dabc::prop_kind, "log");
   if (fHistory>1) item.EnableHistory(fHistory);

   item = fHierarchy.CreateHChild(fHistoryName);
    item.SetField(dabc::prop_kind, "log");
    if (fHistory>1) item.EnableHistory(fHistory);

   SetRateInterval(fRateInterval); // update exported parameters here
   SetHistoryDepth(fHistory);

   if (fCmdPort > 0) {
      dabc::CommandDefinition cmddef = fHierarchy.CreateHChild("CmdMbs");
      cmddef.SetField(dabc::prop_kind, "DABC.Command");
      cmddef.SetField(dabc::prop_auth, true); // require authentication
      cmddef.AddArg("cmd", "string", true, "show rate");

      dabc::CommandDefinition cmddef_rate = fHierarchy.CreateHChild("CmdSetRateInterval");
      cmddef_rate.SetField(dabc::prop_kind, "DABC.Command");
      cmddef_rate.SetField(dabc::prop_auth, true); // require authentication
      cmddef_rate.AddArg("time", "double", true, "1.0");

      dabc::CommandDefinition cmddef_hist = fHierarchy.CreateHChild("CmdSetHistoryDepth");
      cmddef_hist.SetField(dabc::prop_kind, "DABC.Command");
      cmddef_hist.SetField(dabc::prop_auth, true); // require authentication
      cmddef_hist.AddArg("entries", "int", true, "100");
   }

   dabc::Hierarchy ui = fHierarchy.CreateHChild("ControlGUI");
   ui.SetField(dabc::prop_kind, "DABC.HTML");
   ui.SetField("_UserFilePath", "${DABCSYS}/plugins/mbs/htm/");
   ui.SetField("_UserFileMain", "main.htm");

   CreateTimer("update", 5.);
   CreateTimer("MbsUpdate", fPeriod);

   fCounter = 0;

   memset(&fStatus, 0, sizeof(mbs::DaqStatus));

   // from this point on Publisher want to get regular update for the hierarchy
   if (publish)
      Publish(fHierarchy, std::string("/MBS/") + fMbsNode);

   if (fLoggerPort <= 0) NewMessage("!!! logger not activated !!!");
}


mbs::Monitor::~Monitor()
{
}

void mbs::Monitor::OnThreadAssigned()
{
   if (fLoggerPort > 0) {
      DaqLogWorker* logger = new DaqLogWorker(this, "DaqLogger", fMbsNode, fLoggerPort);
      logger->AssignToThread(thread());
   }

   dabc::Worker* remcmd = 0;
   if (IsPrompter()) {
      remcmd = new PrompterWorker(this, "DaqCmd", fMbsNode, fCmdPort);
   } else
   if (fCmdPort>0) {
      remcmd = new DaqRemCmdWorker(this, "DaqCmd", fMbsNode, fCmdPort);
   }
   if (remcmd) remcmd->AssignToThread(thread());

   mbs::MonitorSlowControl::OnThreadAssigned();
}

void mbs::Monitor::FillStatistic(const std::string &options, const std::string &itemname, mbs::DaqStatus* old_daqst, mbs::DaqStatus* new_daqst, double diff_time)
{
   int bStreams_n = 0, bBuffers_n = 0, bEvents_n = 0, bData_n = 0;
   int bStreams_r = 0, bBuffers_r = 0, bEvents_r = 0, bData_r = 0;
   int bSrvStreams_n = 0, bSrvEvents_n = 0, bSrvBuffers_n = 0, bSrvData_n = 0;
   int bSrvStreams_r = 0, bSrvEvents_r = 0, bSrvBuffers_r = 0, bSrvData_r = 0;
   int bFilename = 0, bFileFilled = 0, bFileData = 0, bFileData_r = 0, bFileIndex = 0;
   int bStreamsFree = 0, bStreamsFilled = 0, bStreamsSrv = 0;

   int bl_file = 0, bl_server = 0, bl_mbs = 0, bl_streams = 0;

   int l_free_stream(0), l_trans_stream(0), l_serv_stream(0);

   char c_head0[1000];
   char c_head[1000];
   char c_out[1000];

   char c_line[1000];

   strcpy(c_line, options.c_str());

   if (strstr (c_line, "-nst") != NULL )
     bStreams_n = 1;
   if (strstr (c_line, "-rst") != NULL )
     bStreams_r = 1;
   if (strstr (c_line, "-nsst") != NULL )
     bSrvStreams_n = 1;
   if (strstr (c_line, "-rsst") != NULL )
     bSrvStreams_r = 1;
   if (strstr (c_line, "-est") != NULL )
     bStreamsFree = 1;
   if (strstr (c_line, "-fst") != NULL )
     bStreamsFilled = 1;
   if (strstr (c_line, "-kst") != NULL )
     bStreamsSrv = 1;
   if (strstr (c_line, "-nbu") != NULL )
     bBuffers_n = 1;
   if (strstr (c_line, "-rbu") != NULL )
     bBuffers_r = 1;
   if (strstr (c_line, "-nsbu") != NULL )
     bSrvBuffers_n = 1;
   if (strstr (c_line, "-rsbu") != NULL )
     bSrvBuffers_r = 1;
   if (strstr (c_line, "-nev") != NULL )
     bEvents_n = 1;
   if (strstr (c_line, "-rev") != NULL )
     bEvents_r = 1;
   if (strstr (c_line, "-nsev") != NULL )
     bSrvEvents_n = 1;
   if (strstr (c_line, "-rsev") != NULL )
     bSrvEvents_r = 1;
   if (strstr (c_line, "-nda") != NULL )
     bData_n = 1;
   if (strstr (c_line, "-rda") != NULL )
     bData_r = 1;
   if (strstr (c_line, "-nsda") != NULL )
     bSrvData_n = 1;
   if (strstr (c_line, "-rsda") != NULL )
     bSrvData_r = 1;
   if (strstr (c_line, "-sfi") != NULL )
     bFileData = 1;
   if (strstr (c_line, "-ffi") != NULL )
     bFileFilled = 1;
   if (strstr (c_line, "-rfi") != NULL )
     bFileData_r = 1;
   if (strstr (c_line, "-nfi") != NULL )
     bFilename = 1;
   if (strstr (c_line, "-ifi") != NULL )
     bFileIndex = 1;
   if (strstr (c_line, "-mbs") != NULL )
   {
     // printf ("-mbs              # mbs part\n");
     bStreams_n = 1;
     bStreams_r = 1;
     bStreamsFree = 1;
     bStreamsFilled = 1;
     bStreamsSrv = 1;
     bBuffers_n = 1;
     bBuffers_r = 1;
     bEvents_n = 1;
     bEvents_r = 1;
     bData_n = 1;
     bData_r = 1;
   }
   if (strstr (c_line, "-u") != NULL )
   {
     // printf ("-user             # default selection\n");
     bEvents_n = 1;
     bEvents_r = 1;
     bData_n = 1;
     bData_r = 1;
     if (new_daqst->bh_running[SYS__event_serv])
       bSrvEvents_r = 1;
     if (new_daqst->bh_running[SYS__stream_serv])
       bSrvData_r = 1;
     bFileData_r = 1;
     bFileIndex = 1;
   }
   if (strstr (c_line, "-fi") != NULL )
   {
     //printf ("-fi[le]           # all file switches\n");
     bFilename = 1;
     bFileData = 1;
     bFileFilled = 1;
     bFileData_r = 1;
   }
   if (strstr (c_line, "-st") != NULL )
   {
     // printf ("-st[reams]        # stream usage\n");
     bStreamsFree = 1;
     bStreamsFilled = 1;
     bStreamsSrv = 1;
     bSrvStreams_r = 1;
   }
   if (strstr (c_line, "-se") != NULL )
   {
     // printf ("-se[rver]         # all server switches\n");
     if (new_daqst->bh_running[SYS__event_serv])
     {
       bSrvEvents_n = 1;
       bSrvEvents_r = 1;
     }
     if (new_daqst->bh_running[SYS__stream_serv])
     {
       bSrvBuffers_n = 1;
       bSrvBuffers_r = 1;
       bSrvData_n = 1;
       bSrvData_r = 1;
       bSrvStreams_r = 1;
       bSrvStreams_n = 1;
     }
   }
   if (strstr (c_line, "-ra") != NULL )
   {
     // printf ("-ra[tes]          # all rates\n");
     bStreams_r = 1;
     bBuffers_r = 1;
     bEvents_r = 1;
     bData_r = 1;
     bFileData_r = 1;
     if (new_daqst->bh_running[SYS__event_serv])
       bSrvEvents_r = 1;
     if (new_daqst->bh_running[SYS__stream_serv])
     {
       bSrvStreams_r = 1;
       bSrvBuffers_r = 1;
       bSrvData_r = 1;
     }
   }

   bl_mbs = bData_n + bEvents_n + bBuffers_n + bStreams_n + bData_r + bEvents_r + bBuffers_r + bStreams_r;
   bl_server = bSrvData_n + bSrvEvents_n + bSrvBuffers_n + bSrvStreams_n + bSrvData_r + bSrvEvents_r + bSrvBuffers_r
       + bSrvStreams_r;
   bl_streams = bStreamsFree + bStreamsFilled + bStreamsSrv;
   bl_file = bFileData + bFileFilled + bFileData_r + bFilename + bFileIndex;

   strcpy(c_head0,"");
   strcpy(c_head,"");
   if (bl_mbs)
   {
     strcat (c_head0, " Event building");
     if (bData_n)
     {
       strcat (c_head, "   MB      ");
       strcat (c_head0, "           ");
     }
     if (bEvents_n)
     {
       strcat (c_head, "   Events  ");
       strcat (c_head0, "           ");
     }
     if (bBuffers_n)
     {
       strcat (c_head, "   Buffers ");
       strcat (c_head0, "           ");
     }
     if (bStreams_n)
     {
       strcat (c_head, "Streams ");
       strcat (c_head0, "        ");
     }
     if (bData_r)
     {
       strcat (c_head, " Kb/sec ");
       strcat (c_head0, "        ");
     }
     if (bEvents_r)
     {
       strcat (c_head, " Ev/sec ");
       strcat (c_head0, "        ");
     }
     if (bBuffers_r)
     {
       strcat (c_head, "Buf/sec ");
       strcat (c_head0, "        ");
     }
     if (bStreams_r)
     {
       strcat (c_head, "Str/sec ");
       strcat (c_head0, "        ");
     }
     c_head0[strlen (c_head0) - 15] = 0;
   }
   bl_server = bSrvData_n + bSrvEvents_n + bSrvBuffers_n + bSrvStreams_n + bSrvData_r + bSrvEvents_r + bSrvBuffers_r
       + bSrvStreams_r;
   if (bl_server)
   {
     strcat (c_head0, "| Server ");
     strcat (c_head, "|");
     if (bSrvData_n)
     {
       strcat (c_head, "   MB      ");
       strcat (c_head0, "           ");
     }
     if (bSrvEvents_n)
     {
       strcat (c_head, "  Events   ");
       strcat (c_head0, "           ");
     }
     if (bSrvBuffers_n)
     {
       strcat (c_head, "  Buffers  ");
       strcat (c_head0, "           ");
     }
     if (bSrvStreams_n)
     {
       strcat (c_head, "Streams ");
       strcat (c_head0, "        ");
     }
     if (bSrvData_r)
     {
       strcat (c_head, " Kb/sec ");
       strcat (c_head0, "        ");
     }
     if (bSrvEvents_r)
     {
       strcat (c_head, " Ev/sec ");
       strcat (c_head0, "        ");
     }
     if (bSrvBuffers_r)
     {
       strcat (c_head, "Buf/sec ");
       strcat (c_head0, "        ");
     }
     if (bSrvStreams_r)
     {
       strcat (c_head, "Str/sec ");
       strcat (c_head0, "        ");
     }
     c_head0[strlen (c_head0) - 8] = 0;
   }
   bl_streams = bStreamsFree + bStreamsFilled + bStreamsSrv;
   if (bl_streams)
   {
     strcat (c_head0, "| Streams ");
     strcat (c_head, "|");
     if (bStreamsFree)
     {
       strcat (c_head, "Empty ");
       strcat (c_head0, "      ");
     }
     if (bStreamsFilled)
     {
       strcat (c_head, "Full ");
       strcat (c_head0, "     ");
     }
     if (bStreamsSrv)
     {
       strcat (c_head, "Hold ");
       strcat (c_head0, "     ");
     }
     c_head0[strlen (c_head0) - 9] = 0;
   }
   bl_file = bFileData + bFileFilled + bFileData_r + bFilename + bFileIndex;
   if (bl_file)
   {
     strcat (c_head0, "| File output ");
     if (bFilename)
       strcat (c_head0, "                ");
     strcat (c_head, "|");
     if (bFileData)
       strcat (c_head, "    MB   ");
     if (bFileFilled)
       strcat (c_head, "Filled  ");
     if (bFileData_r)
       strcat (c_head, "  Kb/sec ");
     if (bFileIndex)
       strcat (c_head, "Index ");
     if (bFilename)
       strcat (c_head, "Filename");
   }

   uint32_t bl_ev_buf_len = new_daqst->l_block_size;
   uint32_t bl_n_ev_buf = new_daqst->bl_no_stream_buf;
   //uint32_t bl_n_stream = new_daqst->bl_no_streams;


   double r_new_buf = new_daqst->bl_n_buffers;
   double r_new_evt = new_daqst->bl_n_events;
   double r_new_stream = new_daqst->bl_n_bufstream;
   double r_new_evsrv_evt = new_daqst->bl_n_evserv_events;
   //double r_new_evsrv_kb = new_daqst->bl_n_evserv_kbytes * 1.024;
   double r_new_kb = new_daqst->bl_n_kbyte * 1.024;
   //double r_new_tape_kb = new_daqst->bl_n_kbyte_tape * 1.024;
   double r_new_file_kb = new_daqst->bl_n_kbyte_file * 1.024;
   double r_new_strsrv_str = new_daqst->bl_n_strserv_bufs;
   double r_new_strsrv_buf = new_daqst->bl_n_strserv_bufs;
   double r_new_strsrv_kb = new_daqst->bl_n_strserv_kbytes * 1.024;


   double r_old_buf = old_daqst->bl_n_buffers;
   double r_old_evt = old_daqst->bl_n_events;
   double r_old_stream = old_daqst->bl_n_bufstream;
   double r_old_evsrv_evt = old_daqst->bl_n_evserv_events;
   //double r_old_evsrv_kb = old_daqst->bl_n_evserv_kbytes * 1.024;
   double r_old_kb = old_daqst->bl_n_kbyte * 1.024;
   //double r_old_tape_kb = old_daqst->bl_n_kbyte_tape * 1.024;
   double r_old_file_kb = old_daqst->bl_n_kbyte_file * 1.024;
   double r_old_strsrv_str = old_daqst->bl_n_strserv_bufs;
   double r_old_strsrv_buf = old_daqst->bl_n_strserv_bufs;
   double r_old_strsrv_kb = old_daqst->bl_n_strserv_kbytes * 1.024;

   double r_rate_buf = (r_new_buf - r_old_buf) / diff_time;
   double r_rate_kb = (r_new_kb - r_old_kb) / diff_time;
   double r_rate_evt = (r_new_evt - r_old_evt) / diff_time;
   double r_rate_stream = (r_new_stream - r_old_stream) / diff_time;
   double r_rate_evsrv_evt = (r_new_evsrv_evt - r_old_evsrv_evt) / diff_time;
   //double r_rate_evsrv_kb = (r_new_evsrv_kb - r_old_evsrv_kb) / diff_time;
   //double r_rate_tape_kb = (r_new_tape_kb - r_old_tape_kb) / diff_time;
   double r_rate_file_kb = (r_new_file_kb - r_old_file_kb) / diff_time;
   double r_rate_strsrv_str = (r_new_strsrv_str - r_old_strsrv_str) / diff_time / bl_n_ev_buf;
   double r_rate_strsrv_buf = (r_new_strsrv_buf - r_old_strsrv_buf) / diff_time;
   double r_rate_strsrv_kb = (r_new_strsrv_kb - r_old_strsrv_kb) / diff_time;

   strcpy (c_out, "");
   if (bl_mbs)
   {
     if (bData_n)
     {
       sprintf (c_line, "%10.0f ", r_new_buf / 1000000. * bl_ev_buf_len);
       strcat (c_out, c_line);
     }
     if (bEvents_n)
     {
       sprintf (c_line, "%10u ", (unsigned) new_daqst->bl_n_events);
       strcat (c_out, c_line);
     }
     if (bBuffers_n)
     {
       sprintf (c_line, "%10u ", (unsigned) new_daqst->bl_n_buffers);
       strcat (c_out, c_line);
     }
     if (bStreams_n)
     {
       sprintf (c_line, "%7u ", (unsigned) new_daqst->bl_n_bufstream);
       strcat (c_out, c_line);
     }
     if (bData_r)
     {
       sprintf (c_line, "%7.1f ", r_rate_kb);
       strcat (c_out, c_line);
     }
     if (bEvents_r)
     {
       sprintf (c_line, "%7.0f ", r_rate_evt);
       strcat (c_out, c_line);
     }
     if (bBuffers_r)
     {
       sprintf (c_line, "%7.0f ", r_rate_buf);
       strcat (c_out, c_line);
     }
     if (bStreams_r)
     {
       sprintf (c_line, "%7.0f ", r_rate_stream);
       strcat (c_out, c_line);
     }
   }
   if (bl_server)
   {
     strcat (c_out, "|");
     if (bSrvData_n)
     {
       sprintf (c_line, "%10.0f ", r_new_strsrv_kb / 1000.);
       strcat (c_out, c_line);
     }
     if (bSrvEvents_n)
     {
       sprintf (c_line, "%10u ", (unsigned) new_daqst->bl_n_evserv_events);
       strcat (c_out, c_line);
     }
     if (bSrvBuffers_n)
     {
       sprintf (c_line, "%10u ", (unsigned) new_daqst->bl_n_strserv_bufs);
       strcat (c_out, c_line);
     }
     if (bSrvStreams_n)
     {
       sprintf (c_line, "%7u ", (unsigned) new_daqst->bl_n_strserv_bufs / bl_n_ev_buf);
       strcat (c_out, c_line);
     }
     if (bSrvData_r)
     {
       sprintf (c_line, "%7.1f ", r_rate_strsrv_kb);
       strcat (c_out, c_line);
     }
     if (bSrvEvents_r)
     {
       sprintf (c_line, "%7.0f ", r_rate_evsrv_evt);
       strcat (c_out, c_line);
     }
     if (bSrvBuffers_r)
     {
       sprintf (c_line, "%7.0f ", r_rate_strsrv_buf);
       strcat (c_out, c_line);
     }
     if (bSrvStreams_r)
     {
       sprintf (c_line, "%7.0f ", r_rate_strsrv_str);
       strcat (c_out, c_line);
     }
   }
   if (bl_streams)
   {
     strcat (c_out, "|");
     if (bStreamsFree)
     {
       sprintf (c_line, "%5d ", l_free_stream);
       strcat (c_out, c_line);
     }
     if (bStreamsFilled)
     {
       sprintf (c_line, "%4d ", l_trans_stream);
       strcat (c_out, c_line);
     }
     if (bStreamsSrv)
     {
       sprintf (c_line, "%4d ", l_serv_stream);
       strcat (c_out, c_line);
     }
   }
   if (bl_file)
   {
     strcat (c_out, "|");
     if (bFileData)
     {
       sprintf (c_line, "%8.0f ", r_new_file_kb / 1000.);
       strcat (c_out, c_line);
     }
     if (bFileFilled)
     {
       sprintf (c_line, "%5.1f %% ", new_daqst->l_file_size > 0 ? r_new_file_kb / new_daqst->l_file_size * 0.1  : 0.);
       strcat (c_out, c_line);
     }
     if (bFileData_r)
     {
       sprintf (c_line, "%8.1f ", r_rate_file_kb);
       strcat (c_out, c_line);
     }
     if (bFileIndex)
     {
       sprintf (c_line, " %04u ", (unsigned) new_daqst->l_file_cur);
       strcat (c_out, c_line);
     }
     if (bFilename)
     {
       sprintf (c_line, "%s", new_daqst->c_file_name);
       strcat (c_out, c_line);
     }
     if (new_daqst->l_open_file)
       strcat (c_out, " op");
     else
       strcat (c_out, " cl");
   }

   dabc::Hierarchy item = fHierarchy.GetHChild(itemname);

   if (fCounter % 20 == 0) {
      item.SetField("value", c_head0);
      item.MarkChangedItems();
      item.SetField("value", c_head);
      item.MarkChangedItems();
   }
   item.SetField("value", c_out);
   item.SetFieldModified("value");

   if (options=="-u") {
      // printf("%s\n",c_out);
      fHierarchy.GetHChild("DataRate").SetField("value", dabc::format("%3.1f", r_rate_kb));
      fHierarchy.GetHChild("DataRate").SetFieldModified("value");
      fHierarchy.GetHChild("EventRate").SetField("value", dabc::format("%3.1f", r_rate_evt));
      fHierarchy.GetHChild("EventRate").SetFieldModified("value");
      fHierarchy.GetHChild("ServerRate").SetField("value", dabc::format("%3.1f", r_rate_strsrv_kb));
      fHierarchy.GetHChild("ServerRate").SetFieldModified("value");
      fHierarchy.GetHChild("FileRate").SetField("value", dabc::format("%3.1f", r_rate_file_kb));
      fHierarchy.GetHChild("FileRate").SetFieldModified("value");
   }

   if (fDoRec) {
      std::string prefix = std::string("MBS.") + fMbsNode + std::string(".");
      fRec.AddDouble(prefix + "DataRate", r_rate_kb, true);
      fRec.AddDouble(prefix + "EventRate", r_rate_evt, true);
      fRec.AddDouble(prefix + "ServerRate", r_rate_strsrv_kb, true);
      fRec.AddDouble(prefix + "FileRate", r_rate_file_kb, true);
   }

   fHierarchy.MarkChangedItems();
}

void mbs::Monitor::ProcessTimerEvent(unsigned timer)
{
   if (TimerName(timer) != "MbsUpdate") {
      mbs::MonitorSlowControl::ProcessTimerEvent(timer);
      return;
   }

   if (fMbsNode.empty()) {
       fCounter++;

       double v1 = 100. * (1.3 + sin(dabc::Now().AsDouble()/5.));
       fHierarchy.GetHChild("DataRate").SetField("value", dabc::format("%4.2f", v1));

       v1 = 100. * (1.3 + cos(dabc::Now().AsDouble()/8.));
       fHierarchy.GetHChild("EventRate").SetField("value", dabc::format("%4.2f", v1));

       fHierarchy.GetHChild("rate_log").SetField("value", dabc::format("| Header  |   Entry      |      Rate  |"));
       fHierarchy.GetHChild("rate_log").SetField("value", dabc::format("|         |    %5d       |     %6.2f  |", fCounter,v1));

       fHierarchy.MarkChangedItems();
       return;
   }

//   DOUT0("+++++++++++++++++++++++++++ Process timer!!!");

   // this indicated that addon is active and we should not touch it
   // SL 20.05.2015: allow to access status record also with prompter
   if (!fAddon.null() || (fStatPort<=0)) return;

   int fd = dabc::SocketThread::StartClient(fMbsNode, fStatPort);
   if (fd<=0)
      EOUT("FAIL status port %d for node %s", fStatPort, fMbsNode.c_str());
   else
      AssignAddon(new DaqStatusAddon(fd));
}

void mbs::Monitor::NewMessage(const std::string &msg)
{
   dabc::Hierarchy item = fHierarchy.GetHChild("logger");

   if (!item.null()) {
      item.SetField("value", msg);
      item.SetFieldModified("value");
      fHierarchy.MarkChangedItems();
   }

   if (fPrintf) printf("%s\n", msg.c_str());
}


void mbs::Monitor::NewSendCommand(const std::string &cmd, int res)
{
   if (!fPrintf) return;
   if (res>=0) printf("replcmd>%s res=%s\n", cmd.c_str(), DBOOL(res));
          else printf("sendcmd>%s\n", cmd.c_str());
}


void mbs::Monitor::NewStatus(mbs::DaqStatus& stat)
{
   dabc::TimeStamp stamp;
   stamp.GetNow();

   double tmdiff = stamp.AsDouble() - fStatStamp.AsDouble();
   if (tmdiff<=0) {
      EOUT("Wrong time calculation");
      return;
   }
   double deltaT=fabs((tmdiff-fRateInterval)/tmdiff); // JAM smooth glitches between timer period and time stamp by this
   DOUT3("NEW STATUS with rate interval:%f , dt=%f\n, delta=%f", fRateInterval, tmdiff, deltaT);
   if ((tmdiff>0) && ((deltaT<1/fRateInterval) || (ceil(tmdiff) > 1 + fRateInterval) ) )
     {
     // last term ensures that we enter this section if fRateInterval is interactively changed to lower values
     if (!fStatus.null()){
      FillStatistic("-u", "rate_log", &fStatus, &stat, tmdiff);
      FillStatistic("-rev -rda -nev -nda", "rash_log", &fStatus, &stat, tmdiff);
      FillStatistic("-rev -rda -nev -nda -rsda", "rast_log", &fStatus, &stat, tmdiff);
      FillStatistic("-rev -rda -nev -nda -rsda -fi", "ratf_log", &fStatus, &stat, tmdiff);

      DOUT3("Filled statistics with rate interval:%f after dt=%f\n", fRateInterval, tmdiff);
      fCounter++;
     }

      memcpy(&fStatus, &stat, sizeof(stat));
      fStatStamp = stamp;
   }
   DOUT3("Got acquisition running=%d, file open=%d", stat.bh_acqui_running, stat.l_open_file);

   UpdateSetupState((stat.bh_setup_loaded) && (stat.bh_running[SYS__util]));
     // <- after shutdown, check also if util task is still there, bh_setup_loaded is not reset
   UpdateMbsState(stat.bh_acqui_running);
   UpdateFileState((stat.l_open_file) && (stat.bh_running[SYS__transport]));
   // <- after shutdown, check also if transport task is still there, l_open_file is not reset




}


void mbs::Monitor::UpdateFileState(int on)
{
   dabc::Hierarchy chld = fHierarchy.GetHChild(fFileStateName);
   if (!chld.null()) {
      chld.SetField("value", dabc::format("%d", on));
      // par.ScanParamFields(&chld()->Fields());
      fHierarchy.MarkChangedItems();
      // DOUT0("ChangeFileState to %d", on);
   } else {
      DOUT0("UpdateFileState Could not find hierarchy child %s", fFileStateName.c_str());
   }
}

void mbs::Monitor::UpdateMbsState(int on)
{
   dabc::Hierarchy chld = fHierarchy.GetHChild(fAcqStateName);
   if (!chld.null()) {
      chld.SetField("value", dabc::format("%d", on));
      // par.ScanParamFields(&chld()->Fields());
      fHierarchy.MarkChangedItems();
      // DOUT0("ChangeMBSState to %d", on);
   } else {
      DOUT0("UpdateMbsState Could not find hierarchy child %s", fAcqStateName.c_str());
   }
}

void mbs::Monitor::UpdateSetupState(int on)
{
  dabc::Hierarchy chld=fHierarchy.GetHChild(fSetupStateName);
    if (!chld.null())
    {
        chld.SetField("value", dabc::format("%d", on));
         //par.ScanParamFields(&chld()->Fields());
         fHierarchy.MarkChangedItems();
         //DOUT0("ChangeSetup state to %d", on);
    }
    else
    {
        DOUT0("UpdateSetupState Could not find hierarchy child %s", fSetupStateName.c_str());
    }

}

void mbs::Monitor::SetRateInterval(double t)
{
   if (t < fPeriod)
      t = fPeriod;
   fRateInterval = t;
   dabc::Hierarchy chld = fHierarchy.GetHChild(fRateIntervalName);
   if (!chld.null()) {
      chld.SetField("value", dabc::format("%f", t));
      // par.ScanParamFields(&chld()->Fields());
      fHierarchy.MarkChangedItems();
      DOUT0("Changed rate interval to %f seconds", t);
   } else {
      DOUT0("SetRateInterval Could not find hierarchy child %s", fRateIntervalName.c_str());
   }
}

void mbs::Monitor::SetHistoryDepth(int entries)
{
   fHistory = entries;
   dabc::Hierarchy chld = fHierarchy.GetHChild(fHistoryName);
   if (!chld.null()) {
      chld.SetField("value", dabc::format("%d", entries));
      // par.ScanParamFields(&chld()->Fields());
      fHierarchy.MarkChangedItems();
      DOUT0("Changed history depth to %d entries", entries);
   } else {
      DOUT0("SetHistoryDepth  Could not find hierarchy child %s", fHistoryName.c_str());
   }

   // here activate it immediately, does it work recursively?
   //  fHierarchy.EnableHistory(0,true);
   //  fHierarchy.EnableHistory(fHistory,true);
   // NO, lets update explicitely all interesting records:

   fHierarchy.GetHChild("DataRate").EnableHistory(0, true);
   fHierarchy.GetHChild("DataRate").EnableHistory(fHistory, true);
   fHierarchy.GetHChild("EventRate").EnableHistory(0, true);
   fHierarchy.GetHChild("EventRate").EnableHistory(fHistory, true);
   fHierarchy.GetHChild("ServerRate").EnableHistory(0, true);
   fHierarchy.GetHChild("ServerRate").EnableHistory(fHistory, true);
   fHierarchy.GetHChild("FileRate").EnableHistory(0, true);
   fHierarchy.GetHChild("FileRate").EnableHistory(fHistory, true);

   fHierarchy.GetHChild("rate_log").EnableHistory(0, true);
   fHierarchy.GetHChild("rate_log").EnableHistory(fHistory, true);
   fHierarchy.GetHChild("rash_log").EnableHistory(0, true);
   fHierarchy.GetHChild("rash_log").EnableHistory(fHistory, true);
   fHierarchy.GetHChild("rast_log").EnableHistory(0, true);
   fHierarchy.GetHChild("rast_log").EnableHistory(fHistory, true);
   fHierarchy.GetHChild("ratf_log").EnableHistory(0, true);
   fHierarchy.GetHChild("ratf_log").EnableHistory(fHistory, true);

   fCounter = 0; // to print heading on top
}

int mbs::Monitor::ExecuteCommand (dabc::Command cmd)
{
   if (cmd.IsName("ProcessDaqStatus")) {
      mbs::DaqStatusAddon *tr = dynamic_cast<mbs::DaqStatusAddon *>(fAddon());

      if (tr)
         NewStatus(tr->GetStatus());

      AssignAddon(0);

      return dabc::cmd_true;
   } else if (cmd.IsName(dabc::CmdHierarchyExec::CmdName())) {

      std::string cmdpath = cmd.GetStr("Item");

      // if (cmdpath != "CmdMbs") return dabc::cmd_false;

      if (cmdpath == "CmdMbs") {
         dabc::WorkerRef wrk = FindChildRef("DaqCmd");

         if ((fCmdPort <= 0) || wrk.null())
            return dabc::cmd_false;

         wrk.Submit(cmd);

         return dabc::cmd_postponed;
      } else if (cmdpath == "CmdSetRateInterval") {
         DOUT0("ExecuteCommand  sees CmdSetRateInterval");
         double deltat = cmd.GetDouble("time", 3.0); // JAM todo: put string identifier to define or static variable
         SetRateInterval(deltat);
         SetHistoryDepth(fHistory); // need to clear old history entries when changing sampling period
         return dabc::cmd_true;
      } else if (cmdpath == "CmdSetHistoryDepth") {
         DOUT0("ExecuteCommand  sees CmdSetHistoryDepth");
         int entries = cmd.GetInt("entries", 200); // JAM todo: put string identifier to define or static variable
         SetHistoryDepth(entries);
         return dabc::cmd_true;
      } else {
         return dabc::cmd_false;
      }
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

unsigned mbs::Monitor::WriteRecRawData(void* ptr, unsigned maxsize)
{
   unsigned len = mbs::MonitorSlowControl::WriteRecRawData(ptr,maxsize);
   fRec.Clear();
   return len;
}

// =====================================================================

mbs::DaqLogWorker::DaqLogWorker(const dabc::Reference& parent, const std::string &name, const std::string &mbsnode, int port) :
   dabc::Worker(parent, name),
   fMbsNode(mbsnode),
   fPort(port)
{
}

mbs::DaqLogWorker::~DaqLogWorker()
{
}


bool mbs::DaqLogWorker::CreateAddon()
{
   if (!fAddon.null()) return true;

   int fd = dabc::SocketThread::StartClient(fMbsNode, fPort);
   if (fd<=0) {
      EOUT("Fail open log %d port on node %s", fPort, fMbsNode.c_str());
      return false;
   }

   dabc::SocketIOAddon* add = new dabc::SocketIOAddon(fd);
   add->SetDeliverEventsToWorker(true);
   AssignAddon(add);

   memset(&fRec, 0, sizeof(fRec));
   add->StartRecv(&fRec, sizeof(fRec));

   return true;
}


void mbs::DaqLogWorker::OnThreadAssigned()
{
   dabc::Worker::OnThreadAssigned();

   if (!CreateAddon()) ActivateTimeout(5);

   DOUT2("mbs::DaqLogWorker::OnThreadAssigned parent = %p", GetParent());
}

double mbs::DaqLogWorker::ProcessTimeout(double last_diff)
{
   // use timeout to reconnect with the logger
   if (CreateAddon()) return -1;
   return 5.;
}


void mbs::DaqLogWorker::ProcessEvent(const dabc::EventId& evnt)
{
   switch (evnt.GetCode()) {
      case dabc::SocketAddon::evntSocketRecvInfo: {

         if (fRec.iOrder!=1)
            mbs::SwapData(&fRec, 3 * sizeof(int32_t));

         if (fRec.iOrder==1) {

            if (fRec.iType == 1) {
               DOUT4("Keep alive message from MBS logger");
            } else {
               DOUT2("Get MSG: %s",fRec.fBuffer);
               mbs::Monitor* pl = dynamic_cast<mbs::Monitor*> (GetParent());
               if (pl) pl->NewMessage(fRec.fBuffer);
            }
         }

         memset(&fRec, 0, sizeof(fRec));

         dabc::SocketIOAddon* add = dynamic_cast<dabc::SocketIOAddon*>(fAddon());
         if (add) add->StartRecv(&fRec, sizeof(fRec));
         break;
      }
      case dabc::SocketAddon::evntSocketErrorInfo:
      case dabc::SocketAddon::evntSocketCloseInfo:
         EOUT("Problem with logger - reconnect");
         AssignAddon(0);
         ActivateTimeout(1);
         break;
      default:
         dabc::Worker::ProcessEvent(evnt);
   }

}

// =================================================================

mbs::DaqRemCmdWorker::DaqRemCmdWorker(const dabc::Reference& parent, const std::string &name,
                                     const std::string &mbsnode, int port) :
   dabc::Worker(parent, name),
   fMbsNode(mbsnode),
   fPort(port),
   fCmds(dabc::CommandsQueue::kindPostponed),
   fState(ioInit),
   fSendCmdId(1)
{
}

mbs::DaqRemCmdWorker::~DaqRemCmdWorker()
{
   DOUT3("Destroy DaqRemCmdWorker");
}

void mbs::DaqRemCmdWorker::OnThreadAssigned()
{
   dabc::Worker::OnThreadAssigned();

   CreateAddon();
}

bool mbs::DaqRemCmdWorker::CreateAddon()
{
   if (!fAddon.null()) return true;

   int fd = dabc::SocketThread::StartClient(fMbsNode, fPort);
   if (fd<=0) {
      EOUT("Fail open command port %d on node %s", fPort, fMbsNode.c_str());
      ActivateTimeout(5);
      return false;
   }

   dabc::SocketIOAddon* addon = new dabc::SocketIOAddon(fd);
   addon->SetDeliverEventsToWorker(true);

   DOUT2("ADDON:%p Created cmd socket %d to mbs %s:%d", addon, fd, fMbsNode.c_str(), fPort);

   AssignAddon(addon);

   // in any case receive next buffer
   memset(&fRecvBuf, 0, sizeof(fRecvBuf));
   addon->StartRecv(&fRecvBuf, sizeof(fRecvBuf));

   return true;
}



void mbs::DaqRemCmdWorker::ProcessEvent(const dabc::EventId& evnt)
{
   switch (evnt.GetCode()) {
      case dabc::SocketAddon::evntSocketSendInfo: {
         // this is just confirmation that data was send - do nothing
         break;
      }
      case dabc::SocketAddon::evntSocketRecvInfo: {
         //DOUT0("mbs::DaqRemCmdWorker get evntSocketRecvInfo");

         if (!fRecvBuf.CheckByteOrder()) {
            EOUT("Fail to decode data in receive buffer");
         } else
         if (fRecvBuf.l_cmdid == 0xffffffff) {
            // keep-alive buffer
            //DOUT0("mbs::DaqRemCmdWorker keep alive buffer");
         } else
         if ((fCmds.Size()>0) && (fState == ioWaitReply)) {
            // TODO: when reply command - check result

            DOUT3("mbs::DaqRemCmdWorker get reply for the command id %u", (unsigned) fRecvBuf.l_cmdid);

            bool res = fRecvBuf.l_status==0;

            if (fSendCmdId!= fRecvBuf.l_cmdid) {
               EOUT("Mismatch of command id in the MBS reply");
               res = false;
            }

            mbs::Monitor* pl = dynamic_cast<mbs::Monitor*> (GetParent());
            if (pl) pl->NewSendCommand(fCmds.Front().GetStr("cmd"), res ? 1 : 0);

            fCmds.Pop().ReplyBool(res);
            fState = ioInit;
         }

         dabc::SocketIOAddon* addon = dynamic_cast<dabc::SocketIOAddon*> (fAddon());

         if (addon==0) {
            EOUT("ADDON disappear !!!");
            ActivateTimeout(3.);
         } else {
            memset(&fRecvBuf, 0, sizeof(fRecvBuf));
            addon->StartRecv(&fRecvBuf, sizeof(fRecvBuf));
         }

         break;
      }
      case dabc::SocketAddon::evntSocketErrorInfo:
      case dabc::SocketAddon::evntSocketCloseInfo:
         // error, we cancel command execution and issue timeout to try again
         AssignAddon(0);
         if ((fState==ioWaitReply) && (fCmds.Size()>0)) {
            fCmds.Pop().Reply(dabc::cmd_false);
            fState = ioInit;
         }

         ActivateTimeout(1.);
         break;
      default:
         dabc::Worker::ProcessEvent(evnt);
   }
}

double mbs::DaqRemCmdWorker::ProcessTimeout(double last_diff)
{
   if (CreateAddon()) ProcessNextMbsCommand();

   return -1;
}

int mbs::DaqRemCmdWorker::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName(dabc::CmdHierarchyExec::CmdName())) {
      fCmds.Push(cmd);
      ProcessNextMbsCommand();
      return dabc::cmd_postponed;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}

void mbs::DaqRemCmdWorker::ProcessNextMbsCommand()
{
   // start next command when previous is completed
   if (fState != ioInit) return;

   if (fCmds.Size()==0) return;

   dabc::SocketIOAddon* addon = dynamic_cast<dabc::SocketIOAddon*> (fAddon());

   if ((addon==0) || !addon->IsSocket()) {
      EOUT("Something went wrong");
      exit(5);
   }

   std::string mbscmd = fCmds.Front().GetStr("cmd");
   if (mbscmd.length() >= sizeof(fSendBuf.c_cmd)-1) {
      EOUT("Send command too long %u", mbscmd.length());
      fCmds.Pop().Reply(dabc::cmd_false);
      ProcessNextMbsCommand();
      return;
   }

   DOUT2("Send MBS-CMD:  %s", mbscmd.c_str());

   mbs::Monitor* pl = dynamic_cast<mbs::Monitor*> (GetParent());
   if (pl) pl->NewSendCommand(mbscmd);

   fState = ioWaitReply;

   if (++fSendCmdId > 0x7fff0000) fSendCmdId = 1;

   memset(&fSendBuf, 0, sizeof(fSendBuf));
   fSendBuf.l_order = 1;
   fSendBuf.l_cmdid = fSendCmdId;
   fSendBuf.l_status = 0;
   strcpy(fSendBuf.c_cmd, mbscmd.c_str());
   addon->StartSend(&fSendBuf, sizeof(fSendBuf));
}

// ===============================================================================================

mbs::PrompterWorker::PrompterWorker(const dabc::Reference& parent, const std::string &name,
                                     const std::string &mbsnode, int port) :
   dabc::Worker(parent, name),
   fMbsNode(mbsnode),
   fPort(port),
   fPrefix(),
   fCmds(dabc::CommandsQueue::kindPostponed),
   fState(ioInit)
{
   fPrefix = dabc::SocketThread::DefineHostName() + ":";
   printf("Create prompter client with prefix %s\n", fPrefix.c_str());
}

mbs::PrompterWorker::~PrompterWorker()
{
   DOUT3("Destroy PrompterWorker");
}

void mbs::PrompterWorker::OnThreadAssigned()
{
   dabc::Worker::OnThreadAssigned();

   CreateAddon();
}

bool mbs::PrompterWorker::CreateAddon()
{
   if (!fAddon.null()) return true;

   int fd = dabc::SocketThread::StartClient(fMbsNode, fPort);
   if (fd<=0) {
      EOUT("Fail open command port %d on node %s", fPort, fMbsNode.c_str());
      ActivateTimeout(5);
      return false;
   }

   dabc::SocketIOAddon* addon = new dabc::SocketIOAddon(fd);
   addon->SetDeliverEventsToWorker(true);

   DOUT2("ADDON:%p Created cmd socket %d to mbs %s:%d", addon, fd, fMbsNode.c_str(), fPort);

   AssignAddon(addon);

   // in any case receive next buffer
   memset(fRecvBuf, 0, sizeof(fRecvBuf));
   addon->StartRecv(fRecvBuf, sizeof(fRecvBuf));

   return true;
}


void mbs::PrompterWorker::ProcessEvent(const dabc::EventId& evnt)
{
   switch (evnt.GetCode()) {
      case dabc::SocketAddon::evntSocketSendInfo: {
         // this is just confirmation that data was send - do nothing
         break;
      }
      case dabc::SocketAddon::evntSocketRecvInfo: {
         //DOUT0("mbs::PrompterWorker get evntSocketRecvInfo");

         if ((fCmds.Size()>0) && (fState == ioWaitReply)) {
            // TODO: when reply command - check result

            bool res = true;

            if (fRecvBuf[0]!=1)
               mbs::SwapData(fRecvBuf, sizeof(fRecvBuf));

            DOUT3("mbs::PrompterWorker get reply for the command id %u", (unsigned) fRecvBuf[1]);

            if (fRecvBuf[0]!=1) {
               EOUT("Wrong reply from the prompter");
               res = false;
            } else
            if (fRecvBuf[1]!=0) {
               res = false;
            }

            mbs::Monitor* pl = dynamic_cast<mbs::Monitor*> (GetParent());
            if (pl) pl->NewSendCommand(fCmds.Front().GetStr("cmd"), res ? 1 : 0);

            fCmds.Pop().ReplyBool(res);
            fState = ioInit;
         }

         dabc::SocketIOAddon* addon = dynamic_cast<dabc::SocketIOAddon*> (fAddon());

         if (addon==0) {
            EOUT("ADDON disappear !!!");
            ActivateTimeout(3.);
         } else {
            memset(fRecvBuf, 0, sizeof(fRecvBuf));
            addon->StartRecv(fRecvBuf, sizeof(fRecvBuf));
         }

         break;
      }
      case dabc::SocketAddon::evntSocketErrorInfo:
      case dabc::SocketAddon::evntSocketCloseInfo:
         // error, we cancel command execution and issue timeout to try again
         AssignAddon(0);
         if ((fState==ioWaitReply) && (fCmds.Size()>0)) {
            fCmds.Pop().Reply(dabc::cmd_false);
            fState = ioInit;
         }

         ActivateTimeout(1.);
         break;
      default:
         dabc::Worker::ProcessEvent(evnt);
   }
}

double mbs::PrompterWorker::ProcessTimeout(double last_diff)
{
   if (CreateAddon()) ProcessNextMbsCommand();

   return -1;
}

int mbs::PrompterWorker::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName(dabc::CmdHierarchyExec::CmdName())) {
      fCmds.Push(cmd);
      ProcessNextMbsCommand();
      return dabc::cmd_postponed;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}

void mbs::PrompterWorker::ProcessNextMbsCommand()
{
   // start next command when previous is completed
   if (fState != ioInit) return;

   if (fCmds.Size()==0) return;

   dabc::SocketIOAddon* addon = dynamic_cast<dabc::SocketIOAddon*> (fAddon());

   if ((addon==0) || !addon->IsSocket()) {
      EOUT("Something went wrong");
      exit(5);
   }

   std::string mbscmd = fCmds.Front().GetStr("cmd");
   if (mbscmd.length() >= sizeof(fSendBuf) - fPrefix.length()) {
      EOUT("Send command too long %u", mbscmd.length());
      fCmds.Pop().ReplyBool(false);
      ProcessNextMbsCommand();
      return;
   }

   DOUT2("Send MBS-CMD:  %s", mbscmd.c_str());

   mbs::Monitor* pl = dynamic_cast<mbs::Monitor*> (GetParent());
   if (pl) pl->NewSendCommand(mbscmd);

   fState = ioWaitReply;

   strcpy(fSendBuf, fPrefix.c_str());
   strcat(fSendBuf, mbscmd.c_str());
   addon->StartSend(fSendBuf, sizeof(fSendBuf));
}
