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


mbs::Player::Player(const std::string& name, dabc::Command cmd) :
   dabc::ModuleAsync(name, cmd),
   fHierarchy(),
   fCounter(0)
{

   fMbsNode = Cfg("node", cmd).AsStdStr();
   fPeriod = Cfg("period", cmd).AsDouble(1.);

   fHierarchy.Create("MBS");

   // this is just emulation, later one need list of real variables
   dabc::Hierarchy item = fHierarchy.CreateChild("DataRate");
   item.Field(dabc::prop_kind).SetStr("rate");
   item.EnableHistory(100,"value");

   item = fHierarchy.CreateChild("EventRate");
   item.Field(dabc::prop_kind).SetStr("rate");
   item.EnableHistory(100,"value");

   item = fHierarchy.CreateChild("ServerRate");
   item.Field(dabc::prop_kind).SetStr("rate");
   item.EnableHistory(100,"value");

   item = fHierarchy.CreateChild("rate_log");
   item.Field(dabc::prop_kind).SetStr("log");
   item.EnableHistory(200,"value");

   item = fHierarchy.CreateChild("rash_log");
   item.Field(dabc::prop_kind).SetStr("log");
   item.EnableHistory(200,"value");

   item = fHierarchy.CreateChild("rast_log");
   item.Field(dabc::prop_kind).SetStr("log");
   item.EnableHistory(200,"value");

   item = fHierarchy.CreateChild("ratf_log");
   item.Field(dabc::prop_kind).SetStr("log");
   item.EnableHistory(200,"value");

   CreateTimer("update", fPeriod, false);

   fCounter = 0;

   memset(&fStatus,0,sizeof(mbs::DaqStatus));
}


mbs::Player::~Player()
{
}


bool mbs::Player::ReadSetup(int fd, mbs::DaqSetup* setup)
{
   memset(setup,0,sizeof(mbs::DaqSetup));
   int32_t l_cmd = 2;
   int l_status = dabc::SocketThread::SendBuffer(fd, &l_cmd, 4);
   if (l_status != 4) { EOUT("here1"); return false; }

   l_status = dabc::SocketThread::RecvBuffer(fd, setup, 16);
   if (l_status!=16) { EOUT("here2"); return false; }

   bool l_swap = (setup->l_endian != 1);
   if(l_swap) mbs::SwapData(setup,16);

   if(setup->l_version != VERSION__SETUP) { EOUT("here3 %x", (unsigned) setup->l_version); return false; }

   uint32_t l_items(0),l_size(0);

   dabc::SocketThread::RecvBuffer(fd, &setup->bl_sbs__n_cr, (setup->l_fix_lw-4)*4);
   dabc::SocketThread::RecvBuffer(fd, &l_items,4);
   dabc::SocketThread::RecvBuffer(fd, &l_size,4);

   if(l_swap) mbs::SwapData(&setup->bl_sbs__n_cr, (setup->l_fix_lw-4)*4);
   if(l_swap) mbs::SwapData(&l_items,4);
   if(l_swap) mbs::SwapData(&l_size,4);
   uint32_t* pl_b = new uint32_t[l_size * l_items];

   dabc::SocketThread::RecvBuffer(fd, pl_b, l_size * l_items * 4);
   if(l_swap==1) mbs::SwapData(pl_b, l_size * l_items * 4);

   uint32_t* pl_o = pl_b;
   for(uint32_t i=0;i<l_items;i++) {
     uint32_t l_crate = *pl_o++;
     setup->lp_rem_mem_base[l_crate] = *pl_o++;
     setup->bl_rem_mem_off[l_crate] = *pl_o++;
     setup->bl_rem_mem_len[l_crate] = *pl_o++;
     setup->lp_rem_cam_base[l_crate] = *pl_o++;
     setup->bl_rem_cam_off[l_crate] = *pl_o++;
     setup->bl_rem_cam_len[l_crate] = *pl_o++;
     setup->lp_loc_mem_base[l_crate] = *pl_o++;
     setup->bl_loc_mem_len[l_crate] = *pl_o++;
     setup->lp_loc_pipe_base[l_crate] = *pl_o++;
     setup->bl_pipe_off[l_crate] = *pl_o++;
     setup->bl_pipe_seg_len[l_crate] = *pl_o++;
     setup->bl_pipe_len[l_crate] = *pl_o++;
     setup->bh_controller_id[l_crate] = *pl_o++;
     setup->bh_sy_asy_flg[l_crate] = *pl_o++;
     setup->bh_trig_stat_nr[l_crate] = *pl_o++;
     setup->bl_trig_cvt[l_crate] = *pl_o++;
     setup->bl_trig_fct[l_crate] = *pl_o++;
     setup->i_se_typ[l_crate] = *pl_o++;
     setup->i_se_subtyp[l_crate] = *pl_o++;
     setup->i_se_procid[l_crate] = *pl_o++;
     setup->bh_rd_flg[l_crate] = *pl_o++;
     setup->bl_init_tab_off[l_crate] = *pl_o++;
     setup->bi_init_tab_len[l_crate] = *pl_o++;
     for(uint32_t k=0;k<setup->bl_sbs__n_trg_typ;k++) setup->bl_max_se_len[l_crate][k] = *pl_o++;
     for(uint32_t k=0;k<setup->bl_sbs__n_trg_typ;k++) setup->bl_rd_tab_off[l_crate][k] = *pl_o++;
     for(uint32_t k=0;k<setup->bl_sbs__n_trg_typ;k++) setup->bi_rd_tab_len[l_crate][k] = *pl_o++;
   } /* setup */
   delete[] pl_b;
   return true;
}


