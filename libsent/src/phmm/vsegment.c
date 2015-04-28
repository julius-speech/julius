/**
 * @file   vsegment.c
 * 
 * <JA>
 * @brief  入力に対するViterbi アライメントの実行
 * </JA>
 * 
 * <EN>
 * @brief  Do viterbi alignment for the input
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Fri Feb 18 19:29:22 2005
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
#include <sent/htk_param.h>
#include <sent/hmm.h>

/** 
 * @brief  Perform Viterbi alignment.
 *
 * This function performs viterbi alignment for the given sentence %HMM,
 * input parameter and unit definition.  Any segmentatino unit (word, phoneme
 * state, etc.) is allowed: the segmentation unit should be specified by
 * specifying a list of state id which are the end of each unit.
 * For example, if you want to obtain phoneme alignment, the list of state
 * number that exist at the end of phones should be specified by @a endstates.
 * 
 * @param hmm [in] sentence HMM to be matched
 * @param param [in] input parameter data
 * @param wrk [i/o] HMM computation work area
 * @param multipath [in] TRUE if need multi-path handling
 * @param endstates [in] list of state id that corrsponds to the ends of units
 * @param ulen [in] total number of units in the @a hmm
 * @param id_ret [out] Pointer to store the newly allocated array of the resulting id sequence of units on the best path.
 * @param seg_ret [out] Pointer to store the newly allocated array of the resulting end frame of each unit on the best path.
 * @param uscore_ret [out] Pointer to store the newly allocated array of the resulting score at the end frame of each unit on the best path.
 * @param slen_ret [out] Pointer to store the total number of units on the best path.
 * 
 * @return the total acoustic score for the whole input.
 */
