// N.Kurz, EE, GSI, 15-Jan-2010

//-------------------------------------------------------------
//        Go4 Release Package v3.03-05 (build 30305)
//                      05-June-2008
//---------------------------------------------------------------
//   The GSI Online Offline Object Oriented (Go4) Project
//   Experiment Data Processing at EE department, GSI
//---------------------------------------------------------------
//
//Copyright (C) 2000- Gesellschaft f. Schwerionenforschung, GSI
//                    Planckstr. 1, 64291 Darmstadt, Germany
//Contact:            http://go4.gsi.de
//----------------------------------------------------------------
//This software can be used under the license agreements as stated
//in Go4License.txt file which is part of the distribution.
//----------------------------------------------------------------
#include "TWRcheckBasicProc.h"
#include "stdint.h"

#include "Riostream.h"
using namespace std;

#include "TH1.h"
#include "TH2.h"
#include "TCutG.h"
#include "snprintf.h"

#include "TGo4MbsEvent.h"
//#include "TGo4WinCond.h"
//#include "TGo4PolyCond.h"
//#include "TGo4CondArray.h"
//#include "TGo4Picture.h"
//#include "TGo4Fitter.h"

#ifdef USE_MBS_PARAM
 static UInt_t    l_tr    [MAX_TRACE_SIZE];
 static Double_t  f_tr_blr[MAX_TRACE_SIZE];
 static UInt_t    l_sfp_slaves[MAX_SFP] = {MAX_SLAVE, MAX_SLAVE, MAX_SLAVE, MAX_SLAVE};
 static UInt_t    l_slaves=0;
 static UInt_t    l_trace=0;
 static UInt_t    l_e_filt=0;
#else
 static UInt_t    l_tr    [TRACE_SIZE];
 static Double_t  f_tr_blr[TRACE_SIZE];
 static UInt_t    l_sfp_slaves  [MAX_SFP] = NR_SLAVES;
 static UInt_t    l_sfp_adc_type[MAX_SFP] = ADC_TYPE;
#endif //

static UInt_t    l_adc_type;
static UInt_t    l_more_1_hit_ct=0;

static UInt_t    l_first=0;




