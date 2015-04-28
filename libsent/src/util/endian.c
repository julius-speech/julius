/**
 * @file   endian.c
 * 
 * <JA>
 * @brief  バイトオーダー変換
 * </JA>
 * 
 * <EN>
 * @brief  Byte order swapping
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 15:43:46 2005
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

/** 
 * Generic byte-swapping functions for any size of unit.
 * 
 * @param buf [i/o] data buffer
 * @param unitbyte [in] size of unit in bytes
 * @param unitnum [in] number of unit in the buffer
 */
void
swap_bytes(char *buf, size_t unitbyte, size_t unitnum)
{
  char *p, c;
  int i, j;

  p = buf;
  while (unitnum > 0) {
    i=0; j=unitbyte-1;
    while(i<j) {
      c = p[i]; p[i] = p[j]; p[j] = c;
      i++;j--;
    }
    p += unitbyte;
    unitnum--;
  }
}

/** 
 * Byte swapping of 16bit audio samples.
 * 
 * @param buf [i/o] data buffer
 * @param len [in] length of above
 */
void
swap_sample_bytes(SP16 *buf, int len)
{
  char *p;
  char t;
  int i;
  
  p = (char *)buf;

  for (i=0;i<len;i++) {
    t = *p;
    *p = *(p + 1);
    *(p + 1) = t;
    p += 2;
  }
}
