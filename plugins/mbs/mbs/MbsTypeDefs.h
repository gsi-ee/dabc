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

#ifndef MBS_MbsTypeDefs
#define MBS_MbsTypeDefs

#ifndef MBS_LmdTypeDefs
#include "LmdTypeDefs.h"
#endif

namespace mbs {

   enum EMbsTriggerTypes {
      tt_Event         = 1,
      tt_SpillOn       = 12,
      tt_SpillOff      = 13,
      tt_StartAcq      = 14,
      tt_StopAcq       = 15
   };

#pragma pack(push, 1)


   /** \brief MBS subevent  */

   struct SubeventHeader : public Header {
      union {

        struct {

#if BYTE_ORDER == LITTLE_ENDIAN
      int16_t  iProcId;     /*  Processor ID [as loaded from VAX] */
      int8_t   iSubcrate;   /*  Subcrate number */
      int8_t   iControl;    /*  Processor type code */
#else
      int8_t   iControl;    /*  Processor type code */
      int8_t   iSubcrate;   /*  Subcrate number */
      int16_t  iProcId;     /*  Processor ID [as loaded from VAX] */
#endif
        };

        uint32_t fFullId;   /** full subevent id */
      };

      void Init(uint8_t crate = 0, uint16_t procid = 0, uint8_t control = 0)
      {
         iWords = 0;
         iType = MBS_TYPE(10,1);
         iProcId = procid;
         iSubcrate = crate;
         iControl = control;
      }

      void *RawData() const { return (char*) this + sizeof(SubeventHeader); }

      // RawDataSize - size of raw data without subevent header
      uint32_t RawDataSize() const { return FullSize() - sizeof(SubeventHeader); }
      void SetRawDataSize(uint32_t sz) { SetFullSize(sz + sizeof(SubeventHeader)); }

   };

   // _______________________________________________________________

   typedef uint32_t EventNumType;

   /** \brief MBS event  */

   struct EventHeader : public Header {
#if BYTE_ORDER == LITTLE_ENDIAN
      int16_t       iDummy;     /*  Not used yet */
      int16_t       iTrigger;   /*  Trigger number */
#else
      int16_t       iTrigger;   /*  Trigger number */
      int16_t       iDummy;     /*  Not used yet */
#endif
      EventNumType  iEventNumber;

      void Init(EventNumType evnt = 0)
      {
         iWords = 0;
         iType = MBS_TYPE(10,1);
         iDummy = 0;
         iTrigger = tt_Event;
         iEventNumber = evnt;
      }

      void CopyHeader(EventHeader* src)
      {
         iDummy = src->iDummy;
         iTrigger = src->iTrigger;
         iEventNumber = src->iEventNumber;
      }

      inline EventNumType EventNumber() const { return iEventNumber; }
      inline void SetEventNumber(EventNumType ev) { iEventNumber = ev; }

      // SubEventsSize - size of all subevents, not includes events header
      inline uint32_t SubEventsSize() const { return FullSize() - sizeof(EventHeader); }
      inline void SetSubEventsSize(uint32_t sz) { SetFullSize(sz + sizeof(EventHeader)); }

      SubeventHeader* SubEvents() const
         { return (SubeventHeader*) ((char*) this + sizeof(EventHeader)); }

      SubeventHeader* NextSubEvent(SubeventHeader* prev) const
         { return prev == 0 ? (FullSize() > sizeof(EventHeader) ? SubEvents() : 0):
                (((char*) this + FullSize() > (char*) prev + prev->FullSize() + sizeof(SubeventHeader)) ?
                   (SubeventHeader*) (((char*) prev) + prev->FullSize()) : 0); }

      unsigned NumSubevents() const;
   };

   // _______________________________________________________________

   /** \brief MBS buffer header
    *
    * Used in LMD files, written by MBS directly. Not used in DABC  */

   struct BufferHeader : public Header {
      union {
         struct {
#if BYTE_ORDER == LITTLE_ENDIAN
            int16_t  i_used;   /*  Used length of data field in words */
            int8_t   h_end;   /*  Fragment at begin of buffer */
            int8_t   h_begin;   /*  Fragment at end of buffer */
#else
            int8_t   h_begin;   /*  Fragment at end of buffer */
            int8_t   h_end;    /*  Fragment at begin of buffer */
            int16_t  i_used;   /*  Used length of data field in words */
#endif
         };
         int32_t iUsed;        // not used for type=100, low 16bits only
      };

