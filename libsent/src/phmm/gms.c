/**
 * @file   gms.c
 * 
 * <JA>
 * @brief  Gaussian Mixture Selection による状態尤度計算
 *
 * 実装方法についてはソース内のコメントをご覧ください．
 * </JA>
 * 
 * <EN>
 * @brief  Calculate state probability with Gaussian Mixture Selection
 *
 * See the comments in the source for details about implementation.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 14:52:18 2005
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

/*
  Implementation of Gaussian Mixture Selection (old doc...)
  
  It is called from gs_calc_selected_mixture_and_cache_{safe,heu,beam} in
  the first pass for each frame.  It calculates all GS HMM outprob for
  given input frame and get the N-best GS HMM states. Then,
       for the selected (N-best) states:
           calculate the corresponding codebook,
           and set fallback_score[t][book] to LOG_ZERO.
       else:
           set fallback_score[t][book] to the GS HMM outprob.
  Later, when calculating state outprobs, the fallback_score[t][book]
  is consulted and,
       if fallback_score[t][book] == LOG_ZERO:
           it means it has been selected, so calculate the outprob with
           the corresponding codebook and its weights.
       else:
           it means it was pruned, so use the fallback_score[t][book]
           as its outprob.

           
  For triphone, GS HMMs should be assigned to each state.
  So the fallback_score[][] is kept according to the GS state ID,
  and corresponding GS HMM state id for each triphone state id should be
  kept beforehand.
  GS HMM Calculation:
       for the selected (N-best) GS HMM states:
           set fallback_score[t][gs_stateid] to LOG_ZERO.
       else:
           set fallback_score[t][gs_stateid] to the GS HMM outprob.
  triphone HMM probabilities are assigned as:
       if fallback_score[t][state2gs[tri_stateid]] == LOG_ZERO:
           it has been selected, so calculate the original outprob.
       else:
           as it was pruned, re-use the fallback_score[t][stateid]
           as its outprob.
*/


#include <sent/stddefs.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/hmm_calc.h>

#undef NORMALIZE_GS_SCORE	/* normalize score (ad-hoc) */

  /* GS HMMs must be defined at STATE level using "~s NAME" macro,
     where NAMES are like "i:4m", "s2m", etc. */


/** 
 * Register all state defs in GS HMM to GS_SET.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
static void
build_gsset(HMMWork *wrk)
{
  HTK_HMM_State *st;

  /* allocate */
  wrk->gsset = (GS_SET *)mymalloc(sizeof(GS_SET) * wrk->OP_gshmm->totalstatenum);
  wrk->gsset_num = wrk->OP_gshmm->totalstatenum;
  /* make ID */
  for(st = wrk->OP_gshmm->ststart; st; st=st->next) {
    wrk->gsset[st->id].state = st;
  }
}

/**
 * Free gsset.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
static void
free_gsset(HMMWork *wrk)
{
  free(wrk->gsset);
}

/** 
 * Build the correspondence from GS states to triphone states.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
build_state2gs(HMMWork *wrk)
{
  HTK_HMM_Data *dt;
  HTK_HMM_State *st, *cr;
  int i;
  char gstr[MAX_HMMNAME_LEN], cbuf[MAX_HMMNAME_LEN];
  boolean ok_p = TRUE;

  /* initialize */
  wrk->state2gs = (int *)mymalloc(sizeof(int) * wrk->OP_hmminfo->totalstatenum);
  for(i=0;i<wrk->OP_hmminfo->totalstatenum;i++) wrk->state2gs[i] = -1;

  /* parse through all HMM macro to register their state */
  for(dt = wrk->OP_hmminfo->start; dt; dt=dt->next) {
    if (strlen(dt->name) >= MAX_HMMNAME_LEN - 2) {
      jlog("Error: gms: too long hmm name (>%d): \"%s\"\n",
	   MAX_HMMNAME_LEN-3, dt->name);
      jlog("Error: gms: change value of MAX_HMMNAME_LEN\n");
      ok_p = FALSE;
      continue;
    }
    for(i=1;i<dt->state_num-1;i++) { /* for all state */
      st = dt->s[i];
      /* skip if already assigned */
      if (wrk->state2gs[st->id] != -1) continue;
      /* set corresponding gshmm name */
      sprintf(gstr, "%s%dm", center_name(dt->name, cbuf), i + 1);
      /* look up the state in OP_gshmm */
      if ((cr = state_lookup(wrk->OP_gshmm, gstr)) == NULL) {
	jlog("Error: gms: GS HMM \"%s\" not defined\n", gstr);
	ok_p = FALSE;
	continue;
      }
      /* store its ID */
      wrk->state2gs[st->id] = cr->id;
    }
  }
