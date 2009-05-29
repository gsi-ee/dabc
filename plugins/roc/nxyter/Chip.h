#ifndef SYSCOREUTIL_NX_H_
#define SYSCOREUTIL_NX_H_

//============================================================================
/*! \file nxyter/Chip.h
 *  \author Norbert Abel
 *
 * copyright: Kirchhoff-Institut fuer Physik
 *
 * This class implements useful nXYTER functions
 */
//============================================================================

#include "roc/Peripheral.h"
#include "nxyter/I2Cbus.h"

namespace nxyter {

class Chip : public roc::Peripheral
{
   protected:
      nxyter::I2Cbus fI2C;
      int fNumber;
      int fConnector;
   public:

      Chip(roc::Board* board, int num, int con);

      virtual ~Chip();

      void setNumber(int num);
      int  getNumber();
      void activate();
      void deactivate();
      uint32_t getChannelDelay();
      void setChannelDelay(uint32_t val);
      nxyter::I2Cbus& I2C() { return fI2C; }
};

}
#endif /*SYSCOREUTIL_NX_H_*/
