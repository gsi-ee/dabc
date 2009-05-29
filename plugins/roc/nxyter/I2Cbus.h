#ifndef NXYTER_I2Cbus_H
#define NXYTER_I2Cbus_H

//============================================================================
/*! \file nxyter/I2Cbus.h
 *  \author Norbert Abel
 *
 * copyright: Kirchhoff-Institut fuer Physik
 *
 * This class implements useful nXYTER I2C functions
 */
//============================================================================


#include "roc/Peripheral.h"

namespace nxyter {

class I2Cbus : public roc::Peripheral
{
   protected:
      int fI2C_Number;
      int fSlaveAddr;
   public:

      I2Cbus(roc::Board* board, int num);

      virtual ~I2Cbus();

      void setNumber(int num);
      int  getNumber();
      void setSlaveAddr(int addr);
      void setRegister(int reg, uint32_t val);
      uint32_t getRegister(int reg);
      void listRegisters();
      void testsetup();
};

}

#endif /*SYSCOREUTIL_I2C_H_*/
