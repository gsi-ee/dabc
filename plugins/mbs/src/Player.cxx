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

#include "mbs/Player.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "dabc/SocketThread.h"
#include "dabc/Pointer.h"

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

      if ((fStatus.l_version != 51) && (fStatus.l_version != 62))  {
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

mbs::Player::Player(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fHierarchy(),
   fCounter(0),
   fMbsNode(),
   fPeriod(1.),
   fStatus(),
   fStatStamp()
{

   fMbsNode = Cfg("node", cmd).AsStr();
   fPeriod = Cfg("period", cmd).AsDouble(1.);
   int history = Cfg("history", cmd).AsInt(200);
   fPrompter = Cfg("prompter", cmd).AsStr();
   fWithLogger = Cfg("logger", cmd).AsBool(false);

   fHierarchy.Create("MBS");
   //fHierarchy.Field(dabc::prop_producer).SetStr(WorkerAddress());

   // this is just emulation, later one need list of real variables
   dabc::Hierarchy item = fHierarchy.CreateChild("DataRate");
   item.SetField(dabc::prop_kind, "rate");
   if (history>1) item.EnableHistory(history);

   item = fHierarchy.CreateChild("EventRate");
   item.SetField(dabc::prop_kind, "rate");
   if (history>1) item.EnableHistory(history);

   item = fHierarchy.CreateChild("ServerRate");
   item.SetField(dabc::prop_kind, "rate");
   if (history>1) item.EnableHistory(history);

   if (fWithLogger) {
      item = fHierarchy.CreateChild("logger");
      item.SetField(dabc::prop_kind, "log");
      if (history>1) item.EnableHistory(history);
   }

   item = fHierarchy.CreateChild("rate_log");
   item.SetField(dabc::prop_kind, "log");
   if (history>1) item.EnableHistory(history);

   item = fHierarchy.CreateChild("rash_log");
   item.SetField(dabc::prop_kind, "log");
   if (history>1) item.EnableHistory(history);

   item = fHierarchy.CreateChild("rast_log");
   item.SetField(dabc::prop_kind, "log");
   if (history>1) item.EnableHistory(history);

   item = fHierarchy.CreateChild("ratf_log");
   item.SetField(dabc::prop_kind, "log");
   if (history>1) item.EnableHistory(history);

/* item = fHierarchy.CreateChild("CmdStart");
   item.Field(dabc::prop_kind).SetStr("DABC.Command");

   item = fHierarchy.CreateChild("CmdNodes");
   item.Field(dabc::prop_kind).SetStr("DABC.Command");

   item = fHierarchy.CreateChild("CmdFiles");
   item.Field(dabc::prop_kind).SetStr("DABC.Command");
*/

   if (!fPrompter.empty()) {
      dabc::CommandDefinition cmddef = fHierarchy.CreateChild("CmdMbs");
      cmddef.SetField(dabc::prop_kind, "DABC.Command");
      cmddef.AddArg("cmd", "string", true, "show rate");
   }

   CreateTimer("update", fPeriod, false);

   fCounter = 0;

   memset(&fStatus,0,sizeof(mbs::DaqStatus));

   // from this point on Publisher want to get regular update for the hierarchy
   Publish(fHierarchy, std::string("/MBS/") + fMbsNode);
}


mbs::Player::~Player()
{
}

void mbs::Player::OnThreadAssigned()
{
   if (fWithLogger) {
      DaqLogWorker* logger = new DaqLogWorker(this, "DaqLogger", fMbsNode);
      logger->AssignToThread(thread());
   }

   dabc::ModuleAsync::OnThreadAssigned();
}

void mbs::Player::FillStatistic(const std::string& options, const std::string& itemname, mbs::DaqStatus* old_daqst, mbs::DaqStatus* new_daqst, double diff_time)
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
       sprintf (c_line, "%5.1f %% ", r_new_file_kb / new_daqst->l_file_size * 0.1);
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

   dabc::Hierarchy item = fHierarchy.FindChild(itemname.c_str());

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
      fHierarchy.FindChild("DataRate").SetField("value", dabc::format("%3.1f", r_rate_kb));
      fHierarchy.FindChild("EventRate").SetField("value", dabc::format("%3.1f", r_rate_evt));
      fHierarchy.FindChild("ServerRate").SetField("value", dabc::format("%3.1f", r_rate_strsrv_kb));
   }

//   DOUT0("Set %s cnt %d changed %s", itemname.c_str(), fCounter, DBOOL(fHierarchy()->IsNodeChanged(true)));

   fHierarchy.MarkChangedItems();

//   if (item.IsName("rate_log")) {
//      std::string res = item.SaveToXml(dabc::xmlmask_History);
//      DOUT0("RATE\n%s", res.c_str());
//   }

//   DOUT0("After %s cnt %d changed %s", itemname.c_str(), fCounter, DBOOL(fHierarchy()->IsNodeChanged(true)));
}

