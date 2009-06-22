#include "nxyter/Data.h"

#include <string.h>
#include <stdio.h>

/*****************************************************************************
 * nxyter::Data
 ****************************************************************************/

nxyter::Data::Data()
{
   memset(data, 0, 6);
}

nxyter::Data::Data(const Data& src)
{
   for (unsigned n=0;n<6;n++)
      data[n] = src.data[n];
}

void nxyter::Data::assignData(const uint8_t* src)
{
   memcpy(data, src, 6);
}

const char* nxyter::Data::getCharData()
{
   static char sbuf[30];
   sprintf(sbuf, "%x : %x : %x : %x : %x : %x", data[0], data[1], data[2], data[3], data[4], data[5]);
   return sbuf;
}

uint64_t nxyter::Data::CalcDistance(uint64_t start, uint64_t stop)
{
   if (start>stop) {
      stop += 0x3FFFFFFFFFFFLLU;
      if (start>stop) {
         printf("Civiar error in CalcDistance\n");
         return 0;
      }
   }

   return stop - start;
}

void nxyter::Data::printData(unsigned kind) const
{
   if (kind & 7 == 0) return;

   if (kind & 4)
      printf("%02X:%02X:%02X:%02X:%02X:%02X ", data[0], data[1], data[2], data[3], data[4], data[5]);

   if (kind & 1)
     printf("Msg:%u Roc:%u ", getMessageType(), rocNumber());

   if (kind & 2)
      switch (getMessageType()) {
         case ROC_MSG_NOP:
            printf("NOP");
            break;
         case ROC_MSG_HIT:
            printf("Nx:%1x Id:%02x Ts:%04x Last:%1x Adc:%03x Msb:%1x Pup:%1x Oflw:%1x",
                   getNxNumber(), getId(), getNxTs(), getLastEpoch(), getAdcValue(), getMsbOfLts(), getPileUp(), getOverFlow());
            break;
         case ROC_MSG_EPOCH:
            printf("Epoch:%08x Missed:%02x", getEpoch(), getMissed());
            break;
         case ROC_MSG_SYNC:
            printf("SyncChn:%1x EpochLSB:%1x Ts:%04x Evnt:%06x  Flag:%02x", getSyncChNum(), getSyncEpochLowBit(), getSyncTs(), getSyncEvNum(), getSyncStFlag());
            break;
         case ROC_MSG_AUX:
            printf("AuxChn:%02x EpochLSB:%1x Ts:%04x Falling:%1x PileUp:%1x", getAuxChNum(), getAuxEpochLowBit(), getAuxTs(), getAuxFalling(), getAuxPileUp());
            break;
         case ROC_MSG_SYS:
            printf("SysType:%2x SysEpoch:%08x Nx:%1x", getSysMesType(), getSysMesEpoch(), getNxNumber());
            break;
         default: printf("Error in type id");
      }

   printf("\n");
}

