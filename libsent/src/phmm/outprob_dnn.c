/*
 * Copyright (c) 1991-2016 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2016 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/speech.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/hmm_calc.h>

#define LOG_UNDEF (LOG_ZERO - 1) ///< Value to be used as the initial cache value

/** 
 * Initialize the cache data, should be called once on startup.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
outprob_cache_init(HMMWork *wrk)
{
  wrk->statenum = wrk->OP_hmminfo->totalstatenum;
  wrk->outprob_cache = NULL;
  wrk->outprob_allocframenum = 0;
  wrk->OP_time = -1;
  wrk->croot = NULL;
  return TRUE;
}

/** 
 * Prepare cache for the next input, by clearing the existing cache.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
outprob_cache_prepare(HMMWork *wrk)
{
  int s,t;

  /* clear already allocated area */
  for (t = 0; t < wrk->outprob_allocframenum; t++) {
    for (s = 0; s < wrk->statenum; s++) {
      wrk->outprob_cache[t][s] = LOG_UNDEF;
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
outprob_cache_extend(HMMWork *wrk, int reqframe)
{
  int newnum;
  int size;
  int t, s;
  LOGPROB *tmpp;

  /* if enough length are already allocated, return immediately */
  if (reqframe < wrk->outprob_allocframenum) return;

  /* allocate per certain period */
  newnum = reqframe + 1;
  if (newnum < wrk->outprob_allocframenum + OUTPROB_CACHE_PERIOD) newnum = wrk->outprob_allocframenum + OUTPROB_CACHE_PERIOD;
  size = (newnum - wrk->outprob_allocframenum) * wrk->statenum;
  
  /* allocate */
  if (wrk->outprob_cache == NULL) {
    wrk->outprob_cache = (LOGPROB **)mymalloc(sizeof(LOGPROB *) * newnum);
  } else {
    wrk->outprob_cache = (LOGPROB **)myrealloc(wrk->outprob_cache, sizeof(LOGPROB *) * newnum);
  }
  tmpp = (LOGPROB *)mybmalloc2(sizeof(LOGPROB) * size, &(wrk->croot));
  /* clear the new part */
  for(t = wrk->outprob_allocframenum; t < newnum; t++) {
    wrk->outprob_cache[t] = &(tmpp[(t - wrk->outprob_allocframenum) * wrk->statenum]);
    for (s = 0; s < wrk->statenum; s++) {
      wrk->outprob_cache[t][s] = LOG_UNDEF;
    }
  }

  /*jlog("outprob cache: %d->%d\n", outprob_allocframenum, newnum);*/
  wrk->outprob_allocframenum = newnum;
}

/**
 * Free work area for cache.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
void
outprob_cache_free(HMMWork *wrk)
{
  if (wrk->croot != NULL) mybfree2(&(wrk->croot));
  if (wrk->outprob_cache != NULL) free(wrk->outprob_cache);
}


/** 
 * @brief  Compute output probability of a state.
 *
 * Set the needed values to the global variables
 * that begins with "OP_", and call calc_outprob_state().  The
 * calc_outprob_state() is actually a function pointer, and the entity is
 * either calc_tied_mix() for tied-mixture model and calc_mix() for others.
 * (If you use GMS, the entity will be gms_state() instead.)
 *
 * The state-level cache is also consulted here.
 *
 * @param wrk [i/o] HMM computation work area
 * @param t [in] time frame
 * @param stateinfo [in] state information to compute the output probability
 * @param param [in] input parameter vectors
 * 
 * @return output log probability.
 */
LOGPROB
outprob_state(HMMWork *wrk, int t, HTK_HMM_State *stateinfo, HTK_Param *param)
{
  LOGPROB outp;
  int sid;
  int i, d;
  HTK_HMM_State *s;

  sid = stateinfo->id;
  
  /* set global values for outprob functions to access them */
  wrk->OP_state = stateinfo;
  wrk->OP_state_id = sid;
  wrk->OP_param = param;
  if (wrk->OP_time != t) {
    wrk->OP_last_time = wrk->OP_time;
    wrk->OP_time = t;
    for(d=0,i=0;i<wrk->OP_nstream;i++) {
      wrk->OP_vec_stream[i] = &(param->parvec[t][d]);
      d += wrk->OP_veclen_stream[i];
    }

    outprob_cache_extend(wrk, t);	/* extend cache if needed */
    wrk->last_cache = wrk->outprob_cache[t]; /* reduce 2-d array access */
  }

  if (param->is_outprob) {
    /* return the param as output probability */
    if (sid >= param->veclen) {
      jlog("Error: state id in the dummy HMM exceeds vector length (%d > %d)\n", sid, param->veclen);
      return(LOG_ZERO);
    }
    return(param->parvec[t][sid]);
  }

  if (wrk->batch_computation) {
    /* batch computation: if the frame is not computed yet, pre-compute all */
    s = wrk->OP_hmminfo->ststart;
    if (wrk->last_cache[s->id] == LOG_UNDEF) {
      for (; s; s = s->next) {
	wrk->OP_state = s;
	wrk->OP_state_id = s->id;
	wrk->last_cache[s->id] = (*(wrk->calc_outprob_state))(wrk);
      }
    }
    wrk->OP_state = stateinfo;
    wrk->OP_state_id = sid;
  }
  
  /* consult cache */
  if ((outp = wrk->last_cache[sid]) == LOG_UNDEF) {
    outp = wrk->last_cache[sid] = (*(wrk->calc_outprob_state))(wrk);
  }
  return(outp);
}

/** 
 * Initialize work area for outprob_cd_nbest().
 * 
 * @param wrk [i/o] HMM computation work area
 * @param num [in] number of top states to be calculated.
 */
void
outprob_cd_nbest_init(HMMWork *wrk, int num)
{
  wrk->cd_nbest_maxprobs = (LOGPROB *)mymalloc(sizeof(LOGPROB) * num);
  wrk->cd_nbest_maxn = num;
}

/**
 * Free work area for outprob_cd_nbest().
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
void
outprob_cd_nbest_free(HMMWork *wrk)
{
  free(wrk->cd_nbest_maxprobs);
}
  
/** 
 * Return average of N-beat outprob for pseudo state set.
 * 
 * @param wrk [i/o] HMM computation work area
 * @param t [in] input frame
 * @param lset [in] pseudo state set
 * @param param [in] input parameter data
 * 
 * @return outprob log probability, average of top N states in @a lset.
 */
static LOGPROB
outprob_cd_nbest(HMMWork *wrk, int t, CD_State_Set *lset, HTK_Param *param)
{
  LOGPROB prob;
  int i, k, n;

  n = 0;
  for(i=0;i<lset->num;i++) {
    prob = outprob_state(wrk, t, lset->s[i], param);
    /*jlog("\t\t%d:%f\n", i, prob);*/
    if (prob <= LOG_ZERO) continue;
    if (n == 0 || prob <= wrk->cd_nbest_maxprobs[n-1]) {
      if (n == wrk->cd_nbest_maxn) continue;
      wrk->cd_nbest_maxprobs[n] = prob;
      n++;
    } else {
      for(k=0; k<n; k++) {
	if (prob > wrk->cd_nbest_maxprobs[k]) {
	  memmove(&(wrk->cd_nbest_maxprobs[k+1]), &(wrk->cd_nbest_maxprobs[k]),
		  sizeof(LOGPROB) * (n - k - ( (n == wrk->cd_nbest_maxn) ? 1 : 0)));
	  wrk->cd_nbest_maxprobs[k] = prob;
	  break;
	}
      }
      if (n < wrk->cd_nbest_maxn) n++;
    }
  }
  prob = 0.0;
  for(i=0;i<n;i++) {
    /*jlog("\t\t\t- %d: %f\n", i, wrk->cd_nbest_maxprobs[i]);*/
    prob += wrk->cd_nbest_maxprobs[i];
  }
  return(prob/(float)n);
}
  
/** 
 * Return maximum outprob of the pseudo state set.
 * 
 * @param wrk [i/o] HMM computation work area
 * @param t [in] input frame
 * @param lset [in] pseudo state set
 * @param param [in] input parameter data
 * 
 * @return maximum output log probability among states in @a lset.
 */
static LOGPROB
outprob_cd_max(HMMWork *wrk, int t, CD_State_Set *lset, HTK_Param *param)
{
  LOGPROB maxprob, prob;
  int i;

  maxprob = LOG_ZERO;
  for(i=0;i<lset->num;i++) {
    prob = outprob_state(wrk, t, lset->s[i], param);
    if (maxprob < prob) maxprob = prob;
  }
  return(maxprob);
}

/** 
 * Return average outprob of the pseudo state set.
 * 
 * @param wrk [i/o] HMM computation work area
 * @param t [in] input frame
 * @param lset [in] pseudo state set
 * @param param [in] input parameter data
 * 
 * @return average output log probability of states in @a lset.
 */
static LOGPROB
outprob_cd_avg(HMMWork *wrk, int t, CD_State_Set *lset, HTK_Param *param)
{
  LOGPROB sum, p;
  int i,j;
  sum = 0.0;
  j = 0;
  for(i=0;i<lset->num;i++) {
    p = outprob_state(wrk, t, lset->s[i], param);
    if (p > LOG_ZERO) {
      sum += p;
      j++;
    }
  }
  return(sum/(float)j);
}

/** 
 * Compute the log output probability of a pseudo state set.
 * 
 * @param wrk [i/o] HMM computation work area
 * @param t [in] input frame
 * @param lset [in] pseudo state set
 * @param param [in] input parameter data
 * 
 * @return the computed log output probability.
 */
LOGPROB
outprob_cd(HMMWork *wrk, int t, CD_State_Set *lset, HTK_Param *param)
{
  LOGPROB ret;

  /* select computation method */
  switch(wrk->OP_hmminfo->cdset_method) {
  case IWCD_AVG:
    ret = outprob_cd_avg(wrk, t, lset, param);
    break;
  case IWCD_MAX:
    ret = outprob_cd_max(wrk, t, lset, param);
    break;
  case IWCD_NBEST:
    ret = outprob_cd_nbest(wrk, t, lset, param);
    break;
  }
  return(ret);
}
  

/** 
 * Top function to compute the output probability of a HMM state.
 *
 * @param wrk [i/o] HMM computation work area
 * @param t [in] input frame
 * @param hmmstate [in] HMM state
 * @param param [in] input parameter data
 * 
 * @return the computed log output probability.
 */
LOGPROB
outprob(HMMWork *wrk, int t, HMM_STATE *hmmstate, HTK_Param *param)
{
  if (hmmstate->is_pseudo_state) {
    return(outprob_cd(wrk, t, hmmstate->out.cdset, param));
  } else {
    return(outprob_state(wrk, t, hmmstate->out.state, param));
  }
}




static boolean
mywrite(char *buf, size_t unitbyte, int unitnum, FILE *fp, boolean needswap)
{
  size_t tmp;

  if (needswap) swap_bytes(buf, unitbyte, unitnum);
  if ((tmp = myfwrite(buf, unitbyte, unitnum, fp)) < (size_t)unitnum) {
    jlog("Error: outprob_cache_output: failed to write %d bytes\n", unitbyte * unitnum);
    return(FALSE);
  }
  //  if (needswap) swap_bytes(buf, unitbyte, unitnum);
  return(TRUE);
}

boolean
outprob_cache_output(FILE *fp, HMMWork *wrk, int framenum)
{
  int s,t;
  boolean needswap;

#ifdef WORDS_BIGENDIAN
  needswap = FALSE;
#else  /* LITTLE ENDIAN */
  needswap = TRUE;
#endif

  needswap = TRUE;

  if (wrk->outprob_allocframenum < framenum) {
    jlog("Error: outprob_cache_output: framenum > allocated (%d > %d)\n", framenum, wrk->outprob_allocframenum);
    return FALSE;
  }

  {
    unsigned int ui;
    unsigned short us;
    short st;
    float f;

    jlog("Stat: outprob_cache_output: %d states, %d samples\n", wrk->statenum, framenum);

    ui = framenum;
    if (!mywrite((char *)&ui, sizeof(unsigned int), 1, fp, needswap)) return FALSE;
    ui = wrk->OP_param->header.wshift;
    if (!mywrite((char *)&ui, sizeof(unsigned int), 1, fp, needswap)) return FALSE;
    us = wrk->statenum * sizeof(float);
    if (!mywrite((char *)&us, sizeof(unsigned short), 1, fp, needswap)) return FALSE;
    st = F_USER;
    if (!mywrite((char *)&st, sizeof(short), 1, fp, needswap)) return FALSE;

    for (t = 0; t < framenum; t++) {
      for (s = 0; s < wrk->statenum; s++) {
	f = wrk->outprob_cache[t][s];
	if (!mywrite((char *)&f, sizeof(float), 1, fp, needswap)) return FALSE;
      }
    }
  }

  return TRUE;
}