void mbs::Player::ProcessTimerEvent(unsigned timer)
{
//   DOUT0("+++++++++++++++++++++++++++ Process timer!!!");

   if (fMbsNode.empty()) {
       fCounter++;

       double v1 = 100. * (1.3 + sin(dabc::Now().AsDouble()/5.));
       fHierarchy.FindChild("DataRate").SetField("value", dabc::format("%4.2f", v1));

       v1 = 100. * (1.3 + cos(dabc::Now().AsDouble()/8.));
       fHierarchy.FindChild("EventRate").SetField("value", dabc::format("%4.2f", v1));

       fHierarchy.FindChild("rate_log").SetField("value", dabc::format("| Header  |   Entry      |      Rate  |"));
       fHierarchy.FindChild("rate_log").SetField("value", dabc::format("|         |    %5d       |     %6.2f  |", fCounter,v1));

       fHierarchy.MarkChangedItems();
       return;
   }

//   DOUT0("+++++++++++++++++++++++++++ Process timer!!!");

   // this indicated that addon is active and we should not touch it
   if (!fAddon.null()) return;

   int fd = dabc::SocketThread::StartClient(fMbsNode.c_str(), 6008);
   if (fd<=0)
      EOUT("FAIL port 6008 for node %s", fMbsNode.c_str());
   else
      AssignAddon(new DaqStatusAddon(fd));
}

void mbs::Player::NewMessage(const std::string& msg)
{
   dabc::Hierarchy item = fHierarchy.FindChild("logger");

   if (!item.null()) {
      item.SetField("value", msg);
      item.SetFieldModified("value");
      fHierarchy.MarkChangedItems();
   }
}

void mbs::Player::NewStatus(mbs::DaqStatus& stat)
{
   dabc::TimeStamp stamp;
   stamp.GetNow();

   double tmdiff = stamp.AsDouble() - fStatStamp.AsDouble();
   if (tmdiff<=0) {
      EOUT("Wrong time calculation");
      return;
   }

   if (!fStatus.null() && (tmdiff>0)) {
      FillStatistic("-u", "rate_log", &fStatus, &stat, tmdiff);
      FillStatistic("-rev -rda -nev -nda", "rash_log", &fStatus, &stat, tmdiff);
      FillStatistic("-rev -rda -nev -nda -rsda", "rast_log", &fStatus, &stat, tmdiff);
      FillStatistic("-rev -rda -nev -nda -rsda -fi", "ratf_log", &fStatus, &stat, tmdiff);

      //DOUT0("MBS version is %u", fHierarchy.GetVersion());

      fCounter++;
   }

   memcpy(&fStatus, &stat, sizeof(stat));

   fStatStamp = stamp;
}


