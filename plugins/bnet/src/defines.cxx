#include "bnet/defines.h"

#include <stdlib.h>
#include <math.h>

#include "dabc/logging.h"

double dabc::rand_0_1()
{
   return drand48();
}

double dabc::GaussRand(double mean, double sigma)
{
  double x,y,z;
  y = drand48();
  z = drand48();
  x = z * 6.28318530717958623;
  return mean + sigma*sin(x)*sqrt(-2*log(y));
}

bool bnet::TestEventHandling::GenerateSubEvent(EventId evid, int subid, int numsubids, dabc::Buffer& buf)
{
   if (buf.GetTotalSize() < 16) return false;

   DOUT2(("Produce event %u", (unsigned) evid));

   uint64_t data[2] = { evid, subid };

   buf.CopyFrom(data, sizeof(data));

   return true;
}

bool bnet::TestEventHandling::ExtractEventId(const dabc::Buffer& buf, EventId& evid)
{
   evid = 0;
   buf.CopyTo(&evid, sizeof(evid));
   return true;
}

dabc::Buffer bnet::TestEventHandling::BuildFullEvent(bnet::EventId evid, dabc::Buffer* bufs, int numbufs)
{
   dabc::Buffer res;
   if (bufs==0) return res;

   for (int n=0;n<numbufs;n++) {
      if (n==0) res << bufs[n];
      bufs[n].Release();
   }

   return res;
}
