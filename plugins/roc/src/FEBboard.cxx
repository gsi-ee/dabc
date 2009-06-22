//============================================================================
// Name        : roc/FEBboard.cpp
// Author      : Norbert Abel
// Copyright   : Kirchhoff-Institut für Physik
//============================================================================

#include "roc/FEBboard.h"

#include <stdexcept>

//-------------------------------------------------------------------------------
roc::FEBboard::FEBboard(roc::Board* board, int conn) :
   Peripheral(board),
   fConnector(conn),
   fNX(),
   fADC(board)
{
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
roc::FEBboard::~FEBboard()
{
   for (unsigned n=0; n<fNX.size();n++)
      delete fNX[n];
   fNX.clear();
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
void roc::FEBboard::selectFEB4nx(uint32_t val)
{
   brd().put(ROC_FEB4NX, val);
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
void roc::FEBboard::reset_I2C()
{
   if (fConnector==CON19) {
      brd().put(ROC_I2C1_RESET, 0);
      brd().put(ROC_I2C1_REGRESET, 0);
      brd().put(ROC_I2C1_RESET, 1);
      brd().put(ROC_I2C1_REGRESET, 1);
   } else {
      brd().put(ROC_I2C2_RESET, 0);
      brd().put(ROC_I2C2_REGRESET, 0);
      brd().put(ROC_I2C2_RESET, 1);
      brd().put(ROC_I2C2_REGRESET, 1);
   }

   for (int i=0; i<numNX(); i++) NX(i).I2C().testsetup();
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
nxyter::Chip &roc::FEBboard::NX(int index)
{
   if (index>=fNX.size()) throw std::out_of_range("FATAL ERROR: You tried to access an nXYTER Number, that is not available!\r\n");
   return *fNX[index];
}
//-------------------------------------------------------------------------------

