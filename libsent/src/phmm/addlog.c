/**
 * @file   addlog.c
 * 
 * <JA>
 * @brief  対数値の高速和算関数
 * </JA>
 * 
 * <EN>
 * @brief  Rapid addition of log values
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 13:23:50 2005
 *
 * $Revision: 1.5 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/hmm.h>

#define TBLSIZE 500000		///< Table size (precision depends on this)
#define VRANGE 15               ///< Must be larger than -LOG_ADDMIN
#define TMAG 33333.3333         ///< TBLSIZE / VRANGE

static LOGPROB tbl[TBLSIZE];    ///< Table of @f$\log (1+e^x)@f$
static boolean built_tbl = FALSE;///< TRUE after tbl has built

/** 
 * @brief  Generate a value tables of @f$\log (1+e^x)@f$.
 *
 * @f$x@f$ is from 0 to (- VRANGE), and table size is TBLSIZE.
 * 
 */
void
make_log_tbl()
{
  LOGPROB f;
  int i;

  if (built_tbl == FALSE) {
    jlog("Stat: addlog: generating addlog table (size = %d kB)\n", (TBLSIZE * sizeof(LOGPROB)) / 1024);
    for (i=0;i<TBLSIZE;i++){
      f = - ((float)VRANGE * (float)i / (float)TBLSIZE);
      tbl[i] = log(1 + exp(f));
      /*if (i < 10 || i > TBLSIZE - 10) j_printf("%f: %d(%f)\n", f, i, tbl[i]);*/
    }
    jlog("Stat: addlog: addlog table generated\n");
    built_tbl = TRUE;
  }
}

/** 
 * Rapid computation of @f$\log (e^x + e^y)@f$.
 *
 * If value differs more than LOG_ADDMIN, the larger value will be returned
 * as is.
 * 
 * @param x [in] log value
 * @param y [in] log value
 * 
 * @return result value.
 */
LOGPROB
addlog(LOGPROB x, LOGPROB y)
{
  /* return(log(exp(x)+exp(y))) */
  LOGPROB tmp;
  unsigned int idx;
  
  if (x < y) {
    if ((tmp = x - y) < LOG_ADDMIN) return y;
    else {
      idx = (unsigned int)((- tmp) * TMAG + 0.5);
      /* jlog("%f == %f\n",tbl[idx],log(1 + exp(tmp))); */
      return (y + tbl[idx]);
    }
  } else {
    if ((tmp = y - x) < LOG_ADDMIN) return x;
    else {
      idx =(unsigned int)((- tmp) * TMAG + 0.5);
      /* jlog("%f == %f\n",tbl[idx],log(1 + exp(tmp))); */
      return (x + tbl[idx]);
    }
  }
}

/** 
 * Rapid computation of @f$\log (\sum_{i=1}^N e^{x_i})@f$.
 * 
 * @param a [in] array of log values
 * @param n [in] length of above
 * 
 * @return the result value.
 */
LOGPROB
addlog_array(LOGPROB *a, int n)
{
  LOGPROB tmp;
  LOGPROB x,y;
  unsigned int idx;

  y = LOG_ZERO;
  for(n--; n >= 0; n--) {
    x = a[n];
    if (x > y) {
      tmp = x; x = y; y = tmp;
    }
    /* always y >= x */
    if ((tmp = x - y) < LOG_ADDMIN) continue;
    else {
      idx = (unsigned int)((- tmp) * TMAG + 0.5);
      y += tbl[idx];
    }
  }
  return(y);
}