      int32_t iBufferId;    // consequent buffer id
      int32_t iNumEvents;   // number of events in buffer
      int32_t iTemp;        // Used volatile
      int32_t iSeconds;
      int32_t iNanosec;
      int32_t iEndian;      // compatible with s_bufhe free[0]
      int32_t iLast;        // length of last event, free[1]
      int32_t iUsedWords;   // total words without header to read for type=100, free[2]
      int32_t iFree3;       // free[3]

      void Init(bool newformat);

      // length of buffer, which will be transported over socket
      uint32_t BufferLength() const;

      // UsedBufferSize - size of data after buffer header
      uint32_t UsedBufferSize() const;
      void SetUsedBufferSize(uint32_t len);

      void SetEndian() { iEndian = 1; }
      bool IsCorrectEndian() const { return iEndian == 1; }
   };

   // ________________________________________________________

   /** \brief MBS server info structure
    *
    * Send by server after connection to server is established */

   struct TransportInfo {
      int32_t iEndian;      // byte order. Set to 1 by sender
      int32_t iMaxBytes;    // maximum buffer size
      int32_t iBuffers;     // buffers per stream
      int32_t iStreams;     // number of streams (could be set to -1 to indicate variable length buffers)
   };

   // __________________________________________________________________________


   struct DaqSetup {
      enum {
         SBS__N_TRG_TYP = 16,
         SBS__N_CR = 16
      };

