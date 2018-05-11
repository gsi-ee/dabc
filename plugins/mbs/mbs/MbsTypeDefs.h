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

#ifndef MBS_MbsTypeDefs
#define MBS_MbsTypeDefs

#ifndef MBS_LmdTypeDefs
#include "LmdTypeDefs.h"
#endif

#include <string>

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

      void InitFull(uint32_t fullid)
      {
         iWords = 0;
         iType = MBS_TYPE(10,1);
         fFullId = fullid;
      }

      void *RawData() const { return (char*) this + sizeof(SubeventHeader); }

      // RawDataSize - size of raw data without subevent header
      uint32_t RawDataSize() const { return FullSize() - sizeof(SubeventHeader); }
      void SetRawDataSize(uint32_t sz) { SetFullSize(sz + sizeof(SubeventHeader)); }

      int16_t ProcId() const { return iProcId; }
      int8_t Control() const { return iControl; }
      int8_t Subcrate() const { return iSubcrate; }

      /** Prints sub-event header */
      void PrintHeader();

      /** Prints sub-event data in hex/decimal and long/short form */
      void PrintData(bool ashex = true, bool aslong = true);
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

      inline uint16_t TriggerNumber() const { return iTrigger; }

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

      void PrintHeader();
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
            int8_t   h_begin;  /*  Fragment at begin of buffer */
            int8_t   h_end;    /*  Fragment at end of buffer */
#else
            int8_t   h_end;    /*  Fragment at end of buffer */
            int8_t   h_begin;  /*  Fragment at begin of buffer */
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
   extern int StrToServerKind(const std::string &str);
   extern int DefualtServerPort(int kind);

   extern void SwapData(void* data, unsigned bytessize);

   extern const char* protocolLmd;
   extern const char* protocolMbs;
   extern const char* protocolMbss;
   extern const char* protocolMbst;

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
