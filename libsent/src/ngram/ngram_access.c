/**
 * @file   ngram_access.c
 * 
 * <JA>
 * @brief  単語列・クラス列の N-gram 確率を求める
 *
 * </JA>
 * 
 * <EN>
 * @brief  Get N-gram probability of a word/class sequence.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 07:46:18 2005
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
#include <sent/ngram2.h>

#undef ADEBUG

/** 
 * Search for n-gram tuple.
 * 
 * @param ndata [in] word/class N-gram
 * @param n [in] N of N-gram
 * @param nid_prev [in] context (N-1)-gram tuple ID
 * @param wkey [in] the target word ID
 * 
 * @return corresponding index to the 2-gram data part if found, or
 * NNID_INVALID if the tuple does not exist in 2-gram.
 */
static NNID
search_ngram_core(NGRAM_INFO *ndata, int n, NNID nid_prev, WORD_ID wkey)
{
  NGRAM_TUPLE_INFO *t, *tprev;
  NNID nnid;
  NNID left,right,mid;
  NNID x;

  if (ndata->bigram_index_reversed && n == 2) {
    /* old binary format builds 1gram->2gram mapping using LR 2-gram,
       although the main model is RL 3-gram.  This hacks this problem */
    x = nid_prev;
    nid_prev = wkey;
    wkey = x;
  }

  t = &(ndata->d[n-1]);
  tprev = &(ndata->d[n-2]);
  
  if (tprev->ct_compaction) {
    nnid = tprev->nnid2ctid_upper[nid_prev];
    if (nnid == NNID_INVALID_UPPER) return (NNID_INVALID);
    nnid = (nnid << 16) + (NNID)(tprev->nnid2ctid_lower[nid_prev]);
  } else {
    nnid = nid_prev;
  }
  if (t->is24bit) {
    left = t->bgn_upper[nnid];
    if (left == NNID_INVALID_UPPER) return (NNID_INVALID);
    left = (left << 16) + (NNID)(t->bgn_lower[nnid]);
  } else {
    left = t->bgn[nnid];
    if (left == NNID_INVALID) return (NNID_INVALID);
  }
  right = left + t->num[nnid] - 1;

  while(left < right) {
    mid = (left + right) / 2;
    if (t->nnid2wid[mid] < wkey) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  if (t->nnid2wid[left] == wkey) {
    return (left);
  } else {
    return (NNID_INVALID);
  }
}

/** 
 * Search for N-tuples.
 * 
 * @param ndata [in] word/class N-gram
 * @param n [in] N of N-gram (= number of words in @a w)
 * @param w [in] word sequence
 * 
 * @return 
 */
NNID
search_ngram(NGRAM_INFO *ndata, int n, WORD_ID *w)
{
  int i;
  NNID prev, next;

  if (n == 1) {
    /* wid = nnid in 1-gram */
    return(w[0]);
  }

  prev = w[0];
  for(i=2;i<=n;i++) {
    next = search_ngram_core(ndata, i, prev, w[i-1]);
    if (next == NNID_INVALID) {
      return NNID_INVALID;
    }
    prev = next;
  }
  return(next);
}


/** 
 * Get N-gram probability of the last word w_n, given context w_1^n-1. 
 * 
 * @param ndata [in] word/class N-gram
 * @param n [in] N of N-gram (= number of words in @a w)
 * @param w [in] word sequence
 * 
 * @return 
 */
LOGPROB
ngram_prob(NGRAM_INFO *ndata, int n, WORD_ID *w)
{
  int i;
  NNID prev, next, bid;
  LOGPROB p;
  NGRAM_TUPLE_INFO *t;

  if (n > ndata->n) {
    jlog("ERROR: no %d-gram exist (max %d)\n", n, ndata->n);
    return LOG_ZERO;
  }

#ifdef ADEBUG
  printf("[");
  if (n > 1) {
    for(i=0;i<n-1;i++) printf("%s ", ndata->wname[w[i]]);
    printf("| ");
  }
  printf("%s]\n", ndata->wname[w[n-1]]);
#endif

  /* unigram */
  if (n == 1) {
    p = ndata->d[0].prob[w[0]];
    if (w[0] == ndata->unk_id) p -= ndata->unk_num_log;
#ifdef ADEBUG
    printf("hit: %f\n", p);
#endif
    return(p);
  }

  /* parse for ngram to reach the N-gram tuple */
  prev = w[0];
  for(i=2;i<=n;i++) {
    next = search_ngram_core(ndata, i, prev, w[i-1]);
    if (next == NNID_INVALID) break;
    prev = next;
  }
  if (next == NNID_INVALID) {	/* not reached */
    /* both back-off or fallback uses (n-1) gram of the target word */
    /* recursive call to get the fallback likelihood */
#ifdef ADEBUG
    printf("--(not found)->\n");
#endif
    p = ngram_prob(ndata, n-1, &(w[1]));
    if (i == n) {     /* the last parse was terminated at last step */
      /* get back-off weight on prev */
      t = &(ndata->d[i-2]);
      if (t->ct_compaction) {
	if ((bid = t->nnid2ctid_upper[prev]) == NNID_INVALID_UPPER) {
	  /* in case back-off entry not found, it means bo_wt == 0.0 */
#ifdef ADEBUG
	  printf("fall: %f\n", p);
#endif
	  return(p);
	} else {
	  bid = (bid << 16) + (NNID)(t->nnid2ctid_lower[prev]);
	}
      } else {
	bid = prev;
      }
      /* return back-off likelihood */
#ifdef ADEBUG
      printf("back: %f + %f\n", t->bo_wt[bid], p);
#endif
      return(t->bo_wt[bid] + p);
    } else {
      /* previous context not found, fallback to (n-1)-gram */
      return(p);
    }
  }
  /* n-gram found */
  /* trigram exist */
  p = ndata->d[n-1].prob[next];
  if (w[n-1] == ndata->unk_id) p -= ndata->unk_num_log;

#ifdef ADEBUG
  printf("hit: %f\n", p);
#endif
  return(p);
}

/* ---------------------------------------------------------------------- */
/* separate access functions for the 1st pass */

/** 
 * Get 1-gram probability of @f$w@f$ in log10.
 *
 * @param ndata [in] word/class N-gram
 * @param w [in] word/class ID in N-gram
 * 
 * @return log10 probability @f$\log p(w)@f$.
 */
LOGPROB
uni_prob(NGRAM_INFO *ndata, WORD_ID w)
{
  if (w != ndata->unk_id) {
    return(ndata->d[0].prob[w]);
  } else {
    return(ndata->d[0].prob[w] - ndata->unk_num_log);
  }
}

/**
 * Find bi-gram entry.  Assumes ct_compaction and is24bit is FALSE on 2-gram
 * 
 * @param ndata [in] N-gram data that holds the 2-gram
 * @param w_context [in] context word ID
 * @param w [in] target word ID
 * 
 * @return the ID of N-gram tuple entry ID where the (w_context, w) exists.
 * 
 */
static NNID
search_bigram(NGRAM_INFO *ndata, WORD_ID w_context, WORD_ID w)
{
  /* do binary search to find bigram entry */
  /* assume ct_compaction and is24bit is FALSE on 2-gram */
  NNID left,right,mid;		/* n2 */
  NGRAM_TUPLE_INFO *t;

  t = &(ndata->d[1]);

  if ((left = t->bgn[w_context]) == NNID_INVALID) /* has no bigram */
    return (NNID_INVALID);
  right = left + t->num[w_context] - 1;
  while(left < right) {
    mid = (left + right) / 2;
    if (t->nnid2wid[mid] < w) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  if (t->nnid2wid[left] == w) {
    return (left);
  } else {
    return (NNID_INVALID);
  }
}

/** 
 * Get LR bi-gram prob: for LR N-gram
 * 
 * @param ndata [in] N-gram data that holds the 2-gram
 * @param w1 [in] left context word

 * @param w2 [in] right target word
 * 
 * @return the log N-gram probability P(w2|w1)
 * 
 */
static LOGPROB
bi_prob_normal(NGRAM_INFO *ndata, WORD_ID w1, WORD_ID w2)
{
  NNID n2;
  LOGPROB prob;

  /* index is LR */
  /* prob is in main N-gram area */
  if ((n2 = search_bigram(ndata, w1, w2)) != NNID_INVALID) {
    prob = ndata->d[1].prob[n2];
  } else {
    prob = ndata->d[0].bo_wt[w1] + ndata->d[0].prob[w2];
  }
  if (w2 != ndata->unk_id) {
    return(prob);
  } else {
    return(prob - ndata->unk_num_log);
  }
}

/** 
 * Get LR bi-gram prob: for RL N-gram with additional LR 2-gram, in
 * old bingram format.  The old format has 2-gram index in reversed
 * orfer, so this function is for old bingram formats.
 * 
 * @param ndata [in] N-gram data that holds the 2-gram
 * @param w1 [in] left context word
 * @param w2 [in] right target word
 * 
 * @return the log N-gram probability P(w2|w1)
 * 
 */
static LOGPROB
bi_prob_additional_oldbin(NGRAM_INFO *ndata, WORD_ID w1, WORD_ID w2)
{
  NNID n2;
  LOGPROB prob;

  /* index is LR */
  /* prob is in additional N-gram area */
  if ((n2 = search_bigram(ndata, w1, w2)) != NNID_INVALID) {
    prob = ndata->p_2[n2];
  } else {
    prob = ndata->bo_wt_1[w1] + ndata->d[0].prob[w2];
  }
  if (w2 != ndata->unk_id) {
    return(prob);
  } else {
    return(prob - ndata->unk_num_log);
  }
}


/** 
 * Get LR bi-gram prob: for RL N-gram with additional LR 2-gram.
 * 
 * @param ndata [in] N-gram data that holds the 2-gram
 * @param w1 [in] left context word
 * @param w2 [in] right target word
 * 
 * @return the log N-gram probability P(w2|w1)
 * 
 */
static LOGPROB
bi_prob_additional(NGRAM_INFO *ndata, WORD_ID w1, WORD_ID w2)
{
  NNID n2;
  LOGPROB prob;

  /* index is RL */
  /* prob is in additional N-gram area */
  if ((n2 = search_bigram(ndata, w2, w1)) != NNID_INVALID) {
    prob = ndata->p_2[n2];
  } else {
    prob = ndata->bo_wt_1[w1] + ndata->d[0].prob[w2];
  }
  if (w2 != ndata->unk_id) {
    return(prob);
  } else {
    return(prob - ndata->unk_num_log);
  }
}


/** 
 * Get LR bi-gram prob: for RL N-gram with no LR 2-gram.
 * This function will compute the LR 2-gram from the RL 2-gram.
 * 
 * @param ndata [in] N-gram data that holds the 2-gram
 * @param w1 [in] left context word
 * @param w2 [in] right target word
 * 
 * @return the log N-gram probability P(w2|w1)
 * 
 */
static LOGPROB
bi_prob_compute(NGRAM_INFO *ndata, WORD_ID w1, WORD_ID w2)
{
  NNID n2;
  LOGPROB prob;

  /* index is RL */
  /* no additional N-gram, compute it directly */
  /* get p(w1|w2) */
  if ((n2 = search_bigram(ndata, w2, w1)) != NNID_INVALID) {
    prob = ndata->d[1].prob[n2];
  } else {
    prob = ndata->d[0].bo_wt[w2] + ndata->d[0].prob[w1];
  }
  /* p(w2|w1) = p(w1|w2) * p(w2) / p(w1) */
  prob = prob + ndata->d[0].prob[w2] - ndata->d[0].prob[w1];
  if (w2 != ndata->unk_id) {
    return(prob);
  } else {
    return(prob - ndata->unk_num_log);
  }
}


/** 
 * Get 2-gram probability
 * This function is not used in Julius, since each function of bi_prob_*
 * will be called directly from the search.
 * 
 * @param ndata [in] N-gram data that holds the 2-gram
 * @param w1 [in] left context word
 * @param w2 [in] right target word
 * 
 * @return the log N-gram probability P(w2|w1)
 * 
 */
LOGPROB
bi_prob(NGRAM_INFO *ndata, WORD_ID w1, WORD_ID w2)
{
  LOGPROB p;
  if (ndata->bigram_index_reversed) {
    /* old binary format */
    /* RL 3-gram with additional LR 2-gram, index by LR */
    /* indexes are LR (swap not needed), probs are in additional area */
    p = bi_prob_additional_oldbin(ndata, w1, w2);
  } else if (ndata->dir == DIR_LR) {
    /* LR 3-gram, index by LR */
    p = bi_prob_normal(ndata, w1, w2);
  } else if (ndata->bo_wt_1 != NULL) {
    /* RL 3-gram with additional LR 2-gram, index by RL */
    p = bi_prob_additional(ndata, w1, w2);
  } else {
    /* RL 3-gram only, index by RL */
    p = bi_prob_compute(ndata, w1, w2);
  }
  return p;
}

/** 
 * Determinte which bi-gram computation function to be used according to
 * the N-gram type, and set pointer to the proper function into the
 * N-gram data.
 * 
 * @param ndata [i/o] N-gram information to use
 * 
 */    
void
bi_prob_func_set(NGRAM_INFO *ndata)
{
  if (ndata->bigram_index_reversed) {
    /* old binary format */
    /* RL 3-gram with additional LR 2-gram, index by LR */
    /* indexes are LR (swap not needed), probs are in additional area */
    ndata->bigram_prob = bi_prob_additional_oldbin;
  } else if (ndata->dir == DIR_LR) {
    /* LR 3-gram, index by LR */
    ndata->bigram_prob = bi_prob_normal;
  } else if (ndata->bo_wt_1 != NULL) {
    /* RL 3-gram with additional LR 2-gram, index by RL */
    ndata->bigram_prob = bi_prob_additional;
  } else {
    /* RL 3-gram only, index by RL */
    ndata->bigram_prob = bi_prob_compute;
  }
}
