#include "TRocProc.h"

#include "Riostream.h"

#include "TH1.h"
#include "TH2.h"

#include "TRocParam.h"
#include "TGo4MbsEvent.h"

#include "SysCoreData.h"

//***********************************************************
TRocProc::~TRocProc()
{
  cout << "**** TRocProc: Delete instance " << endl;
}
//***********************************************************
// this one is used in standard factory
TRocProc::TRocProc(const char* name) : TGo4EventProcessor(name)
{
  cout << "**** TRocProc: Create instance " << name << endl;

   fParam   = (TRocParam *)  GetParameter("RocPar"); 
   if (fParam == 0) {
      fParam = new TRocParam("RocPar");
      AddParameter(fParam);
   }
   
   fNumRocs = 3;
   
   if (fParam->numRocs > 0) fNumRocs = (unsigned) fParam->numRocs;
   
   for (unsigned n=0; n<fNumRocs;n++) {
      fLastAUX.push_back(0);   
      
//      fAUX.push_back(0);
      fAUXt.push_back(0);
      fAUXch.push_back(0);
      fADCs.push_back(0);
      fChs.push_back(0);
      fLastTm.push_back(0);
      fCorrel.push_back(0);
      fHitQuality.push_back(0);
      fMsgTypes.push_back(0);
      
      fLastCh.push_back(0);
   }
   
   fCurrEpoch = 0;
   fCurrEvent = 0;
   
   TH1::AddDirectory(kFALSE);

   fEvntSize = (TH1I*) GetHistogram("EvntSize");
   if (fEvntSize==0) {
      fEvntSize = new TH1I ("EvntSize", "Number of syscore hits in event", 250, 1., 1000.);
      AddHistogram(fEvntSize);
   }

   fLastAUXtm = 0;
//   fAUXall = (TH1I*) GetHistogram("AUXall");
//   if (fAUXall==0) {
//      fAUXall = new TH1I ("AUXall", "all AUX signals", 1000, 0., 2000.);
//      AddHistogram(fAUXall);
//   }
   
   fGEM = 0;

/*
   fADC = (TH1I*) GetHistogram("ADC");
   if (fADC==0) {
      fADC = new TH1I ("ADC", "all adc values", 4096, 0., 4096.);
      AddHistogram(fADC);
   }
   
   fChannels = (TH1I*) GetHistogram("Channels");
   if (fChannels==0) {
      fChannels = new TH1I ("Channels", "all channels of all nXYTERS values", 128, 0., 127.);
      AddHistogram(fChannels);
   }

   fTmStamp = (TH1I*) GetHistogram("TimeStamps");
   if (fTmStamp==0) {
      fTmStamp = new TH1I ("TimeStamps", "all time stamps", 4095, 1., 16.*1024);
      AddHistogram(fTmStamp);
   }
*/

   for (unsigned n=0; n<fNumRocs;n++) {
      TString folder = Form("ROC%u",n);
      TString hname = Form("AUX%u", n);
       
//      fAUX[n] = (TH1I*) GetHistogram(folder+"/"+hname);
//      if (fAUX[n]==0) {
//         fAUX[n] = new TH1I (hname, "Distance between two AUX signals (in nanosec)", 1000, 0., 1000.);
//         AddHistogram(fAUX[n], folder);
//      }

      hname = Form("MsgTypes%u", n);
      fMsgTypes[n] = (TH1I*) GetHistogram(folder+"/"+hname);
      if (fMsgTypes[n]==0) {
         fMsgTypes[n] = new TH1I (hname, "Distribution of messages", 8, 0., 8.);
         fMsgTypes[n]->GetXaxis()->SetBinLabel(1 + ROC_MSG_NOP, "NOP");
         fMsgTypes[n]->GetXaxis()->SetBinLabel(1 + ROC_MSG_HIT, "HIT");
         fMsgTypes[n]->GetXaxis()->SetBinLabel(1 + ROC_MSG_EPOCH, "EPOCH");
         fMsgTypes[n]->GetXaxis()->SetBinLabel(1 + ROC_MSG_SYNC, "SYNC");
         fMsgTypes[n]->GetXaxis()->SetBinLabel(1 + ROC_MSG_AUX, "AUX");
         fMsgTypes[n]->GetXaxis()->SetBinLabel(1 + ROC_MSG_SYS, "SYS");
         AddHistogram(fMsgTypes[n], folder);
      }

      hname = Form("HitQuality%u", n);
      fHitQuality[n] = (TH1I*) GetHistogram(folder+"/"+hname);
      if (fHitQuality[n]==0) {
         fHitQuality[n] = new TH1I (hname, "Is hit ok (=0) or bad (=1)", 2, 0., 2.);
         AddHistogram(fHitQuality[n], folder);
      }

      hname = Form("Aux%u_t", n);
      fAUXt[n] = (TH1I*) GetHistogram(folder+"/"+hname);
      if (fAUXt[n]==0) {
         fAUXt[n] = new TH1I (hname, "Time distribution of AUX signal", 10000, 0., 1000.);
         AddHistogram(fAUXt[n], folder);
      }

      hname = Form("Aux%u_ch", n);
      fAUXch[n] = (TH1I*) GetHistogram(folder+"/"+hname);
      if (fAUXch[n]==0) {
         fAUXch[n] = new TH1I(hname, "Number events per aux channel", 8, 0., 8.);
         AddHistogram(fAUXch[n], folder);
      }
      
      hname = Form("Chls%u", n);
      fChs[n] = (TH1I*) GetHistogram(folder+"/"+hname);
      if (fChs[n]==0) {
         fChs[n] = new TH1I (hname, "Channels from single ROC", 128, 0., 128.);
         AddHistogram(fChs[n], folder);
      }

      hname = Form("Correl%u", n);
      fCorrel[n] = (TH2I*) GetHistogram(folder+"/"+hname);
      if (fCorrel[n]==0) {
         fCorrel[n] = new TH2I (hname, "Correlation between channels", 250, -50., 200., 128, 0., 128.);
         AddHistogram(fCorrel[n], folder);
      }

      if (fParam->fillADC) { 
          hname = Form("ADC%u", n);
          fADCs[n] = (TH2I*) GetHistogram(folder+"/"+hname);
          if (fADCs[n]==0) {
             fADCs[n] = new TH2I (hname, "ADC over all roc channels", 128, 0., 128., 1024, 0., 4096.);
             AddHistogram(fADCs[n], folder);
          }
      }
      
      if (n!=2) continue;
      
      hname = "GEM";
      fGEM = (TH2I*) GetHistogram(folder+"/"+hname);
      if (fGEM==0) {
         fGEM = new TH2I (hname, "Distribution of GEM channels", 26, 0., 13., 10, 0., 10.);
         AddHistogram(fGEM, folder);
      }
     
   }
}

