#ifndef SYSCORETEST_H_
#define SYSCORETEST_H_

//============================================================================
/*! \file roc/Test.h
 *  \author Norbert Abel
 *
 * copyright: Kirchhoff-Institut fuer Physik
 *
 * This class implements autocalibration functions
 */
//============================================================================

#include "roc/Calibrate.h"

namespace roc {

class Test : public Calibrate {
   protected:
      int fC19;
      int fC20;
      int fFebn;
      int fNxn[2];
      bool fDataOK;

   public:

      Test(roc::Board* board);

      virtual ~Test();

      bool I2C_Test(int Connector, int SlaveAddr);
      bool ROC_HW_Test();
      bool Search_and_Connect_FEB();
      bool ADC_SPI_Test();
      bool Full_Data_Test();
      bool Summary();
      bool Persistence();
      bool FULL_TEST();
};

}

#endif /*SYSCORETEST_H_*/