#ifdef PARANOIA
  {
    HTK_HMM_State *st;
    for(st=wrk->OP_hmminfo->ststart; st; st=st->next) {
      printf("%s -> %s\n", (st->name == NULL) ? "(NULL)" : st->name,
	     (wrk->gsset[wrk->state2gs[st->id]].state)->name);
    }
  }
#endif
  return ok_p;
}

/**
 * free state2gs.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
static void
free_state2gs(HMMWork *wrk)
{
  free(wrk->state2gs);
}


/* sort to find N-best states */
#define SD(A) idx[A-1]	///< Index macro for heap sort
#define SCOPY(D,S) D = S	///< Element copy macro for heap sort
#define SVAL(A) (fs[idx[A-1]]) ///< Element evaluation macro for heap sort
#define STVAL (fs[s]) ///< Element current value macro for heap sort

/** 
 * Heap sort of @a gsindex to determine which model gets N best likelihoods.
 * 
 * @param wrk [i/o] HMM computation work area
 *
 */
static void
sort_gsindex_upward(HMMWork *wrk)
{
  int n,root,child,parent;
  int s;
  int *idx;
  LOGPROB *fs;
  int neednum, totalnum;

  idx = wrk->gsindex;
  fs = wrk->t_fs;
  neednum = wrk->my_nbest;
  totalnum = wrk->gsset_num;

  for (root = totalnum/2; root >= 1; root--) {
    SCOPY(s, SD(root));
    parent = root;
    while ((child = parent * 2) <= totalnum) {
      if (child < totalnum && SVAL(child) < SVAL(child+1)) {
	child++;
      }
      if (STVAL >= SVAL(child)) {
	break;
      }
      SCOPY(SD(parent), SD(child));
      parent = child;
    }
    SCOPY(SD(parent), s);
  }
  n = totalnum;
  while ( n > totalnum - neednum) {
    SCOPY(s, SD(n));
    SCOPY(SD(n), SD(1));
    n--;
    parent = 1;
    while ((child = parent * 2) <= n) {
      if (child < n && SVAL(child) < SVAL(child+1)) {
	child++;
      }
      if (STVAL >= SVAL(child)) {
	break;
      }
      SCOPY(SD(parent), SD(child));
      parent = child;
    }
    SCOPY(SD(parent), s);
  }
}

/** 
 * Calculate all GS state scores and select the best ones.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
static void
do_gms(HMMWork *wrk)
{
  int i;
  
  /* compute all gshmm scores (in gs_score.c) */
  compute_gs_scores(wrk);
  /* sort and select */
  sort_gsindex_upward(wrk);
  for(i=wrk->gsset_num - wrk->my_nbest;i<wrk->gsset_num;i++) {
    /* set scores of selected states to LOG_ZERO */
    wrk->t_fs[wrk->gsindex[i]] = LOG_ZERO;
  }

  /* power e -> 10 */
#ifdef NORMALIZE_GS_SCORE
  /* normalize other fallback scores (rate of max) */
  for(i=0;i<wrk->gsset_num;i++) {
    if (wrk->t_fs[i] != LOG_ZERO) {
      wrk->t_fs[i] *= 0.975;
    }
  }
#endif
}  


