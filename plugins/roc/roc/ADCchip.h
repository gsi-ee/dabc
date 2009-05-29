#ifndef SYSCOREUTIL_ADC_H_
#define SYSCOREUTIL_ADC_H_

//============================================================================
/*! \file roc/ADCchip.h
 *  \author Norbert Abel
 *
 * copyright: Kirchhoff-Institut fuer Physik
 *
 * This class implements useful ADC functions
 */
//============================================================================

#include "roc/Peripheral.h"

namespace roc {

class ADCchip : public Peripheral
{
   protected:
      int fADC_Connector;
   public:

      ADCchip(roc::Board* board);

      virtual ~ADCchip();

      void setConnector(int num);
      int getConnector();
      void setRegister(int reg, uint32_t val);
      uint32_t getRegister(int reg);

      uint32_t getChannelLatency(int channel);
      void setChannelLatency(int channel, uint32_t val);
      void setChannelPort(int channel, uint32_t val);

      uint32_t getSR_INIT();
      void setSR_INIT(uint32_t val);
      uint32_t getBUFG();
      void setBUFG(uint32_t val);
      void setSHIFT(uint32_t shift);
};

}

#endif /*SYSCOREUTIL_ADC_H_*/
