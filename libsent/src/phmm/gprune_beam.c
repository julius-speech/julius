/**
 * @file   gprune_beam.c
 * 
 * <JA>
 * @brief  混合ガウス分布計算: Gaussian pruning (beam algorithm)
 *
 * gprune_beam()は混合ガウス分布集合の計算ルーチンの一つです．
 * beam pruning を使って上位のガウス分布の出力確率のみを高速に求めます．
 * Tied-mixture %HMM 使用時に Julius でGPRUNE_DEFAULT_BEAM が定義されているか，
 * あるいはJuliusのオプション "-gprune beam" を指定することでこの関数が
 * 使用されます．
 *
 * beam pruning は最も積極的に枝刈りを行ないます．計算は最も高速ですが，
 * 上位のガウス分布が正しく得られず出力確率の誤りが大きくなる可能性があります．
 * 
 * gprune_beam() は outprob_init() によってその関数へのポインタが
 * compute_gaussset にセットされることで使用されます．このポインタが
 * calc_tied_mix() または calc_mix() から呼び出されます．
 * </JA>
 * 
 * <EN>
 * @brief  Calculate probability of a set of Gaussian densities by
 * Gaussian pruning: beam algorithm
 *
 * gprune_beam() is one of the functions to compute output probability of
 * a set of Gaussian densities.  This function does beam pruning, trying
 * to compute only the best ones for faster computation.  If a tied-mixture
 * %HMM model with GPRUNE_DEFAULT_BEAM defined in Julius, or explicitly
 * specified by "-gprune beam" option, this function will be used.
 *
 * The beam pruning is the most aggressive pruning method.  This is the fastest
 * method, but they may miss the N-best Gaussian to be found, which may
 * result in some likelihood error.
 *
 * gprune_beam() will be used by calling outprob_init() to set its pointer
 * to the global variable @a compute_gaussset.  Then it will be called from
 * calc_tied_mix() or calc_mix().
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 03:27:53 2005
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

#define TEST2

/*

  best_mixtures_on_last_frame[]
  
  dim:  0 1 2 3 4 .... veclen-1    -> sum up
 ================================
  thres
 --------------------------------
  mix1  | |
  mix2  | |
  mix3  v v
  ...
  mixN  
 ================================
         \_\_ vecprob[0],vecprob[1]

  algorithm 1:
	 
  foreach dim {
     foreach all_mixtures in best_mixtures_on_last_frame {
        compute score
     }
     threshold = the current lowest score + beam_width?
     foreach rest_mixtures {
        if (already marked as pruned at previous dim) {
	   skip
	}
	compute score
        if (score < threshold) {
	   mark as pruned
	   skip
	}
	if (score > threshold) {
	   update threshold
	}
     }
  }

  algorithm 2:

  foreach all_mixtures in best_mixtures_on_last_frame {
     foreach dim {
       compute score
       if (threshold[dim] < score) update
     }
     threshold[dim] += beam_width
  }
  foreach rest_mixtures {
     foreach dim {
        compute score
	if (score < threshold[dim]) skip this mixture
	update thres
     }
  }
     
*/

/** 
 * Clear per-dimension thresholds.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
static void
clear_dimthres(HMMWork *wrk)
{
  int i;
  for(i=0;i<wrk->dimthres_num;i++) wrk->dimthres[i] = 0.0;
}

/** 
 * Set beam thresholds by adding TMBEAMWIDTH to the maximum values
 * of each dimension.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
static void
set_dimthres(HMMWork *wrk)
{
  int i;
  for(i=0;i<wrk->dimthres_num;i++) wrk->dimthres[i] += TMBEAMWIDTH;
}

/**
 * @brief  Calculate probability with threshold update.
 * 
 * Calculate probability of a Gaussian toward OP_vec,
 * while storing the maximum values of each dimension to @a dimthres.
 * for future pruning.  The pruning itself is not performed here.
 * This function will be used to compute the first N Gaussians.
 * 
 * @param wrk [i/o] HMM computation work area
 * @param binfo [in] Gaussian density
 * 
 * @return the output log probability.
 */
static LOGPROB
compute_g_beam_updating(HMMWork *wrk, HTK_HMM_Dens *binfo)
{
  VECT tmp, x;
  VECT *mean;
  VECT *var;
  VECT *th = wrk->dimthres;
  VECT *vec = wrk->OP_vec;
  short veclen = wrk->OP_veclen;

#ifndef TEST2
  if (binfo == NULL) return(LOG_ZERO);
#endif

  mean = binfo->mean;
  var = binfo->var->vec;

  tmp = 0.0;
  for (; veclen > 0; veclen--) {
    x = *(vec++) - *(mean++);
    tmp += x * x * *(var++);
    if ( *th < tmp) *th = tmp;
    th++;
  }
  return((tmp + binfo->gconst) * -0.5);
}

