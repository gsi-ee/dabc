#ifndef ROC_Peripheral
#define ROC_Peripheral

//============================================================================
/*! \file roc/Peripheral.h
 *  \author ??? & Norbert Abel
 *
 * copyright: Kirchhoff-Institut fuer Physik
 *
 * This class implements basic peek + poke functions incl. error messaging
 */
//============================================================================


#include "roc/Board.h"


namespace roc {

class Peripheral {
   protected:
      bool fVerbose;
      roc::Board* fBoard;
   public:

      Peripheral(roc::Board* board);
      virtual ~Peripheral() {}

      roc::Board& brd();

      void setVerbose(bool on = true) { fVerbose = on; }
      bool verbose() const { return fVerbose; }
};

}

#endif
