/**
 * @file   ngram_read_arpa.c
 * 
 * <JA>
 * @brief  ARPA形式のN-gramファイルを読み込む
 *
 * ARPA形式のN-gramファイルを用いる場合，2-gram と逆向き 3-gram を
 * それぞれ別々のファイルから読み込みます．
 * </JA>
 * 
 * <EN>
 * @brief  Read ARPA format N-gram files
 *
 * When N-gram data is given in ARPA format, both 2-gram file and
 * reverse 3-gram file should be specified.
 * </EN>
 *
 * @sa ngram2.h
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 16:52:24 2005
 *
 * $Revision: 1.21 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* $Id: ngram_read_arpa.c,v 1.21 2013/06/20 17:14:24 sumomo Exp $ */

/* words should be alphabetically sorted */

#include <sent/stddefs.h>
#include <sent/ngram2.h>

static char buf[800];			///< Local buffer for reading
static char pbuf[800];			///< Local buffer for error string 


/** 
 * Set number of N-gram entries, for reading the first LR 2-gram.
 * 
 * @param fp [in] file pointer
 * @param numlist [out] set the values to this buffer (malloc)
 *
 * @return the value of N, or -1 on error.
 */
static int
get_total_info(FILE *fp, NNID **numlist)
{
  char *p;
  int n;
  int maxn;
  unsigned long entry_num;
  int numnum;

  maxn = 0;

  numnum = 10;
  *numlist = (NNID *)mymalloc(sizeof(NNID) * numnum);

  while (getl(buf, sizeof(buf), fp) != NULL && buf[0] != '\\') {
    if (strnmatch(buf, "ngram", 5)) { /* n-gram num */
      //p = strtok(buf, " =");
      //n = atoi(p);
      //p = strtok(NULL, " =");
      //entry_num = atol(p);
      //sscanf(p, "%lu", &entry_num);
      sscanf(buf, "ngram %d = %lu", &n, &entry_num);
      /* check maximum number */
      if (entry_num > NNID_MAX) {
	jlog("Error: too big %d-gram (exceeds %d bit)\n", n, sizeof(NNID) * 8);
	return -1;
      }
      /* ignore empty entry */
      if (entry_num == 0) {
	jlog("Warning: empty %d-gram, skipped\n", n);
      } else {
	if (maxn < n) maxn = n;
	if (n >= numnum) {
	  numnum *= 2;
	  *numlist = (NNID *)myrealloc(*numlist, sizeof(NNID) * numnum);
	}
	(*numlist)[n-1] = entry_num;
      }
    }
  }

  return(maxn);
}

/** 
 * Read word/class entry names and 1-gram data from LR 2-gram file.
 * 
 * @param fp [in] file pointer
 * @param ndata [out] N-gram to set the read data.
 */
