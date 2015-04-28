/**
 * @file   mkwhmm.c
 * 
 * <JA>
 * @brief  音素列から計算用の結合%HMMを生成する
 * </JA>
 * 
 * <EN>
 * @brief  Generate compound %HMM instance for recognition from phoneme sequence.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Fri Feb 18 18:31:40 2005
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

/* When not a multi-path mode, initial & accept arc will be stripped and
   trans prob to accept state will be stored in accept_ac_a. */

#include <sent/stddefs.h>
#include <sent/hmm.h>

/** 
 * Calculate total number of states in a phoneme sequence.
 * 
 * @param hdseq [in] phoneme sequence as given by pointer list of logical %HMM
 * @param hdseqlen [in] length of above
 * @param has_sp [in] indicates where short-pause insertion is possible
 * @param hmminfo [in] HMM definition
 * 
 * @return the total number of states in the sequence.
 */
static int
totalstatelen(HMM_Logical **hdseq, int hdseqlen, boolean *has_sp, HTK_HMM_INFO *hmminfo)
{
  int i, len;

  len = 0;
  for (i=0;i<hdseqlen;i++) {
    len += hmm_logical_state_num(hdseq[i]) - 2;
    if (has_sp) {
      if (has_sp[i]) {
	len += hmm_logical_state_num(hmminfo->sp) - 2;
      }
    }
  }
  if (hmminfo->multipath) {
    /* add count for the initial and final state */
    len += 2;
  }
  return(len);
}

/** 
 * Add a transition arc on the HMM state.
 * 
 * @param state [out] HMM state to add the arc
 * @param arc [in] state id of destination 
 * @param a [in] transition log probability
 */
static void
add_arc(HMM_STATE *state, int arc, LOGPROB a)
{
  A_CELL *atmp;

  atmp = (A_CELL *)mymalloc(sizeof(A_CELL));
  atmp->a = a;
  atmp->arc = arc;
  atmp->next = state->ac;
  state->ac = atmp;
}

/* make word(phrase) HMM from HTK_HMM_INFO */
/* LM prob will be assigned for cross-word arcs */
/* new HMM is malloced and returned */

/** 
 * Make a HMM instance for recognition from phoneme sequence, with connection
 * probabiliry given for each phoneme.
 * 
 * @param hmminfo [in] HTK %HMM definitions data
 * @param hdseq [in] phoneme sequence as given by pointer list of logical %HMM
 * @param hdseqlen [in] length of above
 * @param has_sp [in] indicates where short-pause insertion is possible
 * @param lscore [in] list of log probability to be added at the emitting
 * transition of each phoneme, or NULL if not needed.
 * 
 * @return newly allocated HMM instance generated from the given data.
 */
