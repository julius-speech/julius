/**
 * @file   gms_gprune.c
 * 
 * <JA>
 * @brief  Gaussian Mixture Selection のための Gaussian pruning を用いたモノフォンHMMの計算
 * </JA>
 * 
 * <EN>
 * @brief  Calculate the GMS monophone %HMM for Gaussian Mixture Selection using Gaussian pruning
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 15:05:08 2005
 *
 * $Revision: 1.7 $
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

/* activate experimental methods */
#define GS_MAX_PROB		///< Compute only max for GS states
#define LAST_BEST		///< Compute last best Gaussians first

/************************************************************************/
/** 
 * Initialization of GMS %HMM likelihood computation.
 * 
 * @param wrk [i/o] HMM computation work area
 *
 */
void
gms_gprune_init(HMMWork *wrk)
{
  int i;
  wrk->gms_last_max_id_list = (int **)mymalloc(sizeof(int *) * wrk->gsset_num);
  for(i=0;i<wrk->gsset_num;i++) {
    wrk->gms_last_max_id_list[i] = (int *)mymalloc(sizeof(int) * wrk->OP_nstream);
  }
}

/** 
 * Prepare GMS %HMM computation for the next speech input.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
void
gms_gprune_prepare(HMMWork *wrk)
{
  int i, j;
  for(i=0;i<wrk->gsset_num;i++) {
    for(j=0;j<wrk->OP_nstream;j++) {
      wrk->gms_last_max_id_list[i][j] = -1;
    }
  }
}

/**
 * Free GMS related work area.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
void
gms_gprune_free(HMMWork *wrk)
{
  int i;
  for(i=0;i<wrk->gsset_num;i++) free(wrk->gms_last_max_id_list[i]);
  free(wrk->gms_last_max_id_list);
}

/**********************************************************************/
/* LAST_BEST ... compute the maximum component in last frame first */
/** 
 * Compute only max by safe pruning
 * 
 * @param wrk [i/o] HMM computation work area
 * @param binfo [in] Gaussian density
 * @param thres [in] constant pruning threshold
 * 
 * @return the computed likelihood.
 */
static LOGPROB
calc_contprob_with_safe_pruning(HMMWork *wrk, HTK_HMM_Dens *binfo, LOGPROB thres)
{
  LOGPROB tmp, x;
  VECT *mean;
  VECT *var;
  LOGPROB fthres = thres * (-2.0);
  VECT *vec = wrk->OP_vec;
  short veclen = wrk->OP_veclen;

  if (binfo == NULL) return(LOG_ZERO);
  mean = binfo->mean;
  var = binfo->var->vec;

  tmp = binfo->gconst;
  for (; veclen > 0; veclen--) {
    x = *(vec++) - *(mean++);
    tmp += x * x * *(var++);
    if ( tmp > fthres) {
      return LOG_ZERO;
    }
  }
  return(tmp * -0.5);
}

#ifdef LAST_BEST

/** 
 * Compute log output likelihood of a state.  Only maximum Gaussian will be
 * computed.
 * 
 * @param wrk [i/o] HMM computation work area
 * @param stateinfo [in] %HMM state to compute
 * @param last_maxi [in] the mixture id that got the maximum value at the previous frame, or -1 if not exist.
 * @param maxi_ret [out] tue mixture id that get the maximum value at this call.
 * 
 * @return the log likelihood.
 */