bool mbs::Player::ReadMO(int fd, mbs::StatusMO* stat)
{
   bool l_swap = false;
   uint32_t l_cmd=4;
   int l_status = dabc::SocketThread::SendBuffer(fd, &l_cmd, 4);
   if (l_status != 4) return false;

   l_status = dabc::SocketThread::RecvBuffer(fd, stat, 16);
   if (l_status!=16) return false;

   if(stat->l_endian != 1) l_swap = true;
   if(l_swap) mbs::SwapData(stat,16);

   dabc::SocketThread::RecvBuffer(fd, &stat->l_max_nodes,(stat->l_set_mo_lw-4)*4);
   if(l_swap) mbs::SwapData(&stat->l_max_nodes, (stat->l_swap_lw-4)*4);

   return true;
}


bool mbs::Player::ReadStatus(int fd, mbs::DaqStatus* stat)
{
  int l_swap=0;

  int32_t len_64, n_trg, max_proc, l_cmd;

  int l_status;

  memset(stat, 0, sizeof(mbs::DaqStatus));
  l_cmd=1;
  l_status = dabc::SocketThread::SendBuffer(fd, &l_cmd, 4);
  if (l_status != 4) { stat->clear(); return false; }

  l_status = dabc::SocketThread::RecvBuffer(fd, &stat->l_endian, 28);
  if (l_status != 28) { stat->clear(); return false; }

  if(stat->l_endian != 1) l_swap = 1;
  if(l_swap == 1) mbs::SwapData(&stat->l_endian, 28);

  len_64 = stat->l_sbs__str_len_64;
  n_trg = stat->l_sbs__n_trg_typ;
  max_proc = stat->l_sys__n_max_procs;
  if(stat->l_version == 1)
  {
     // MBS v44 and previous no longer supported
     stat->clear();
     return false;
  }

  // MBS v50
  if(stat->l_version == 2) {
     int k = (48+n_trg*3)*4; // up to bl_n_evt inclusive
     l_status = dabc::SocketThread::RecvBuffer(fd, &stat->bh_daqst_initalized, k);
     k = (24+max_proc*5)*4; // bh_running up to bl_event_build_on inclusive
     l_status = dabc::SocketThread::RecvBuffer(fd, &stat->bh_running[0], k);
     k = len_64*15; // strings up to c_file_name inclusive
     l_status = dabc::SocketThread::RecvBuffer(fd, &stat->c_user[0], k);
     l_status = dabc::SocketThread::RecvBuffer(fd, &stat->c_out_chan[0], len_64);

     stat->l_fix_lw += n_trg*3 + 212 + len_64/4*3;
     if(l_swap == 1)
        mbs::SwapData(&stat->bh_daqst_initalized, (stat->l_fix_lw-7)*4 - (19 * len_64));
  }

  // MBS v51
  if(stat->l_version == 51) {
     l_status = dabc::SocketThread::RecvBuffer(fd, &stat->bh_daqst_initalized, (stat->l_fix_lw-7)*4);
     if(l_swap == 1)
        mbs::SwapData(&stat->bh_daqst_initalized, (stat->l_fix_lw-7)*4 - (19 * len_64));
  }

  //l_status = f_stc_read (&stat->c_pname[0], stat->l_procs_run * len_64, l_tcp,-1);
  // workaround:
  l_status = dabc::SocketThread::RecvBuffer(fd, &stat->c_pname[0], stat->l_procs_run * len_64 -1);

  return true;
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

   strcpy (c_out, " ");
   if (bl_mbs)
   {
     if (bData_n)
     {
       sprintf (c_line, "%10.0f ", r_new_buf / 1000000. * bl_ev_buf_len);
       strcat (c_out, c_line);
     }
     if (bEvents_n)
     {
       sprintf (c_line, "%10d ", new_daqst->bl_n_events);
       strcat (c_out, c_line);
     }
     if (bBuffers_n)
     {
       sprintf (c_line, "%10d ", new_daqst->bl_n_buffers);
       strcat (c_out, c_line);
     }
     if (bStreams_n)
     {
       sprintf (c_line, "%7d ", new_daqst->bl_n_bufstream);
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
       sprintf (c_line, "%10d ", new_daqst->bl_n_evserv_events);
       strcat (c_out, c_line);
     }
     if (bSrvBuffers_n)
     {
       sprintf (c_line, "%10d ", new_daqst->bl_n_strserv_bufs);
       strcat (c_out, c_line);
     }
     if (bSrvStreams_n)
     {
       sprintf (c_line, "%7d ", new_daqst->bl_n_strserv_bufs / bl_n_ev_buf);
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
       sprintf (c_line, " %04d ", new_daqst->l_file_cur);
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
      item.Field("value").SetStr(c_head0);
      item.Field("value").SetStr(c_head);
   }
   item.Field("value").SetStr(c_out);

   if (options=="-u") {
      // printf("%s\n",c_out);
      fHierarchy.FindChild("DataRate").Field("value").SetStr(dabc::format("%3.1f", r_rate_buf));
      fHierarchy.FindChild("ServerRate").Field("value").SetStr(dabc::format("%3.1f", r_rate_strsrv_kb));
   }

}



void mbs::Player::ProcessTimerEvent(unsigned timer)
{

   //mbs::DaqSetup setup;
   mbs::DaqStatus stat;
   //mbs::StatusMO mo;
   dabc::TimeStamp stamp;
   stat.clear();
   bool isnew = false;

   if (!fMbsNode.empty()) {
      int fd = dabc::SocketThread::StartClient(fMbsNode.c_str(), 6008, false);
      if (fd>0) {
         if (ReadStatus(fd, &stat) /* && ReadSetup(fd, &setup)*/) isnew = true;
         close(fd);
         stamp.GetNow();
      }
   } else {

      fCounter++;

      double v1 = 100. * (1.3 + sin(dabc::Now().AsDouble()/5.));
      fHierarchy.FindChild("DataRate").Field("value").SetStr(dabc::format("%4.2f", v1));

      v1 = 100. * (1.3 + cos(dabc::Now().AsDouble()/8.));
      fHierarchy.FindChild("EventRate").Field("value").SetStr(dabc::format("%4.2f", v1));

      fHierarchy.FindChild("rate_log").Field("value").SetStr(dabc::format("| Header  |   Entry      |      Rate  |"));
      fHierarchy.FindChild("rate_log").Field("value").SetStr(dabc::format("|         |    %5d       |     %6.2f  |", fCounter,v1));
      return;
   }

   if (!fStatus.null() && !stat.null() && isnew) {

      double tmdiff = stamp.AsDouble() - fStamp.AsDouble();
      if (tmdiff<=0) {
         EOUT("Wrong time calculation");
         return;
      }

      // DOUT0("Diff = %f", tmdiff);
      FillStatistic("-u", "rate_log", &fStatus, &stat, tmdiff);
      FillStatistic("-rev -rda -nev -nda", "rash_log", &fStatus, &stat, tmdiff);
      FillStatistic("-rev -rda -nev -nda -rsda", "rast_log", &fStatus, &stat, tmdiff);
      FillStatistic("-rev -rda -nev -nda -rsda -fi", "ratf_log", &fStatus, &stat, tmdiff);

      fCounter++;
   }


   if (!stat.null()) {
      memcpy(&fStatus, &stat, sizeof(mbs::DaqStatus));
      fStamp = stamp;
   }

}


int mbs::Player::ExecuteCommand(dabc::Command cmd)
{
   if (cmd.IsName("GetBinary")) {

      std::string itemname = cmd.GetStdStr("Item");

      dabc::Hierarchy item = fHierarchy.FindChild(itemname.c_str());

      if (item.null()) {
         EOUT("No find item %s to get binary", itemname.c_str());
         return dabc::cmd_false;
      }

      dabc::Buffer buf;

      if (cmd.GetBool("history"))
         buf = item.ExecuteHistoryRequest(cmd);

      if (buf.null()) {
         EOUT("No find binary data for item %s", itemname.c_str());
         return dabc::cmd_false;
      }

      cmd.SetRawData(buf);

      return dabc::cmd_true;
   }

   return dabc::ModuleAsync::ExecuteCommand(cmd);
}


void mbs::Player::BuildWorkerHierarchy(dabc::HierarchyContainer* cont)
{
   // indicate that we are bin producer of down objects

   // do it here, while all properties of main node are ignored when hierarchy is build
   dabc::Hierarchy(cont).Field(dabc::prop_binary_producer).SetStr(ItemName());

   fHierarchy()->BuildHierarchy(cont);
}
