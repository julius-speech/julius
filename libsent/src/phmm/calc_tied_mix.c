/**
 * @file   calc_tied_mix.c
 * 
 * <JA>
 * @brief  混合ガウス分布の重みつき和の計算：tied-mixture用，キャッシュ有り
 *
 * Tied-mixture 用のガウス混合分布計算ではキャッシュが考慮されます．
 * 計算された混合分布の音響尤度はコードブック単位でフレームごとに
 * キャッシュされ，同じコードブックが同じ時間でアクセスされた場合は
 * そのキャッシュから値を返します．
 * </JA>
 * 
 * <EN>
 * @brief  Compute weighed sum of Gaussian mixture for tied-mixture model (cache enabled)
 *
 * In tied-mixture computation, the computed output probability of each
 * Gaussian component will be cache per codebook, for each input frame.
 * If the same codebook of the same time is accessed later, the cached
 * value will be returned.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 14:22:44 2005
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
#include <sent/speech.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/hmm_calc.h>


/** 
 * Initialize codebook cache area.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
calc_tied_mix_init(HMMWork *wrk)
{
  wrk->mixture_cache = NULL;
  wrk->mixture_cache_num = NULL;
  wrk->tmix_allocframenum = 0;
  wrk->mroot = NULL;
  wrk->tmix_last_id = (int *)mymalloc(sizeof(int) * wrk->OP_hmminfo->maxmixturenum * wrk->OP_nstream);
  return TRUE;
}

/** 
 * Setup codebook cache for the next incoming input.
 * 
 * @param wrk [i/o] HMM computation work area
 * @param framenum [in] length of the next input.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
calc_tied_mix_prepare(HMMWork *wrk, int framenum)
{
  int bid, t;

  /* clear */
  for(t=0;t<wrk->tmix_allocframenum;t++) {
    for(bid=0;bid<wrk->OP_hmminfo->codebooknum;bid++) {
      wrk->mixture_cache_num[t][bid] = 0;
    }
  }

  return TRUE;
}

/** 
 * Expand the cache to time axis if needed.
 * 
 * @param wrk [i/o] HMM computation work area
 * @param reqframe [in] required frame length
 */
static void
calc_tied_mix_extend(HMMWork *wrk, int reqframe)
{
  int newnum;
  int bid, t, size;
  
  /* if enough length are already allocated, return immediately */
  if (reqframe < wrk->tmix_allocframenum) return;

  /* allocate per certain period */
  newnum = reqframe + 1;
  if (newnum < wrk->tmix_allocframenum + OUTPROB_CACHE_PERIOD)
    newnum = wrk->tmix_allocframenum + OUTPROB_CACHE_PERIOD;

  if (wrk->mixture_cache == NULL) {
    wrk->mixture_cache = (MIXCACHE ***)mymalloc(sizeof(MIXCACHE **) * newnum);
    wrk->mixture_cache_num = (short **)mymalloc(sizeof(short *) * newnum);
  } else {
    wrk->mixture_cache = (MIXCACHE ***)myrealloc(wrk->mixture_cache, sizeof(MIXCACHE **) * newnum);
    wrk->mixture_cache_num = (short **)myrealloc(wrk->mixture_cache_num, sizeof(short *) * newnum);
  }

  size = wrk->OP_gprune_num * wrk->OP_hmminfo->codebooknum;

  for(t = wrk->tmix_allocframenum; t < newnum; t++) {
    wrk->mixture_cache[t] = (MIXCACHE **)mybmalloc2(sizeof(MIXCACHE *) * wrk->OP_hmminfo->codebooknum, &(wrk->mroot));
    wrk->mixture_cache_num[t] = (short *)mybmalloc2(sizeof(short) * wrk->OP_hmminfo->codebooknum, &(wrk->mroot));
    wrk->mixture_cache[t][0] = (MIXCACHE *)mybmalloc2(sizeof(MIXCACHE) * size, &(wrk->mroot));
    for(bid=1;bid<wrk->OP_hmminfo->codebooknum;bid++) {
      wrk->mixture_cache[t][bid] = &(wrk->mixture_cache[t][0][wrk->OP_gprune_num * bid]);
    }
    /* clear the new part */
    for(bid=0;bid<wrk->OP_hmminfo->codebooknum;bid++) {
      wrk->mixture_cache_num[t][bid] = 0;
    }
  }

  wrk->tmix_allocframenum = newnum;
}

