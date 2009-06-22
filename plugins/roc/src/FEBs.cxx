//============================================================================
// Name        : rocsrc/FEBs.cpp
// Author      : Norbert Abel
// Copyright   : Kirchhoff-Institut für Physik
//============================================================================

#include "roc/FEBs.h"


//-------------------------------------------------------------------------------
roc::FEB1nxC::FEB1nxC(roc::Board* board, int conn, bool init) :
   roc::FEBboard(board, conn)
{
   if (getConnector()==CON19) fNX.push_back(new nxyter::Chip(board, 0, getConnector()));
   if (getConnector()==CON20) fNX.push_back(new nxyter::Chip(board, 2, getConnector()));

   ADC().setConnector(getConnector());

   if (init) Init();
}
//-------------------------------------------------------------------------------


void roc::FEB1nxC::Init(bool reseti2c)
{
   NX(0).I2C().setSlaveAddr(8);

   if (reseti2c) reset_I2C();

   ADC().setRegister(33, 3);

   if (getConnector()==CON19) ADC().setChannelPort(1, 1);
   if (getConnector()==CON20) ADC().setChannelPort(3, 1);

   selectFEB4nx(0);

   NX(0).activate();
}



//-------------------------------------------------------------------------------
roc::FEB2nx::FEB2nx(roc::Board* board, int conn, bool init) :
   roc::FEBboard(board, conn)
{
   if (getConnector()==CON19) {
      fNX.push_back(new nxyter::Chip(board, 0, getConnector()));
      fNX.push_back(new nxyter::Chip(board, 1, getConnector()));
   }
   if (getConnector()==CON20) {
      fNX.push_back(new nxyter::Chip(board, 2, getConnector()));
      fNX.push_back(new nxyter::Chip(board, 3, getConnector()));
   }

   ADC().setConnector(getConnector());

   if (init) Init();
}
//-------------------------------------------------------------------------------

void roc::FEB2nx::Init(bool reseti2c)
{
   NX(0).I2C().setSlaveAddr(8);
   NX(1).I2C().setSlaveAddr(9);

   if (reseti2c) reset_I2C();

   ADC().setRegister(33, 3);

   if (getConnector()==CON19) {
      ADC().setChannelPort(1, 1);  // ????
      ADC().setChannelPort(2, 0);  // ????
   }
   if (getConnector()==CON20) {
      ADC().setChannelPort(3, 1);  // ????
      ADC().setChannelPort(4, 0);  // ????
   }
   selectFEB4nx(0);

   NX(0).activate();
   NX(1).activate();
}


//-------------------------------------------------------------------------------
roc::FEB4nx::FEB4nx(roc::Board* board, bool init) :
   roc::FEBboard(board, CON19)
{
   fNX.push_back(new nxyter::Chip(board, 0, CON19));
   fNX.push_back(new nxyter::Chip(board, 1, CON19));
   fNX.push_back(new nxyter::Chip(board, 2, CON19));
   fNX.push_back(new nxyter::Chip(board, 3, CON19));

   ADC().setConnector(CON20);

   if (init) Init();
}

//-------------------------------------------------------------------------------

void roc::FEB4nx::Init(bool reseti2c)
{
   NX(0).I2C().setSlaveAddr(8);
   NX(1).I2C().setSlaveAddr(9);
   NX(2).I2C().setSlaveAddr(10);
   NX(3).I2C().setSlaveAddr(11);

   if (reseti2c) reset_I2C();

   ADC().setRegister(33, 3);

   ADC().setChannelPort(1, 0); // ???
   ADC().setChannelPort(2, 1); // ???
   ADC().setChannelPort(3, 2); // ???
   ADC().setChannelPort(4, 3); // ???

   selectFEB4nx(1);

   NX(0).activate();
   NX(1).activate();
   NX(2).activate();
   NX(3).activate();
}
