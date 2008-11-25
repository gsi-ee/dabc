#include "dabc/logging.h"
#include "dabc/timing.h"
#include "dabc/string.h"

#include "mbs/EventAPI.h"
#include "mbs/MbsTypeDefs.h"
#include "mbs/LmdOutput.h"
#include "mbs/LmdInput.h"

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

      std::string fname;

      int uniqueid = nodeid* 1000 + nr;

      dabc::formats(fname,"%s_inp_%d_%d%s", fnamebase, nodeid, nr, (multiple ? "" : ".lmd"));

      if (onlycheck) {
         mbs::LmdInput* inp = new mbs::LmdInput(fname.c_str());
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

      mbs::LmdOutput* out = 0;
      if (multiple)
         out = new mbs::LmdOutput(fname.c_str(), 20, -1);
      else
         out = new mbs::LmdOutput(fname.c_str());

      if (!out->Init()) {
         EOUT(("Cannot create output lmd file %s", fname.c_str()));
         return 1;
      }

      int evid = 1;

      int startacq = 1;
      int stopacq = -1;

      while (evid<=numevents) {
         dabc::Buffer* buf = dabc::Buffer::CreateBuffer(32*1024);

         if (!bnet::GenerateMbsPacket(buf, uniqueid, evid, 240, 4, startacq, stopacq)) {
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

