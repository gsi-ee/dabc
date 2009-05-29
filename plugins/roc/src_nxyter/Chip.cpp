//============================================================================
// Name        : nxytersrc/Chip.cpp
// Author      : Norbert Abel
// Copyright   : Kirchhoff-Institut für Physik
//============================================================================

#include "nxyter/Chip.h"

//-------------------------------------------------------------------------------
nxyter::Chip::Chip(roc::Board* board, int num, int con) :
   roc::Peripheral(board),
   fI2C(board, con)
{
   fNumber = num;
   fConnector = con;
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
nxyter::Chip::~Chip()
{
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
void nxyter::Chip::setNumber(int num)
{
   if ((num >= 0) && (num <= 3)) fNumber = num;
   else EOUT(("Not a valid I2C-Number!\n"));
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
int nxyter::Chip::getNumber()
{
    return fNumber;
}
//-------------------------------------------------------------------------------




//-------------------------------------------------------------------------------
void nxyter::Chip::activate()
{
   uint32_t mask = brd().NX_getActive();

   mask = mask | (1<<fNumber);

   brd().NX_setActive(mask);
}
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
void nxyter::Chip::deactivate()
{
   uint32_t mask = brd().NX_getActive();

   mask = mask & (~(1<<fNumber));

   brd().NX_setActive(mask);
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
uint32_t nxyter::Chip::getChannelDelay()
{
   uint32_t val;

   if (fNumber==0) brd().get(ROC_DELAY_NX0, val);
   if (fNumber==1) brd().get(ROC_DELAY_NX1, val);
   if (fNumber==2) brd().get(ROC_DELAY_NX2, val);
   if (fNumber==3) brd().get(ROC_DELAY_NX3, val);
   return val;
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
void nxyter::Chip::setChannelDelay(uint32_t val)
{
    if (fNumber==0) brd().put(ROC_DELAY_NX0, val);
    if (fNumber==1) brd().put(ROC_DELAY_NX1, val);
    if (fNumber==2) brd().put(ROC_DELAY_NX2, val);
    if (fNumber==3) brd().put(ROC_DELAY_NX3, val);
}
//-------------------------------------------------------------------------------