   /*---------------------------------------------------------------------------*/
   uint32_t l_endian;            /* set to 1 */
   uint32_t l_version;           /* increment in f_ut_setup_ini after changes */
   uint32_t bl_struc_len;      /* sizeof(s_setup)/4 : length of this structure             */
   uint32_t l_fix_lw;      /* (&lp_rem_mem_base-ps_setup)/4 : swapping size          */
   uint32_t bl_sbs__n_cr;        /* set to SBS__N_CR  */
   uint32_t bl_sbs__n_trg_typ;   /* set to SBS__N_TRG_TYP */
   /*---------------------------------------------------------------------------*/
   uint32_t bi_master;      /* indicates type of master event builder                 */
                         /* meb:   1 = CVC, 2 = E6, 3 = E7                         */
   uint32_t bl_no_crates;      /* Number of crates to read (bh_rd_flg > 0)               */
   uint32_t bh_crate_nr;      /* crate nr. of readout processor: meb = 0, slave = 1-15  */
                         /* this value will be set by the load setup command and   */
                         /* is not parsed from the .usf file                       */
   /*---------------------------------------------------------------------------*/
   uint32_t bl_ev_buf_len;      /* length of single event buffer                          */
   uint32_t bl_n_ev_buf;      /* number of event buffers in a stream                    */
   uint32_t bl_n_stream;      /* number of streams                                      */
   /*---------------------------------------------------------------------------*/
   uint32_t bi_evt_typ_sy;      /* event type of synchronous events,     default = 10     */
   uint32_t bi_evt_typ_asy;      /* event type of asynchronous events,    default = 10     */
   uint32_t bi_evt_subtyp_sy;      /* event subtype of synchronous events,  default = 1      */
   uint32_t bi_evt_subtyp_asy;   /* event subtype of asynchronous events, default = 2      */
   int32_t h_se_control;      /* serves as branch identifier in a multi-branch system   */
   /*---------------------------------------------------------------------------*/
   uint32_t bh_rd_typ;      /* 0 = standard readout with  readout tables              */
                         /* 1 = user readout (function)                            */
                    /* if the following bh_meb_asy_flg is set to 1 there will be a second    */
                    /* async. subevent pipe installed, which will be collected by the meb    */
                    /* collector, but must be filled by a seperate process                   */
   uint32_t bh_col_mode;       /* indicates mode of this MEB collector or master readout:*/
               /* 0 = standard, the collector formats events and passes  */
               /*     the formatted event buffer streams to the          */
               /*     transport                                          */
               /* 1 = collector collects subevents from the various      */
               /*     SBS pipes, but no event formatting will be done.   */
               /*     instead the collected subevents are written into a */
               /*     output pipe for the next layer node of the         */
               /*     multi-branch multi-layer daq system. this mode     */
               /*     requires bl_ml_pipe_base_addr, bl_ml_pipe_seg_len  */
               /*     and bl_ml_pipe_len to be specified (see below).    */
               /* 2 = only m_read_meb and no collector runs on the       */
               /*     SBS master (MEB). Its subevent pipe will be read   */
               /*     by a node of the multi-branch multi-layer daq      */
               /*     system. in this case the mb ml node gets the pipe  */
               /*     specs from bl_pipe_seg_len and bl_pipe_len         */
               /*     (see above).                                       */
               /* 3 = includes both 0 and 1 mode.                        */
   /*---------------------------------------------------------------------------*/
   uint32_t bl_loc_esone_base;   /* base address to execute CAMAC cnafs via the ESONE      */
                         /* window on the local crate*/
   uint32_t bl_rem_esone_base;   /* base address to execute CAMAC cnafs via the ESONE      */
                         /* window on the remote crate*/
   uint32_t bl_esone_off;      /* offset from standard to ESONE window in bytes          */
   uint32_t bl_cvc_crr_off;      /* offset from  to the CVC CAMAC Read Register            */
   uint32_t bl_cvc_csr_off;      /* offset to the CVC Control and Status Register          */
   uint32_t bl_cvc_clb_off;      /* offset to the CVC CAMAC Lam Buffer                     */
   uint32_t bl_se_meb_asy_len;   /* length of the async. pipe of the master, including     */
                         /* control and data                                       */
   uint32_t bh_meb_asy_flg;      /* indicates if meb must collect must an async. pipe      */
                         /* residing on the meb: 0 = no, 1 = yes                   */
   uint32_t bl_ml_pipe_base_addr;/* base address of the multi-branch multi-layer daq       */
                         /* system output pipe.                                    */
                         /* only requested ig bh_col_mode = 1,2                    */
   uint32_t bl_ml_pipe_seg_len;  /* length of the multi-branch multi-layer daq system      */
                         /* output pipe.  (total lengt of subevent pipe)           */
                         /* only requested ig bh_col_mode = 1,2                    */
   uint32_t bl_ml_pipe_len;       /* number of subevent/fragment slots in the multi-branch  */
                         /* multi-layer daq system output pipe                     */
                         /* only requested ig bh_col_mode = 1,2                    */
   uint32_t bl_n_col_retry;      /* number of retries the subevent collect process makes   */
                         /* until giving up time slice                             */
   /*---------------------------------------------------------------------------*/
   uint32_t bh_meb_trig_mode;    /* if this is set to NON zero a special mode is requested */
               /* for the trigger module serving m_read_meb.             */
               /* this could be:                                         */
               /*       NOT irq (LAM, VME irq)                           */
               /*    or NOT standard base address (VME)                  */
               /*    or NOT local crate                                  */
               /* if bh_meb_trig_mode is NOT 0 always the following two  */
               /* setup paramters will be taken for type and base addr.  */
               /*                                                        */
               /* NOTE: the fast clear and conversion time will be taken */
               /*       from bl_trig_fct[0] and bl_trig_cvt[0] from this */
               /*       setup structure                                  */
               /*                                                        */
               /*  0 = standard = local  interrupt (CAMAC LAM, VME IRQ)  */
               /*  1 = special:   local  interrupt (CAMAC LAM, VME IRQ)  */
               /*  2 = special:   remote VSB interrupt (not yet impleme) */
               /*  3 = special:   polling                                */
   uint32_t bh_special_meb_trig_type;
                              /*  1 = CAMAC   type trigger module                       */
                         /*  2 = VME     type trigger module                       */
                         /*  3 = FASTBUS type triggermodule                        */
   uint32_t bl_special_meb_trig_base;
                              /* base address of trigger module acting together with    */
                              /* m_read_meb if bh_meb_trig_mode != 0                    */
                              /* NOTE: if trigger module is remote the complete (VSB)   */
                              /*       base address must be specified. example:         */
                              /*       f0580000 for a CAMAC trigger module in crate 2   */
                              /*       serving m_read_meb running on an E7 in a VME     */
                              /*       crate. (see sketch at end of this file)          */
   /*---------------------------------------------------------------------------*/
   uint32_t lp_cvc_irq;       /* start address of the CVC irq physical segment          */
   uint32_t bl_cvc_irq_len;       /* length of CVC irq phys. segment                        */
   uint32_t bl_cvc_irq_source_off;/* offset from lp_cvc_irq to the CVC irq source register  */
   uint32_t bl_cvc_irq_mask_off;  /* offset from lp_cvc_irq to the CVC irq mask register    */
   /*---------------------------------------------------------------------------*/
   /* all values in this section will be initalized by a loader task */
   int32_t h_rd_tab_flg;      /* -1 = init and readout tables invalid (this is set by   */
               /*      the write_sbs_setup task                          */
               /* 0  = init and readout tables have to be modified to    */
               /*      virtual cnafs                                     */
               /* 1  = this modification is done and must not be done    */
               /*      before a new table was loaded                     */
   uint32_t bl_init_read_len;      /* total length of init and readout CAMAC cnaf list for   */
                         /* all crates and trigger types (in bytes)                */
   /* ------------------ end of fixed block --------------------------*/
   /*---------------------------------------------------------------------------*/
   uint32_t lp_rem_mem_base[SBS__N_CR];
             /* physical base address for accessing slave crate memory */
             /* seen from the MEB (pipes). This could be the VSB       */
             /* base address for accessing remote crates               */
   uint32_t bl_rem_mem_off[SBS__N_CR];
             /* offset from  lp_rem_mem_base[SBS__N_CR] to the memory  */
             /* space of the remote slaves (seen from MEB)             */
   uint32_t bl_rem_mem_len[SBS__N_CR];
             /* lenght of the memory address window with the physical  */
             /* base adress lp_rem_mem_base[SBS__N_CR]                 */
   /*---------------------------------------------------------------------------*/
   uint32_t lp_rem_cam_base[SBS__N_CR];
             /* physical base address for accessing slave directly via CAMAC */
   uint32_t bl_rem_cam_off[SBS__N_CR];
             /* offset from lp_rem_cam_base[SBS__N_CR] to the remote   */
             /* CAMAC space of the slaves (seen from MEB).             */
   uint32_t bl_rem_cam_len[SBS__N_CR];
             /* lenght of the CAMAC address window with the physical   */
   /*---------------------------------------------------------------------------*/
   uint32_t lp_loc_mem_base[SBS__N_CR];
             /* base address for accessing local crate. (could be for  */
             /* example local CAMAC base or extended VME base.)        */
   uint32_t bl_loc_mem_len[SBS__N_CR];
             /* length of physical segment for accessing local crate   */
             /* starting from lp_loc_mem_base[SBS__N_CR].              */
   /*---------------------------------------------------------------------------*/
   uint32_t l_loc_pipe_type[SBS__N_CR];
             /* type of subevent pipe:   0: with smem_create           */
             /*                          1: direct mapping             */
   uint32_t lp_loc_pipe_base[SBS__N_CR];
             /* RAM start address of the crate controller (used for    */
             /* location of subevent pipe)                             */
   uint32_t bl_pipe_off[SBS__N_CR];
             /* offset from lp_ram_loc_start to start of the remote    */
             /* communication segment slave <-> master seen from the   */
             /* slave in bytes = start of subevent pipe                */
   uint32_t bl_pipe_seg_len[SBS__N_CR];
             /* length of the remote communication segment in bytes    */
             /* = total lengt of subevent pipe                         */
   uint32_t bl_pipe_len[SBS__N_CR];      /* number of subevent slots in a pipe                     */
   uint32_t bh_controller_id[SBS__N_CR];
             /* bh_controller_id MUST be set for ESONE CNAF execution  */
             /* 1 = CVC, 2 = E6, 3 = E7, 4 = AEB, 5 = CBV, 6 = CVI     */
             /* 7 = CAV (Q and X inverted)                             */
   uint32_t bh_sy_asy_flg[SBS__N_CR];
             /* indicates if this crate must be readout synchronous    */
             /* or asynchronous: 0 = synchronous, 1 = asynchronous     */
                  /* this flag is only valid for crates with intelligent    */
                  /* controller. At this point it is decided wether a crate */
                  /* is readout sync. or async.                             */
   uint32_t bh_trig_stat_nr[SBS__N_CR];/* CAMAC station nr. of the trigger module, must be 1     */
   uint32_t bl_trig_cvt[SBS__N_CR];      /* conversion time of trigger module                      */
   uint32_t bl_trig_fct[SBS__N_CR];     /* fast clear acceptance time of trigger module           */
   int32_t i_se_typ[SBS__N_CR];     /* subevent typ, default = 10                             */
   int32_t i_se_subtyp[SBS__N_CR];     /* subevent subtyp: CAMAC = 1, FASTBUS = 2                */
   int32_t i_se_procid[SBS__N_CR];     /* subevent processor id                                  */
   /*---------------------------------------------------------------------------*/
   uint32_t bh_rd_flg[SBS__N_CR];
             /* 0 = crate not readout                                  */
             /* 1 = crate read out by meb                              */
             /* 2 = crate readout by intelligent crate controller      */
   uint32_t bl_init_tab_off[SBS__N_CR];
             /* offset from start of readout table segment to the      */
             /* start of the init table as a function of crate number  */
             /* (in longwords)                                         */
   uint32_t bi_init_tab_len[SBS__N_CR];
             /* lenght of the init tables. if one of this values is 0  */
             /* it means that this crate must not be not initalized    */
             /* (in longwords)                                         */
   uint32_t bl_max_se_len[SBS__N_CR][SBS__N_TRG_TYP];
             /* maximal subevent length [b] of this crate and trigger  */
   uint32_t bl_rd_tab_off[SBS__N_CR][SBS__N_TRG_TYP];
             /* offset from start of readout table segment to the      */
             /* start of the readout tables as a function of crate     */
             /* number and trigger type (in longwords)                 */
   uint32_t bi_rd_tab_len[SBS__N_CR][SBS__N_TRG_TYP];
             /* lenght of the readout tables. if one of this values    */
             /* is 0 it means that this crate for this trigger type is */
             /* not read out (in longwords)                            */
   /*---------------------------------------------------------------------------*/
   };

