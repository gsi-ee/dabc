#include "bnet/defines.h"

#include <stdlib.h>
#include <math.h>

int IbTestMatrixMean = 1000;
int IbTestMatrixSigma = 250;

double rand_0_1()
{
   return drand48();
}

double GaussRand(double mean, double sigma)
{
  double x,y,z;
  y = drand48();
  z = drand48();
  x = z * 6.28318530717958623;
  return mean + sigma*sin(x)*sqrt(-2*log(y));
}
