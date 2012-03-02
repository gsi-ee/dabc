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
