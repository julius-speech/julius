/**
 * @file   outprob_init.c
 * 
 * <JA>
 * @brief  音響尤度計算ルーチンの初期化とセットアップ
 *
 * 音響モデルのタイプにあわせた計算ルーチンの選択や，計算用の各種
 * パラメータの初期化を行います．これらの初期化関数は音響尤度計算を始める前に
 * 呼び出される必要があります．
 * 
 * 音響尤度計算関数の使用方法:
 *    -# 最初に outprob_init() を呼んで初期化とセットアップを行います．
 *    -# 各入力に対して，以下を行います．
 *      -# outprob_prepare() で必要な尤度キャッシュを確保します．
 *      -# outprob(t, hmmstate, param) で各状態に対する音響尤度を計算して
 *         返します．
 * </JA>
 * 
 * <EN>
 * @brief  Initialize and setup the acoustic computation routines
 *
 * These functions switch computation function suitable for the given %HMM
 * types (tied-mixture or shared-state, use GMS or not, and so on).  It also
 * sets various parameters and global pointers for the likelihood computation.
 * These functions should be called at first.
 *
 * How to usage these acoustic computation routines:
 *    -# First call outprob_init() on startup to configure and setup functions
 *    -# For each input, 
 *      -# call outprob_prepare() to prepare cache
 *      -# use outprob(t, hmmstate, param) to get output probability of a state
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 13:35:37 2005
 *
 * $Revision: 1.8 $
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
 * Initialize and setup acoustic computation functions.
 * 
 * @param wrk [i/o] HMM computation work area
 * @param hmminfo [in] HMM definition
 * @param gshmm [in] GMS HMM definition if exist, or NULL if not
 * @param gms_num [in] number of GMS HMM to compute (valid if gshmm != NULL)
 * @param gprune_method [in] gaussian pruning method
 * @param gprune_mixnum [in] number of pdf to compute at a codebook
 * in gaussian pruning
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
outprob_init(HMMWork *wrk, HTK_HMM_INFO *hmminfo,
	     HTK_HMM_INFO *gshmm, int gms_num,
	     int gprune_method, int gprune_mixnum
	     )
{
  int i;
  /* check if variances are inversed */
  if (!hmminfo->variance_inversed) {
    /* here, inverse all variance values for faster computation */
    htk_hmm_inverse_variances(hmminfo);
    hmminfo->variance_inversed = TRUE;
  }
  /* check if variances are inversed */
  if (gshmm) {
    if (!gshmm->variance_inversed) {
      /* here, inverse all variance values for faster computation */
      htk_hmm_inverse_variances(gshmm);
      gshmm->variance_inversed = TRUE;
    }
  }

  /** select functions **/
  /* select pruning function to compute likelihood of a mixture component
     and set the pointer to global */
#ifdef ENABLE_MSD
  /* currently MSD model works only for non pruning mode */
  if (hmminfo->has_msd && gprune_method != GPRUNE_SEL_NONE) {
    jlog("Error: outprob_init: only \"-gprune none\" is supported when MSD-HMM enabled\n");
    return FALSE;
  }