   // __________________________________________________________________________

   struct DaqStatus {
     enum {
        SBS__N_TRG_TYP = 16,
        SYS__N_MAX_PROCS = 30,
        SBS__STR_LEN_64 = 64
     };

     bool null() const { return l_version == 0; }
     void clear() { l_version = 0; }

     uint32_t l_endian;                    /* set to 1 if sent */
     uint32_t l_version;                   /* increment in f_ut_status_ini after changes */
     uint32_t l_daqst_lw;                  /* sizeof(s_daqst)/4 : number of lw */
     uint32_t l_fix_lw;                    /* (&c_pname-ps_daqst)/4 : fix number of longwords to read */
     uint32_t l_sys__n_max_procs;          /* maximum number of processes */
     uint32_t l_sbs__str_len_64;           /* String length of process names */
     uint32_t l_sbs__n_trg_typ;            /* maximum number of triggers */
     uint32_t bh_daqst_initalized;         /* crea_daqst */
     uint32_t bh_acqui_started;            /* util(f_ut_op_trig_mod), read_cam_slav, read_meb */
     uint32_t bh_acqui_running;            /* collector, read_cam_slav, read_meb  */
     uint32_t l_procs_run;                 /* processes running (index in l_pid) */
     uint32_t bh_setup_loaded;             /* util(f_ut_load_setup) */
     uint32_t bh_set_ml_loaded;            /* util(f_ut_load_ml_setup) */
     uint32_t bh_set_mo_loaded;            /* util(f_ut_load_mo_setup) */
     uint32_t bh_cam_tab_loaded;           /* read_cam_slav, read_meb) */
     int32_t  l_free_streams;              /* transport */
     uint32_t bl_n_events;                 /* collector */
     uint32_t bl_n_buffers;                /* collector f_col_format */
     uint32_t bl_n_bufstream;              /* transport */
     uint32_t bl_n_kbyte;                  /* transport */
     uint32_t bl_n_evserv_events;          /* event_serv f_ev_checkevt  */
     uint32_t bl_n_evserv_kbytes;          /* event_serv f_ev_send */
     uint32_t bl_n_strserv_bufs;           /* stream_serv  */
     uint32_t bl_n_strserv_kbytes;         /* stream_serv  */
     uint32_t bl_n_kbyte_tape;             /* transport  */
     uint32_t bl_n_kbyte_file;             /* transport */
     uint32_t bl_r_events;                 /* rate  */
     uint32_t bl_r_buffers;                /* rate  */
     uint32_t bl_r_bufstream;              /* rate */
     uint32_t bl_r_kbyte;                  /* rate  */
     uint32_t bl_r_kbyte_tape;             /* rate  (from l_block_count) */
     uint32_t bl_r_evserv_events;          /* rate */
     uint32_t bl_r_evserv_kbytes;          /* rate  */
     uint32_t bl_r_strserv_bufs;           /* rate  */
     uint32_t bl_r_strserv_kbytes;         /* rate */
     uint32_t bl_flush_time;               /* stream flush time                */
     int32_t  l_pathnum;                   /* path number of open device */
     uint32_t l_block_length;              /* current block length */
     uint32_t l_pos_on_tape;               /* current tape position in kB */
     uint32_t l_max_tape_size;             /* maximal tape length in kB */
     uint32_t l_file_count;                /* file count on volume */
     uint32_t l_file_auto;                 /* file count on volume */
     uint32_t l_file_cur;                  /* file count on volume */
     uint32_t l_file_size;                 /* file size */
     uint32_t l_block_count;               /* buffers on file */
     uint32_t l_block_size;                /* block size (=buffer) in bytes */
     uint32_t l_record_size;               /* record size on bytes */
     int32_t  l_open_vol;                  /* open mode of volume */
     int32_t  l_open_file;                 /* open  file flag */
     int32_t  l_rate_on;                   /* for m_daq_rate */
     int32_t  l_rate_sec;                  /* for m_daq_rate */
     uint32_t bh_trig_master;              /* util(f_ut_op_trig_mod) */
     uint32_t bh_histo_enable;             /* collector */
     uint32_t bh_histo_ready;              /* collector */
     uint32_t bh_ena_evt_copy;             /* collector */
     uint32_t bl_n_trig[SBS__N_TRG_TYP];   /* Trigger counter (read_cam_slav or read_meb) */
     uint32_t bl_n_si  [SBS__N_TRG_TYP];   /* Invalid subevents (collector) */
     uint32_t bl_n_evt [SBS__N_TRG_TYP];   /* Valid triggers (collector) */
     uint32_t bl_r_trig[SBS__N_TRG_TYP];   /* Rate Trigger counter (read_cam_slav or read_meb) */
     uint32_t bl_r_si  [SBS__N_TRG_TYP];   /* Rate Invalid subevents (collector) */
     uint32_t bl_r_evt [SBS__N_TRG_TYP];   /* Rate Valid triggers (collector) */
     uint32_t bh_running[SYS__N_MAX_PROCS];/* run bit for tasks */
     uint32_t l_pid[SYS__N_MAX_PROCS];     /* pid table */
     uint32_t l_type[SYS__N_MAX_PROCS];    /* Type number defined in sys_def.h */
     int32_t l_pprio[SYS__N_MAX_PROCS];   /* daq processes priority */
     /*   f_ut_init_daq_proc,  */
     /*   f_ut_clear_daqst,    */
     /*   f_ut_exit_daq_proc,  */
     /*   f_ut_show_acq,       */
     /*   dispatch,            */
     /*   prompt               */
     /*   tasks                */
     uint32_t bh_pact[SYS__N_MAX_PROCS];   /* daq processes, 1 = active, as pprio */
     uint32_t bh_verbose_flg;              /* */
     uint32_t bl_histo_port;                /* not used */
     uint32_t bl_run_time;                 /* not used */
     int32_t l_irq_driv_id;               /* 0=irq driver/device not installed */
     int32_t l_irq_maj_dev_id;            /*            "="                    */
     uint32_t bh_event_serv_ready;         /* event_serv, stream_serv */
     uint32_t bl_strsrv_scale;             /* stream server */
     uint32_t bl_strsrv_sync;              /* stream server  */
     uint32_t bl_strsrv_nosync;            /* stream server  */
     uint32_t bl_strsrv_keep;              /* stream server  */
     uint32_t bl_strsrv_nokeep;            /* stream server  */
     uint32_t bl_strsrv_scaled;            /* stream server  */
     uint32_t bl_evtsrv_scale;             /* event server  */
     uint32_t bl_evtsrv_events;            /* event server  */
     uint32_t bl_evtsrv_maxcli;            /* event server  */
     uint32_t bl_evtsrv_all;               /* event server  */
     uint32_t bl_esosrv_maxcli;            /* esone server  */
     uint32_t bl_pipe_slots;               /* sub event slots in readout pipe */
     uint32_t bl_pipe_slots_filled;        /* sub event slots used */
     uint32_t bl_pipe_size_KB;             /* readout pipe size */
     uint32_t bl_pipe_filled_KB;           /* readout pipe size occupied */
     uint32_t bl_spill_on;                 /* Spill on/off */
     uint32_t bl_delayed_eb_ena;           /* Delayed event building enabled/disab.*/
     uint32_t bl_event_build_on;           /* Event building on/off */
     uint32_t bl_dabc_enabled;             /* DABC event builder mode off/on */
     uint32_t bl_trans_ready;              /* transport server ready */
     uint32_t bl_trans_connected;          /* Client to transport connected */
     uint32_t bl_no_streams;               /* Number of streams */
     uint32_t bl_user[16];                /* for user */
     uint32_t bl_filler[190];              /* filler */
     uint32_t bl_no_stream_buf;             /* bufs per stream */
     uint32_t bl_rfio_connected;           /* RFIO connected */
     char c_user[SBS__STR_LEN_64];     /* username */
     char c_date[SBS__STR_LEN_64];     /* date of last update (m_daq_rate) */
     char c_exprun[SBS__STR_LEN_64];   /* run name */
     char c_exper[SBS__STR_LEN_64];    /* experiment */
     char c_host[SBS__STR_LEN_64];           /* name of host */
     char c_remote[SBS__STR_LEN_64];         /* name of remote control node */
     char c_display[SBS__STR_LEN_64];         /* name of remote display node */
     char c_anal_segm_name[SBS__STR_LEN_64]; /* name of histogram segment in use */
     /* by f_his_anal() in m_collector   */
     char c_setup_name[SBS__STR_LEN_64];     /* setup table loaded */
     char c_ml_setup_name[SBS__STR_LEN_64];  /* ml setup table loaded */
     char c_readout_name[SBS__STR_LEN_64];   /* readout table loaded */
     char c_pathstr[SBS__STR_LEN_64];        /* path string */
     char c_devname[SBS__STR_LEN_64];        /* Name of tape device */
     char c_tape_label[SBS__STR_LEN_64];     /* current tape label */
     char c_file_name[256];                  /* current file name */
     char c_out_chan[SBS__STR_LEN_64];       /* active ouput media */
     /* ------------------ end of fixed block --------------------------*/
     char c_pname[SYS__N_MAX_PROCS][SBS__STR_LEN_64]; /* as pprio */
   };


