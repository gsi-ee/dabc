//============================================================================
// Name        : SysCoreSimulatorApp.cpp
// Author      : Stefan Müller-Klieser
// Version     :
// Copyright   : Kirchhoff-Institut für Physik
// Description : this app uses the SysCoreSimulator class to compile to an unix app
//============================================================================

#include "SysCoreSimulator.h"

#include <iostream>
using namespace std;

int main(int argc, char* argv[])
{
   cout << "Usage: simulator [rocNum [type [lostRate]]]" << endl;
   cout << "          type = 0 : original (hits for each timestamp)" << endl;
   cout << "          type = 1 : rare sync data" << endl;
   cout << "          type = 2 : big hit multiplicity peaks " << endl;
   cout << "          type = 3 : rare hits on different channels " << endl;
   cout << "          lostRate : relative artifitial 'lost' of inp/out packets" << endl;

   int rocNum = 0;
   if (argc>1) rocNum = atoi(argv[1]);

   int simtype = 1;
   if (argc>2) simtype = atoi(argv[2]);

   double lostRate = 0.;
   if (argc>3) lostRate = atof(argv[3]);

   cout << "Apply rocNum:" << rocNum << " simulation type:"<<simtype<<", loss rate:"<<lostRate<<"..." << endl << endl;
   SysCoreSimulator simulator(rocNum, simtype);
   simulator.setLostRates(lostRate, lostRate);
   
   if (!simulator.initSockets()) return -1;
   
   simulator.mainLoop();

   return 0;
}
