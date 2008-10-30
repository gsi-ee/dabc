#ifndef F_BUFHE_SWAP
#define F_BUFHE_SWAP

#include "typedefs.h"
typedef struct
{
INTS4  l_dlen;   /*  Length of data field in words */
INTS2 i_type;
INTS2 i_subtype;
INTS2 i_used;   /*  Used length of data field in words */
CHARS  h_end;   /*  Fragment at begin of buffer */
CHARS  h_begin;   /*  Fragment at end of buffer */
INTS4  l_buf;         /*  Current buffer number */
INTS4  l_evt;         /*  Number of fragments */
INTS4  l_current_i;   /*  Index, temporarily used */
INTS4  l_time[2];
INTS4  l_free[4];
} s_bufhe;

#endif