#endif
  switch(gprune_method) {
  case GPRUNE_SEL_NONE:
    wrk->compute_gaussset = gprune_none;
    wrk->compute_gaussset_init = gprune_none_init;
    wrk->compute_gaussset_free = gprune_none_free;
    break;
  case GPRUNE_SEL_SAFE:
    wrk->compute_gaussset = gprune_safe;
    wrk->compute_gaussset_init = gprune_safe_init;
    wrk->compute_gaussset_free = gprune_safe_free;
    break;
  case GPRUNE_SEL_HEURISTIC:
    wrk->compute_gaussset = gprune_heu;
    wrk->compute_gaussset_init = gprune_heu_init;
    wrk->compute_gaussset_free = gprune_heu_free;
    break;
  case GPRUNE_SEL_BEAM:
    wrk->compute_gaussset = gprune_beam;
    wrk->compute_gaussset_init = gprune_beam_init;
    wrk->compute_gaussset_free = gprune_beam_free;
    break;
  case GPRUNE_SEL_USER:
    /* assume user functions are already registered to the entries */
    break;
  }
  /* select caching function to compute output probability of a mixture */
  if (hmminfo->is_tied_mixture) {
    /* check if all mixture PDFs are tied-mixture */
    {
      HTK_HMM_PDF *p;
      boolean ok_p = TRUE;
      for (p = hmminfo->pdfstart; p; p = p->next) {
	if (p->tmix == FALSE) {
	  ok_p = FALSE;
	  break;
	}
      }
      if (ok_p) {
	jlog("Stat: outprob_init: all mixture PDFs are tied-mixture, use calc_tied_mix()\n");
	wrk->calc_outprob = calc_tied_mix; /* enable book-level cache, typically for a tied-mixture model */
      } else {
	jlog("Stat: outprob_init: tied-mixture PDF exist (not all), calc_compound_mix()\n");
	wrk->calc_outprob = calc_compound_mix; /* enable book-level cache, typically for a tied-mixture model */
      }
    }
  } else {
    jlog("Stat: outprob_init: state-level mixture PDFs, use calc_mix()\n");
    wrk->calc_outprob = calc_mix; /* no mixture-level cache, for a shared-state, non tied-mixture model */
  }
  
  /* select back-off functon for state probability calculation */
  if (gshmm != NULL) {
    wrk->calc_outprob_state = gms_state; /* enable GMS */
  } else {
    wrk->calc_outprob_state = wrk->calc_outprob; /* call mixture outprob directly */
  }

  /* store common variable to global */
  wrk->OP_hmminfo = hmminfo;
  wrk->OP_gshmm = gshmm;		/* NULL if GMS not used */
  wrk->OP_gprune_num = gprune_mixnum;

  /* store multi-stream data */
  wrk->OP_nstream = hmminfo->opt.stream_info.num;
  for(i=0;i<wrk->OP_nstream;i++) {
    wrk->OP_veclen_stream[i] = hmminfo->opt.stream_info.vsize[i];
  }

  /* generate addlog table */
  make_log_tbl();
  
  /* initialize work area for mixture component pruning function */
  if ((*(wrk->compute_gaussset_init))(wrk) == FALSE) return FALSE; /* OP_gprune may change */
  /* initialize work area for book level cache on tied-mixture model */
  if (hmminfo->is_tied_mixture) {
    if (calc_tied_mix_init(wrk) == FALSE) return FALSE;
  }
  /* initialize work area for GMS */
  if (wrk->OP_gshmm != NULL) {
    wrk->my_nbest = gms_num;
    if (gms_init(wrk) == FALSE) return FALSE;
  }
  /* initialize cache for all output probabilities */
  if (outprob_cache_init(wrk) == FALSE)  return FALSE;

  /* initialize word area for computation of pseudo HMM set when N-max is specified */
  if (hmminfo->cdset_method == IWCD_NBEST) {
    outprob_cd_nbest_init(wrk, hmminfo->cdmax_num);
  }

  wrk->batch_computation = FALSE;

  return TRUE;
}

void
outprob_set_batch_computation(HMMWork *wrk, boolean flag)
{
  wrk->batch_computation = flag;
}

/** 
 * Prepare for the next input of given frame length.
 *
 * @param wrk [i/o] HMM computation work area
 * @param framenum [in] input length in frame.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
outprob_prepare(HMMWork *wrk, int framenum)
{
  if (outprob_cache_prepare(wrk) == FALSE) return FALSE;
  if (wrk->OP_gshmm != NULL) {
    if (gms_prepare(wrk, framenum) == FALSE) return FALSE;
  }
  if (wrk->OP_hmminfo->is_tied_mixture) {
    if (calc_tied_mix_prepare(wrk, framenum) == FALSE) return FALSE;
  }
  /* reset last time */
  wrk->OP_last_time = wrk->OP_time = -1;
  return TRUE;
}

/**
 * Free all work area for outprob computation.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
void
outprob_free(HMMWork *wrk)
{
  (*(wrk->compute_gaussset_free))(wrk);
  if (wrk->OP_hmminfo->is_tied_mixture) {
    calc_tied_mix_free(wrk);
  }
  if (wrk->OP_gshmm != NULL) {
    gms_free(wrk);
  }
  outprob_cache_free(wrk);
  if (wrk->OP_hmminfo->cdset_method == IWCD_NBEST) {
    outprob_cd_nbest_free(wrk);
  }

}
