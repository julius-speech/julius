/**
 * @file   strip.c
 *
 * <JA>
 * @brief  音声データからゼロサンプル列を取り除く
 * </JA>
 * <EN>
 * @brief  Strip zero samples from speech data
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Tue Feb 15 00:30:38 2005
 *
 * $Revision: 1.6 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/speech.h>

/// Distinction function: sequence of 0 and -2^15 are invalid
#define IS_INVALID_SAMPLE(A) ((A) == 0 || (A) == -32767)
/// Length of zero sample to detect as invalid sequence.
#define WINDOWLEN 16

/** 
 * Strip zero samples from speech data.
 * 
 * @param a [I/O] speech data
 * @param len [in] length of above
 * 
 * @return new length after stripping.
 */
int
strip_zero(SP16 a[], int len)
{
  int src,dst;
  int bgn,mode,j;

  dst = 0;
  bgn = 0;
  mode = 0;

  for (src = 0; src < len; src++) {
    if (IS_INVALID_SAMPLE(a[src])) {
      if (mode == 0) {          /* first time */
        bgn = src;
        mode = 1;
      }
      /* skip */
    } else {
      if (mode == 1) {
        mode = 0;
        if ((src - bgn) < WINDOWLEN) {
          for(j=bgn;j<src;j++) {
            a[dst++] = a[j];
	  }
        } else {
          /* deleted (leave uncopied) */
	  jlog("Warning: strip: sample %d-%d has zero value, stripped\n", bgn, src-1);
        }
      }
      a[dst++] = a[src];
    }
  }
  /* end process */
  if (mode == 1) {
    mode = 0;
    if ((src - bgn) < WINDOWLEN) {
      /* restore */
      for(j=bgn;j<src;j++) {
        a[dst++] = a[j];
      }
    } else {
      /* deleted (leave uncopied) */
      jlog("Warning: strip: sample %d-%d is invalid, stripped\n", bgn, src-1);
    }
  }
  
  return(dst);
}