//***********************************************************
TWRcheckBasicProc::TWRcheckBasicProc() : TGo4EventProcessor("Proc"),l_wr_timestamp(0), l_wr_timestamp_prev(0), f_wr_delta_t(0)
{
  cout << "**** TWRcheckBasicProc: Create instance " << endl;
}
//***********************************************************
TWRcheckBasicProc::~TWRcheckBasicProc()
{
  cout << "**** TWRcheckBasicProc: Delete instance " << endl;
}
//***********************************************************
// this one is used in standard factory
TWRcheckBasicProc::TWRcheckBasicProc(const char *name) : TGo4EventProcessor(name)
{
  cout << "**** TWRcheckBasicProc: Create instance " << name << endl;
 TString obname;
  TString obtitle;
  h_wr_delta_t= MakeTH1('F', "WR_Delta_time", "WR Time difference subsequent events",50000,0,200000,"#Delta t (ns)");



for(int i=0; i<NUM_TS_SUBCONTROL;++i)
	{

	 obname.Form("WR/WR_Delta_time_subctrl%d", i);
    obtitle.Form("White Rabbit subsequent dt within subevent of ctrlid=%d" , i);
    h_wr_delta_t_subevent[i]= MakeTH1('F', obname.Data(), obtitle.Data(),50000,0,200000,"#Delta t (ns)");
	}

  l_first=0;
  //printf ("Histograms created \n");  fflush (stdout);
}
//-----------------------------------------------------------
// event function
Bool_t TWRcheckBasicProc::BuildEvent(TGo4EventElement* target)
{  // called by framework. We dont fill any output event here at all

  UInt_t         l_i, l_j, l_k, l_l;
  uint32_t      *pl_se_dat;
  uint32_t      *pl_tmp;

  UInt_t         l_dat_len;
  UInt_t         l_dat_len_byte;

  UInt_t         l_dat;
  UInt_t         l_trig_type;
  UInt_t         l_trig_type_triva;
  UInt_t         l_sfp_id;
  UInt_t         l_feb_id;
  UInt_t         l_cha_id;
  UInt_t         l_n_hit;
  UInt_t         l_hit_id;
  UInt_t         l_hit_cha_id;
  Long64_t       ll_time;
  Long64_t       ll_trg_time;
  Long64_t       ll_hit_time;
  UInt_t         l_ch_hitpat   [MAX_SFP][MAX_SLAVE][N_CHA];
  UInt_t         l_ch_hitpat_tr[MAX_SFP][MAX_SLAVE][N_CHA];
  UInt_t         l_first_trace [MAX_SFP][MAX_SLAVE];

  UInt_t         l_cha_head;
  UInt_t         l_cha_size;
  UInt_t         l_trace_head;
  UInt_t         l_trace_size;
  UInt_t         l_trace_trail;

  UInt_t         l_spec_head;
  UInt_t         l_spec_trail;
  UInt_t         l_n_hit_in_cha;
  UInt_t         l_only_one_hit_in_cha;
  UInt_t         l_more_than_1_hit_in_cha;
  UInt_t         l_hit_time_sign;
   Int_t         l_hit_time;
  UInt_t         l_hit_cha_id2;
  UInt_t         l_fpga_energy_sign;
   Int_t         l_fpga_energy;

  UInt_t         l_trapez_e_found [MAX_SFP][MAX_SLAVE][N_CHA];
  UInt_t         l_fpga_e_found   [MAX_SFP][MAX_SLAVE][N_CHA];
  UInt_t         l_trapez_e       [MAX_SFP][MAX_SLAVE][N_CHA];
  UInt_t         l_fpga_e         [MAX_SFP][MAX_SLAVE][N_CHA];

  UInt_t         l_dat_fir;
  UInt_t         l_dat_sec;

  static UInt_t  l_evt_ct=0;
  static UInt_t  l_evt_ct_phys=0;

  UInt_t         l_bls_start = BASE_LINE_SUBT_START;
  UInt_t         l_bls_stop  = BASE_LINE_SUBT_START + BASE_LINE_SUBT_SIZE; //
  Double_t       f_bls_val=0.;

  Int_t       l_fpga_filt_on_off;
  Int_t       l_fpga_filt_mode;
  Int_t       l_dat_trace;
  Int_t       l_dat_filt;
  Int_t       l_filt_sign;



  TGo4MbsSubEvent* psubevt;

  fInput = (TGo4MbsEvent* ) GetInputEvent();
  if(fInput == 0)
  {
    cout << "AnlProc: no input event !"<< endl;
    return kFALSE;
  }

  l_trig_type_triva = fInput->GetTrigger();
  if (l_trig_type_triva == 1)
  {
     l_evt_ct_phys++;
  }

  //if(fInput->GetTrigger() > 11)
  //{
  //cout << "**** TWRcheckBasicProc: Skip trigger event"<<endl;
  //return kFALSE;
  //}
  // first we fill the arrays fCrate1,2 with data from MBS source
  // we have up to two subevents, crate 1 and 2
  // Note that one has to loop over all subevents and select them by
  // crate number:   psubevt->GetSubcrate(),
  // procid:         psubevt->GetProcid(),
  // and/or control: psubevt->GetControl()
  // here we use only crate number

  l_evt_ct++;

  fInput->ResetIterator();
  while((psubevt = fInput->NextSubEvent()) != 0) // loop over subevents
  {
	Int_t controlid= psubevt->GetControl();
  //psubevt = fInput->NextSubEvent(); // only one subevent
  //if(psubevt==0) return kTRUE; //JAM data error handling without crashing go4


  //printf ("         psubevt: 0x%x \n", (UInt_t)psubevt); fflush (stdout);
  //printf ("-------------------------------next event-----------\n");
  //sleep (1);

  pl_se_dat = (uint32_t *)psubevt->GetDataField();
  l_dat_len = psubevt->GetDlen();
  l_dat_len_byte = (l_dat_len - 2) * 2;
  //printf ("sub-event data size:         0x%x, %d \n", l_dat_len, l_dat_len);
  //printf ("sub-event data size (bytes): 0x%x, %d \n", l_dat_len_byte, l_dat_len_byte);
  //fflush (stdout);

  pl_tmp = pl_se_dat;

  if (pl_se_dat == (UInt_t*)0)
  {
    printf ("ERROR>> ");
    printf ("pl_se_dat: 0x%x, ", pl_se_dat);
    printf ("l_dat_len: 0x%x, ", (UInt_t)l_dat_len);
    printf ("l_trig_type_triva: 0x%x \n", (UInt_t)l_trig_type_triva); fflush (stdout);
    goto bad_event;
  }

  if ( (*pl_tmp) == 0xbad00bad)
  {
    printf ("ERROR>> found bad event (0xbad00bad) \n");
    goto bad_event;
  }

  #ifdef WR_TIME_STAMP
  // 4 first 32 bits must be TITRIS time stamp
  l_dat = *pl_tmp++;
  if (l_dat != SUB_SYSTEM_ID)
  {
    //printf ("ERROR>> 1. data word is not sub-system id: %d \n");
    //printf ("should be: 0x%x, but is: 0x%x\n", SUB_SYSTEM_ID, l_dat);
  }

//  if (l_dat != 0x100)
//  {
//    goto bad_event;
//  }
//
//  l_dat = (*pl_tmp++) >> 16;
//  if (l_dat != TS__ID_L16)
//  {
//    printf ("ERROR>> 2. data word does not contain low 16bit identifier: %d \n");
//    printf ("should be: 0x%x, but is: 0x%x\n", TS__ID_L16, l_dat);
//  }
//  l_dat = (*pl_tmp++) >> 16;
//  if (l_dat != TS__ID_M16)
//  {
//    printf ("ERROR>> 3. data word does not contain low 16bit identifier: %d \n");
//    printf ("should be: 0x%x, but is: 0x%x\n", TS__ID_M16, l_dat);
//  }
//  l_dat = (*pl_tmp++) >> 16;
//	if (l_dat != TS__ID_H16)
//  {
//    printf ("ERROR>> 4. data word does not contain low 16bit identifier: %d \n");
//    printf ("should be: 0x%x, but is: 0x%x\n", TS__ID_H16, l_dat);
//  }

// new format JAM22:
//  TS__ID_L16         0x3e1
//  #define TS__ID_M16         0x4e1
//  #define TS__ID_H16         0x5e1
//  #define TS__ID_X16         0x6e1

	  l_dat = (*pl_tmp) >> 16;
	  if (l_dat != TS__ID_L16)
	  {
	    printf ("ERROR>> 2. data word does not contain low 16bit identifier:\n");
	    printf ("should be: 0x%x, but is: 0x%x\n", TS__ID_L16, l_dat);
	  }
	  l_wr_timestamp = *pl_tmp++ & 0xFFFF;

	  l_dat = (*pl_tmp) >> 16;
	  if (l_dat != TS__ID_M16)
	  {
	    printf ("ERROR>> 3. data word does not contain mid 16bit identifier:\n");
	    printf ("should be: 0x%x, but is: 0x%x\n", TS__ID_M16, l_dat);
	  }
	  l_wr_timestamp +=( (*pl_tmp++ & 0xFFFF) << 16);

	  l_dat = (*pl_tmp) >> 16;
	  if (l_dat != TS__ID_H16)
	  {
	    printf ("ERROR>> 4. data word does not contain high 16bit identifier:\n");
	    printf ("should be: 0x%x, but is: 0x%x\n", TS__ID_H16, l_dat);
	  }
	  l_wr_timestamp +=((*pl_tmp++ & 0xFFFF)<< 32);

	  l_dat = (*pl_tmp) >> 16;
	  if (l_dat != TS__ID_X16)
	      {
	      printf ("ERROR>> 5. data word does not contain xhigh 16bit identifier:\n");
	      printf ("should be: 0x%x, but is: 0x%x\n", TS__ID_X16, l_dat);
	      }
	  l_wr_timestamp +=((*pl_tmp++ & 0xFFFF)<< 48);

	  if(l_wr_timestamp_prev)
	      {
		  f_wr_delta_t=l_wr_timestamp - l_wr_timestamp_prev;
		  h_wr_delta_t->Fill(f_wr_delta_t);
	      }
	  l_wr_timestamp_prev=l_wr_timestamp;

// JAM24: add check within each subevent

	if(controlid >=0 && controlid<NUM_TS_SUBCONTROL)
	{
	 if(l_wr_ts_prev[controlid])
	      {
			   f_wr_delta_t_subevent[controlid]=l_wr_timestamp - l_wr_ts_prev[controlid];
			   h_wr_delta_t_subevent[controlid]->Fill(f_wr_delta_t_subevent[controlid]);
		  }
	l_wr_ts_prev[controlid]=l_wr_timestamp;
	}




  #endif // WR_TIME_STAMP


 if( controlid != 8) continue; // do not evaluate trace data unless we have febex subevent
// timestamps are taken from both subevents.


  // extract analysis parameters from MBS data
  // ATTENTION:  these data is only present if WRITE_ANALYSIS_PARAM
  //             is enabled in corresponding f_user.c
  // WRITE_ANALYSIS_PARAM (in mbs) and USE_MBS_PARAM (in go4) must be used always together

  #ifdef USE_MBS_PARAM
  l_slaves = *pl_tmp++;
  l_trace  = *pl_tmp++;
  l_e_filt = *pl_tmp++;
  pl_tmp  += 4;
  #endif

  if (l_first == 0)
  {
    l_first = 1;
    #ifdef USE_MBS_PARAM
    printf ("debug: 0x%x, 0x%x, 0x%x \n", l_slaves, l_trace, l_e_filt);
    fflush (stdout);
    #endif
    f_make_histo (0);
  }

  for (l_i=0; l_i<MAX_SFP; l_i++)
  {
    if (l_sfp_slaves[l_i] != 0)
    {
      for (l_j=0; l_j<l_sfp_slaves[l_i]; l_j++)
      {
        for (l_k=0; l_k<N_CHA; l_k++)
        {
          h_trace       [l_i][l_j][l_k]->Reset ("");
          h_trace_blr   [l_i][l_j][l_k]->Reset ("");
          h_trapez_fpga [l_i][l_j][l_k]->Reset ("");
          l_ch_hitpat   [l_i][l_j][l_k] = 0;
          l_ch_hitpat_tr[l_i][l_j][l_k] = 0;
          l_fpga_e_found[l_i][l_j][l_k] = 0;
          l_trapez_e    [l_i][l_j][l_k] = 0;
          l_fpga_e      [l_i][l_j][l_k] = 0;
        }
        h_hitpat     [l_i][l_j]->Fill (-2, 1);
        h_hitpat_tr  [l_i][l_j]->Fill (-2, 1);
        l_first_trace[l_i][l_j] = 0;
      }
    }
  }

  while ( (pl_tmp - pl_se_dat) < (l_dat_len_byte/4) )
  {
    //sleep (1);
    //printf (" begin while loop \n");  fflush (stdout);
    l_dat = *pl_tmp++;   // must be padding word or channel header
    //printf ("l_dat 0x%x \n", l_dat);
    if ( (l_dat & 0xfff00000) == 0xadd00000 ) // begin of padding 4 byte words
    {
      //printf ("padding found \n");
      l_dat = (l_dat & 0xff00) >> 8;
      pl_tmp += l_dat - 1;  // increment by pointer by nr. of padding  4byte words
    }
    else if ( (l_dat & 0xff) == 0x34) //channel header
    {
      l_cha_head = l_dat;
      //printf ("l_cha_head: 0x%x \n", l_cha_head);

      l_trig_type = (l_cha_head & 0xf00)      >>  8;
      l_sfp_id    = (l_cha_head & 0xf000)     >> 12;
      l_feb_id    = (l_cha_head & 0xff0000)   >> 16;
      l_cha_id    = (l_cha_head & 0xff000000) >> 24;

      if ((l_sfp_id > (MAX_SFP-1)) || (l_sfp_id < 0))
      {
        printf ("ERROR>> l_spf_id: %d \n", l_sfp_id);  fflush (stdout);
        goto bad_event;
      }
      if ((l_feb_id > (MAX_SLAVE-1)) || (l_feb_id < 0))
      {
        printf ("ERROR>> l_feb_id: %d \n", l_feb_id); fflush (stdout);
        goto bad_event;
      }
      if ((l_cha_id > (N_CHA-1)) || (l_cha_id < 0))
      {
        if (l_cha_id != 0xff)
        {
          printf ("ERROR>> l_cha_id: %d \n", l_cha_id); fflush (stdout);
          goto bad_event;
        }
      }

      if ( ((l_cha_head & 0xff) >> 0) != 0x34 )
      {
        printf ("ERROR>> channel header type is not 0x34 \n");
        goto bad_event;
      }

      if ( (l_cha_head & 0xff000000) == 0xff000000) // special channel 0xff for E,t from fpga
      {
        //printf ("special channel \n");
        // special channel data size
        l_cha_size = *pl_tmp++;
        //printf ("l_cha_head: 0x%x \n", l_cha_head); sleep (1);
        //printf ("l_cha_size: 0x%x \n", l_cha_size);

        l_spec_head = *pl_tmp++;
        if ( (l_spec_head & 0xff000000) != 0xaf000000)
        {
          printf ("ERROR>> E,t summary: wrong header is 0x%x, must be: 0x%x\n",
                                                 (l_spec_head & 0xff000000)>>24, 0xaf);
          goto bad_event;
          //sleep (1);
        }
        ll_trg_time  = (Long64_t)*pl_tmp++;
        ll_time      = (Long64_t)*pl_tmp++;
        ll_trg_time += ((ll_time & 0xffffff) << 32);

        l_n_hit = (l_cha_size - 16) >> 3;
        //printf ("#hits: %d \n", l_n_hit);
        if (l_trig_type_triva == 1) // physics event
        {
          h_hitpat[l_sfp_id][l_feb_id]->Fill (-1, 1);

          for (l_i=0; l_i<l_n_hit; l_i++)
          {
            l_dat = *pl_tmp++;      // hit time from fpga (+ other info)
            l_hit_cha_id             = (l_dat & 0xf0000000) >> 28;
            l_n_hit_in_cha           = (l_dat & 0xf000000)  >> 24;

            l_more_than_1_hit_in_cha = (l_dat & 0x400000)   >> 22;
            l_only_one_hit_in_cha    = (l_dat & 0x100000)   >> 20;

            l_ch_hitpat[l_sfp_id][l_feb_id][l_hit_cha_id] = l_n_hit_in_cha;

            if (l_more_than_1_hit_in_cha == 1)
            {
              l_more_1_hit_ct++;
              printf ("%d More than 1 hit found for SFP: %d FEBEX: %d CHA: %d:: %d \n",
                      l_more_1_hit_ct, l_sfp_id, l_feb_id, l_hit_cha_id, l_n_hit_in_cha);
              fflush (stdout);
            }

            if ((l_more_than_1_hit_in_cha == 1) && (l_only_one_hit_in_cha == 1))
            {
              printf ("ERROR>> haeh? \n"); fflush (stdout);
            }

            if (l_only_one_hit_in_cha == 1)
            {
              l_hit_time_sign = (l_dat & 0x8000) >> 15;
              l_hit_time = l_dat & 0x7ff;     // positive := AFTER  trigger, relative to trigger time
              if (l_hit_time_sign == 1)       // negative sign
              {
                l_hit_time = l_hit_time * (-1); // negative := BEFORE trigger, relative to trigger time
              }
              //printf ("cha: %d, hit fpga time:  %d \n", l_hit_cha_id,  l_hit_time);
              h_trgti_hitti[l_sfp_id][l_feb_id][l_hit_cha_id]->Fill (l_hit_time);
              //h_hitpat[l_sfp_id][l_feb_id]->Fill (l_hit_cha_id, 1);
            }
            h_hitpat[l_sfp_id][l_feb_id]->Fill (l_hit_cha_id, l_n_hit_in_cha);

            l_dat = *pl_tmp++;      // energy from fpga (+ other info)
            l_hit_cha_id2  = (l_dat & 0xf0000000) >> 28;
            if (l_hit_cha_id != l_hit_cha_id2)
            {
              printf ("ERROR>> hit channel ids differ in energy and time data word\n");
              goto bad_event;
            }
            if ((l_hit_cha_id > (N_CHA-1)) || (l_hit_cha_id < 0))
            {
              printf ("ERROR>> hit channel id: %d \n", l_hit_cha_id); fflush (stdout);
              goto bad_event;
            }
            if (l_only_one_hit_in_cha == 1)
            {
              l_fpga_energy_sign = (l_dat & 0x800000) >> 23;
              //l_fpga_energy      =  l_dat & 0x7ffff;      // positiv
              l_fpga_energy      =  l_dat & 0x3fffff;     // positiv
              if (l_fpga_energy_sign == 1)                // negative sign
              {
                l_fpga_energy = l_fpga_energy * (-1);     // negative
              }
              //printf ("cha: %d, hit fpga energy: %d \n", l_hit_cha_id2,  l_fpga_energy);
              //printf ("sfp: %d, feb: %d, cha: %d \n", l_sfp_id, l_feb_id, l_hit_cha_id);
              h_fpga_e[l_sfp_id][l_feb_id][l_hit_cha_id]->Fill (l_fpga_energy);
              l_fpga_e_found [l_sfp_id][l_feb_id][l_hit_cha_id] = 1;
              l_fpga_e[l_sfp_id][l_feb_id][l_hit_cha_id] = l_fpga_energy;
            }
          }
        }
        l_spec_trail = *pl_tmp++;
        if ( (l_spec_trail & 0xff000000) != 0xbf000000)
        {
          printf ("ERROR>> E,t summary: wrong header is 0x%x, must be: 0x%x\n",
                                                 (l_spec_trail & 0xff000000)>>24, 0xbf);
          goto bad_event;
          //sleep (1);
        }
      }
      else // real channel
      {
        //printf ("real channel \n");
        // channel data size
        l_cha_size = *pl_tmp++;

        // trace header
        l_trace_head = *pl_tmp++;
        //printf ("trace header \n");
        if ( ((l_trace_head & 0xff000000) >> 24) != 0xaa)
        {
          printf ("ERROR>> trace header id is not 0xaa \n");
          goto bad_event;
        }

        l_fpga_filt_on_off = (l_trace_head & 0x80000) >> 19;
        l_fpga_filt_mode   = (l_trace_head & 0x40000) >> 18;
        //printf ("fpga filter on bit: %d, fpga filter mode: %d \n", l_fpga_filt_on_off, l_fpga_filt_mode);
        //fflush (stdout);
        //sleep (1);

        if (l_trig_type == 1) // physics event
        {
          if (l_first_trace[l_sfp_id][l_feb_id] == 0)
          {
            l_first_trace[l_sfp_id][l_feb_id] = 1;
            h_hitpat_tr[l_sfp_id][l_feb_id]->Fill (-1, 1);
          }
          h_hitpat_tr[l_sfp_id][l_feb_id]->Fill (l_cha_id, 1);
          l_ch_hitpat_tr[l_sfp_id][l_feb_id][l_cha_id]++;

          // now trace
          l_trace_size = (l_cha_size/4) - 2;     // in longs/32bit

          //das folgende kommentierte noch korrigieren!
          //falls trace + filter trace: cuttoff bei 2000 slices, da 4fache datenmenge!

          //if (l_trace_size != (TRACE_SIZE>>1))
          //{
          //  printf ("ERROR>> l_trace_size: %d \n", l_trace_size); fflush (stdout);
          //  goto bad_event;
          //}

          if (l_fpga_filt_on_off == 0) // only trace. no fpga filter trace data
          {
            for (l_l=0; l_l<l_trace_size; l_l++)   // loop over traces
            {
              // disentangle data
              l_dat_fir = *pl_tmp++;
              l_dat_sec = l_dat_fir;

              #ifdef USE_MBS_PARAM
              l_adc_type = (l_trace_head & 0x800000) >> 23;
              #else
              l_adc_type = (l_sfp_adc_type[l_sfp_id] >> l_feb_id) & 0x1;
              #endif

              if (l_adc_type == 0) // 12 bit
              {
                l_dat_fir =  l_dat_fir        & 0xfff;
                l_dat_sec = (l_dat_sec >> 16) & 0xfff;
              }

              if (l_adc_type == 1)  // 14 bit
              {
                l_dat_fir =  l_dat_fir        & 0x3fff;
                l_dat_sec = (l_dat_sec >> 16) & 0x3fff;
              }
              h_trace[l_sfp_id][l_feb_id][l_cha_id]->SetBinContent (l_l*2  +1, l_dat_fir);
              h_trace[l_sfp_id][l_feb_id][l_cha_id]->SetBinContent (l_l*2+1+1, l_dat_sec);

              l_tr[l_l*2]   = l_dat_fir;
              l_tr[l_l*2+1] = l_dat_sec;
            }
            l_trace_size = l_trace_size * 2;
          }

          if (l_fpga_filt_on_off == 1) // trace AND fpga filter data
          {
            for (l_l=0; l_l<(l_trace_size>>1); l_l++)   // loop over traces
            {
              // disentangle data
              l_dat_trace = *pl_tmp++;
              l_dat_filt  = *pl_tmp++;
              l_filt_sign  =  (l_dat_filt & 0x800000) >> 23;

              #ifdef USE_MBS_PARAM
              l_adc_type = (l_trace_head & 0x800000) >> 23;
              #else
              l_adc_type = (l_sfp_adc_type[l_sfp_id] >> l_feb_id) & 0x1;
              #endif

              if (l_adc_type == 0) // 12 bit
              {
                l_dat_trace = l_dat_trace  & 0xfff;
              }

              if (l_adc_type == 1)  // 14 bit
              {
                l_dat_trace = l_dat_trace  & 0x3fff;
              }

              l_dat_filt  = l_dat_filt   & 0x7fffff;
              if (l_filt_sign == 1) {l_dat_filt = l_dat_filt * -1;}

              h_trace      [l_sfp_id][l_feb_id][l_cha_id]->SetBinContent (l_l+1, l_dat_trace);
              h_trapez_fpga[l_sfp_id][l_feb_id][l_cha_id]->SetBinContent (l_l+1, l_dat_filt);

              l_tr[l_l] = l_dat_trace;
            }
            l_trace_size = l_trace_size >> 1;
          }

          // find base line value of trace and correct it to baseline 0
          f_bls_val = 0.;
          for (l_l=l_bls_start; l_l<l_bls_stop; l_l++)
          {
            f_bls_val += (Double_t)l_tr[l_l];
          }
          f_bls_val = f_bls_val / (Double_t)(l_bls_stop - l_bls_start);
          for (l_l=0; l_l<l_trace_size; l_l++)   // create baseline restored trace
          {
            f_tr_blr[l_l] =  (Double_t)l_tr[l_l] - f_bls_val;
            h_trace_blr[l_sfp_id][l_feb_id][l_cha_id]->Fill (l_l, f_tr_blr[l_l]);
            //h_trace_blr[l_sfp_id][l_feb_id][l_cha_id]->SetBinContent (l_l+1, f_tr_blr[l_l]);
          }

          // find peak and fill histogram
          h_peak  [l_sfp_id][l_feb_id][l_cha_id]->Fill (h_trace[l_sfp_id][l_feb_id][l_cha_id]->GetMaximum ());
          h_valley[l_sfp_id][l_feb_id][l_cha_id]->Fill (h_trace[l_sfp_id][l_feb_id][l_cha_id]->GetMinimum ());
        }

        // jump over trace
        //pl_tmp += (l_cha_size >> 2) - 2;

        // trace trailer
        //printf ("trace trailer \n");
        l_trace_trail = *pl_tmp++;
        if ( ((l_trace_trail & 0xff000000) >> 24) != 0xbb)
        {
          printf ("ERROR>> trace trailer id is not 0xbb, ");
          printf ("SFP: %d, FEB: %d, CHA: %d \n", l_sfp_id, l_feb_id, l_cha_id);
          goto bad_event;
        }
      }
    }

    else
    {
      //printf ("ERROR>> data word neither channel header nor padding word \n");
    }
  }


  for (l_i=0; l_i<MAX_SFP; l_i++)
  {
    if (l_sfp_slaves[l_i] != 0)
    {
      for (l_j=0; l_j<l_sfp_slaves[l_i]; l_j++)
      {
        for (l_k=0; l_k<N_CHA; l_k++)
        {
          h_ch_hitpat   [l_i][l_j][l_k]->Fill (l_ch_hitpat   [l_i][l_j][l_k]);
          h_ch_hitpat_tr[l_i][l_j][l_k]->Fill (l_ch_hitpat_tr[l_i][l_j][l_k]);
        }
      }
    }
  }


} // while subevents

bad_event:

  return kTRUE;
}

