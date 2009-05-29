//============================================================================
// Name        : rocsrc/Peripheral.cpp
// Author      : Norbert Abel
// Copyright   : Kirchhoff-Institut für Physik
//============================================================================

#include "roc/Peripheral.h"

#include <stdexcept>

roc::Peripheral::Peripheral(roc::Board* board)
{
  fBoard = board;
  fVerbose = true;
}

roc::Board& roc::Peripheral::brd()
{
   if (fBoard==0) throw std::runtime_error("Board object not specified");
   return *fBoard;
}
