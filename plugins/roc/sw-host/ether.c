#include "ether.h"

void cpy_ether_addr(Xuint8 *dest, Xuint8 *src)
{
   //i think the compiler will optimize possibly better than this
   
//   *( (Xuint32*)dest ) = *( (Xuint32*)src );
//   *( (Xuint16*)(dest + 4) ) = *( (Xuint16*)(src + 4) );
    
   int i = ETHER_ADDR_LEN;
   while (i--) dest[i] = src[i];
}

int ether_addr_equal(Xuint8 *one, Xuint8 *two)
{
   int i;
   for(i = 0; i < ETHER_ADDR_LEN; i++)
      if (one[i] != two[i])
         return 0;
   return 1;
}
