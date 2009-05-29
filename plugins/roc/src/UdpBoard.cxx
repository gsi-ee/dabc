/********************************************************************
 * The Data Acquisition Backbone Core (DABC)
 ********************************************************************
 * Copyright (C) 2009-
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
 * Planckstr. 1
 * 64291 Darmstadt
 * Germany
 * Contact:  http://dabc.gsi.de
 ********************************************************************
 * This software can be used under the GPL license agreements as stated
 * in LICENSE.txt file which is part of the distribution.
 ********************************************************************/

#include "roc/UdpBoard.h"

#include "roc/defines.h"
#include "roc/udpdefines.h"

#include "dabc/logging.h"

roc::UdpBoard::UdpBoard() :
   roc::Board()
{
}

roc::UdpBoard::~UdpBoard()
{
}

const char* roc::xmlTransferWindow = "RocTransferWindow";


uint32_t roc::UdpBoard::getSW_Version()
{
   uint32_t val;
   get(ROC_SOFTWARE_VERSION, val);
   return val;
}

void roc::UdpBoard::BURST(uint32_t val)
{
   put(ROC_BURST, val);
}

bool roc::UdpBoard::getLowHighWater(int& lowWater, int& highWater)
{
   uint32_t low, high;
   if (get(ROC_LOWWATER, low) != 0) return false;
   if (get(ROC_HIGHWATER, high) != 0) return false;

   double k = 1024. / (UDP_PAYLOAD_OFFSET + MAX_UDP_PAYLOAD);

   lowWater = int(low / k);
   highWater = int(high / k);

   if (low/k - lowWater >= 0.5) lowWater++;
   if (high/k - highWater >= 0.5) highWater++;

   return true;
}

bool roc::UdpBoard::setLowHighWater(int lowWater, int highWater)
{
   double k = 1024. / (UDP_PAYLOAD_OFFSET + MAX_UDP_PAYLOAD);

   uint32_t low = uint32_t(lowWater * k);
   uint32_t high = uint32_t(highWater * k);

   if (lowWater*k - low >= 0.5) low++;
   if (highWater*k - high >= 0.5) high++;

   if ((low < 10) || (high < low + 10)) {
      EOUT(("Wrong values for high/low water markers"));
      return false;
   }

   // to avoid error messages, just move high limit before setting new low limit
   uint32_t curr_high;
   if (get(ROC_HIGHWATER, curr_high) != 0) return false;
   if (low > curr_high)
      if (put(ROC_HIGHWATER, low+1) != 0) return false;

   if (put(ROC_LOWWATER, low) !=0) return false;
   if (put(ROC_HIGHWATER, high) != 0) return false;

   return true;
}

void roc::UdpBoard::setConsoleOutput(bool terminal, bool network)
{
   put(ROC_CONSOLE_OUTPUT, (terminal ? 1 : 0) | (network ? 2 : 0));
}

void roc::UdpBoard::switchToConsole()
{
   put(ROC_SWITCHCONSOLE, 1);
}


