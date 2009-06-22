#ifndef ROC_FEB_H
#define ROC_FEB_H

//============================================================================
/*! \file roc/FEB.h
 *  \author Norbert Abel
 *
 * copyright: Kirchhoff-Institut fuer Physik
 *
 * This class implements useful slow control functions
 */
//============================================================================

#include "roc/Peripheral.h"

#include "roc/defines.h"
#include "roc/ADCchip.h"
#include "nxyter/Chip.h"

#include <vector>

namespace roc {

class FEBboard : public Peripheral
{
   private:
   protected:
      int fConnector;
      std::vector<nxyter::Chip*> fNX;
      ADCchip fADC;
   public:

      FEBboard(roc::Board* board, int conn = CON19);
      ~FEBboard();

      void selectFEB4nx(uint32_t val);
      void reset_I2C();
      int numNX() const { return (int) fNX.size(); }
      nxyter::Chip &NX(int index);
      ADCchip &ADC() { return fADC; }
      int getConnector() { return fConnector; }

      virtual void Init(bool reseti2c = true) {}
};

}

#endif /*SysCoreFEB_H_*/