//-----------------------------------------------------------
// event function
Bool_t TRocProc::BuildEvent(TGo4EventElement*)
{
  // called by framework from TRocFastEvent to fill it

  TGo4MbsEvent* fInput = (TGo4MbsEvent* ) GetInputEvent();
  if(fInput == 0) {
     cout << "AnlProc: no input event !"<< endl;
     return kFALSE;
  }
  if(fInput->GetTrigger() > 11) {
      cout << "**** TRocProc: Skip trigger event"<<endl;
      return kFALSE;
    }
  
  uint64_t fulltm = 0;

  TGo4MbsSubEvent* psubevt = 0;

  fInput->ResetIterator();
  while((psubevt = fInput->NextSubEvent()) != 0) { // loop over subevents
     SysCoreData* data = (SysCoreData*) psubevt->GetDataField();
     
     int datasize = (psubevt->GetDlen()-2) * 2 / 6;
     
     fEvntSize->Fill(datasize); 
     //cout << "AnlProc: found subevent subcrate="<<(int) psubevt->GetSubcrate()<<", procid="<<(int)psubevt->GetProcid()<<", control="<<(int) psubevt->GetControl()<< endl;
     
     for (; datasize>0; data++, datasize--) {
         
        unsigned typ = data->getMessageType();
        unsigned rocid = data->getRocNumber();
        
        if (rocid>=fNumRocs) continue;
        
        fMsgTypes[rocid]->Fill(typ);
        
        switch (typ) {
           case ROC_MSG_NOP:
 //           fHitQuality[rocid]->Fill(1.); 
//            printf("NOP message - why?\n"); 
              break;
           case ROC_MSG_HIT:
              fulltm = data->getMsgFullTime(fCurrEpoch);
           
              if (fLastTm[rocid] < fulltm + 256) {
                 uint16_t ch_id = data->getId();
                 fCorrel[rocid]->Fill(0.0 + fulltm - fLastTm[rocid], fabs(0.0 + ch_id - fLastCh[rocid]));
                 fLastTm[rocid] = fulltm;
                 fLastCh[rocid] = ch_id;
              
                 fHitQuality[rocid]->Fill(0.);
              } else 
                 fHitQuality[rocid]->Fill(1.);

//           if (fLastTm[rocid] > fulltm + 200) {
//              printf("Roc:%u Det:%u Negative value -%llu %s\n", 
//                 rocid, data->getId(), fLastTm[rocid] - fulltm, ((data->getId()==0) ? "ok" : "!!!!!!!!!!!!!!!!"));
//           }

            
//           fADC->Fill(data->getAdcValue());
//           fTmStamp->Fill(data->getNxTs());
//           fChannels->Fill(data->getId());
           
              fChs[rocid]->Fill(data->getId());
           
              if (fParam->fillADC)
                 fADCs[rocid]->Fill(data->getId(), data->getAdcValue());
           
              if ((rocid==2) && (fGEM!=0)) {
                 int nrow = 0;
                 int ncol = 0;
                 if (data->getId()<12) {
                    nrow = data->getId();
                    fGEM->Fill(nrow + 0.75, ncol);
                    fGEM->Fill(nrow + 1.25, ncol);
                 } else 
                 if (data->getId()>= 12 + 8*13) { 
                    ncol = 9; 
                    nrow = data->getId() - (12 + 8*13); 
                    fGEM->Fill(nrow + 0.75, ncol);
                    fGEM->Fill(nrow + 1.25, ncol);
                 } else { 
                    ncol = (data->getId() - 12) / 13 + 1; 
                    nrow = (data->getId() - 12) % 13; 
                    fGEM->Fill(nrow + 0.25, ncol);
                    fGEM->Fill(nrow + 0.75, ncol);
                 }
              }   
              
              break;
              
           case ROC_MSG_EPOCH:
              fCurrEpoch = data->getEpoch(); 
              break;
           case ROC_MSG_SYNC:
              fCurrEvent = data->getSyncEvNum(); 
              break;
           case ROC_MSG_AUX:
              if (data->getAuxFalling()==0) {
                fAUXch[rocid]->Fill(data->getAuxChNum());
          
                 uint64_t full = data->getMsgFullTime(fCurrEpoch);
      //           if (fLastAUX[rocid]!=0) {
      //              double dist = (full - fLastAUX[rocid]) / 1.; // distance in microsec
      //              if (rocid!=0) printf("ROC1 Dist = %5.4f\n", dist);
      //              fAUX[rocid]->Fill(dist);
      //           }
      //           fAUXall->Fill(full - fLastAUXtm);
      //	   	   fLastAUX[rocid] = full; 
      //           fLastAUXtm = full;
            
                full = full / 100000000L;
            
                fAUXt[rocid]->Fill((full % 10000) * 0.1);
              }
              break; 
           case ROC_MSG_SYS:
              break;
        }
     }
  }

  return kTRUE;
}