int mbs::Player::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("ProcessDaqStatus")) {

      mbs::DaqStatusAddon* tr = dynamic_cast<mbs::DaqStatusAddon*> (fAddon());

      if (tr) NewStatus(tr->GetStatus());

      AssignAddon(0);

      return dabc::cmd_true;
   } else

   if (cmd.IsName("CmdStart") ||
       cmd.IsName("CmdFiles") ||
       cmd.IsName("CmdNodes") ||
       cmd.IsName("CmdMbs")) {

      if (fPrompter.empty()) return dabc::cmd_false;

      dabc::WorkerRef wrk = FindChildRef("CmdPrompt");

      if (wrk.null()) {
         DaqCmdWorker* cmdwrk = new DaqCmdWorker(this, "CmdPrompt", fMbsNode, fPrompter);
         cmdwrk->AssignToThread(thread());
         wrk = cmdwrk;
      }

      wrk.Submit(cmd);

      // DOUT0("Get new command %s", cmd.GetName());

      return dabc::cmd_postponed;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}

// =====================================================================

mbs::DaqLogWorker::DaqLogWorker(const dabc::Reference& parent, const std::string& name, const std::string& mbsnode) :
   dabc::Worker(parent, name),
   fMbsNode(mbsnode)
{
}

mbs::DaqLogWorker::~DaqLogWorker()
{
   DOUT0("Destroy DaqLogWorker");
}


bool mbs::DaqLogWorker::CreateAddon()
{
   if (!fAddon.null()) return true;

   int fd = dabc::SocketThread::StartClient(fMbsNode.c_str(), 6007);
   if (fd<=0) {
      EOUT("Fail open log 6007 port on node %s", fMbsNode.c_str());
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
   if (!CreateAddon()) ActivateTimeout(5);

   DOUT0("mbs::DaqLogWorker::OnThreadAssigned parent = %p", GetParent());
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
               DOUT0("Get MSG: %s",fRec.fBuffer);
               mbs::Player* pl = dynamic_cast<mbs::Player*> (GetParent());
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

mbs::DaqCmdWorker::DaqCmdWorker(const dabc::Reference& parent, const std::string& name,
                                const std::string& mbsnode, const std::string& prompter) :
   dabc::Worker(parent, name),
   fMbsNode(mbsnode),
   fPrompter(prompter),
   fCmds(dabc::CommandsQueue::kindPostponed),
   fState(ioInit),
   fExtraBlock(0),
   fExtraBuf()
{
}

mbs::DaqCmdWorker::~DaqCmdWorker()
{
   DOUT0("Destroy DaqCmdWorker");
}


void mbs::DaqCmdWorker::ProcessEvent(const dabc::EventId& evnt)
{
   switch (evnt.GetCode()) {
      case dabc::SocketAddon::evntSocketSendInfo: {
         // this is just confirmation that data was send
//         dabc::SocketIOAddon* addon = dynamic_cast<dabc::SocketIOAddon*> (fAddon());

//         addon->StartRecv(fRecvBuf, sizeof(fRecvBuf));

         // DOUT0("dabc::SocketAddon::evntSocketSendInfo");
         break;
      }
      case dabc::SocketAddon::evntSocketRecvInfo: {

         // DOUT0("dabc::SocketAddon::evntSocketRecvInfo");

         if (fState == ioRecvHeader) {

            if (GetEndian()!=1)
               mbs::SwapData(fRecvBuf, sizeof(fRecvBuf));

            if (GetEndian()!=1) {
               fState = ioError; // indicate that we get wrong data
            } else
            if (fExtraBlock * GetStatus() == 0) {
               fState = (GetStatus() == 0) ? ioDone : ioError; // if status==0, everything went ok
            } else
            if (GetStatus() < 2) {
               fState = ioError; // when requesting extra information, status should be at least 2
            } else {

               // prompter sends endian and status after extra block again
               unsigned extra_size = fExtraBlock * GetStatus() + 8;

               if (fExtraBuf.GetTotalSize() < extra_size)
                  fExtraBuf = dabc::Buffer::CreateBuffer(extra_size);

               dabc::SocketIOAddon* addon = dynamic_cast<dabc::SocketIOAddon*> (fAddon());

               addon->StartRecv(fExtraBuf.SegmentPtr(), extra_size);

               fState = ioRecvExtra;
            }
         } else
         if (fState == ioRecvExtra) {
            fState = ioDone;
         } else {
            EOUT("Get recv complete at wrong moment of time");
            fState = ioError;
         }

         // DOUT0("mbs::DaqCmdWorker::OnRecvCompleted finishing %d", fState);

         if ((fState == ioDone) || (fState == ioError)) {
            fCmds.Pop().Reply(cmd_bool(fState == ioDone));
            fState = ioInit;
            ProcessNextMbsCommand();
         }

         break;
      }
      case dabc::SocketAddon::evntSocketErrorInfo:
      case dabc::SocketAddon::evntSocketCloseInfo:
         // error, we cancel command execution and issue timeout to try again
         AssignAddon(0);
         if ((fState!=ioInit) && (fCmds.Size()>0)) {
            fCmds.Pop().Reply(dabc::cmd_false);
            fState = ioInit;
         }

         ActivateTimeout(1.);
         break;
      default:
         dabc::Worker::ProcessEvent(evnt);
   }
}

double mbs::DaqCmdWorker::ProcessTimeout(double last_diff)
{
   ProcessNextMbsCommand();

   return -1;
}

int mbs::DaqCmdWorker::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("CmdStart") ||
       cmd.IsName("CmdFiles") ||
       cmd.IsName("CmdNodes") ||
       cmd.IsName("CmdMbs")) {

      // DOUT0("Get new command %s", cmd.GetName());

      fCmds.Push(cmd);
      ProcessNextMbsCommand();
      return dabc::cmd_postponed;
   }

   return dabc::Worker::ExecuteCommand(cmd);
}