/** 
 * @brief  Calculate probability with pruning.
 * 
 * Calculate probability of a Gaussian toward OP_vec,
 * performing pruning using the dimension thresholds
 * that has been set by compute_g_beam_updating() and
 * set_dimthres().
 * 
 * @param wrk [i/o] HMM computation work area
 * @param binfo [in] Gaussian density
 * 
 * @return the output log probability.
 */
static LOGPROB
compute_g_beam_pruning(HMMWork *wrk, HTK_HMM_Dens *binfo)
{
  VECT tmp, x;
  VECT *mean;
  VECT *var;
  VECT *th = wrk->dimthres;
  VECT *vec = wrk->OP_vec;
  short veclen = wrk->OP_veclen;

#ifndef TEST2
  if (binfo == NULL) return(LOG_ZERO);
#endif
  mean = binfo->mean;
  var = binfo->var->vec;

  tmp = 0.0;
  for (; veclen > 0; veclen--) {
    x = *(vec++) - *(mean++);
    tmp += x * x * *(var++);
    if ( tmp > *(th++)) {
      return LOG_ZERO;
    }
  }
  return((tmp + binfo->gconst) * -0.5);
}


/** 
 * Initialize and setup work area for Gaussian pruning by beam algorithm.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
gprune_beam_init(HMMWork *wrk)
{
  int i;

  /* maximum Gaussian set size = maximum mixture size * nstream */
  wrk->OP_calced_maxnum = wrk->OP_hmminfo->maxmixturenum * wrk->OP_nstream;
  wrk->OP_calced_score = (LOGPROB *)mymalloc(sizeof(LOGPROB) * wrk->OP_calced_maxnum);
  wrk->OP_calced_id = (int *)mymalloc(sizeof(int) * wrk->OP_calced_maxnum);
  wrk->mixcalced = (boolean *)mymalloc(sizeof(int) * wrk->OP_calced_maxnum);
  for(i=0;i<wrk->OP_calced_maxnum;i++) wrk->mixcalced[i] = FALSE;
  wrk->dimthres_num = wrk->OP_hmminfo->opt.vec_size;
  wrk->dimthres = (LOGPROB *)mymalloc(sizeof(LOGPROB) * wrk->dimthres_num);

  return TRUE;
}

/**
 * Free gprune_beam related work area.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
void
gprune_beam_free(HMMWork *wrk)
{
  free(wrk->OP_calced_score);
  free(wrk->OP_calced_id);
  free(wrk->mixcalced);
  free(wrk->dimthres);
}

/** 
 * @brief  Compute a set of Gaussians with beam pruning.
 *
 * If the N-best mixtures in the previous frame is specified in @a last_id,
 * They are first computed to set the thresholds for each dimension.
 * After that, the rest of the Gaussians will be computed with those dimension
 * thresholds to drop unpromising Gaussians from computation at early stage
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
gprune_beam(HMMWork *wrk, HTK_HMM_Dens **g, int gnum, int *last_id, int lnum)
{
  int i, j, num = 0;
  LOGPROB score, thres;

  if (last_id != NULL) {	/* compute them first to form thresholds */
    /* 1. clear dimthres */
    clear_dimthres(wrk);
    /* 2. calculate first $OP_gprune_num and set initial thresholds */
    for (j=0; j<lnum; j++) {
      i = last_id[j];
#ifdef TEST2
      if (!g[i]) {
	score = LOG_ZERO;
      } else {
	score = compute_g_beam_updating(wrk, g[i]);
      }
      num = cache_push(wrk, i, score, num);
#else
      score = compute_g_beam_updating(wrk, g[i]);
      num = cache_push(wrk, i, score, num);
#endif
      wrk->mixcalced[i] = TRUE;      /* mark them as calculated */
    }
    /* 3. set pruning thresholds for each dimension */
    set_dimthres(wrk);

    /* 4. calculate the rest with pruning*/
    for (i = 0; i < gnum; i++) {
      /* skip calced ones in 1. */
      if (wrk->mixcalced[i]) {
        wrk->mixcalced[i] = FALSE;
        continue;
      }
#ifdef TEST2
      /* compute with safe pruning */
      if (!g[i]) continue;
      score = compute_g_beam_pruning(wrk, g[i]);
      if (score > LOG_ZERO) {
	num = cache_push(wrk, i, score, num);
      }
#else
      /* compute with safe pruning */
      score = compute_g_beam_pruning(wrk, g[i]);
      if (score > LOG_ZERO) {
	num = cache_push(wrk, i, score, num);
      }
#endif
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
