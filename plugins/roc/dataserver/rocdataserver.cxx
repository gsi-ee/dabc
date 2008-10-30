#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/logging.h"
   
#include "DataServerPlugin.h"

#include <unistd.h>

/*  
#include "SysCoreControl.h"
#include "SysCoreData.h"

void TestSpeed()
{
   SysCoreData data;

   double t1 = TimeStamp();
   
   int cnt = 10000000;
   
   unsigned typ, rocid, id;
   
   for (int n=0;n<cnt;n++) {
      typ = data.getMessageType();
      rocid = data.getRocNumber();
      id = data.getNxNumber();
      id = data.getNxTs();
      id = data.getId();
      id = data.getAdcValue();
   }
   
   double t2 = TimeStamp();
   
   double tm = (t2-t1)*1e-6;
   
   DOUT1(("Tm %5.3f s Get speed mps = %5.1f MBs %5.1f ", tm, cnt / tm * 1e-6, cnt / tm * 6e-6));

   t1 = TimeStamp();
   
   for (int n=0;n<cnt;n++) {
      data.setMessageType(1);
      data.setRocNumber(1);
      data.setNxTs(1345);
      data.setLastEpoch(0);
      
      data.setSyncTs(123);
      data.setSyncEvNum(56789);

   }
   
   t2 = TimeStamp();
   
   tm = (t2-t1)*1e-6;

   DOUT1(("Tm %5.3f s Set speed mps = %5.1f MBs %5.1f ", tm, cnt / tm * 1e-6, cnt / tm * 6e-6));
   
   for(unsigned n=0;n<=0x3fff;n++) {
      data.setNxTs(n);
      if (data.getNxTs()!=n) DOUT1(("Ts Miss %x != %x", n, data.getNxTs()));
   }

   for(unsigned n=0;n<=0xfffffff;n+=1234) {
      data.setEpoch(n);
      if (data.getEpoch()!=n) DOUT1(("Epoch Miss %x != %x", n, data.getEpoch()));
   }

   for(unsigned n=0;n<=0xffffff;n+=7) {
      data.setSyncEvNum(n);
      if (data.getSyncEvNum()!=n) DOUT1(("Sync Miss %x != %x", n, data.getSyncEvNum()));
   }

   for(unsigned n=0;n<=0x3fff;n++) {
      unsigned ts = n & 0x3ffe; 
      data.setSyncTs(ts);
      data.setSyncEvNum(n*0x2f7);
      if (data.getSyncTs()!=ts) DOUT1(("SyncTs Miss %x != %x", ts, data.getSyncTs()));
      if (data.getSyncEvNum()!=n*0x2f7) DOUT1(("Sync Miss %x != %x", n*0x2f7, data.getSyncEvNum()));
   }

   for(unsigned n=0;n<=0x3fff;n++) {
      unsigned ts = n & 0x3ffe; 
      data.setAuxTs(ts);
      if (data.getAuxTs()!=ts) DOUT1(("AUXTs Miss %x != %x", ts, data.getAuxTs()));
   }

   data.setSyncEvNum(0x111111);
   
   DOUT1(("%x %x %s", data.getSyncEvNum(), 
             *((uint32_t*)(data.getData()+2)), data.getCharData()));

   t1 = TimeStamp();
   
   for (int n=0;n<cnt;n++) {
      id = data.getSyncEvNum();
   }
   
   t2 = TimeStamp();
   tm = (t2-t1)*1e-6;

   DOUT1(("Tm %5.3f s Get old speed mps = %5.1f MBs %5.1f ", tm, cnt / tm * 1e-6, cnt / tm * 6e-6));
}
*/

#include "SysCoreData.h"
#include "SysCoreSorter.h"
#include "mbs/LmdFile.h"


