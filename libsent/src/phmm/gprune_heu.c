/**
 * @file   gprune_heu.c
 * 
 * <JA>
 * @brief  混合ガウス分布計算: Gaussian pruning (heuristic algorithm)
 *
 * gprune_heu()は混合ガウス分布集合の計算ルーチンの一つです．
 * heuristic pruning を使って上位のガウス分布の出力確率のみを高速に求めます．
 * Tied-mixture %HMM 使用時に Julius でGPRUNE_DEFAULT_HEURISTIC が
 * 定義されているか，あるいはJuliusのオプション "-gprune heuristic" を
 * 指定することでこの関数が使用されます．
 *
 * heuristic pruning はあまり使われていない手法で，高速化の効果は
 * 他の手法の中間です．
 * 
 * gprune_heu() は outprob_init() によってその関数へのポインタが
 * compute_gaussset にセットされることで使用されます．このポインタが
 * calc_tied_mix() または calc_mix() から呼び出されます．
 * </JA>
 * 
 * <EN>
 * @brief  Calculate probability of a set of Gaussian densities by
 * Gaussian pruning: heuristic algorithm
 *
 * gprune_heu() is one of the functions to compute output probability of
 * a set of Gaussian densities.  This function does heuristic pruning, trying
 * to compute only the best ones for faster computation.  If a tied-mixture
 * %HMM model with GPRUNE_DEFAULT_HEURISTIC defined in Julius, or explicitly
 * specified by "-gprune heuristic" option, this function will be used.
 *
 * The effect of heuristic pruning is practically a middle of others.
 *
 * gprune_heu() will be used by calling outprob_init() to set its pointer
 * to the global variable @a compute_gaussset.  Then it will be called from
 * calc_tied_mix() or calc_mix().
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 05:44:52 2005
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

/* mail routines to calculate mixtures for given frame, and store to cache */
/*

  best_mixtures_on_last_frame[]
  
  dim:  0 1 2 3 4 .... veclen-1    -> sum up
 ================================
  backsum <-<-<-<-<-<-<-<-<-
 --------------------------------
  mix1  ->->
  mix2  ->->
  mix3  ->->
  ...
  mixN  ->->
 ================================
  score = forward sum + backmax

  algorithm 1:

     // store sub-optimal max prob for each dimension
     init backsum[]
     foreach all_mixtures in best_mixtures_on_last_frame {
        foreach dim {
           p = prob of this dim
	   if (backmax[dim] < p) backmax[dim] = p
	   sumup score
        }
	store to cache
     }
     // sum-up backward to make backmax
     foreach dim-1 backward {
        backmax[dim] += backmax[dim+1]
     }
     // compute rest with pruning with heuristics
     threshold = the current lowest score
     foreach rest_mixtures {
        foreach dim {
	   sumup score
	   if (score + backmax[dim+1] < threshold) {
	      skip this mixture
	   }
	}
	update threshold
     }
  }
     
*/

/** 
 * Clear backmax information.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
static void
init_backmax(HMMWork *wrk)
{
  int i;
  for(i=0;i<wrk->backmax_num;i++) wrk->backmax[i] = 0;
}

/** 
 * Build backmax information for each frame, by summing up
 * current maximum values of each dimensions.
 * 
 * @param wrk [i/o] HMM computation work area
 */
/*                        |
 *  0 1 2 ... max-3 max-2 | max-1
 *
 *  a b c      x      y   | ????
 *             |
 *             v
 *  .....     x+y     y     0.0
 */
static void
make_backmax(HMMWork *wrk)
{
  int i;

  wrk->backmax[wrk->backmax_num-1] = 0.0;
  /* backmax[len-1] = backmax[len-1];*/
  for(i=wrk->backmax_num-2;i>=0;i--) {
    wrk->backmax[i] += wrk->backmax[i+1];
  }
  /*  for(i=0;i<=len;i++) {
    printf("backmax[%d]=%f\n",i,backmax[i]);
    }*/
}

/**
 * @brief  Calculate probability with maximum value update.
 * 
 * Calculate probability of a Gaussian toward OP_vec,
 * while storing the maximum values of each dimension to @a backmax.
 * for future pruning.  The pruning itself is not performed here.
 * This function will be used to compute the first N Gaussians.
 * 
 * @param wrk [i/o] HMM computation work area
 * @param binfo [in] Gaussian density
 * 
 * @return the output log probability.
 */
static LOGPROB
compute_g_heu_updating(HMMWork *wrk, HTK_HMM_Dens *binfo)
{
  VECT tmp, x, sum = 0.0;
  VECT *mean;
  VECT *var;
  VECT *bm = wrk->backmax;
  VECT *vec = wrk->OP_vec;
  short veclen = wrk->OP_veclen;

  if (binfo == NULL) return(LOG_ZERO);
  mean = binfo->mean;
  var = binfo->var->vec;

  tmp = 0.0;
  for (; veclen > 0; veclen--) {
    x = *(vec++) - *(mean++);
    tmp = x * x * *(var++);
    sum += tmp;
    if ( *bm < tmp) *bm = tmp;
    bm++;
  }
  return((sum + binfo->gconst) * -0.5);
}

/** 
 * @brief  Calculate probability with pruning.
 * 
 * Calculate probability of a Gaussian toward OP_vec,
 * performing pruning using threshold and the backmax information
 * that has been set by compute_g_heu_updating() and
 * make_backmax().
 * 
 * @param wrk [i/o] HMM computation work area
 * @param binfo [in] Gaussian density
 * @param thres [in] threshold 
 * 
 * @return the output log probability.
 */