void mbs::DaqCmdWorker::ProcessNextMbsCommand()
{
   // start next command when previous is completed
   if (fState != ioInit) return;

   if (fCmds.Size()==0) return;

   dabc::SocketIOAddon* addon = 0;

   if (fAddon.null()) {
      int fd = dabc::SocketThread::StartClient(fMbsNode.c_str(), 6006);
      if (fd<=0) {
         EOUT("Fail open prompter 6006 port on node %s", fMbsNode.c_str());
         ActivateTimeout(5);
         return;
      }

      addon = new dabc::SocketIOAddon(fd);
      addon->SetDeliverEventsToWorker(true);

      DOUT0("ADDON:%p Create cmd socket %d to mbs %s:6006", addon, fd, fMbsNode.c_str());

      AssignAddon(addon);
   } else {
     addon = dynamic_cast<dabc::SocketIOAddon*> (fAddon());
   }

   if ((addon==0) || !addon->IsSocket()) {
      EOUT("Something went wrong");
      exit(5);
   }

   std::string mbscmd = "show rate";
   int mbsblock = 0;

   if (fCmds.Front().IsName("CmdStart")) mbscmd = "@startup"; else
   if (fCmds.Front().IsName("CmdNodes")) mbscmd = "$nodes"; else
   if (fCmds.Front().IsName("CmdFiles")) mbscmd = "$file abc"; else
   if (fCmds.Front().IsName("CmdMbs")) mbscmd = fCmds.Front().GetStr("cmd");

   if (mbscmd.find("$nodes")==0) mbsblock = 64; else
   if (mbscmd.find("$file ")==0) mbsblock = 80;


   std::string s = dabc::format("%s:%s", fPrompter.c_str(), mbscmd.c_str());
   if (s.length() >= sizeof(fSendBuf)) {
      EOUT("Send command too long %u", s.length());
      fCmds.Pop().Reply(dabc::cmd_false);
      ProcessNextMbsCommand();
      return;
   }

   DOUT0("MBS-CMD:  %s", s.c_str());

   fExtraBlock = mbsblock;

   memset(fSendBuf, 0, sizeof(fSendBuf));
   memcpy(fSendBuf, s.c_str(), s.length()+1);
   memset(fRecvBuf, 0, sizeof(fRecvBuf));

   addon->StartRecv(fRecvBuf, sizeof(fRecvBuf));
   addon->StartSend(fSendBuf, sizeof(fSendBuf));

   fState = ioRecvHeader;
}