//--------------------------------------------------------------------------------------------------------

void TWRcheckBasicProc:: f_make_histo (Int_t l_mode)
{
  Text_t chis[256];
  Text_t chead[256];
  UInt_t l_i, l_j, l_k;
  UInt_t l_tra_size;
  UInt_t l_trap_n_avg;
  UInt_t l_left;
  UInt_t l_right;

  #ifdef USE_MBS_PARAM
  l_tra_size   = l_trace & 0xffff;
  l_trap_n_avg = l_e_filt >> 21;
  printf ("f_make_histo: trace size: %d, avg size %d \n", l_tra_size, l_trap_n_avg);
  fflush (stdout);
  l_sfp_slaves[0] =  l_slaves & 0xff;
  l_sfp_slaves[1] = (l_slaves & 0xff00)     >>  8;
  l_sfp_slaves[2] = (l_slaves & 0xff0000)   >> 16;
  l_sfp_slaves[3] = (l_slaves & 0xff000000) >> 24;
  printf ("f_make_histo: # of sfp slaves: 3:%d, 2:%d, 1: %d, 0: %d \n",
          l_sfp_slaves[3], l_sfp_slaves[2], l_sfp_slaves[1], l_sfp_slaves[0]);
  fflush (stdout);
  #else
  l_tra_size   = TRACE_SIZE;
  l_trap_n_avg = TRAPEZ_N_AVG;
  #endif // USE_MBS_PARAM

  for (l_i=0; l_i<MAX_SFP; l_i++)
  {
    if (l_sfp_slaves[l_i] != 0)
    {
      for (l_j=0; l_j<l_sfp_slaves[l_i]; l_j++)
      {
        for (l_k=0; l_k<N_CHA; l_k++)
        {
          sprintf(chis,"Traces/TRACE  SFP: %2d FEBEX: %2d CHAN: %2d", l_i, l_j, l_k);
          sprintf(chead,"Trace");
          h_trace[l_i][l_j][l_k] = MakeTH1('I', chis,chead,l_tra_size,0,l_tra_size);
        }

        for (l_k=0; l_k<N_CHA; l_k++)
        {
          sprintf(chis,"Traces BLR/TRACE, base line restored SFP: %2d FEBEX: %2d CHAN: %2d", l_i, l_j, l_k);
          sprintf(chead,"Trace, base line restored");
          h_trace_blr[l_i][l_j][l_k] = MakeTH1('F', chis,chead,l_tra_size,0,l_tra_size);
        }

        for (l_k=0; l_k<N_CHA; l_k++)
        {
          sprintf(chis,"FPGA/FPGA Trapez SFP: %2d FEBEX: %2d CHAN: %2d", l_i, l_j, l_k);
          sprintf(chead,"FPGA Trapez");
          h_trapez_fpga[l_i][l_j][l_k] = MakeTH1('F', chis,chead,l_tra_size,0,l_tra_size);
        }

        for (l_k=0; l_k<N_CHA; l_k++)
        {
          sprintf(chis,"FPGA/FPGA Energy(hitlist) SFP: %2d FEBEX: %2d CHAN: %2d", l_i, l_j, l_k);
          sprintf(chead,"FPGA Energy");
          l_right = 0x1000 * l_trap_n_avg;
          l_left = -1 * l_right;
          //printf ("depp: %d %d\n", l_left, l_right); fflush (stdout);
          //h_fpga_e[l_i][l_j][l_k] = MakeTH1('F', chis,chead,0x1000,l_left,l_right);
          h_fpga_e[l_i][l_j][l_k] = MakeTH1('F', chis,chead,40000,-400000,400000);
        }

        for (l_k=0; l_k<N_CHA; l_k++)
        {
          sprintf(chis,"Peaks/PEAK   SFP: %2d FEBEX: %2d CHAN: %2d", l_i, l_j, l_k);
          sprintf(chead,"Peak");
          h_peak[l_i][l_j][l_k] = MakeTH1('I', chis,chead,0x1000,0,0x4000);
        }

        for (l_k=0; l_k<N_CHA; l_k++)
        {
          sprintf(chis,"Valleys/VALLEY   SFP: %2d FEBEX: %2d CHAN: %2d", l_i, l_j, l_k);
          sprintf(chead,"Valley");
          h_valley[l_i][l_j][l_k] = MakeTH1('I', chis,chead,0x1000,0,0x4000);
        }
        for (l_k=0; l_k<N_CHA; l_k++)
        {
          sprintf(chis,"Timediff/Trigger time - Hit time   SFP: %2d FEBEX: %2d CHAN: %2d", l_i, l_j, l_k);
          sprintf(chead,"TRIG-HIT");
          h_trgti_hitti[l_i][l_j][l_k] = MakeTH1('I', chis,chead,2000,-1000,1000);
        }

        for (l_k=0; l_k<N_CHA; l_k++)
        {
          sprintf(chis,"Hitpat_Cha_List/Channel hit pattern per event (list)  SFP: %2d FEBEX: %2d CHAN: %2d", l_i, l_j, l_k);
          sprintf(chead,"HITPAT_Cha_List");
          h_ch_hitpat[l_i][l_j][l_k] = MakeTH1('I', chis,chead,11,-1,10);
        }

        for (l_k=0; l_k<N_CHA; l_k++)
        {
          sprintf(chis,"Hitpat_Cha_Trace/Channel hit pattern per event (trace)  SFP: %2d FEBEX: %2d CHAN: %2d", l_i, l_j, l_k);
          sprintf(chead,"HITPAT_Cha_Trace");
          h_ch_hitpat_tr[l_i][l_j][l_k] = MakeTH1('I', chis,chead,11,-1,10);
        }

        sprintf(chis,"Hitpat_Feb_List/Hit Pattern (list)  SFP: %2d FEBEX: %2d", l_i, l_j);
        sprintf(chead,"Hitpat_List");
        h_hitpat[l_i][l_j] = MakeTH1('I', chis,chead,20,-2,18);

        sprintf(chis,"Hitpat_Feb_Trace/Hit Pattern (trace)  SFP: %2d FEBEX: %2d", l_i, l_j);
        sprintf(chead,"Hitpat_Trace");
        h_hitpat_tr[l_i][l_j] = MakeTH1('I', chis,chead,20,-2,18);
      }
    }
  }
}

//----------------------------END OF GO4 SOURCE FILE ---------------------