   struct StatusMO {
     enum { MO__N_NODE = 16 };

     uint32_t l_endian;                 /* must be 1 */
     uint32_t l_version;                /* structure version */
     uint32_t l_set_mo_lw;              /* length of structure */
     uint32_t l_swap_lw;                /* longwords to be swapped */
     uint32_t l_max_nodes;              /* set to MO__N_NODE */
     uint32_t l_no_senders;             /* actual number of senders */
     uint32_t l_no_receivers;           /* actual number of receivers */
     uint32_t bl_dr_active[MO__N_NODE]; /* active receivers */

     uint32_t l_rd_pipe_base_addr [MO__N_NODE] [MO__N_NODE];
     /* base addresses of all pipes residing on the ds nodes seen remote    */
     /* from dr nodes  l_rd_pipe_base_addr[dr_node_id][ds_node_id].         */
     /* this setup parametes are only  necessary for an address mapped      */
     /* transfer mode (l_trans_mode = 0).                                   */
     uint32_t l_rd_pipe_seg_len [MO__N_NODE];
     /* size of pipe segments on ds nodes. index is ds id. this information */
     /* is only necessary if transfer type 0 (single shot data transfer)    */
     /* from any of the ds nodes. !!!! WARNING !!!! this information is     */
     /* redundant. the size is also specified in the setup.usf or           */
     /* set_ml.usf of the ds node and must match.                           */
     uint32_t l_rd_pipe_trans_mode [MO__N_NODE] [MO__N_NODE];
     /* data transfer mode from ds nodes to dr nodes                            */
     /* l_rd_pipe_base_addr[dr_node_id][ds_node_id].                        */
     /* transfer mode: 0xxx = address mapped, dr reads data                 */
     /*                1xxx = message oriented mode ( tcp ), ds sends data  */
     /*                01xx = address mapped DMA modes (various)            */
     /*                                                                     */
     /*              0x   0 = standard singe cycle address mapped           */
     /*              0x 101 = VME DMA E7                                    */
     /*              0x 102 = VME DMA RIO2 <- RIO2                          */
     /*              0x 103 = VSB DMA RIO2 <- "any" VSB slave device        */
     /*              0x 104 = PVIC DMA RIO2                                 */
     /*              0x 105 = VME DMA RIO2 <- E7 / VME memory               */
     /*              0x1001 = TCP sockets                                   */
     uint32_t l_form_mode;
     /* two format modes have been forseen, whereas only mode 1 is imple-   */
     /* mented yet.                                                         */
     /* mode 1: ds node sends data, or dr node reads data (depending on     */
     /*         transfer type) to an intermediate event buffer of size      */
     /*         l_max_evt_size. formatting, and possible byte swapping is   */
     /*         done afterwards from the intermediate event buffer into the */
     /*         streams.                                                    */
     /* mode 2: ds node sends data, or dr node reads data and formats       */
     /*         immediately into the streams. not yet implemented           */
     uint32_t l_base_addr_evt_buf[MO__N_NODE];
     /* if l_form_mode = 1:       (intermediate formatting)                 */
     /* for transfer types where address mapped writing from ds node or dma */
     /* data transfer is done a shared segement is needed and created with  */
     /* physical base address l_base_addr_evt_buf and size l_max_evt_size.  */
     /* if l_form_mode = 2:       (direct formatting)                       */
     /* a shared segment for this buffer must also be created for           */
     /* direct formatting into streams if the ds node writes address mapped */
     /* or dma is done. this buffer will be used to hold an event if the    */
     /* actual stream has less space than l_max_evt_size since in this mode */
     /* the actual event size is not known before the transfer of the last  */
     /* fraction.                                                           */
     /*                                                                     */
     /* if l_base_addr_evt_buf != 0 an smem_create segment will be created  */
     /* if l_base_addr_evt_buf  = 0 ( or not specified) a buffer of size    */
     /* l_max_evt_size will be created with malloc and is therefore not     */
     /* contigious in the address space.                                    */
     /* MO__N_NODE runs over dr node ids.                                   */
     uint32_t l_max_evt_size;
     /* TOTAL maximum event size, which could occur within this setup.      */
     /* in bytes.                                                           */
     uint32_t l_n_frag;
     /* many subevents/fragments can be send/read in one transfer (i.e.     */
     /* tcp, vme block transfer) from m_ds to m_dr if the data is conscu-   */
     /* tive. l_n_frag specifies the number fragments sent in such a packet */
     uint32_t l_ev_buf_len [MO__N_NODE];
     uint32_t l_n_ev_buf [MO__N_NODE];
     uint32_t l_n_stream [MO__N_NODE];
     uint32_t l_base_addr_stream_seg [MO__N_NODE];
     /* only nedded if l_form mode = 2. see explanation above               */
     uint32_t l_out_mode [MO__N_NODE];
     char c_ds_hostname [MO__N_NODE][16];
     /* name of ds nodes */
     char c_dr_hostname [MO__N_NODE][16];
     /* name of dr nodes */

   };

#pragma pack(pop)

