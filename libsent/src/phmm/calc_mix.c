/**
 * @file   calc_mix.c
 * 
 * <JA>
 * @brief  混合ガウス分布の重みつき和の計算：非 tied-mixture 用，キャッシュ無し
 * </JA>
 * 
 * <EN>
 * @brief Compute weighed sum of Gaussian mixture for non tied-mixture model (no cache)
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 14:18:52 2005
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

/** 
 * @brief  Compute the output probability of current state OP_State.
 *
 * No codebook-level cache is done.  
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 * @return the output probability of the state OP_State in log10
 */
LOGPROB
calc_mix(HMMWork *wrk)
{
  int i;
  LOGPROB logprob, logprobsum;
  PROB *w;
  int *id;
  int s;
  PROB stream_weight;

  /* compute Gaussian set */
  logprobsum = 0.0;
  for(s=0;s<wrk->OP_nstream;s++) {
    /* set stream weight */
    if (wrk->OP_state->w) stream_weight = wrk->OP_state->w->weight[s];
    else stream_weight = 1.0;
    /* setup storage pointer for this mixture pdf */
    wrk->OP_vec = wrk->OP_vec_stream[s];
    wrk->OP_veclen = wrk->OP_veclen_stream[s];
    /* compute output probabilities */
    /* computed Gaussians will be set in:
       score ... OP_calced_score[0..OP_calced_num]
       id    ... OP_calced_id[0..OP_calced_num] */    
    (*(wrk->compute_gaussset))(wrk, wrk->OP_state->pdf[s]->b, wrk->OP_state->pdf[s]->mix_num, NULL, 0);
    /* add weights */
    id = wrk->OP_calced_id;
    w = wrk->OP_state->pdf[s]->bweight;
    for(i=0;i<wrk->OP_calced_num;i++) {
      //printf("s%d-m%d: %f %f\n", s+1, i+1, wrk->OP_calced_score[i], w[id[i]]);
      wrk->OP_calced_score[i] += w[id[i]];
    }
    /* add log probs */
    logprob = addlog_array(wrk->OP_calced_score, wrk->OP_calced_num);
    /* if outprob of a stream is zero, skip this stream */
    if (logprob <= LOG_ZERO) continue;
    /* sum all the obtained mixture scores */
    logprobsum += logprob * stream_weight;
  }
  if (logprobsum == 0.0) return(LOG_ZERO); /* no valid stream */
  if (logprobsum <= LOG_ZERO) return(LOG_ZERO);	/* lowest == LOG_ZERO */
  return (logprobsum * INV_LOG_TEN);
}