/** 
 * Initialize the GMS related functions and data.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
gms_init(HMMWork *wrk)
{
  int i;
  
  /* Check gshmm type */
  if (wrk->OP_gshmm->is_triphone) {
    jlog("Error: gms: GS HMM should be a monophone model\n");
    return FALSE;
  }
  if (wrk->OP_gshmm->is_tied_mixture) {
    jlog("Error: gms: GS HMM should not be a tied mixture model\n");
    return FALSE;
  }

  /* Register all GS HMM states in GS_SET */
  build_gsset(wrk);
  /* Make correspondence of all triphone states to GS HMM states */
  if (build_state2gs(wrk) == FALSE) {
    jlog("Error: gms: failed in assigning GS HMM state for each state\n");
    return FALSE;
  }
  jlog("Stat: gms: GS HMMs are mapped to HMM states\n");

  /* prepare index buffer for heap sort */
  wrk->gsindex = (int *)mymalloc(sizeof(int) * wrk->gsset_num);
  for(i=0;i<wrk->gsset_num;i++) wrk->gsindex[i] = i;

  /* init cache status */
  wrk->fallback_score = NULL;
  wrk->gms_is_selected = NULL;
  wrk->gms_allocframenum = -1;

  /* initialize gms_gprune functions */
  gms_gprune_init(wrk);
  
  return TRUE;
}

/** 
 * Setup GMS parameters for next input.
 * 
 * @param wrk [i/o] HMM computation work area
 * @param framenum [in] length of next input in frames
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
gms_prepare(HMMWork *wrk, int framenum)
{
  LOGPROB *tmp;
  int t;

  /* allocate cache */
  if (wrk->gms_allocframenum < framenum) {
    if (wrk->fallback_score != NULL) {
      free(wrk->fallback_score[0]);
      free(wrk->fallback_score);
      free(wrk->gms_is_selected);
    }
    wrk->fallback_score = (LOGPROB **)mymalloc(sizeof(LOGPROB *) * framenum);
    tmp = (LOGPROB *)mymalloc(sizeof(LOGPROB) * wrk->gsset_num * framenum);
    for(t=0;t<framenum;t++) {
      wrk->fallback_score[t] = &(tmp[wrk->gsset_num * t]);
    }
    wrk->gms_is_selected = (boolean *)mymalloc(sizeof(boolean) * framenum);
    wrk->gms_allocframenum = framenum;
  }
  /* clear */
  for(t=0;t<framenum;t++) wrk->gms_is_selected[t] = FALSE;

  /* prepare gms_gprune functions */
  gms_gprune_prepare(wrk);
  
  return TRUE;
}

/**
 * Free GMS related work areas.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 */
void
gms_free(HMMWork *wrk)
{
  free_gsset(wrk);
  free_state2gs(wrk);
  free(wrk->gsindex);
  if (wrk->fallback_score != NULL) {
    free(wrk->fallback_score[0]);
    free(wrk->fallback_score);
    free(wrk->gms_is_selected);
  }
  gms_gprune_free(wrk);
}



/** 
 * Get %HMM State probability of current state with Gaussiam Mixture Selection.
 *
 * If the GMS %HMM score of the corresponding basephone is below the
 * N-best, the triphone score will not be computed, and the score of
 * the GMS %HMM will be returned instead as a fallback score.
 * Else, the precise triphone will be computed and returned.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 * @return the state output probability score in log10.
 */
LOGPROB
gms_state(HMMWork *wrk)
{
  LOGPROB gsprob;
  if (wrk->OP_last_time != wrk->OP_time) { /* different frame */
    /* set current buffer */
    wrk->t_fs = wrk->fallback_score[wrk->OP_time];
    /* select state if not yet */
    if (!wrk->gms_is_selected[wrk->OP_time]) {
      do_gms(wrk);
      wrk->gms_is_selected[wrk->OP_time] = TRUE;
    }
  }
  if ((gsprob = wrk->t_fs[wrk->state2gs[wrk->OP_state_id]]) != LOG_ZERO) {
    /* un-selected: return the fallback value */
    return(gsprob);
  }
  /* selected: calculate the real outprob of the state */
  return((*(wrk->calc_outprob))(wrk));
}