   enum EMbsBufferTypes {
      mbt_MbsEvents   = 103   // several mbs events without buffer header
   };

   // this is list of server kind, supported up to now by DABC
   enum EMbsServerKinds {
      NoServer = 0,
      TransportServer = 1,
      StreamServer = 2,
      OldTransportServer = 3,
      OldStreamServer = 4
   };

   extern const char* ServerKindToStr(int kind);
   extern int StrToServerKind(const char* str);
   extern int DefualtServerPort(int kind);

   extern void SwapData(void* data, unsigned bytessize);

   extern const char* protocolLmd;
   extern const char* protocolMbs;

   extern const char* comStartServer;
   extern const char* comStopServer;
   extern const char* comStartFile;
   extern const char* comStopFile;

   extern const char* xmlServerName;
   extern const char* xmlServerKind;
   extern const char* xmlServerPort;
   extern const char* xmlServerScale;
   extern const char* xmlServerLimit;

   extern const char* xmlTextDataFormat;
   extern const char* xmlTextNumData;
   extern const char* xmlTextHeaderLines;
   extern const char* xmlTextCharBuffer;

   extern const char* xmlCombineCompleteOnly;
   extern const char* xmlCheckSubeventIds;
   extern const char* xmlEvidMask;
   extern const char* xmlEvidTolerance;
   extern const char* xmlSpecialTriggerLimit;
   extern const char* xmlCombinerRatesPrefix;

};

#endif
