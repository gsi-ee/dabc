#include "crc32.h"

#define CRC32_TABLE_SIZE 256

static const Xuint32 CRC32_POLYNOMIAL = 0x04c11db7;

static Xuint32 crc32_table[CRC32_TABLE_SIZE];

Xuint32 reflect(Xuint32 value, Xuint32 bits)
{
   Xuint32 i, v = 0;
   for (i=0; i < bits; ++i) {
      if (value & 0x01)
         v = v | ( 1 << (bits - i - 1) );
      value = value >> 1;
   }
   return v;
}

void crc32_init()
{
   Xuint32 i, j;
   for (i=0; i < CRC32_TABLE_SIZE; ++i) {
      Xuint32 temp;
      Xuint32 reflect_value = reflect(i, 8);
      for (j=0; j < 8; ++j) {
         Xuint32 v = 0;
         if (reflect_value & (1 << 31))
            v = CRC32_POLYNOMIAL;
         temp = (reflect_value << 1) ^ v;
      }
      crc32_table[i] = reflect(temp, 32);
   }
}

Xuint32 crc32(void *data, Xuint32 len)
{
   Xuint32 i, crc = 0xffffffff;
   Xuint8 *ptr = data;
   for (i=0; i < len; ++i) {
      Xuint32 index = (crc & 0xff) ^ ptr[i];
      crc = crc32_table[index] ^(crc >> 8);
   }
   return crc ^ 0xffffffff;
}

/***************************************************************************
 * Function: ip_sum_calc
 * Description: Calculate the 16 bit IP sum.
****************************************************************************/

Xuint16 crc16(Xuint16* iph, int len)
{
/*   
      Xuint16 word16;
Xuint32 sum=0;
Xuint16 i;
    
        // make 16 bit words out of every two adjacent 8 bit words in the packet
        // and add them up
        for (i=0;i < len;i=i+2){
                word16 =((iph[i]<<8)&0xFF00)+(iph[i+1]&0xFF);
                sum = sum + (Xuint32) word16;       
        }
        
        // take only 16 bits out of the 32 bit sum and add up the carries
        while (sum>>16)
          sum = (sum & 0xFFFF)+(sum >> 16);

        // one's complement the result
   sum = ~sum;
        
   return ((Xuint16) sum);
   /*/

    
/*    
   Xuint32   sum = 0;
   int i = 0;
   Xuint8* ptr = (Xuint8*)iph;      
   for (i=0; i < len; i = i + 2) {
      sum += *((Xuint16 *)(ptr + i));
   }
   while (sum>>16)
      sum = (sum & 0xFFFF)+(sum >> 16);
   
   return ~sum;
*/    

    Xuint32   sum = 0;
    len = len >> 1; // size in 16 bit values
    while (len--) sum += *iph++;

   while (sum>>16)
      sum = (sum & 0xFFFF)+(sum >> 16);

   return ~sum;
}