static LOGPROB
compute_g_heu_pruning(HMMWork *wrk, HTK_HMM_Dens *binfo, LOGPROB thres)
{
  VECT tmp, x;
  VECT *mean;
  VECT *var;
  VECT *bm = wrk->backmax;
  VECT *vec = wrk->OP_vec;
  short veclen = wrk->OP_veclen;
  LOGPROB fthres;

  if (binfo == NULL) return(LOG_ZERO);
  mean = binfo->mean;
  var = binfo->var->vec;
  fthres = thres * (-2.0);

  tmp = 0.0;
  bm++;
  for (; veclen > 0; veclen--) {
    x = *(vec++) - *(mean++);
    tmp += x * x * *(var++);
    if ( tmp + *bm > fthres) {
      return LOG_ZERO;
    }
    bm++;
  }
  return((tmp + binfo->gconst) * -0.5);
}


/** 
 * Initialize and setup work area for Gaussian pruning by heuristic algorithm.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
gprune_heu_init(HMMWork *wrk)
{
  int i;

  /* maximum Gaussian set size = maximum mixture size * nstream */
  wrk->OP_calced_maxnum = wrk->OP_hmminfo->maxmixturenum * wrk->OP_nstream;
  wrk->OP_calced_score = (LOGPROB *)mymalloc(sizeof(LOGPROB) * wrk->OP_calced_maxnum);
  wrk->OP_calced_id = (int *)mymalloc(sizeof(int) * wrk->OP_calced_maxnum);
  wrk->mixcalced = (boolean *)mymalloc(sizeof(int) * wrk->OP_calced_maxnum);
  for(i=0;i<wrk->OP_calced_maxnum;i++) wrk->mixcalced[i] = FALSE;
  wrk->backmax_num = wrk->OP_hmminfo->opt.vec_size + 1;
  wrk->backmax = (LOGPROB *)mymalloc(sizeof(LOGPROB) * wrk->backmax_num);

  return TRUE;
}

/**
 * Free gprune_heu related work area.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
void
gprune_heu_free(HMMWork *wrk)
{
  free(wrk->OP_calced_score);
  free(wrk->OP_calced_id);
  free(wrk->mixcalced);
  free(wrk->backmax);
}

/** 
 * @brief  Compute a set of Gaussians with heuristic pruning.
 *
 * If the N-best mixtures in the previous frame is specified in @a last_id,
 * They are first computed to get the maximum value for each dimension.
 * After that, the rest of the Gaussians will be computed using the maximum
 * values as heuristics of uncomputed dimensions to drop unpromising
 * Gaussians from computation at early stage
 * of likelihood computation.  If the @a last_id is not specified (typically
 * at the first frame of the input), a safe pruning as same as one in
 * gprune_safe.c will be applied.
 *
 * The calculated scores will be stored to OP_calced_score, with its
 * corresponding mixture id to OP_calced_id.  These are done by calling
 * cache_push().
 * The number of calculated mixtures is also stored in OP_calced_num.
 * 
 * This can be called from calc_tied_mix() or calc_mix().
 * 
 * @param wrk [i/o] HMM computation work area
 * @param g [in] set of Gaussian densities to compute the output probability
 * @param gnum [in] length of above
 * @param last_id [in] ID list of N-best mixture in previous input frame,
 * or NULL if not exist
 * @param lnum [in] length of last_id
 */
void
gprune_heu(HMMWork *wrk, HTK_HMM_Dens **g, int gnum, int *last_id, int lnum)
{
  int i, j, num = 0;
  LOGPROB score, thres;

  if (last_id != NULL) {	/* compute them first to form thresholds */
    /* 1. clear backmax */
    init_backmax(wrk);
    /* 2. calculate first $OP_gprune_num with setting max for each dimension */
    for (j=0; j<lnum; j++) {
      i = last_id[j];
      score = compute_g_heu_updating(wrk, g[i]);
      num = cache_push(wrk, i, score, num);
      wrk->mixcalced[i] = TRUE;      /* mark them as calculated */
    }
    /* 3. set backmax for each dimension */
    make_backmax(wrk);
    /* 4. calculate the rest with pruning*/
    thres = wrk->OP_calced_score[num-1];
    for (i = 0; i < gnum; i++) {
      /* skip calced ones in 1. */
      if (wrk->mixcalced[i]) {
        wrk->mixcalced[i] = FALSE;
        continue;
      }
      /* compute with safe pruning */
      score = compute_g_heu_pruning(wrk, g[i], thres);
      if (score > LOG_ZERO) {
	num = cache_push(wrk, i, score, num);
	thres = wrk->OP_calced_score[num-1];
      }
    }
  } else {			/* in case the last_id not available */
    /* at the first 0 frame */
    /* calculate with safe pruning */
    thres = LOG_ZERO;
    for (i = 0; i < gnum; i++) {
      if (num < wrk->OP_gprune_num) {
	score = compute_g_base(wrk, g[i]);
      } else {
	score = compute_g_safe(wrk, g[i], thres);
	if (score <= thres) continue;
      }
      num = cache_push(wrk, i, score, num);
      thres = wrk->OP_calced_score[num-1];
    }
  }
  wrk->OP_calced_num = num;
}
