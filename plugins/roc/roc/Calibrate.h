#ifndef SYSCORECALIBRATE_H_
#define SYSCORECALIBRATE_H_

//============================================================================
/*! \file roc/Calibrate.h
 *  \author Norbert Abel
 *
 * copyright: Kirchhoff-Institut fuer Physik
 *
 * This class implements autocalibration functions
 */
//============================================================================

#include "roc/Peripheral.h"

#include "roc/FEBs.h"

#include <vector>

namespace roc {

class Calibrate : public roc::Peripheral {
   protected:

      std::vector<roc::FEBboard*> _febs;

      int fLTS_Delay;
      int fNX_number;
      int fFEB_Number;
      int fsec;

      int Start_DAQ();
      int Stop_DAQ();
      void my_start(void);
      unsigned long int my_stop(void);

   public:
      Calibrate(roc::Board* board);

      virtual ~Calibrate();

      void addFEB(roc::FEBboard* _feb);
      roc::FEBboard& FEB(int n);

      int gui_prepare(int feb, int nx_number);
      int prepare(int feb, int nx_number);
      int autodelay();
      int autolatency(int range_min, int range_max);
      uint32_t ungray(int x, int n);
};

}

#endif //SYSCORECALIBRATE_H_
