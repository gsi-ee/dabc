#ifndef SysCoreFEBs_H_
#define SysCoreFEBs_H_

//============================================================================
/*! \file roc/FEBs.h
 *  \author Norbert Abel
 *
 * copyright: Kirchhoff-Institut fuer Physik
 *
 * This class implements useful slow control functions
 */
//============================================================================


#include "roc/FEBboard.h"

namespace roc {

   class FEB1nxC : public FEBboard {
      public:
         FEB1nxC(roc::Board* board, int Connector, bool init = true);
         virtual void Init();
   };

   class FEB2nx : public FEBboard {
      public:
         FEB2nx(roc::Board* board, int Connector, bool init = true);
         virtual void Init();
   };

   class FEB4nx : public FEBboard {
      public:
         FEB4nx(roc::Board* board, bool init = true);
         virtual void Init();
   };

}

#endif /*SysCoreFEBs_H_*/
