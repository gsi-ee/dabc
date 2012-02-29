#include "bnet/defines.h"

#include <stdlib.h>
#include <math.h>

#include <algorithm>

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


void bnet::DoublesVector::Sort()
{
   std::sort(begin(), end());
}

double bnet::DoublesVector::Mean(double max_cut)
{
   unsigned right = lrint(max_cut*(size()-1));

   double sum(0);
   for (unsigned n=0; n<=right; n++) sum+=at(n);
   return right>0 ? sum / (right+1) : 0.;
}

double bnet::DoublesVector::Dev(double max_cut)
{
   unsigned right = lrint(max_cut*(size()-1));

   if (right==0) return 0.;

   double sum1(0), sum2(0);
   for (unsigned n=0; n<=right; n++)
      sum1+=at(n);
   sum1 = sum1 / (right+1);

   for (unsigned n=0; n<=right; n++)
      sum2+=(at(n)-sum1)* (at(n)-sum1);

   return ::sqrt(sum2/(right+1));
}

double bnet::DoublesVector::Min()
{
   return size()>0 ? at(0) : 0.;
}

double bnet::DoublesVector::Max()
{
   return size()>0 ? at(size()-1) : 0.;
}