static boolean
set_unigram(FILE *fp, NGRAM_INFO *ndata)
{
  WORD_ID nid;
  int resid;
  LOGPROB prob, bo_wt;
  char *name, *p;
  boolean ok_p = TRUE;
  NGRAM_TUPLE_INFO *t;

  t = &(ndata->d[0]);

  /* malloc name area */
  ndata->wname = (char **)mymalloc(sizeof(char *) * ndata->max_word_num);
  for (nid = 0; nid < ndata->max_word_num; nid++) {
    ndata->wname[nid] = NULL;
  }

  /* malloc data area */
  //t->bgn_upper = t->bgn_lower = t->bgn = t->num = NULL;
  t->bgn_upper = NULL;
  t->bgn_lower = NULL;
  t->bgn = NULL;
  t->num = NULL;
  t->bgnlistlen = 0;
  t->nnid2wid = NULL;
  t->prob = (LOGPROB *)mymalloc_big(sizeof(LOGPROB), t->totalnum);
  t->bo_wt = (LOGPROB *)mymalloc_big(sizeof(LOGPROB), t->totalnum);
  t->context_num = t->totalnum;
  t->nnid2ctid_upper = NULL;
  t->nnid2ctid_lower = NULL;

  nid = 0;
  
  while (getl(buf, sizeof(buf), fp) != NULL && buf[0] != '\\') {
    if ((p = strtok(buf, DELM)) == NULL) {
      jlog("Error: ngram_read_arpa: 1-gram: failed to parse, corrupted or invalid data?\n");
      return FALSE;
    }
    prob = (LOGPROB)atof(p);
    if ((p = strtok(NULL, DELM)) == NULL) {
      jlog("Error: ngram_read_arpa: 1-gram: failed to parse, corrupted or invalid data?\n");
      return FALSE;
    }
    name = strcpy((char *)mymalloc(strlen(p)+1), p);
    if ((p = strtok(NULL, DELM)) == NULL) {
      bo_wt = 0.0;
    } else {
      bo_wt = (LOGPROB)atof(p);
    }

    /* register word entry name */
    ndata->wname[nid] = name;

    /* add entry name to index tree */
    if (ndata->root == NULL) {
      ndata->root = ptree_make_root_node(nid, &(ndata->mroot));
    } else {
      resid = ptree_search_data(name, ndata->root);
      if (resid != -1 && strmatch(name, ndata->wname[resid])) { /* already exist */
	jlog("Error: ngram_read_arpa: duplicate word entry \"%s\" at #%d and #%d in 1-gram\n", name, resid, nid);
	ok_p = FALSE;
	continue;
      } else {
	ptree_add_entry(name, nid, ndata->wname[resid], &(ndata->root), &(ndata->mroot));
      }
    }

    if (nid >= ndata->max_word_num) {
      jlog("Error: ngram_read_arpa: num of 1-gram is bigger than header value (%d)\n", ndata->max_word_num);
      return FALSE;
    }

    /* register entry info */
    t->prob[nid] = prob;
    t->bo_wt[nid] = bo_wt;
  
    nid++;
  }

  if (nid != t->totalnum) {
    jlog("Error: ngram_read_arpa: num of 1-gram (%d) not equal to header value (%d)\n", nid, t->totalnum);
    return FALSE;
  }

  if (ok_p == TRUE) {
    jlog("Stat: ngram_read_arpa: read %d 1-gram entries\n", nid);
  }
  
  return ok_p;
}

/* read-in 1-gram (RL) --- only add back-off weight */
/** 
 * Read 1-gram data from RL 3-gram file.  Only the back-off weights are
 * stored.
 * 
 * @param fp [in] file pointer
 * @param ndata [out] N-gram to store the read data.
 */
static boolean
add_unigram(FILE *fp, NGRAM_INFO *ndata)
{
  WORD_ID read_word_num;
  WORD_ID nid;
  LOGPROB prob, bo_wt;
  char *name, *p;
  boolean ok_p = TRUE;
  boolean mismatched = FALSE;

  ndata->bo_wt_1 = (LOGPROB *)mymalloc_big(sizeof(LOGPROB), ndata->max_word_num);

  read_word_num = 0;
  while (getl(buf, sizeof(buf), fp) != NULL && buf[0] != '\\') {
    if ((p = strtok(buf, DELM)) == NULL) {
      jlog("Error: ngram_read_arpa: RL 1-gram: failed to parse, corrupted or invalid data?\n");
      return FALSE;
    }
    prob = atof(p);
    if ((p = strtok(NULL, DELM)) == NULL) {
      jlog("Error: ngram_read_arpa: RL 1-gram: failed to parse, corrupted or invalid data?\n");
      return FALSE;
    }
    name = strcpy((char *)mymalloc(strlen(p)+1), p);
    if ((p = strtok(NULL, DELM)) == NULL) {
      bo_wt = 0.0;
    } else {
      bo_wt = (LOGPROB)atof(p);
    }

    /* add bo_wt_rl to existing 1-gram entry */
    nid = ngram_lookup_word(ndata, name);
    if (nid == WORD_INVALID) {
      if (mismatched == FALSE) {
	jlog("Error: ngram_read_arpa: vocabulary mismatch between LR n-gram and RL n-gram\n");
	mismatched = TRUE;
      }
      jlog("Error: ngram_read_arpa: \"%s\" does not appears in LR n-gram\n", name);
      ok_p = FALSE;
    } else {
      ndata->bo_wt_1[nid] = bo_wt;
    }
  
    read_word_num++;
    if (read_word_num > ndata->max_word_num) {
      jlog("Error: ngram_read_arpa: vocabulary size of RL n-gram is bigger than header value (%d)\n", ndata->max_word_num);
      return FALSE;
    }
    free(name);
  }
  if (ok_p == TRUE) {
    jlog("Stat: ngram_read_arpa: read %d 1-gram entries\n", read_word_num);
  }

  return ok_p;
}

