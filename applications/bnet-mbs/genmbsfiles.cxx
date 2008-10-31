#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/string.h"

#include "mbs/MbsEventAPI.h"
#include "mbs/MbsFactory.h"
#include "mbs/MbsTypeDefs.h"
#include "mbs/MbsDataInput.h"
#include "mbs/MbsOutputFile.h"
#include "mbs/MbsInputFile.h"

#include "MbsGeneratorModule.h"

int main(int numc, char* args[])
{
   dabc::SetDebugLevel(1);

//   DOUT1(("Generating mbs files. Syntax is:"));
//   DOUT1(("genmbsfiles namebase nodeid numreadouts numevents onlycheck"));
      
   const char* fnamebase = "/tmp/genmbs";
   int nodeid = 0;
   int numreadouts = 4;
   int numevents = 1000;
   bool onlycheck = false;
   bool multiple = false;
                  
   if (numc>1) fnamebase = args[1];
   if (numc>2) nodeid = atoi(args[2]);
   if (numc>3) numreadouts = atoi(args[3]);
   if (numc>4) numevents = atoi(args[4]);
   if (numc>5) onlycheck = (atoi(args[5]) > 0);

   for(int nr=0;nr<numreadouts;nr++) {
      
      dabc::String fname;
      
      int uniqueid = nodeid* 1000 + nr;
      
      dabc::formats(fname,"%s_inp_%d_%d%s", fnamebase, nodeid, nr, (multiple ? "" : ".lmd"));
      
      if (onlycheck) {
         mbs::MbsInputFile* inp = new mbs::MbsInputFile(fname.c_str(), "", -1);
         if (inp->Init()) {
            dabc::Buffer* buf = 0;
            int nbuf = 0;
            while ((buf=inp->ReadBuffer()) != 0) {
               nbuf++; 
               dabc::Buffer::Release(buf);   
            }
            DOUT1(("Test file %s nbuf = %d", fname.c_str(), nbuf));
         }
         delete inp;
         continue;  
      }
      
      mbs::MbsOutputFile* out = 0;
      if (multiple) 
         out = new mbs::MbsOutputFile(fname.c_str(), "", 2000000, -1);
      else    
         out = new mbs::MbsOutputFile(fname.c_str(), "");

      if (!out->Init()) {
         EOUT(("Cannot create output lmd file %s", fname.c_str()));
         return 1; 
      }

      int evid = 1;
      
      int startacq = 1;
      int stopacq = -1;
      
      while (evid<=numevents) {
         dabc::Buffer* buf = dabc::Buffer::CreateBuffer(32*1024);
         
         if (!mbs::GenerateMbsPacket(buf, uniqueid, evid, 240, 4, startacq, stopacq)) {
            EOUT(("Cannot generate MBS buffer"));
            break;   
         }
         
         // build startacq in beginning, stop/start in the middle 
         // and stop in the end of the file
         if ((startacq==1) && (evid>startacq)) {
            startacq = numevents / 2;
            stopacq = startacq + 1;
         } else
         if ((startacq== numevents / 2) && (evid>startacq+1)) {
            startacq = -1;
            stopacq = numevents;
         }
          
         out->WriteBuffer(buf);
         
         dabc::Buffer::Release(buf); 
      }
      
      delete out;
      
      DOUT1(("Did file %s", fname.c_str()));
   }
   
   return 0;
}