HMM *
new_make_word_hmm_with_lm(HTK_HMM_INFO *hmminfo, HMM_Logical **hdseq, int hdseqlen, boolean *has_sp, LOGPROB *lscore)
{
  HMM *new;
  int i,j,n;
  int afrom, ato;
  LOGPROB logprob;
  HTK_HMM_Trans *tr;
  int state_num;

  if (has_sp) {
    if (hmminfo->sp == NULL) {
      jlog("Error: mkwhmm: no short-pause model in hmminfo\n");
      return NULL;
    }
  }

  /* allocate needed states */
  new = (HMM *)mymalloc(sizeof(HMM));
  new->len = totalstatelen(hdseq, hdseqlen, has_sp, hmminfo);
  new->state = (HMM_STATE *)mymalloc(sizeof(HMM_STATE) * new->len);
  for (i=0;i<new->len;i++) {
    new->state[i].ac = NULL;
    new->state[i].is_pseudo_state = FALSE;
    new->state[i].out.state = NULL;
    new->state[i].out.cdset = NULL;
  }

  /* assign outprob informations into the states  */
  n = 0;
  if (hmminfo->multipath) n++;	/* skip first state */
  for (i = 0; i < hdseqlen; i++) {
    if (hdseq[i]->is_pseudo) {
      for (j = 1; j < hdseq[i]->body.pseudo->state_num - 1; j++) {
	new->state[n].is_pseudo_state = TRUE;
	new->state[n].out.cdset = &(hdseq[i]->body.pseudo->stateset[j]);
	n++;
      }
    } else {
      for (j = 1; j < hdseq[i]->body.defined->state_num - 1; j++) {
	new->state[n].is_pseudo_state = FALSE;
	new->state[n].out.state = hdseq[i]->body.defined->s[j];
	n++;
      }
    }
    if (has_sp) {
      if (has_sp[i]) {
	/* append sp at the end of the phone */
	if (hmminfo->sp->is_pseudo) {
	  for (j = 1; j < hmm_logical_state_num(hmminfo->sp) - 1; j++) {
	    new->state[n].is_pseudo_state = TRUE;
	    new->state[n].out.cdset = &(hmminfo->sp->body.pseudo->stateset[j]);
	    n++;
	  }
	} else {
	  for (j = 1; j < hmm_logical_state_num(hmminfo->sp) - 1; j++) {
	    new->state[n].is_pseudo_state = FALSE;
	    new->state[n].out.state = hmminfo->sp->body.defined->s[j];
	    n++;
	  }
	}
      }
    }
  }
  
  /* make transition arcs between each state*/
/* 
 *   for (i=0;i<hdseq[0]->def->state_num;i++) {
 *     if (i != 1 && (hdseq[0]->def->tr->a[0][i]) != LOG_ZERO) {
 *	 jlog("initial state contains more than 1 arc.\n");
 *     }
 *   }
 */

  if (hmminfo->multipath) {

    int *out_from, *out_from_next;
    LOGPROB *out_a, *out_a_next;
    int out_num_prev, out_num_next;
    out_from = (int *)mymalloc(sizeof(int) * new->len);
    out_from_next = (int *)mymalloc(sizeof(int) * new->len);
    out_a = (LOGPROB *)mymalloc(sizeof(LOGPROB) * new->len);
    out_a_next = (LOGPROB *)mymalloc(sizeof(LOGPROB) * new->len);

    n = 0;			/* n points to previous state */

    out_from[0] = 0;
    out_a[0] = 0.0;
    out_num_prev = 1;
    for (i = 0; i < hdseqlen; i++) {
      state_num = hmm_logical_state_num(hdseq[i]);
      tr = hmm_logical_trans(hdseq[i]);
      out_num_next = 0;
      /* arc from initial state */
      for (ato = 1; ato < state_num; ato++) {
	logprob = tr->a[0][ato];
	if (logprob != LOG_ZERO) {
	  /* expand arc */
	  if (ato == state_num-1) {
	    /* from initial to final ... register all previously registered arcs for next expansion */
	    if (lscore != NULL) logprob += lscore[i];
	    for(j=0;j<out_num_prev;j++) {
	      out_from_next[out_num_next] = out_from[j];
	      out_a_next[out_num_next] = out_a[j] + logprob;
	      out_num_next++;
	    }
	  } else {
	    for(j=0;j<out_num_prev;j++) {
	      add_arc(&(new->state[out_from[j]]), n + ato,
		      out_a[j] + logprob);
	    }
	  }
	}
      }
      /* arc from output state */
      for(afrom = 1; afrom < state_num - 1; afrom++) {
	for (ato = 1; ato < state_num; ato++) {
	  logprob = tr->a[afrom][ato];
	  if (logprob != LOG_ZERO) {
	    if (ato == state_num - 1) {
	      /* from output state to final ... register the arc for next expansion */
	      if (lscore != NULL) logprob += lscore[i];
	      out_from_next[out_num_next] = n+afrom;
	      out_a_next[out_num_next++] = logprob;
	    } else {
	      add_arc(&(new->state[n+afrom]), n + ato, logprob);
	    }
	  }
	}
      }
      n += state_num - 2;
      for(j=0;j<out_num_next;j++) {
	out_from[j] = out_from_next[j];
	out_a[j] = out_a_next[j];
      }
      out_num_prev = out_num_next;

      /* inter-word short pause handling */
      if (has_sp && has_sp[i]) {
      
	out_num_next = 0;

	/* arc from initial state */
	for (ato = 1; ato < hmm_logical_state_num(hmminfo->sp); ato++) {
	  logprob = hmm_logical_trans(hmminfo->sp)->a[0][ato];
	  if (logprob != LOG_ZERO) {
	    /* to control short pause insertion, transition probability toward
	       the word-end short pause will be given a penalty */
	    logprob += hmminfo->iwsp_penalty;
	    /* expand arc */
	    if (ato == hmm_logical_state_num(hmminfo->sp)-1) {
	      /* from initial to final ... register all previously registered arcs for next expansion */
	      for(j=0;j<out_num_prev;j++) {
		out_from_next[out_num_next] = out_from[j];
		out_a_next[out_num_next] = out_a[j] + logprob;
		out_num_next++;
	      }
	    } else {
	      for(j=0;j<out_num_prev;j++) {
		add_arc(&(new->state[out_from[j]]), n + ato,
			out_a[j] + logprob);
	      }
	    }
	  }
	}
	/* if short pause model doesn't have a model skip transition, also add it */
	if (hmm_logical_trans(hmminfo->sp)->a[0][hmm_logical_state_num(hmminfo->sp)-1] == LOG_ZERO) {
	  /* to make insertion sp model to have no effect on the original path,
	     the skip transition probability should be 0.0 (=100%) */
	  logprob = 0.0;
	  for(j=0; j<out_num_prev; j++) {
	    out_from_next[out_num_next] = out_from[j];
	    out_a_next[out_num_next] = out_a[j] + logprob;
	    out_num_next++;
	  }
	}
	/* arc from output state */
	for(afrom = 1; afrom < hmm_logical_state_num(hmminfo->sp) - 1; afrom++) {
	  for (ato = 1; ato < hmm_logical_state_num(hmminfo->sp); ato++) {
	    logprob = hmm_logical_trans(hmminfo->sp)->a[afrom][ato];
	    if (logprob != LOG_ZERO) {
	      if (ato == hmm_logical_state_num(hmminfo->sp) - 1) {
		/* from output state to final ... register the arc for next expansion */
		out_from_next[out_num_next] = n+afrom;
		out_a_next[out_num_next++] = logprob;
	      } else {
		add_arc(&(new->state[n+afrom]), n + ato, logprob);
	      }
	    }
	  }
	}
	n += hmm_logical_state_num(hmminfo->sp) - 2;
	for(j=0;j<out_num_next;j++) {
	  out_from[j] = out_from_next[j];
	  out_a[j] = out_a_next[j];
	}
	out_num_prev = out_num_next;
      }
    }
      
    
    for(j=0;j<out_num_prev;j++) {
      add_arc(&(new->state[out_from[j]]), new->len-1, out_a[j]);
    }
    free(out_from);
    free(out_from_next);
    free(out_a);
    free(out_a_next);

  } else {
    /* non-multipath version */

    new->accept_ac_a = LOG_ZERO;
    n = 0;
    for (i = 0; i < hdseqlen; i++) {
      state_num = hmm_logical_state_num(hdseq[i]);
      tr = hmm_logical_trans(hdseq[i]);
      /* for each phoneme, consult the transition matrix to form HMM instance */
      for (afrom = 1; afrom < state_num - 1; afrom++) {
	for (ato = 1; ato < state_num; ato++) {
	  logprob = tr->a[afrom][ato];
	  if (logprob != LOG_ZERO) {
	    /* if emitting transition, add connection probability to the arc */
	    if (ato == state_num - 1 && lscore != NULL){
	      logprob += lscore[i];
	    }
	    if (n + (ato - afrom) >= new->len) { /* arc to accept node */
	      if (new->accept_ac_a != LOG_ZERO) {
		jlog("Error: mkwhmm: more than 1 arc to accept node found\n");
		return NULL;
		
	      } else {
		new->accept_ac_a = logprob;
	      }
	    } else {
	      add_arc(&(new->state[n]), n + (ato - afrom), logprob);
	    }
	  }
	}
	n++;
      }
    }
  }

  return (new);
}

/** 
 * Make a HMM instance for recognition from phoneme sequence.
 * 
 * @param hmminfo [in] HTK %HMM definitions data
 * @param hdseq [in] phoneme sequence as given by pointer list of logical %HMM
 * @param hdseqlen [in] length of above
 * @param has_sp [in] indicates where short-pause insertion is possible
 * 
 * @return newly allocated HMM instance generated from the given data.
 */
HMM *
new_make_word_hmm(HTK_HMM_INFO *hmminfo, HMM_Logical **hdseq, int hdseqlen, boolean *has_sp)
{
  return(new_make_word_hmm_with_lm(hmminfo, hdseq, hdseqlen, has_sp, NULL));
}

/** 
 * Free an HMM instance.
 * 
 * @param d [in] HMM instance to free
 */
void
free_hmm(HMM *d)
{
  A_CELL *ac, *atmp;
  int i;

  for (i=0;i<d->len;i++) {
    ac = d->state[i].ac;
    while (ac) {
      atmp = ac->next;
      free(ac);
      ac = atmp;
    }
  }
  free(d->state);
  free(d);
}