/** 
 * Read forward 2-gram data and set the LR 2-gram probabilities to the
 * already loaded RL N-gram.
 * 
 * @param fp [in] file pointer
 * @param ndata [i/o] N-gram to set the read data.
 */
static boolean
add_bigram(FILE *fp, NGRAM_INFO *ndata)
{
  WORD_ID w[2], wtmp;
  LOGPROB prob;
  NNID bi_count = 0;
  NNID n2;
  boolean ok_p = TRUE;
  char *s;

  ndata->p_2 = (LOGPROB *)mymalloc_big(sizeof(LOGPROB), ndata->d[1].totalnum);

  while (getl(buf, sizeof(buf), fp) != NULL && buf[0] != '\\') {
    strcpy(pbuf, buf);
    if ( ++bi_count % 100000 == 0) {
      jlog("Stat: ngram_read_arpa: 2-gram read %lu (%d%%)\n", bi_count, bi_count * 100 / ndata->d[1].totalnum);
    }
    if ((s = strtok(buf, DELM)) == NULL) {
      jlog("Error: ngram_read_arpa: 2-gram: failed to parse, corrupted or invalid data?\n");
      return FALSE;
    }
    prob = (LOGPROB)atof(s);
    if ((s = strtok(NULL, DELM)) == NULL) {
      jlog("Error: ngram_read_arpa: 2-gram: failed to parse, corrupted or invalid data?\n");
      return FALSE;
    }
    w[0] = ngram_lookup_word(ndata, s);
    if (w[0] == WORD_INVALID) {
      jlog("Error: ngram_read_arpa: 2-gram #%lu: \"%s\": \"%s\" not exist in 1-gram\n", bi_count, pbuf, s);
      ok_p = FALSE;
      continue;
    }
    if ((s = strtok(NULL, DELM)) == NULL) {
      jlog("Error: ngram_read_arpa: 2-gram: failed to parse, corrupted or invalid data?\n");
      return FALSE;
    }
    w[1] = ngram_lookup_word(ndata, s);
    if (w[1] == WORD_INVALID) {
      jlog("Error: ngram_read_arpa: 2-gram #%lu: \"%s\": \"%s\" not exist in 1-gram\n", bi_count, pbuf, s);
      ok_p = FALSE;
      continue;
    }
    if (ndata->dir == DIR_RL) {
      /* word order should be reversed */
      wtmp = w[0];
      w[0] = w[1];
      w[1] = wtmp;
    }
    n2 = search_ngram(ndata, 2, w);
    if (n2 == NNID_INVALID) {
      jlog("Warning: ngram_read_arpa: 2-gram #%d: \"%s\": (%s,%s) not exist in LR 2-gram (ignored)\n", n2+1, pbuf, ndata->wname[w[0]], ndata->wname[w[1]]);
    } else {
      ndata->p_2[n2] = prob;
    }
  }

  if (ok_p == TRUE) {
    jlog("Stat: ngram_read_arpa: 2-gram read %lu end\n", bi_count);
  }

  return ok_p;
}
    
/** 
 * Read n-gram data for a given N from ARPA n-gram file. (n >= 2)
 * 
 * @param fp [in] file pointer
 * @param ndata [out] N-gram to set the read data.
 */