static LOGPROB
compute_g_max(HMMWork *wrk, HTK_HMM_State *stateinfo, int *last_maxi)
{
  int i, maxi;
  LOGPROB prob;
  LOGPROB maxprob = LOG_ZERO;
  int s;
  PROB stream_weight;
  LOGPROB logprobsum;

  logprobsum = 0.0;
  for(s=0;s<wrk->OP_nstream;s++) {
    /* set stream weight */
    if (stateinfo->w) stream_weight = stateinfo->w->weight[s];
    else stream_weight = 1.0;
    /* setup storage pointer for this mixture pdf */
    wrk->OP_vec = wrk->OP_vec_stream[s];
    wrk->OP_veclen = wrk->OP_veclen_stream[s];

    if (last_maxi[s] != -1) {
      maxi = last_maxi[s];
      maxprob = calc_contprob_with_safe_pruning(wrk, stateinfo->pdf[s]->b[maxi], LOG_ZERO);
      for (i = stateinfo->pdf[s]->mix_num - 1; i >= 0; i--) {
	if (i == last_maxi[s]) continue;
	prob = calc_contprob_with_safe_pruning(wrk, stateinfo->pdf[s]->b[i], maxprob);
	if (prob > maxprob) {
	  maxprob = prob;
	  maxi = i;
	}
      }
      last_maxi[s] = maxi;
    } else {
      maxi = stateinfo->pdf[s]->mix_num - 1;
      maxprob = calc_contprob_with_safe_pruning(wrk, stateinfo->pdf[s]->b[maxi],  LOG_ZERO);
      for (i = maxi - 1; i >= 0; i--) {
	prob = calc_contprob_with_safe_pruning(wrk, stateinfo->pdf[s]->b[i], maxprob);
	if (prob > maxprob) {
	  maxprob = prob;
	  maxi = i;
	}
      }
      last_maxi[s] = maxi;
    }
    logprobsum += (maxprob + stateinfo->pdf[s]->bweight[maxi]) * stream_weight;
  }
  return (logprobsum * INV_LOG_TEN);
}
  
#else  /* ~LAST_BEST */
  
/** 
 * Compute log output likelihood of a state.  Only maximum Gaussian will be
 * computed.
 * 
 * @param wrk [i/o] HMM computation work area
 * @param stateinfo [in] %HMM state to compute
 * 
 * @return the log likelihood.
 */
static LOGPROB
compute_g_max(HMMWork *wrk, HTK_HMM_State *stateinfo)
{
  int i, maxi;
  LOGPROB prob;
  LOGPROB maxprob = LOG_ZERO;
  int s;
  PROB stream_weight;
  LOGPROB logprob, logprobsum;

  logprobsum = 0.0;
  for(s=0;s<wrk->OP_nstream;s++) {
    /* set stream weight */
    if (stateinfo->w) stream_weight = stateinfo->w->weight[s];
    else stream_weight = 1.0;
    /* setup storage pointer for this mixture pdf */
    wrk->OP_vec = wrk->OP_vec_stream[s];
    wrk->OP_veclen = wrk->OP_veclen_stream[s];

    i = maxi = stateinfo->pdf[s]->mix_num - 1;
    for (; i >= 0; i--) {
      prob = calc_contprob_with_safe_pruning(wrk, stateinfo->pdf[s]->b[i], maxprob);
      if (prob > maxprob) {
	maxprob = prob;
	maxi = i;
      }
    }
    logprobsum += (maxprob + stateinfo->pdf[s]->bweight[maxi]) * stream_weight;
  }
  return (logprobsum * INV_LOG_TEN);
}
#endif

/**********************************************************************/
/* main function: compute all gshmm scores */
/* *** assume to be called for sequencial frame (using last result) */

/** 
 * Main function to compute all the GMS %HMM states in a frame
 * with the input vectore specified by OP_vec.  This function assumes
 * that this will be called for sequencial frame, since it utilizes the
 * result of previous frame for faster pruning.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
void
compute_gs_scores(HMMWork *wrk)
{
  int i;

  for (i=0;i<wrk->gsset_num;i++) {
#ifdef GS_MAX_PROB
#ifdef LAST_BEST
    /* compute only the maximum with pruning (last best first) */
    wrk->t_fs[i] = compute_g_max(wrk, wrk->gsset[i].state, wrk->gms_last_max_id_list[i]);
#else
    wrk->t_fs[i] = compute_g_max(wrk, wrk->gsset[i].state);
#endif /* LAST_BEST */
#else
    /* compute all mixture */
    wrk->t_fs[i] = compute_g_base(wrk, wrk->gsset[i].state);
#endif
    /*printf("%d:%s:%f\n",i,gsset[i].book->name,t_fs[i]);*/
  }

}
