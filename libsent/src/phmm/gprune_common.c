/**
 * @file   gprune_common.c
 * 
 * <JA>
 * @brief  混合ガウス分布計算: Gaussian pruning (共通部)
 *
 * ここには Gaussian pruningにおいて各アルゴリズムで共通に用いられる
 * キャッシュ操作関数などが含まれています．
 * </JA>
 * 
 * <EN>
 * @brief  Calculate probability of a set of Gaussian densities by
 * Gaussian pruning: common functions
 *
 * This file contains functions concerning codebook level cache
 * manipulation, commonly used for the Gaussian pruning functions.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Fri Feb 18 18:10:58 2005
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
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/hmm_calc.h>

/** 
 * Find where the new value should be inserted to the OP_cacled_score,
 * already sorted by score, using binary search.
 * 
 * @param score [in] the new score to be inserted
 * @param len [in] length of data in OP_calced_score
 * 
 * @return the insertion point.
 */
static int
find_insert_point(LOGPROB *calced_score, LOGPROB score, int len)
{
  /* binary search on score */
  int left = 0;
  int right = len - 1;
  int mid;

  while (left < right) {
    mid = (left + right) / 2;
    if (calced_score[mid] > score) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return(left);
}

/** 
 * @brief  Store a score to the current list of computed Gaussians.
 * 
 * Store the calculated score of a Gaussian to OP_calced_score, with its
 * corresponding mixture id to OP_calced_id.
 *
 * The OP_calced_score and OP_calced_id always holds the
 * (OP_gprune_num)-best scores and ids.  If the number of stored
 * Gaussian from start has reached OP_gprune_num and the given score is
 * below the bottom, it will be dropped.  Else, the new
 * score will be inserted and the bottom will be dropped from the list.
 *
 * The OP_calced_score will always kept sorted by the scores.
 * 
 * @param wrk [i/o] HMM computation work area
 * @param id [in] mixture id of the Gaussian to store
 * @param score [in] score of the Gaussian to store
 * @param len [in] current number of stored scores in OP_calced_score
 * 
 * @return the resulting number of stored scores in OP_calced_score.
 */
int
cache_push(HMMWork *wrk, int id, LOGPROB score, int len)
{
  int insertp;
  LOGPROB *calced_score;
  int *calced_id;

  calced_score = wrk->OP_calced_score;
  calced_id = wrk->OP_calced_id;

  if (len == 0) {               /* first one */
    calced_score[0] = score;
    calced_id[0] = id;
    return(1);
  }
  if (calced_score[len-1] >= score) { /* bottom */
    if (len < wrk->OP_gprune_num) {          /* append to bottom */
      calced_score[len] = score;
      calced_id[len] = id;
      len++;
    }
    return len;
  }
  if (calced_score[0] < score) {
    insertp = 0;
  } else {
    insertp = find_insert_point(calced_score, score, len);
  }
  if (len < wrk->OP_gprune_num) {
    memmove(&(calced_score[insertp+1]), &(calced_score[insertp]), sizeof(LOGPROB)*(len - insertp));
    memmove(&(calced_id[insertp+1]), &(calced_id[insertp]), sizeof(int)*(len - insertp));    
  } else if (insertp < len - 1) {
    memmove(&(calced_score[insertp+1]), &(calced_score[insertp]), sizeof(LOGPROB)*(len - insertp - 1));
    memmove(&(calced_id[insertp+1]), &(calced_id[insertp]), sizeof(int)*(len - insertp - 1));
  }
  calced_score[insertp] = score;
  calced_id[insertp] = id;
  if (len < wrk->OP_gprune_num) len++;
  return(len);
}