static boolean
set_ngram(FILE *fp, NGRAM_INFO *ndata, int n)
{
  NNID i;
  WORD_ID *w;
  WORD_ID *w_last;
  LOGPROB p, bowt;
  NNID nnid;
  NNID cid, cid_last;
  boolean ok_p = TRUE;
  char *s;
  NGRAM_TUPLE_INFO *t;
  NGRAM_TUPLE_INFO *tprev;
  NNID ntmp;

  if (n < 2) {
    jlog("Error: ngram_read_arpa: unable to process 1-gram\n");
    return FALSE;
  }

  w = (WORD_ID *)mymalloc(sizeof(WORD_ID) * n);
  w_last = (WORD_ID *)mymalloc(sizeof(WORD_ID) * n);

  t = &(ndata->d[n-1]);
  tprev = &(ndata->d[n-2]);

  /* initialize pointer storage to access from (N-1)-gram */
  t->bgnlistlen = tprev->context_num;
  if (t->is24bit) {
    t->bgn_upper = (NNID_UPPER *)mymalloc_big(sizeof(NNID_UPPER), t->bgnlistlen);
    t->bgn_lower = (NNID_LOWER *)mymalloc_big(sizeof(NNID_LOWER), t->bgnlistlen);
    for(i = 0; i < t->bgnlistlen; i++) {    
      t->bgn_upper[i] = NNID_INVALID_UPPER;
      t->bgn_lower[i] = 0;
    }
  } else {
    t->bgn = (NNID *)mymalloc_big(sizeof(NNID), t->bgnlistlen);
    for(i = 0;i < t->bgnlistlen; i++) {
      t->bgn[i] = NNID_INVALID;
    }
  }
  t->num = (WORD_ID *)mymalloc_big(sizeof(WORD_ID), t->bgnlistlen);
  for(i = 0; i < t->bgnlistlen; i++) {
    t->num[i] = 0;
  }

  /* allocate data area */
  t->nnid2wid = (WORD_ID *)mymalloc_big(sizeof(WORD_ID), t->totalnum);
  t->prob = (LOGPROB *)mymalloc_big(sizeof(LOGPROB), t->totalnum);
  t->bo_wt = NULL;
  t->nnid2ctid_upper = NULL;
  t->nnid2ctid_lower = NULL;

  nnid = 0;
  cid = cid_last = NNID_INVALID;
  for(i=0;i<n;i++) w_last[i] = WORD_INVALID;

  /* read in N-gram */
  for (;;) {
    if (getl(buf, sizeof(buf), fp) == NULL || buf[0] == '\\') break;
    strcpy(pbuf, buf);
    if ( nnid % 100000 == 0) {
      jlog("Stat: ngram_read_arpa: %d-gram read %d (%d%%)\n", n, nnid, nnid * 100 / t->totalnum);
    }

    /* N-gram probability */
    if ((s = strtok(buf, DELM)) == NULL) {
      jlog("Error: ngram_read_arpa: %d-gram: failed to parse, corrupted or invalid data?\n", n);
      free(w_last); free(w);
      return FALSE;
    }
    p = (LOGPROB)atof(s);
    /* read in context word and lookup the ID */
    for(i=0;i<n;i++) {
      if ((s = strtok(NULL, DELM)) == NULL) {
	jlog("Error: ngram_read_arpa: %d-gram: failed to parse, corrupted or invalid data?\n", n);
	free(w_last); free(w);
	return FALSE;
      }
      if ((w[i] = ngram_lookup_word(ndata, s)) == WORD_INVALID) {
	jlog("Error: ngram_read_arpa: %d-gram #%d: \"%s\": \"%s\" not exist in %d-gram\n", n, nnid+1, pbuf, s, n);
	ok_p = FALSE;
	break;
      }
      /* increment nnid_bgn and nnid_num if context word changed */
    }
    if (i < n) continue;	/* error out */

    /* detect context entry change at this line */
    for(i=0;i<n-1;i++) {
      if (w[i] != w_last[i]) break;
    }
    if (i < n-1) {		/* context changed here */
      /* find new entry point */
      cid = search_ngram(ndata, n-1, w);
      if (cid == NNID_INVALID) {	/* no context */
        //jlog("Warning: ngram_read_arpa: %d-gram #%d: \"%s\": context (%s,%s) not exist in %d-gram (ignored)\n", n, nnid+1, pbuf, ndata->wname[w_m], ndata->wname[w_r], n-1);
        jlog("Warning: ngram_read_arpa: %d-gram #%d: \"%s\": context (",
	     n, nnid+1, pbuf);
	for(i=0;i<n-1;i++) {
	  jlog(" %s", ndata->wname[w[i]]);
	}
        jlog(") not exist in %d-gram (ignored)\n", n-1);
	ok_p = FALSE;
        continue;
      }
      if (cid_last != NNID_INVALID) {
	/* close last entry */
	if (t->is24bit) {
	  ntmp = ((NNID)(t->bgn_upper[cid_last]) << 16) + (NNID)(t->bgn_lower[cid_last]);
	} else {
	  ntmp = t->bgn[cid_last];
	}
	t->num[cid_last] = nnid - ntmp;
      }
      /* the next context word should be an new entry */
      if (t->is24bit) {
	if (t->bgn_upper[cid] != NNID_INVALID_UPPER) {
	  jlog("Error: ngram_read_arpa: %d-gram #%d: \"%s\": word order is not the same as 1-gram\n", n, nnid+1, pbuf);
	  free(w_last); free(w);
	  return FALSE;
	}
	ntmp = nnid & 0xffff;
	t->bgn_lower[cid] = ntmp;
	ntmp = nnid >> 16;
	t->bgn_upper[cid] = ntmp;
      } else {
	if (t->bgn[cid] != NNID_INVALID) {
	  jlog("Error: ngram_read_arpa: %d-gram #%d: \"%s\": word order is not the same as 1-gram\n", n, nnid+1, pbuf);
	  free(w_last); free(w);
	  return FALSE;
	}
	t->bgn[cid] = nnid;
      }

      cid_last = cid;
      w_last[n-1] = WORD_INVALID;
    }

    /* store the probabilities of the target word */
    if (w[n-1] == w_last[n-1]) {
      jlog("Error: ngram_read_arpa: %d-gram #%d: \"%s\": duplicated entry\n", n, nnid+1, pbuf);
      ok_p = FALSE;
      continue;
    } else if (w_last[n-1] != WORD_INVALID && w[n-1] < w_last[n-1]) {
      jlog("Error: ngram_read_arpa: %d-gram #%d: \"%s\": word order is not the same as 1-gram\n", n, nnid+1, pbuf);
      free(w_last); free(w);
      return FALSE;
    }

    /* if the 2-gram has back-off entries, store them here */
    if ((s = strtok(NULL, DELM)) != NULL) {
      bowt = (LOGPROB) atof(s);
      if (t->bo_wt == NULL) {
	t->bo_wt = (LOGPROB *)mymalloc_big(sizeof(LOGPROB), t->totalnum);
	for(i=0;i<nnid;i++) t->bo_wt[i] = 0.0;
      }
      t->bo_wt[nnid] = bowt;
    } else {
      if (t->bo_wt != NULL) t->bo_wt[nnid] = 0.0;
    }

    /* store the entry info */
    t->nnid2wid[nnid] = w[n-1];
    t->prob[nnid] = p;

    nnid++;
    for(i=0;i<n;i++) w_last[i] = w[i];

    /* check total num */
    if (nnid > t->totalnum) {
      jlog("Error: ngram_read_arpa: %d-gram: read num (%d) not match the header value (%d)\n", n, nnid, t->totalnum);
      free(w_last); free(w);
      return FALSE;
    }
  }
  
  /* set the last entry */
  if (t->is24bit) {
    ntmp = ((NNID)(t->bgn_upper[cid_last]) << 16) + (NNID)(t->bgn_lower[cid_last]);
  } else {
    ntmp = t->bgn[cid_last];
  }
  t->num[cid_last] = nnid - ntmp;

  if (t->bo_wt != NULL) t->context_num = t->totalnum;

  if (ok_p == TRUE) {
    jlog("Stat: ngram_read_arpa: %d-gram read %d end\n", n, nnid);
  }

  free(w_last); free(w);
  return ok_p;
}

