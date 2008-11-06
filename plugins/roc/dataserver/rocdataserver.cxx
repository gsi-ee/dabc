#include "dabc/Command.h"
#include "dabc/Manager.h"
#include "dabc/logging.h"

#include "dabc/Application.h"

#include <unistd.h>


/*

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
*/

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

   dabc::Manager manager("Dataserver", false);

   dabc::Logger::Instance()->LogFile("Dataserver.log");

   manager.InstallCtrlCHandler();

   manager.Read_XDAQ_XML_Libs(configuration);

   manager.CreateApplication("RocReadoutApp");

   manager.Read_XDAQ_XML_Pars(configuration);

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

   while(dabc::mgr()->GetApp()->IsModulesRunning()) { ::sleep(1); }
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