LOGPROB
viterbi_segment(HMM *hmm, HTK_Param *param, HMMWork *wrk, boolean multipath, int *endstates, int ulen, int **id_ret, int **seg_ret, LOGPROB **uscore_ret, int *slen_ret)
{
  /* for viterbi */
  LOGPROB *nodescore[2];	/* node buffer */
  SEGTOKEN **tokenp[2];		/* propagating token which holds segment info */
  int startt, endt;
  int *from_node;
  int *u_end, *u_start;	/* the node is an end of the word, or -1 for non-multipath mode*/
  int i, n;
  unsigned int t;
  int tl,tn;
  LOGPROB tmpsum;
  A_CELL *ac;
  SEGTOKEN *newtoken, *token, *tmptoken, *root;
  LOGPROB result_score;
  LOGPROB maxscore, minscore;	/* for debug */
  int maxnode;			/* for debug */
  int *id, *seg, slen;
  LOGPROB *uscore;

  /* assume more than 1 units */
  if (ulen < 1) {
    jlog("Error: vsegment: no unit?\n");
    return LOG_ZERO;
  }

  if (!multipath) {
    /* initialize unit start/end marker */
    u_start = (int *)mymalloc(hmm->len * sizeof(int));
    u_end   = (int *)mymalloc(hmm->len * sizeof(int));
    for (n = 0; n < hmm->len; n++) {
      u_start[n] = -1;
      u_end[n] = -1;
    }
    u_start[0] = 0;
    u_end[endstates[0]] = 0;
    for (i=1;i<ulen;i++) {
      u_start[endstates[i-1]+1] = i;
      u_end[endstates[i]] = i;
    }
#if 0
    for (i=0;i<hmm->len;i++) {
      printf("unit %d: start=%d, end=%d\n", i, u_start[i], u_end[i]);
    }
#endif
  }

  /* initialize node buffers */
  tn = 0;
  tl = 1;
  root = NULL;
  for (i=0;i<2;i++){
    nodescore[i] = (LOGPROB *)mymalloc(hmm->len * sizeof(LOGPROB));
    tokenp[i] = (SEGTOKEN **)mymalloc(hmm->len * sizeof(SEGTOKEN *));
    for (n = 0; n < hmm->len; n++) {
      tokenp[i][n] = NULL;
    }
  }
  for (n = 0; n < hmm->len; n++) {
    nodescore[tn][n] = LOG_ZERO;
    newtoken = (SEGTOKEN *)mymalloc(sizeof(SEGTOKEN));
    newtoken->last_id = -1;
    newtoken->last_end_frame = -1;
    newtoken->last_end_score = 0.0;
    newtoken->list = root;
    root = newtoken;
    newtoken->next = NULL;
    tokenp[tn][n] = newtoken;
  }
  from_node = (int *)mymalloc(sizeof(int) * hmm->len);
  
  /* first frame: only set initial score */
  /*if (hmm->state[0].is_pseudo_state) {
    jlog("Warning: state %d: pseudo state?\n", 0);
    }*/
  if (multipath) {
    nodescore[tn][0] = 0.0;
  } else {
    nodescore[tn][0] = outprob(wrk, 0, &(hmm->state[0]), param);
  }

  /* do viterbi for rest frame */
  if (multipath) {
    startt = 0;  endt = param->samplenum;
  } else {
    startt = 1;  endt = param->samplenum - 1;
  }
  for (t = startt; t <= endt; t++) {
    i = tl;
    tl = tn;
    tn = i;
    maxscore = LOG_ZERO;
    minscore = 0.0;

    /* clear next scores */
    for (i=0;i<hmm->len;i++) {
      nodescore[tn][i] = LOG_ZERO;
      from_node[i] = -1;
    }

    /* select viterbi path for each node */
    for (n = 0; n < hmm->len; n++) {
      if (nodescore[tl][n] <= LOG_ZERO) continue;
      for (ac = hmm->state[n].ac; ac; ac = ac->next) {
        tmpsum = nodescore[tl][n] + ac->a;
        if (nodescore[tn][ac->arc] < tmpsum) {
          nodescore[tn][ac->arc] = tmpsum;
	  from_node[ac->arc] = n;
	}
      }
    }
    /* propagate token, appending new if path was selected between units */
    if (multipath) {
      for (n = 0; n < hmm->len; n++) {
	if (from_node[n] == -1 || nodescore[tn][n] <= LOG_ZERO) {
	  /*tokenp[tn][n] = NULL;*/
	} else {
	  i=0;
	  while (from_node[n] > endstates[i]) i++;
	  if (n > endstates[i]) {
	    newtoken = (SEGTOKEN *)mymalloc(sizeof(SEGTOKEN));
	    newtoken->last_id = i;
	    newtoken->last_end_frame = t-1;
	    newtoken->last_end_score = nodescore[tl][from_node[n]];
	    newtoken->list = root;
	    root = newtoken;
	    newtoken->next = tokenp[tl][from_node[n]];
	    tokenp[tn][n] = newtoken;
	  } else {
	    tokenp[tn][n] = tokenp[tl][from_node[n]];
	  }
	}
      }
    } else {			/* not multipath */
      for (n = 0; n < hmm->len; n++) {
	if (from_node[n] == -1) {
	  tokenp[tn][n] = NULL;
	} else if (nodescore[tn][n] <= LOG_ZERO) {
	  tokenp[tn][n] = tokenp[tl][from_node[n]];
	} else {
	  if (u_end[from_node[n]] != -1 && u_start[n] != -1
	      && from_node[n] !=  n) {
	    newtoken = (SEGTOKEN *)mymalloc(sizeof(SEGTOKEN));
	    newtoken->last_id = u_end[from_node[n]];
	    newtoken->last_end_frame = t-1;
	    newtoken->last_end_score = nodescore[tl][from_node[n]];
	    newtoken->list = root;
	    root = newtoken;
	    newtoken->next = tokenp[tl][from_node[n]];
	    tokenp[tn][n] = newtoken;
	  } else {
	    tokenp[tn][n] = tokenp[tl][from_node[n]];
	  }
	}
      }
    }

    if (multipath) {
      /* if this is next of last frame, loop ends here */
      if (t == param->samplenum) break;
    }
	
    /* calc outprob to new nodes */
    for (n = 0; n < hmm->len; n++) {
      if (multipath) {
	if (hmm->state[n].out.state == NULL) continue;
      }
      if (nodescore[tn][n] > LOG_ZERO) {
	if (hmm->state[n].is_pseudo_state) {
	  jlog("Warning: vsegment: state %d: pseudo state?\n", n);
	}
	nodescore[tn][n] += outprob(wrk, t, &(hmm->state[n]), param);
      }
      if (nodescore[tn][n] > maxscore) { /* for debug */
	maxscore = nodescore[tn][n];
	maxnode = n;
      }
    }
    
#if 0
    for (i=0;i<ulen;i++) {
      printf("%d: unit %d(%d-%d): begin_frame = %d\n", t - 1, i,
	     (i > 0) ? endstates[i-1]+1 : 0, endstates[i],
	     (multipath && tokenp[tl][endstates[i]] == NULL) ? -1 : tokenp[tl][endstates[i]]->last_end_frame + 1);
    }
#endif

    /* printf("t=%3d max=%f n=%d\n",t,maxscore, maxnode); */
    
  }

  result_score = nodescore[tn][hmm->len-1];

  /* parse back the last token to see the trail of best viterbi path */
  /* and store the informations to returning buffer */
  slen = 0;
  if (!multipath) slen++;
  for(token = tokenp[tn][hmm->len-1]; token; token = token->next) {
    if (token->last_end_frame == -1) break;
    slen++;
  }
  id = (int *)mymalloc(sizeof(int)*slen);
  seg = (int *)mymalloc(sizeof(int)*slen);
  uscore = (LOGPROB *)mymalloc(sizeof(LOGPROB)*slen);

  if (multipath) {
    i = slen - 1;
  } else {
    id[slen-1] = ulen - 1;
    seg[slen-1] = t - 1;
    uscore[slen-1] = result_score;
    i = slen - 2;
  }
  for(token = tokenp[tn][hmm->len-1]; token; token = token->next) {
    if (i < 0 || token->last_end_frame == -1) break;
    id[i] = token->last_id;
    seg[i] = token->last_end_frame;
    uscore[i] = token->last_end_score;
    i--;
  }

  /* normalize scores by frame */
  for (i=slen-1;i>0;i--) {
    uscore[i] = (uscore[i] - uscore[i-1]) / (seg[i] - seg[i-1]);
  }
  uscore[0] = uscore[0] / (seg[0] + 1);

  /* set return value */
  *id_ret = id;
  *seg_ret = seg;
  *uscore_ret = uscore;
  *slen_ret = slen;

  /* free memory */
  if (!multipath) {
    free(u_start);
    free(u_end);
  }
  free(from_node);
  token = root;
  while(token) {
    tmptoken = token->list;
    free(token);
    token = tmptoken;
  }
  for (i=0;i<2;i++) {
    free(nodescore[i]);
    free(tokenp[i]);
  }

  return(result_score);

}