void TestSorter(const char* lmffilename = "/misc/linev/rawdata/run026_raw_0000.lmd")
{
   mbs::LmdFile f;
   if (!f.OpenRead(lmffilename)) return;
   
   mbs::EventHeader* hdr = 0;
   mbs::SubeventHeader* subev = 0;
   SysCoreData* data = 0;
   unsigned numdata = 0;

   long totalsz = 0;
   long total_num_msg = 0;
   unsigned numevents = 0;
   
   uint32_t lastepoch = 0;
   uint64_t lasttm = 0;
   uint64_t lastsynctm = 0;
   uint32_t lastsyncnum = 0;
   
   SysCoreSorter sorter(64*1024, 64*1024, 16*1024);
//   sorter.setDataCheck(false);
   
   double t1 = TimeStamp();
   
   // this is the loop over all events in the file
   while ((hdr = f.ReadEvent()) != 0) {

//      if (numevents==5000) return;

      numevents++;
      totalsz += hdr->FullSize();
      
      subev = 0;
      
      // this is the loop over all subevents in one event
      while ((subev = hdr->NextSubEvent(subev)) != 0) {
          
         if (subev->iSubcrate!=1) continue;
//         continue; 
          
         data = (SysCoreData*) subev->RawData();
         numdata = subev->RawDataSize() / 6 - 1;        // exclude sync message
         if (data[numdata-1].isEpochMsg()) numdata--;   // exclude explicit epoch message

//         DOUT1(("\n````````````````Start sorting of original data %d   numdata = %u!!!!```````````````", numevents, numdata));

         total_num_msg += numdata;

//         DOUT1(("\n```````````````` add original data %d!!!!```````````````", numdata));
         if (!sorter.addData(data, numdata)) {
            EOUT(("Cannot add new data to sorter - break"));
            return;   
         }

         // this is the loop over all messages in subevent
         if ((lastepoch>=0x16346e20) && (lastepoch<0x16346ee0)) {
            numdata = subev->RawDataSize() / 6; 
            DOUT1(("\n````````````````Original data %d!!!!```````````````", numdata));
            while (numdata--) {
               data->printData(3); 
               data++;   
            }
         }

         unsigned num_sort = sorter.sizeFilled();
         SysCoreData* out = sorter.filledBuf();
//         DOUT1(("\n````````````````Sorted data %d    num_sort = %u !!!!```````````````", numevents, num_sort));
         while (num_sort--) {
//            out->printData(3); 

//           if (lastepoch > 0x16346e7a - 3) 
//              out->printData(3);
              
            if (out->isEpochMsg()) 
               lastepoch = out->getEpoch(); 
            else {
               uint64_t tm = out->getMsgFullTime(lastepoch);
               
              if (tm!=0) {   
//              data->printData(3); 
//              printf("Full message timestamp:%llu or %llx\n", tm, tm);
                  if (tm<lasttm) {
                     printf("Time sequence error prev:%llu next:%llu diff:-%llu epoch:%x\n", lasttm, tm, lasttm-tm, lastepoch);
                     exit(1);
                  }
                  lasttm = tm;
//                  exit(1);
              }
              
              
              
              if (out->isSyncMsg()) {
                 uint32_t step = (tm - lastsynctm) / (out->getSyncEvNum() - lastsyncnum); 
                 printf("Event %x step %u epoch %x \n", out->getSyncEvNum(), step, lastepoch);
                 if ((step < 16380) || (step > 16400)) 
                    out->printData(3);
                 
                 lastsynctm = tm;
                 lastsyncnum = out->getSyncEvNum();
              }
               
            }
            

           out++;   
         }
         
         sorter.shiftFilledData(sorter.sizeFilled());
          
      }
      
//      sorter.flush();
      
   }

   double t2 = TimeStamp();

   printf("Overall events: %u total size:%ld totalmsg:%ld totaltm:%5.3f\n", numevents, totalsz, total_num_msg, (t2-t1)*1e-6);

   f.Close();
}

void TestCalibr()
{
    unsigned t1, t2, shift, t2corr;
    double k;
    
    
    t1 = 155;
    t2 = 110;
    
    shift = 56; k = 1./1.1;
    
    for (int i=0; i<10000; i++) {
       t1 += 4322450;
       
       unsigned t2_prev = t2;
       
       t2 += 4754695;
       
       if (t2_prev > t2) {
          shift+=0x100000000LLU*k;
       }
       
       t2corr = unsigned(t2*k) + shift;
       
       double diff = 0. + t2corr - t1;
       
       printf("t1: %08x  t2: %08x  t2corr: %08x  diff: %5.1f\n", t1, t2, t2corr, diff);    
    }
    
}


int main(int numc, char* args[])
{

   dabc::SetDebugLevel(1);
   
   
//   TestCalibr();
//   TestSorter();
//   return 0;
   
//   TestSpeed();
//   return 0;
    
   const char* configuration = "SetupRoc.xml"; // master.cluster.domain
   if(numc >= 2) configuration = args[1];
   
   DOUT1(("Using config file: %s", configuration));
   
//   dabc::StandaloneManager manager(0, 1);
   dabc::Manager manager("Dataserver", false);
   
   dabc::Logger::Instance()->LogFile("Dataserver.log");
   
   manager.InstallCtrlCHandler();
   
   ::InitUserPlugin(&manager);
   
   manager.Read_XDAQ_XML_Config(configuration);
   
   // set states of manager to running here:
   if(!manager.DoStateTransition(dabc::Manager::stcmdDoConfigure)) {
      EOUT(("State transition %s failed. Abort", dabc::Manager::stcmdDoConfigure)); 
      return 1;
   }
   DOUT1(("Did configure"));   
      
   if(!manager.DoStateTransition(dabc::Manager::stcmdDoEnable)) {
      EOUT(("State transition %s failed. Abort", dabc::Manager::stcmdDoEnable)); 
      return 1;
   }
   DOUT1(("Did enable"));   

   if(!manager.DoStateTransition(dabc::Manager::stcmdDoStart)) {
      EOUT(("State transition %s failed. Abort", dabc::Manager::stcmdDoStart)); 
      return 1;
   }

   DOUT1(("Data taking from ROC(s) is now running"));
   DOUT1(("       Press ctrl-C for stop"));
   
//   manager.RunManagerMainLoop();
   
   while(roc::DataServerPlugin::PluginWorking()) { ::sleep(1); } 
//   sleep(10);
   
   DOUT1(("Normal finish of data taking"));

   if(!manager.DoStateTransition(dabc::Manager::stcmdDoStop)) {
      EOUT(("State transition %s failed. Abort", dabc::Manager::stcmdDoStop)); 
      return 1;
   }

   if(!manager.DoStateTransition(dabc::Manager::stcmdDoHalt)) {
      EOUT(("State transition %s failed. Abort", dabc::Manager::stcmdDoHalt)); 
      return 1;
   }

   dabc::Logger::Instance()->ShowStat();
   
   return 0; 
}
