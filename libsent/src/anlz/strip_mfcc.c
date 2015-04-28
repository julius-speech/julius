/**
 * @file   strip_mfcc.c
 *
 * <JA>
 * @brief  MFCC列からゼロサンプル列を取り除く
 *
 * エネルギー項の値からゼロサンプルのフレームを検出します．
 * </JA>
 * <EN>
 * @brief  Strip zero frames from MFCC data
 *
 * Zero sample frames will be detected by the energy coefficient of MFCC.
 * </EN>
 *
 * The detection is done by setting valid range of log energy.
 * 
 * However, since HTK parameter file has no information for framesize or
 * frequency, defining precise upper bound of power is impossible.
 *
 * Guess the power that is in log scale as described below,
 *
 *   framesize  :      1    100    400   1000  10000(can't be!)
 *   upper bound: 20.794 25.400 26.786 27.702 30.005
 * 
 * Thus it can be said that the range of [0..30] will work.
 *
 * When energy normalization was on, the parameters are normalized to:
 *
 *                 1.0 - (Emax - value) * ESCALE
 *                 
 * So the range becomes [1.0-Emax*ESCALE..1.0].  But
 * the engine cannot know whether the ENORMALIZE was on for given parameters.
 * 
 * As a conclusion, the safe bit is to set the range of log energy to
 *                   [-30..30]
 * hoping that escale is less than 1.0 (the HTK's default is 0.1).
 *
 * But remember, there are no guarantee that valid segments is not
 * misdetected.  When the misdetection frequently occurs on your MFCC file,
 * please try "-nostrip" option to turn off the stripping.
 *
 * @author Akinobu LEE
 * @date   Tue Feb 15 00:38:57 2005
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
#include <sent/htk_param.h>
#include <sent/mfcc.h>

/// Invalid log value range to detect zero sample frame.
#define IS_INVALID_FRAME_MFCC(A) ((A) < -30.0 || (A) > 30.0)

/** 
 * Guess where the absolute energy coefficient is.
 * 
 * @param param [in] parameter data
 * 
 * @return the guessed dimension number of energy coefficient, -1 on failure.
 */
static int
guess_abs_e_location(HTK_Param *param)
{
  short qualtype;
  int basenum, abs_e_num;
  qualtype = param->header.samptype & ~(F_COMPRESS | F_CHECKSUM);
  qualtype &= ~(F_BASEMASK);
  basenum = guess_basenum(param, qualtype);
  if (qualtype & F_ENERGY) {
    if (qualtype & F_ZEROTH) {
      abs_e_num = basenum + 1;
    } else {
      abs_e_num = basenum;
    }
  } else {
    /* absolute energy not included */
    jlog("Stat: strip_mfcc: absolute energy coef. not exist, stripping disabled\n");
    abs_e_num = -1;
  }
  return abs_e_num;
}

/** 
 * Strip zero frames from MFCC data.
 * 
 * @param param [in] parameter data
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
param_strip_zero(HTK_Param *param)
{
  unsigned int src,dst;
  int eloc;

  /* guess where the absolute energy coefficient is */
  eloc = guess_abs_e_location(param);
  if ((eloc = guess_abs_e_location(param)) < 0) return FALSE;
    
  /* guess the invalid range... */
  dst = 0;
  for(src=0;src<param->samplenum;src++) {
    if (IS_INVALID_FRAME_MFCC(param->parvec[src][eloc])) {
      jlog("Warning: strip_mfcc: frame %d has invalid energy, stripped\n", src);
      continue;
    }
    if (src != dst) {
      memcpy(param->parvec[dst], param->parvec[src], sizeof(VECT) * param->veclen);
    }
    dst++;
  }
  if (dst != param->samplenum) {
    jlog("Warning: strip_mfcc: input shrinked from %d to %d frames\n", param->samplenum, dst);
    param->header.samplenum = param->samplenum = dst;
  }

  return TRUE;
}
 
