#include "typedefs.h"
/* Generated from EE$ROOT:[GOOFY.VME]SA$VE10_1.VMETEMP;  */
/* Swapping enabled           */
  /*  ================= GSI VME Event header =======================  */
typedef struct
{
INTS4  l_dlen;   /*  Data length + 4 in words */
INTS2 i_subtype;
INTS2 i_type;
INTS2 i_trigger;   /*  Trigger number */
INTS2 i_dummy;   /*  Not used yet */
INTS4  l_count;   /*  Current event number */
} s_ve10_1 ;
  /* ------------------------------------------------------------------ */
