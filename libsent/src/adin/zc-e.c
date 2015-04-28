/**
 * @file   zc-e.c
 *
 * <JA>
 * @brief  音声区間検出のための零交差数カウント
 *
 * 与えられたバッファ長内の零交差数をカウントします．
 * 同時に, 呼ばれたバッファを順次バッファ長分だけ古いものに入れ替えます．
 * このため入力はバッファ長分だけ遅延することになります．
 * </JA>
 * <EN>
 * @brief  Count zero cross and level for speech detection
 *
 * Count zero cross number within the given length of cycle buffer.
 * The content of the cycle buffer will be swapped with the newest data,
 * So the input delays for the length of the cycle buffer.
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Mon Feb 14 19:11:34 2005
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

/* Sat Feb 19 13:48:00 JST 1994 */
/*  Kawahara 1986 */
/*  Munetsugu 1991 */
/*  shinohara 1993 */
/*  mikik 1993 */
/*  ri 1997 for cycle buffer */

#include <sent/stddefs.h>
#include <sent/adin.h>

/** 
 * Allocate buffers for zerocross counting.
 *
 * @param zc [i/o] zerocross work area 
 * @param length [in] Cycle buffer size = Number of samples to hold
 */
void
init_count_zc_e(ZEROCROSS *zc, int length)
{
  /* data spool for header-margin */
  zc->data = (SP16 *)mymalloc(length * sizeof(SP16));
  /* zero-cross location */
  zc->is_zc = (int *)mymalloc(length * sizeof(int));

  zc->length = length;
}

/** 
 * Initialize all parameters and buffers for zero-cross counting.
 *
 * @param zc [i/o] zerocross work area 
 * @param c_trigger [in] Tgigger level threshold
 * @param c_length [in] Cycle buffer size = Number of samples to hold
 * @param c_offset [in] Static DC offset of input data
 */
void
reset_count_zc_e(ZEROCROSS *zc, int c_trigger, int c_length, int c_offset)
{
  int i;

  if (zc->length != c_length) {
    jlog("Warning: zerocross buffer length changed, re-allocate it\n");
    free_count_zc_e(zc);
    init_count_zc_e(zc, c_length);
  }

  zc->trigger = c_trigger;
  zc->offset = c_offset;

  zc->zero_cross = 0;
  zc->is_trig = FALSE;
  zc->sign = ZC_POSITIVE;
  zc->top = 0;
  zc->valid_len = 0;

  for (i=0; i<c_length; i++){
    zc->is_zc[i] = ZC_UNDEF;
  }
}

/** 
 * End procedure: free all buffers.
 * 
 * @param zc [i/o] zerocross work area 
 */
void
free_count_zc_e(ZEROCROSS *zc)
{
  free(zc->is_zc);
  free(zc->data);
}

/** 
 * Adding buf[0..step-1] to the cycle buffer and update the count of
 * zero cross.   Also swap them with the oldest ones in the cycle buffer.
 * Also get the maximum level in the cycle buffer.
 * 
 * @param zc [i/o] zerocross work area 
 * @param buf [I/O] new samples, will be swapped by old samples when returned.
 * @param step [in] length of above.
 * 
 * @return zero-cross count of the samples in the cycle buffer.
 */
int
count_zc_e(ZEROCROSS *zc, SP16 *buf, int step)
{
  int i;
  SP16 tmp, level;

  level = 0;
  for (i=0; i<step; i++) {
    if (zc->is_zc[zc->top] == TRUE) {
      zc->zero_cross--;
    }
    zc->is_zc[zc->top] = FALSE;
    /* exchange old data and buf */
    tmp = buf[i] + zc->offset;
    if (zc->is_trig) {
      if (zc->sign == ZC_POSITIVE && tmp < 0) {
	zc->zero_cross++;
	zc->is_zc[zc->top] = TRUE;
	zc->is_trig = FALSE;
	zc->sign = ZC_NEGATIVE;
      } else if (zc->sign == ZC_NEGATIVE && tmp > 0) {
	zc->zero_cross++;
	zc->is_zc[zc->top] = TRUE;
	zc->is_trig = FALSE;
	zc->sign = ZC_POSITIVE;
      }
    }
    if (abs(tmp) > zc->trigger) {
      zc->is_trig = TRUE;
    }
    if (abs(tmp) > level) level = abs(tmp);
    zc->data[zc->top] = buf[i];
    zc->top++;
    if (zc->valid_len < zc->top) zc->valid_len = zc->top;
    if (zc->top >= zc->length) {
      zc->top = 0;
    }
  }
  zc->level = (int)level;
  return (zc->zero_cross);
}

/** 
 * Flush samples in the current cycle buffer.
 * 
 * @param zc [i/o] zerocross work area 
 * @param newbuf [out] the samples in teh cycle buffer will be written here.
 * @param len [out] length of above.
 */
void
zc_copy_buffer(ZEROCROSS *zc, SP16 *newbuf, int *len)
{
  int i, t;
  if (zc->valid_len < zc->length) {
    t = 0;
  } else {
    t = zc->top;
  }
  for(i=0;i<zc->valid_len;i++) {
    newbuf[i] = zc->data[t];
    if (++t == zc->length) t = 0;
  }
  *len = zc->valid_len;
}
