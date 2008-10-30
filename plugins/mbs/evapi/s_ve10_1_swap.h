#ifndef S_VE10_1_SWAP
#define S_VE10_1_SWAP

#include "typedefs.h"
typedef struct
{
INTS4  l_dlen;   /*  Data length + 4 in words */
INTS2 i_type;
INTS2 i_subtype;
INTS2 i_dummy;   /*  Not used yet */
INTS2 i_trigger;   /*  Trigger number */
INTS4  l_count;   /*  Current event number */
} s_ve10_1 ;

#endif
