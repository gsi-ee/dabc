#include "SysCoreData.h"

#include <string.h>
#include <stdio.h>

/*****************************************************************************
 * SysCoreData
 ****************************************************************************/

SysCoreData::SysCoreData()
{
   memset(data, 0, 6);
}

SysCoreData::SysCoreData(const SysCoreData& src)
{
   for (unsigned n=0;n<6;n++) 
      data[n] = src.data[n];
}

void SysCoreData::assignData(const uint8_t* src)
{
   memcpy(data, src, 6);
}

const char* SysCoreData::getCharData()
{
   static char sbuf[30];
   sprintf(sbuf, "%x : %x : %x : %x : %x : %x", data[0], data[1], data[2], data[3], data[4], data[5]);
   return sbuf;
}

uint64_t SysCoreData::CalcDistance(uint64_t start, uint64_t stop)
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

void SysCoreData::printData(unsigned kind) const
{
   if (kind & 7 == 0) return;
    
   if (kind & 4)
      printf("%02X:%02X:%02X:%02X:%02X:%02X ", data[0], data[1], data[2], data[3], data[4], data[5]); 
    
   if (kind & 1)
     printf("Msg:%u Roc:%u ", getMessageType(), getRocNumber());
    
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
            printf("AuxChn:%02x EpochLSB:%1x Ts:%04x Falling:%1x", getAuxChNum(), getAuxEpochLowBit(), getAuxTs(), getAuxFalling());
            break;
         case ROC_MSG_SYS:
            printf("SysType:%2x SysEpoch:%08x Nx:%1x", getSysMesType(), getSysMesEpoch(), getNxNumber());
            break;
         default: printf("Error in type id");
      } 
      
   printf("\n");   
}



// ____________________collection of old code _______________________

/*
uint16_t SysCoreData::getNxTs()
{
   uint32_t t;
   bigEndianMemcpy((uint8_t*)&t, (uint8_t*)&data[1], 4);
   t >>= 15;
   return (uint16_t)(t & SYSCORE_DATA_FOURTEEN_BITS);
}

uint16_t SysCoreData::getAdcValue()
{
   uint16_t t;
   bigEndianMemcpy((uint8_t*)&t, (uint8_t*)&data[4], 2);
   t >>= 3;
   return t & SYSCORE_DATA_TWELVE_BITS;
}

void SysCoreData::setNxTs(uint16_t v)
{
   uint32_t vv(v), tm;
   vv <<= 15;

   bigEndianMemcpy((uint8_t*)&tm, (uint8_t*)&vv, 4);

   uint32_t* tgt = (uint32_t*) &data[1];
   *tgt = (*tgt & ((~SYSCORE_DATA_FOURTEEN_BITS) << 15)) | tm;
}

uint32_t SysCoreData::getEpoch()
{
   uint32_t t;
   bigEndianMemcpy((uint8_t*)&t, (uint8_t*)&data[1], 4);
   return t;
}

void SysCoreData::setEpoch(uint32_t v)
{
   bigEndianMemcpy((uint8_t*)&data[1], (uint8_t*)&v, 4);
}

// sync message time stamp, 14 bit
uint16_t SysCoreData::getSyncTs()
{
   uint16_t t;
   bigEndianMemcpy((uint8_t*)&t, (uint8_t*)&data[1], 2);
   t >>= 2;
   return (uint16_t) (t & SYSCORE_DATA_FOURTEEN_BITS);
}

// sync message event number, 24 bit
uint32_t SysCoreData::getSyncEvNum()
{
   uint32_t t;
   bigEndianMemcpy((uint8_t*)&t, (uint8_t*)&data[2], 4);
   t >>= 2;
   return (uint32_t) (t & SYSCORE_DATA_TWENTYFOUR_BITS);
}

void SysCoreData::setSyncTs(uint16_t v)
{
   v <<= 2;
   uint16_t ts, mask, imask = ~(SYSCORE_DATA_FOURTEEN_BITS << 2);
   bigEndianMemcpy((uint8_t*)&ts, (uint8_t*)&v, 2);
   bigEndianMemcpy((uint8_t*)&mask, (uint8_t*)&imask, 2);

   uint16_t* tgt = (uint16_t*) &data[1];
   
   *tgt = (*tgt & mask) | ts;
}

void SysCoreData::setSyncEvNum(uint32_t v)
{
   v <<= 2;
   uint32_t ev;
   bigEndianMemcpy((uint8_t*)&ev, (uint8_t*)&v, 4);
   
   uint32_t mask, imask = ~(SYSCORE_DATA_TWENTYFOUR_BITS << 2);
   bigEndianMemcpy((uint8_t*)&mask, (uint8_t*)&imask, 4);
   
   uint32_t* tgt = (uint32_t*) &data[2];

   *tgt = (*tgt & mask) | ev;
}

uint8_t SysCoreData::getAuxChNum()
{
   uint16_t t;
   bigEndianMemcpy((uint8_t*)&t, (uint8_t*)&data[0], 2);
   t >>= 3;
   return (uint8_t) (t & SYSCORE_DATA_SEVEN_BITS);
}


uint16_t SysCoreData::getAuxTs()
{
   uint32_t t;
   bigEndianMemcpy((uint8_t*)&t, (uint8_t*)&data[1], 4);
   t >>= 13;
   return (uint16_t) (t & SYSCORE_DATA_FOURTEEN_BITS);

}

void SysCoreData::setAuxTs(uint16_t v)
{
   uint32_t vv = v;
   vv <<= 13;
   uint32_t ts, mask, imask = ~(SYSCORE_DATA_FOURTEEN_BITS << 13);
   bigEndianMemcpy((uint8_t*)&ts, (uint8_t*)&vv, 4);
   bigEndianMemcpy((uint8_t*)&mask, (uint8_t*)&imask, 4);
   uint32_t* tgt = (uint32_t*) &data[1];
   *tgt = (*tgt & mask) | ts;
}

uint32_t SysCoreData::getSysMesEpoch()
{
   uint32_t t;
   bigEndianMemcpy((uint8_t*)&t, (uint8_t*)&data[2], 4);
   return t;
}

void SysCoreData::bigEndianMemcpy(uint8_t* target, const uint8_t* src, unsigned bytes)
{
   for(unsigned i = 0;i < bytes; i++)
      target[i] = src[bytes - i - 1];
}

*/