/** 
 * Free work area for tied-mixture calculation.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
void
calc_tied_mix_free(HMMWork *wrk)
{
  if (wrk->mroot != NULL) mybfree2(&(wrk->mroot));
  if (wrk->mixture_cache_num != NULL) free(wrk->mixture_cache_num);
  if (wrk->mixture_cache != NULL) free(wrk->mixture_cache);
  free(wrk->tmix_last_id);
  wrk->mroot = NULL;
  wrk->mixture_cache_num = NULL;
  wrk->mixture_cache = NULL;
}

/** 
 * @brief  Compute the output probability of current state OP_State on
 * tied-mixture model
 * 
 * This function assumes that the OP_state is assigned to a tied-mixture
 * codebook.  Here the output probability of Gaussian mixture component
 * referred by OP_state is consulted to the book level cache, and if not
 * computed yet on that input frame time, it will be computed here.
 *
 * @param wrk [i/o] HMM computation work area
 * 
 * @return the computed output probability in log10.
 */
LOGPROB
calc_tied_mix(HMMWork *wrk)
{
  GCODEBOOK *book;
  LOGPROB logprob, logprobsum;
  int i, id;
  MIXCACHE *ttcache;
  short ttcachenum;
  MIXCACHE *last_ttcache;
  short last_ttcachenum;
  PROB *weight;
  PROB stream_weight;
  int s;
  int num;

  logprobsum = 0.0;
  for(s=0;s<wrk->OP_nstream;s++) {
    book = (GCODEBOOK *)(wrk->OP_state->pdf[s]->b);
    weight = wrk->OP_state->pdf[s]->bweight;
    /* set stream weight */
    if (wrk->OP_state->w) stream_weight = wrk->OP_state->w->weight[s];
    else stream_weight = 1.0;
    /* setup storage pointer for this mixture pdf */
    wrk->OP_vec = wrk->OP_vec_stream[s];
    wrk->OP_veclen = wrk->OP_veclen_stream[s];
    /* extend cache if needed */
    calc_tied_mix_extend(wrk, wrk->OP_time);
    /* prepare cache for this codebook at this time */
    ttcache = wrk->mixture_cache[wrk->OP_time][book->id];
    ttcachenum = wrk->mixture_cache_num[wrk->OP_time][book->id];
    /* consult cache */
    if (ttcachenum > 0) {
      /* calculate using cache and weight */
      for (i=0;i<ttcachenum;i++) {
	wrk->OP_calced_score[i] = ttcache[i].score + weight[ttcache[i].id];
      }
      num = ttcachenum;
    } else {
      /* compute Gaussian set */
      /* computed Gaussians will be set in:
	 score ... OP_calced_score[0..OP_calced_num]
	 id    ... OP_calced_id[0..OP_calced_num] */
      if (wrk->OP_time >= 1) {
	last_ttcache = wrk->mixture_cache[wrk->OP_time-1][book->id];
	last_ttcachenum = wrk->mixture_cache_num[wrk->OP_time-1][book->id];
	if (last_ttcachenum > 0) {
	  for(i=0;i<last_ttcachenum;i++) wrk->tmix_last_id[i] = last_ttcache[i].id;
	  /* tell last calced best */
	  (*(wrk->compute_gaussset))(wrk, book->d, book->num, wrk->tmix_last_id, last_ttcachenum);
	} else {
	  (*(wrk->compute_gaussset))(wrk, book->d, book->num, NULL, 0);
	}
      } else {
	(*(wrk->compute_gaussset))(wrk, book->d, book->num, NULL, 0);
      }
      /* store to cache */
      wrk->mixture_cache_num[wrk->OP_time][book->id] = wrk->OP_calced_num;
      for (i=0;i<wrk->OP_calced_num;i++) {
	id = wrk->OP_calced_id[i];
	ttcache[i].id = id;
	ttcache[i].score = wrk->OP_calced_score[i];
	/* now OP_calced_{id|score} can be used for work area */
	/* add weights */
	wrk->OP_calced_score[i] += weight[id];
      }
      num = wrk->OP_calced_num;
    }
    /* add log probs */
    logprob = addlog_array(wrk->OP_calced_score, num);
    /* if outprob of a stream is zero, skip this stream */
    if (logprob <= LOG_ZERO) continue;
    /* sum all the obtained mixture scores */
    logprobsum += logprob * stream_weight;
  }
  if (logprobsum == 0.0) return(LOG_ZERO); /* no valid stream */
  if (logprobsum <= LOG_ZERO) return(LOG_ZERO);	/* lowest == LOG_ZERO */
  return (logprobsum * INV_LOG_TEN);
}  


