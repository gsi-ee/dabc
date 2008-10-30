#include "TRocFastProc.h"

#include "Riostream.h"

#include "TH1.h"
#include "TH2.h"
#include "TROOT.h"
#include "TCutG.h"
#include "TGo4WinCond.h"
#include "TGo4PolyCond.h"
#include "TGo4CondArray.h"
#include "TGo4Picture.h"
#include "TGo4MbsEvent.h"
#include "snprintf.h"

#include "TRocFastParam.h"
#include "TRocFastControl.h"
#include "TRocFastEvent.h"

#include "SysCoreData.h"

//***********************************************************
TRocFastProc::TRocFastProc() : TGo4EventProcessor()
{
  cout << "**** TRocFastProc: Create instance " << endl;
}
//***********************************************************
TRocFastProc::~TRocFastProc()
{
  cout << "**** TRocFastProc: Delete instance " << endl;
}
//***********************************************************
// this one is used in standard factory
TRocFastProc::TRocFastProc(const char* name) : TGo4EventProcessor(name)
{
  cout << "**** TRocFastProc: Create instance " << name << endl;

   fParam   = (TRocFastParam *)   GetParameter("Par1");    // created in TRocFastAnalysis, values from auto save
   fControl = (TRocFastControl *) GetParameter("Control"); // created in TRocFastAnalysis, values from auto save
   
   fNumRocs = 3;
   
   if (fControl->numROCs > 0) fNumRocs = (unsigned) fControl->numROCs;
   
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

      if (fControl->fillADC) { 
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


/*

  Text_t chis[16];
  Text_t chead[64];
  Int_t i;

  //// init user analysis objects:


  // This example analysis allows for en-disabling the histogram filling.
  // Macro setfill.C creates macro histofill.C to set histogram fill status in parameter "Control".
  // Executed in Analysis terminal input line by .x setfill.C(n) with n=0,1.
  // This macro histofill.C, not the auto save file, will set status.
  gROOT->ProcessLine(".x histofill.C");
  fControl->PrintParameter("",0);

  // Creation of histograms (check if restored from auto save file):
  if(GetHistogram("Crate1/Cr1Ch01")==0)
    {// no auto save, create new
      for(i =0;i<8;i++)
   {
     snprintf(chis,15,"Cr1Ch%02d",i+1);  snprintf(chead, 63, "Crate 1 channel %2d",i+1);
          fCr1Ch[i] = new TH1I (chis,chead,5000,1,5000);
          AddHistogram(fCr1Ch[i],"Crate1");
     snprintf(chis,15,"Cr2Ch%02d",i+1);  snprintf(chead, 63, "Crate 2 channel %2d",i+1);
          fCr2Ch[i] = new TH1I (chis,chead,5000,1,5000);
          AddHistogram(fCr2Ch[i],"Crate2");
   }
      fCr1Ch1x2 = new TH2I("Cr1Ch1x2","Crate 1 channel 1x2",200,1,5000,200,1,5000);
      AddHistogram(fCr1Ch1x2);
      fHis1     = new TH1I ("His1","Condition histogram",5000,1,5000);
      AddHistogram(fHis1);
      fHis2     = new TH1I ("His2","Condition histogram",5000,1,5000);
      AddHistogram(fHis2);
      fHis1gate = new TH1I ("His1g","Gated histogram",5000,1,5000);
      AddHistogram(fHis1gate);
      fHis2gate = new TH1I ("His2g","Gated histogram",5000,1,5000);
      AddHistogram(fHis2gate);
      cout << "**** TRocFastProc: Created histograms" << endl;
    }
  else // got them from autosave file, restore pointers
    {
      for(i =0;i<8;i++)
   {
     snprintf(chis,15,"Crate1/Cr1Ch%02d",i+1); fCr1Ch[i] = (TH1I*)GetHistogram(chis);
     snprintf(chis,15,"Crate2/Cr2Ch%02d",i+1); fCr2Ch[i] = (TH1I*)GetHistogram(chis);
   }
      fCr1Ch1x2 = (TH2I*)GetHistogram("Cr1Ch1x2");
      fHis1     = (TH1I*)GetHistogram("His1");
      fHis2     = (TH1I*)GetHistogram("His2");
      fHis1gate = (TH1I*)GetHistogram("His1g");
      fHis2gate = (TH1I*)GetHistogram("His2g");
      cout << "**** TRocFastProc: Restored histograms from autosave" << endl;
    }
  // Creation of conditions (check if restored from auto save file):
  if(GetAnalysisCondition("cHis1")==0)
    {// no auto save, create new
      fconHis1= new TGo4WinCond("cHis1");
      fconHis2= new TGo4WinCond("cHis2");
      fconHis1->SetValues(100,2000);
      fconHis2->SetValues(100,2000);
      AddAnalysisCondition(fconHis1);
      AddAnalysisCondition(fconHis2);

      Double_t xvalues[4]={400,700,600,400};
      Double_t yvalues[4]={800,900,1100,800};
      TCutG* mycut= new TCutG("cut1",4,xvalues,yvalues);
      fPolyCon= new TGo4PolyCond("polycon");
      fPolyCon->SetValues(mycut); // copies mycat into fPolyCon
      fPolyCon->Disable(true);   // means: condition check always returns true
      delete mycut; // mycat has been copied into the conditions
      AddAnalysisCondition(fPolyCon);

      xvalues[0]=1000;xvalues[1]=2000;xvalues[2]=1500;xvalues[3]=1000;
      yvalues[0]=1000;yvalues[1]=1000;yvalues[2]=3000;yvalues[3]=1000;
      mycut= new TCutG("cut2",4,xvalues,yvalues);
      fConArr = new TGo4CondArray("polyconar",4,"TGo4PolyCond");
      fConArr->SetValues(mycut);
      fConArr->Disable(true);   // means: condition check always returns true
      delete mycut; // mycat has been copied into the conditions
      AddAnalysisCondition(fConArr);
      cout << "**** TRocFastProc: Created conditions" << endl;
    }
  else // got them from autosave file, restore pointers
    {
      fPolyCon  = (TGo4PolyCond*) GetAnalysisCondition("polycon");
      fConArr   = (TGo4CondArray*)GetAnalysisCondition("polyconar");
      fconHis1  = (TGo4WinCond*)  GetAnalysisCondition("cHis1");
      fconHis2  = (TGo4WinCond*)  GetAnalysisCondition("cHis2");
      fconHis1->ResetCounts();
      fconHis2->ResetCounts();
      fPolyCon->ResetCounts();
      fConArr->ResetCounts();
      cout << "**** TRocFastProc: Restored conditions from autosave" << endl;
    }
  // connect histograms to conditions. will be drawn when condition is edited.
  fconHis1->SetHistogram("His1");
  fconHis2->SetHistogram("His2");
  fconHis1->Enable();
  fconHis2->Enable();
  fPolyCon->Enable();
  ((*fConArr)[0])->Enable();
  ((*fConArr)[1])->Enable(); // 2 and 3 remain disabled

  if (GetPicture("Picture")==0)
    {// no auto save, create new
      // in the upper two pads, the condition limits can be set,
      // in the lower two pads, the resulting histograms are shown
      fcondSet = new TGo4Picture("condSet","Set conditions");
      fcondSet->SetLinesDivision(2,2,2);
      fcondSet->LPic(0,0)->AddObject(fHis1);
      fcondSet->LPic(0,1)->AddObject(fHis2);
      fcondSet->LPic(0,0)->AddCondition(fconHis1);
      fcondSet->LPic(0,1)->AddCondition(fconHis2);
      fcondSet->LPic(1,0)->AddObject(fHis1gate);
      fcondSet->LPic(1,1)->AddObject(fHis2gate);
      fcondSet->LPic(1,0)->SetFillAtt(4, 1001); // solid
      fcondSet->LPic(1,0)->SetLineAtt(4,1,1);
      fcondSet->LPic(1,1)->SetFillAtt(9, 1001); // solid
      fcondSet->LPic(1,1)->SetLineAtt(9,1,1);
      AddPicture(fcondSet);

      fPicture = new TGo4Picture("Picture","Picture example");
      fPicture->SetLinesDivision(3, 2,3,1);
      fPicture->LPic(0,0)->AddObject(fCr1Ch[0]);
      fPicture->LPic(0,0)->SetFillAtt(5, 3001); // pattern
      fPicture->LPic(0,0)->SetLineAtt(5,1,1);
      fPicture->LPic(0,1)->AddObject(fCr1Ch[1]);
      fPicture->LPic(0,1)->SetFillAtt(4, 3001); // pattern
      fPicture->LPic(0,1)->SetLineAtt(4,1,1);
      fPicture->LPic(1,0)->AddObject(fCr1Ch[2]);
      fPicture->LPic(1,0)->SetFillAtt(6, 1001); // solid
      fPicture->LPic(1,0)->SetLineAtt(6,1,1);
      fPicture->LPic(1,1)->AddObject(fCr1Ch[3]);
      fPicture->LPic(1,1)->SetFillAtt(7, 1001); // solid
      fPicture->LPic(1,1)->SetLineAtt(7,1,1);
      fPicture->LPic(1,2)->AddObject(fCr1Ch[4]);
      fPicture->LPic(3,0)->AddObject(fCr1Ch1x2);
      fPicture->LPic(3,0)->SetDrawOption("CONT");
      AddPicture(fPicture);
      cout << "**** TRocFastProc: Created pictures" << endl;
    }
  else  // got them from autosave file, restore pointers
    {
      fPicture = GetPicture("Picture");
      fcondSet = GetPicture("condSet");
      cout << "**** TRocFastProc: Restored pictures from autosave" << endl;
    }
    
*/    
}
//-----------------------------------------------------------
// event function
Bool_t TRocFastProc::BuildEvent(TGo4EventElement* target)
{
  // called by framework from TRocFastEvent to fill it

  TRocFastEvent* RocFastEvent = (TRocFastEvent*) target;

  fInput = (TGo4MbsEvent* ) GetInputEvent();
  if(fInput == 0) {
     cout << "AnlProc: no input event !"<< endl;
     return kFALSE;
  }
  if(fInput->GetTrigger() > 11) {
      cout << "**** TRocFastProc: Skip trigger event"<<endl;
      RocFastEvent->SetValid(kFALSE); // not store
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
           
              if (fControl->fillADC)
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
              if (data->getAuxFailing()==0) {
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

  RocFastEvent->SetValid(kTRUE); // to store
  return kTRUE;
}
