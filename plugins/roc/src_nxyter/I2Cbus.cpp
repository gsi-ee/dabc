//============================================================================
// Name        : nxytersrc/I2Cbus.cpp
// Author      : Norbert Abel
// Copyright   : Kirchhoff-Institut für Physik
//============================================================================

#include "nxyter/I2Cbus.h"


//-------------------------------------------------------------------------------
nxyter::I2Cbus::I2Cbus(roc::Board* board, int num) :
   roc::Peripheral(board)
{
   fI2C_Number = num;
   fSlaveAddr = 8;
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
nxyter::I2Cbus::~I2Cbus()
{
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
void nxyter::I2Cbus::setNumber(int num)
{
    if ((num >= CON19) && (num <= CON20)) fI2C_Number = num;
    else EOUT(("Not a valid I2C-Number!"));
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
void nxyter::I2Cbus::setSlaveAddr(int addr)
{
   fSlaveAddr = addr;
   if (fI2C_Number==CON19)
      brd().put(ROC_I2C1_SLAVEADDR, addr);
   else
      brd().put(ROC_I2C2_SLAVEADDR, addr);
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
int nxyter::I2Cbus::getNumber()
{
    return fI2C_Number;
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
void nxyter::I2Cbus::setRegister(int reg, uint32_t val)
{
   uint32_t err(0);
   setSlaveAddr(fSlaveAddr);
   if (fI2C_Number==CON19) {
     brd().put(ROC_I2C1_REGISTER, reg);
     brd().put(ROC_I2C1_DATA, val);
     brd().get(ROC_I2C1_ERROR, err);
   } else {
     brd().put(ROC_I2C2_REGISTER, reg);
     brd().put(ROC_I2C2_DATA, val);
     brd().get(ROC_I2C2_ERROR, err);
   }
   if (verbose() && (err==1))
      EOUT(("Error writing Value %u to Register %d, of I2C-Bus number %d\n", val, reg, fI2C_Number));

}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
uint32_t nxyter::I2Cbus::getRegister(int reg)
{
   uint32_t val, err;

   setSlaveAddr(fSlaveAddr);
   if (fI2C_Number==CON19) {
     brd().put(ROC_I2C1_REGISTER, reg);
     brd().get(ROC_I2C1_DATA, val);
     brd().get(ROC_I2C1_ERROR, err);
   } else {
     brd().put(ROC_I2C2_REGISTER, reg);
     brd().get(ROC_I2C2_DATA, val);
     brd().get(ROC_I2C2_ERROR, err);
   }
   if (verbose() && (err==1))
      EOUT(("Error reading Register %d of I2C-Bus number %d\n", reg, fI2C_Number));

   return val;
}
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
void nxyter::I2Cbus::listRegisters()
{
   int val;
   for (int i=0; i<=45; i++) printf ("Register %d:\t%d\t0x%X\r\n", i, val=getRegister(i), val);
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
void nxyter::I2Cbus::testsetup()
{
   uint32_t err, val;

   setRegister(0, 0x0F);

   for (int i=1; i<=15; i++) {
      setRegister(i, 0xFF);
   }

   setRegister(16,160);
   setRegister(17,255);
   setRegister(18,35);
   setRegister(19,6);
   setRegister(20,95);
   setRegister(21,87);
   setRegister(22,100);
   setRegister(23,137);
   setRegister(24,255);
   setRegister(25,69);
   setRegister(26,15);
   setRegister(27,54);
   setRegister(28,92);
   setRegister(29,69);
   setRegister(30,30);
   setRegister(31,31);
   setRegister(32, 0);
   setRegister(33, 11);
}
//-------------------------------------------------------------------------------
