//============================================================================
// Name        : rocsrc/ADCchip.cpp
// Author      : Norbert Abel
// Copyright   : Kirchhoff-Institut für Physik
//============================================================================

#include "roc/ADCchip.h"

#include "roc/defines.h"
#include "dabc/logging.h"

//-------------------------------------------------------------------------------
roc::ADCchip::ADCchip(roc::Board* board) : roc::Peripheral(board)
{
   fADC_Connector = CON19;
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
roc::ADCchip::~ADCchip()
{
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
void roc::ADCchip::setConnector(int num)
{
    if ((num >= CON19) && (num <= CON20)) fADC_Connector = num;
    else EOUT(("Not a valid ADC-Connector!%d \n", num));
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
int roc::ADCchip::getConnector()
{
    return fADC_Connector;
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
void roc::ADCchip::setRegister(int reg, uint32_t val)
{
   if (fADC_Connector==CON19) {
      brd().put(ROC_ADC_ADDR, reg);
      brd().put(ROC_ADC_REG,  val);
      brd().put(ROC_ADC_ADDR, 255);
      brd().put(ROC_ADC_REG,  1);
   } else {
      brd().put(ROC_ADC_ADDR2, reg);
      brd().put(ROC_ADC_REG2,  val);
      brd().put(ROC_ADC_ADDR2, 255);
      brd().put(ROC_ADC_REG2,  1);
   }
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
uint32_t roc::ADCchip::getRegister(int reg)
{
   uint32_t val;

   if (fADC_Connector==CON19) {
      brd().put(ROC_ADC_ADDR,   reg);
      brd().get(ROC_ADC_ANSWER, val);
   } else {
      brd().put(ROC_ADC_ADDR2,   reg);
      brd().get(ROC_ADC_ANSWER2, val);
   }
   return val;
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
uint32_t roc::ADCchip::getChannelLatency(int channel)
{
   uint32_t val;
   if (channel==1) brd().get(ROC_ADC_LATENCY1, val);
   if (channel==2) brd().get(ROC_ADC_LATENCY2, val);
   if (channel==3) brd().get(ROC_ADC_LATENCY3, val);
   if (channel==4) brd().get(ROC_ADC_LATENCY4, val);
   return val;
}
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
void roc::ADCchip::setChannelLatency(int channel, uint32_t val)
{
   if (channel==1) brd().put(ROC_ADC_LATENCY1, val);
   if (channel==2) brd().put(ROC_ADC_LATENCY2, val);
   if (channel==3) brd().put(ROC_ADC_LATENCY3, val);
   if (channel==4) brd().put(ROC_ADC_LATENCY4, val);
}
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
void roc::ADCchip::setChannelPort(int channel, uint32_t val)
{
   if (channel==1) brd().put(ROC_ADC_PORT_SELECT1, val);
   if (channel==2) brd().put(ROC_ADC_PORT_SELECT2, val);
   if (channel==3) brd().put(ROC_ADC_PORT_SELECT3, val);
   if (channel==4) brd().put(ROC_ADC_PORT_SELECT4, val);
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
uint32_t roc::ADCchip::getSR_INIT()
{
   uint32_t val;
   if (fADC_Connector==CON19)
     brd().get(ROC_SR_INIT,  val);
   else
     brd().get(ROC_SR_INIT2, val);
   return val;
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
void roc::ADCchip::setSR_INIT(uint32_t val)  {
   if (fADC_Connector==CON19) {
     brd().put(ROC_SR_INIT,  val);

   } else {
     brd().put(ROC_SR_INIT2, val);
   }
}
//-------------------------------------------------------------------------------




//-------------------------------------------------------------------------------
uint32_t roc::ADCchip::getBUFG()  {
   uint32_t val;
   if (fADC_Connector==CON19) {
     brd().get(ROC_BUFG_SELECT,  val);

   } else {
     brd().get(ROC_BUFG_SELECT2, val);
   }
   return val;
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
void roc::ADCchip::setBUFG(uint32_t val)  {
   if (fADC_Connector==CON19) {
     brd().put(ROC_BUFG_SELECT,  val);

   } else {
     brd().put(ROC_BUFG_SELECT2, val);
   }
}
//-------------------------------------------------------------------------------




//-------------------------------------------------------------------------------
void roc::ADCchip::setSHIFT(uint32_t shift)  {
   uint32_t sr_init;
   uint32_t bufg_select;

   bufg_select = shift & 0x01;
   shift = shift >> 1;

   sr_init = 255;
   sr_init = sr_init << shift;

   sr_init += sr_init >> 16;
   sr_init &= 0xffff;

   if (verbose()) DOUT0(("SR_INIT:     %X\r\n", sr_init));
   if (verbose()) DOUT0(("BUFG_SELECT: %X\r\n", bufg_select));

   setSR_INIT(sr_init);
   setBUFG(bufg_select);
}
//-------------------------------------------------------------------------------



//-------------------------------------------------------------------------------
uint32_t roc::ADCchip::getADCdirect(int con, int num) {
   uint32_t val=0;

   if ((con==CON19)&&(num==PORT_A)) brd().get(ROC_ADC_DIRECT_1a, val);
   if ((con==CON19)&&(num==PORT_B)) brd().get(ROC_ADC_DIRECT_1b, val);
   if ((con==CON19)&&(num==PORT_C)) brd().get(ROC_ADC_DIRECT_1c, val);
   if ((con==CON19)&&(num==PORT_D)) brd().get(ROC_ADC_DIRECT_1d, val);
   if ((con==CON20)&&(num==PORT_A)) brd().get(ROC_ADC_DIRECT_2a, val);
   if ((con==CON20)&&(num==PORT_B)) brd().get(ROC_ADC_DIRECT_2b, val);
   if ((con==CON20)&&(num==PORT_C)) brd().get(ROC_ADC_DIRECT_2c, val);
   if ((con==CON20)&&(num==PORT_D)) brd().get(ROC_ADC_DIRECT_2d, val);

   return val;
}
//-------------------------------------------------------------------------------




