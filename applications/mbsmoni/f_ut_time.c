#include "typedefs.h"
/*****************+***********+****************************************/
/*   GSI, Gesellschaft fuer Schwerionenforschung mbH                  */
/*   Postfach 11 05 52                                                */
/*   D-64220 Darmstadt                                                */
/*1+ C Procedure *************+****************************************/
/*+ Module      : f_ut_time                                           */
/*--------------------------------------------------------------------*/
/*+ CALLING     : CHARS * = f_ut_time(CHARS *)                        */
/*--------------------------------------------------------------------*/
/*+ PURPOSE     : Returns date/time string length 18 in format        */
/*                  day-month-year hours:min:sec                      */
/*1- C Procedure *************+****************************************/
#include <sys/types.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <time.h>

CHARS     *f_ut_time (CHARS *pc_time)
{
  time_t    t_time;
  struct timeb tp;
  struct tm st_time;

  ftime (&tp);
  st_time=*localtime(&tp.time);
  strftime(pc_time,30,"%d-%h-%y %T",&st_time);
  return ((CHARS *) pc_time);
}