/** 
 * @brief  Compute the output probability of current state OP_State,
 * regardless of tied-mixture model or state-level mixture PDF.
 * 
 * This function switches calculation function of calc_mix() and
 * calc_tied_mix() based on the mixture PDF information.
 * This will be used on a system which has tied-mixture codebook
 * but some states still has their own mixture PDF.
 *
 * The initialization functions should be the same as calc_tied_mix(),
 * since calc_mix() has no specific initialization.
 *
 * @param wrk [i/o] HMM computation work area
 * 
 * @return the computed output probability in log10.
 */
LOGPROB
calc_compound_mix(HMMWork *wrk)
{
  HTK_HMM_PDF *m;
  GCODEBOOK *book;
  LOGPROB logprob, logprobsum;
  int i, id;
  MIXCACHE *ttcache;
  short ttcachenum;
  MIXCACHE *last_ttcache;
  short last_ttcachenum;
  PROB *weight;
  PROB stream_weight;
  int s;
  int num;

  logprobsum = 0.0;
  for(s=0;s<wrk->OP_nstream;s++) {
    /* set stream weight */
    if (wrk->OP_state->w) stream_weight = wrk->OP_state->w->weight[s];
    else stream_weight = 1.0;
    m = wrk->OP_state->pdf[s];
    /* setup storage pointer for this mixture pdf */
    wrk->OP_vec = wrk->OP_vec_stream[s];
    wrk->OP_veclen = wrk->OP_veclen_stream[s];
    weight = wrk->OP_state->pdf[s]->bweight;
    if (m->tmix) {
      /* tied-mixture PDF */
      book = (GCODEBOOK *)(m->b);
      /* extend cache if needed */
      calc_tied_mix_extend(wrk, wrk->OP_time);
      /* prepare cache for this codebook at this time */
      ttcache = wrk->mixture_cache[wrk->OP_time][book->id];
      ttcachenum = wrk->mixture_cache_num[wrk->OP_time][book->id];
      /* consult cache */
      if (ttcachenum > 0) {
	/* calculate using cache and weight */
	for (i=0;i<ttcachenum;i++) {
	  wrk->OP_calced_score[i] = ttcache[i].score + weight[ttcache[i].id];
	}
	num = ttcachenum;
      } else {
	/* compute Gaussian set */
	/* computed Gaussians will be set in:
	   score ... OP_calced_score[0..OP_calced_num]
	   id    ... OP_calced_id[0..OP_calced_num] */
	if (wrk->OP_time >= 1) {
	  last_ttcache = wrk->mixture_cache[wrk->OP_time-1][book->id];
	  last_ttcachenum = wrk->mixture_cache_num[wrk->OP_time-1][book->id];
	  if (last_ttcachenum > 0) {
	    for(i=0;i<last_ttcachenum;i++) wrk->tmix_last_id[i] = last_ttcache[i].id;
	    /* tell last calced best */
	    (*(wrk->compute_gaussset))(wrk, book->d, book->num, wrk->tmix_last_id, last_ttcachenum);
	  } else {
	    (*(wrk->compute_gaussset))(wrk, book->d, book->num, NULL, 0);
	  }
	} else {
	  (*(wrk->compute_gaussset))(wrk, book->d, book->num, NULL, 0);
	}
	/* store to cache */
	wrk->mixture_cache_num[wrk->OP_time][book->id] = wrk->OP_calced_num;
	for (i=0;i<wrk->OP_calced_num;i++) {
	  id = wrk->OP_calced_id[i];
	  ttcache[i].id = id;
	  ttcache[i].score = wrk->OP_calced_score[i];
	  /* now OP_calced_{id|score} can be used for work area */
	  /* add weights */
	  wrk->OP_calced_score[i] += weight[id];
	}
	num = wrk->OP_calced_num;
      }
    } else {
      /* normal state */
      (*(wrk->compute_gaussset))(wrk, m->b, m->mix_num, NULL, 0);
      /* add weights */
      for(i=0;i<wrk->OP_calced_num;i++) {
	wrk->OP_calced_score[i] += weight[wrk->OP_calced_id[i]];
      }
      num = wrk->OP_calced_num;
    }
    /* add log probs */
    logprob = addlog_array(wrk->OP_calced_score, num);
    /* if outprob of a stream is zero, skip this stream */
    if (logprob <= LOG_ZERO) continue;
    /* sum all the obtained mixture scores */
    logprobsum += logprob * stream_weight;
  }
  if (logprobsum == 0.0) return(LOG_ZERO); /* no valid stream */
  if (logprobsum <= LOG_ZERO) return(LOG_ZERO);	/* lowest == LOG_ZERO */
  return (logprobsum * INV_LOG_TEN);
}