/** 
 * Read in one ARPA N-gram file.  Supported combinations are
 * LR 2-gram, RL 3-gram and LR 3-gram.
 * 
 * @param fp [in] file pointer
 * @param ndata [out] N-gram data to store the read data
 * @param addition [in] TRUE if going to read additional 2-gram
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
ngram_read_arpa(FILE *fp, NGRAM_INFO *ndata, boolean addition)
{
  int i, n;
  NNID *num;

  /* source file is not a binary N-gram */
  ndata->from_bin = FALSE;
  ndata->bigram_index_reversed = FALSE;

  /* read until `\data\' found */
  while (getl(buf, sizeof(buf), fp) != NULL && strncmp(buf,"\\data\\",6) != 0);


  if (addition) {
    /* reading additional forward 2-gram for the 1st pass */

    if (ndata->n < 2) {
      jlog("Error: base N-gram should be longer than 2-gram\n");
      return FALSE;
    }

    /* read n-gram total info */
    n = get_total_info(fp, &num);
    if (n == -1) {		/* error */
      free(num);
      return FALSE;
    }

    /* check N limit */
    if (n < 2) {
      jlog("Error: forward N-gram for pass1 is does not contain 2-gram\n");
      free(num);
      return FALSE;
    }
    if (n > 2) {
      jlog("Warning: forward N-gram for pass1 contains %d-gram, only 2-gram will be used\n", n);
    }

    /* check if the numbers are the same with already read n-gram */
    for(i=0;i<2;i++) {
      if (ndata->d[i].totalnum != num[i]) {
	jlog("Warning: ngram_read_arpa: %d-gram total num differ between forward N-gram and backward N-gram, may cause some error\n", i+1);
      }
    }

    free(num);

    /* read additional 1-gram data */
    if (!strnmatch(buf,"\\1-grams",8)) {
      jlog("Error: ngram_read_arpa: 1-gram not found for additional LR 2-gram\n");
      return FALSE;
    }
    jlog("Stat: ngram_read_arpa: reading 1-gram part...\n");
    if (add_unigram(fp, ndata) == FALSE) return FALSE;
    /* read 2-gram data */
    if (!strnmatch(buf,"\\2-grams", 8)) {
      jlog("Error: ngram_read_arpa: 2-gram not found for additional LR 2-gram\n");
      return FALSE;
    }
    jlog("Stat: ngram_read_arpa: reading 2-gram part...\n");
    if (add_bigram(fp, ndata) == FALSE) return FALSE;


    /* ignore the rest */
    if (strnmatch(buf,"\\3-grams", 8)) {
      jlog("Warning: forward n-gram contains more than 3-gram, ignored\n");
    }

  } else {
    /* read n-gram total info */
    n = get_total_info(fp, &num);
    if (n == -1) {		/* error */
      free(num);
      return FALSE;
    }
    jlog("Stat: ngram_read_arpa: this is %d-gram file\n", n);
    ndata->d = (NGRAM_TUPLE_INFO *)mymalloc(sizeof(NGRAM_TUPLE_INFO) * n);
    memset(ndata->d, 0, sizeof(NGRAM_TUPLE_INFO) * n);
    for(i=0;i<n;i++) {
      ndata->d[i].totalnum = num[i];
    }
    free(num);
    
    /* set word num */
    if (ndata->d[0].totalnum > MAX_WORD_NUM) {
      jlog("Error: ngram_read_arpa: N-gram vocabulary size exceeds the limit (%d)\n", MAX_WORD_NUM);
      return FALSE;
    }
    ndata->max_word_num = ndata->d[0].totalnum;
    
    /* check if each N-gram allows 24bit and back-off compaction mode */
    /* for fast access, 1-gram and 2-gram always use non-compaction mode */
    for(i=0;i<n;i++) {
      if (i < 2) {		/* not use for 1-gram and 2-gram */
	ndata->d[i].is24bit = FALSE;
      } else {
	/* for 3-gram and later 24 bit mode is preferred,
	   but should be disabled if number of entries is over 2^24 */
	if (ndata->d[i].totalnum > NNID_MAX_24) {
	  jlog("Warning: ngram_read_arpa: num of %d-gram exceeds 24bit, now switch to %dbit index\n", i+1, sizeof(NNID) * 8);
	  ndata->d[i].is24bit = FALSE;
	} else {
	  ndata->d[i].is24bit = TRUE;
	}
      }
    }
    /* disable ct_compaction flag while reading ARPA data */
    for(i=0;i<n;i++) {
      ndata->d[i].ct_compaction = FALSE;
    }
    
    /* read 1-gram data */
    if (!strnmatch(buf,"\\1-grams",8)) {
      jlog("Error: ngram_read_arpa: data format error: 1-gram not found\n");
      return FALSE;
    }
    jlog("Stat: ngram_read_arpa: reading 1-gram part...\n");
    if (set_unigram(fp, ndata) == FALSE) return FALSE;
    
    i = 2;
    while(i <= n) {
      /* read n-gram data in turn */
      sprintf(pbuf, "\\%d-grams", i);
      if (!strnmatch(buf, pbuf, 8)) {
	jlog("Error: ngram_read_arpa: data format error: %d-gram not found\n", i);
	return FALSE;
      }
      jlog("Stat: ngram_read_arpa: reading %d-gram part...\n", i);
      if (set_ngram(fp, ndata, i) == FALSE) return FALSE;
      i++;
    }
    /* finished reading file */
    if (!strnmatch(buf, "\\end", 4)) {
      jlog("Error: ngram_read_arpa: data format error: end marker \"\\end\" not found\n");
      return FALSE;
    }

    ndata->n = n;

    for(i=2;i<n;i++) {
      if (ndata->d[i-1].bo_wt != NULL) {
	/* perform back-off compaction */
	if (ngram_compact_context(ndata, i) == FALSE) return FALSE;
      }
    }
    
    /* swap <s> and </s> for backward SRILM N-gram */
    if (ndata->dir == DIR_RL) {
      WORD_ID bos, eos;
      char *p;
      bos = ngram_lookup_word(ndata, BEGIN_WORD_DEFAULT);
      eos = ngram_lookup_word(ndata, END_WORD_DEFAULT);
      if (!ndata->bos_eos_swap) {
	/* check */
	if (bos != WORD_INVALID && eos != WORD_INVALID && ndata->d[0].prob[bos] == -99) {
	  jlog("Stat: \"P(%s) = -99\" in reverse N-gram, may be trained by SRILM\n", BEGIN_WORD_DEFAULT);
	  jlog("Stat: going to swap \"%s\" and \"%s\"\n", BEGIN_WORD_DEFAULT, END_WORD_DEFAULT);
	  ndata->bos_eos_swap = TRUE;
	}
      }
      if (ndata->bos_eos_swap) {
	if (bos == WORD_INVALID) {
	  jlog("Error: ngram_read_arpa: try to swap bos/eos but \"%s\" not found in N-gram\n", BEGIN_WORD_DEFAULT);
	}
	if (eos == WORD_INVALID) {
	  jlog("Error: ngram_read_arpa: try to swap bos/eos but \"%s\" not found in N-gram\n", END_WORD_DEFAULT);
	}
	if (bos == WORD_INVALID || eos == WORD_INVALID) {
	  return FALSE;
	}
	/* do swap */
	jlog("Stat: ngram_read_arpa: swap \"%s\" and \"%s\" at backward N-gram\n", BEGIN_WORD_DEFAULT, END_WORD_DEFAULT);
	/* swap name buffer */
	p = ndata->wname[bos];
	ndata->wname[bos] = ndata->wname[eos];
	ndata->wname[eos] = p;
	/* replace index */
	ptree_replace_data(BEGIN_WORD_DEFAULT, eos, ndata->root);
	ptree_replace_data(END_WORD_DEFAULT, bos, ndata->root);
      }
    }

  }
    
#ifdef CLASS_NGRAM
  /* skip in-class word entries (they should be in word dictionary) */
  if (getl(buf, sizeof(buf), fp) != NULL) {
    if (strnmatch(buf, "\\class", 6)) {
      jlog("Stat: ngram_read_arpa: skipping in-class word entries...\n");
    }
  }
#endif

  bi_prob_func_set(ndata);

  return TRUE;
}
