/**
 * @file   confnet.c
 * 
 * <JA>
 * @brief  Confusion network の生成
 *
 * 認識の結果得られた単語グラフから，confusion network を生成する. 
 * </JA>
 * 
 * <EN>
 * @brief  Confusion network generation
 *
 * Generate confusion network from the obtained word lattice.
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Thu Aug 16 00:15:51 2007
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

#include <julius/julius.h>

/**
 * Define to enable debug output.
 * 
 */
#undef CDEBUG

/**
 * Define to enable further debug output.
 * 
 */
#undef CDEBUG2

/**
 * Use graph-based CM for confusion network generation.  If not
 * defined search-based CM (default of old julius) will be used.
 * However, the clustering process does not work properly with this
 * definition, since sum of the search- based CM for a word set on the
 * same position is not always 1.0.  Thus you'd better always define
 * this.
 * 
 */
#define PREFER_GRAPH_CM

/**
 * Julius identify the words by their dictionary IDs, so words with
 * different entries are treated as a different word.  If this is
 * defined, Julius treat words with the same output string as same
 * words and bundle them in confusion network generation.
 * 
 */
#define BUNDLE_WORD_WITH_SAME_OUTPUT


/** 
 * Determine whether the two words are idential in confusion network
 * generation.
 * 
 * @param w1 [in] first word
 * @param w2 [in] second word
 * @param winfo [in] word dictionary
 * 
 * @return TRUE if they are idential, FALSE if not.
 */
static boolean
is_same_word(WORD_ID w1, WORD_ID w2, WORD_INFO *winfo)
{
  if (w1 == w2
#ifdef BUNDLE_WORD_WITH_SAME_OUTPUT
      || strmatch(winfo->woutput[w1], winfo->woutput[w2])
#endif
      ) return TRUE;
  return FALSE;
}

/**************************************************************/

/**
 * Macro to access the order matrix.
 * 
 */
#define m2i(A, B) (B) * r->order_matrix_count + (A)

/**
 * Judge order between two words by their word graph ID.
 * 
 * @param i [in] id of left graph word
 * @param j [in] id of right graph word
 * 
 * @return TRUE if they are ordered, or FALSE if not.
 */
static boolean
graph_ordered(RecogProcess *r, int i, int j) 
{
  if (i != j  && r->order_matrix[m2i(i,j)] == 0 && r->order_matrix[m2i(j,i)] == 0) {
    return FALSE;
  }
  return TRUE;
}  

/** 
 * Scan the order matrix to update it at initial step and after word
 * (set) marging.
 * 
 */
static void
graph_update_order(RecogProcess *r)
{
  int i, j, k;
  boolean changed;
  int count;

  count = r->order_matrix_count;
  
  do {
    changed = FALSE;
    for(i=0;i<count;i++) {
      for(j=0;j<count;j++) {
	if (r->order_matrix[m2i(i, j)] == 1) {
	  for(k=0;k<count;k++) {
	    if (r->order_matrix[m2i(j, k)] == 1) {
	      if (r->order_matrix[m2i(i, k)] == 0) {
		r->order_matrix[m2i(i, k)] = 1;
		changed = TRUE;
	      }
	    }
	  }
	}
      }
    }
  } while (changed == TRUE);
}

/** 
 * Extract order relationship between any two words in the word graph
 * for confusion network generation.
 * 
 * @param root [in] root pointer to the word graph
 * @param r [in] recognition process instance
 *
 * @callgraph
 * @callergraph
 */
void
graph_make_order(WordGraph *root, RecogProcess *r)
{
  int count;
  WordGraph *wg, *right;
  int i;
  
  /* make sure total num and id are valid */
  count = 0;
  for(wg=root;wg;wg=wg->next) count++;
  if (count == 0) {
    r->order_matrix = NULL;
    return;
  }
  if (count != r->graph_totalwordnum) {
    jlog("Error: graph_make_order: r->graph_totalwordnum differ from actual number?\n");
    r->order_matrix = NULL;
    return;
  }
  r->order_matrix_count = count;
  for(wg=root;wg;wg=wg->next) {
    if (wg->id >= count) {
      jlog("Error: graph_make_order: wordgraph id >= count (%d >= %d)\n", wg->id, count);
      r->order_matrix = NULL;
      return;
    }
  }

  /* allocate and clear matrix */
  r->order_matrix = (char *)mymalloc(count * count);
  for(i=0;i<count*count;i++) r->order_matrix[i] = 0;
  
  /* set initial order info */
  for(wg=root;wg;wg=wg->next) {
    for(i=0;i<wg->rightwordnum;i++) {
      right = wg->rightword[i];
      r->order_matrix[m2i(wg->id, right->id)] = 1;
    }
  }

  /* right propagate loop */
  graph_update_order(r);
}

/**
 * Free the order relation data.
 * 
 * @callgraph
 * @callergraph
 */
void
graph_free_order(RecogProcess *r)
{
  if (r->order_matrix) {
    free(r->order_matrix);
    r->order_matrix = NULL;
  }
}

/**************************************************************/

/**
 * Create a new cluster holder.
 * 
 * @return the newly allocated cluster holder.
 */
static CN_CLUSTER *
cn_new()
{
  CN_CLUSTER *new;
  new = (CN_CLUSTER *)mymalloc(sizeof(CN_CLUSTER));
  new->wg = (WordGraph **)mymalloc(sizeof(WordGraph *) * CN_CLUSTER_WG_STEP);
  new->wgnum_alloc = CN_CLUSTER_WG_STEP;
  new->wgnum = 0;
  new->words = NULL;
  new->pp = NULL;
  new->next = NULL;
  return new;
}

/**
 * Free a cluster holder
 * 
 * @param c [out] a cluster holder to be released.
 * </EN>
 */
static void
cn_free(CN_CLUSTER *c)
{
  free(c->wg);
  if (c->words) free(c->words);
  if (c->pp) free(c->pp);
  free(c);
}

/** 
 * Free all cluster holders.
 * 
 * @param croot [out] pointer to root pointer of cluster holder list.
 *
 * @callgraph
 * @callergraph
 * 
 */
void
cn_free_all(CN_CLUSTER **croot)
{
  CN_CLUSTER *c, *ctmp;
  c = *croot;
  while(c) {
    ctmp = c->next;
    cn_free(c);
    c = ctmp;
  }
  *croot = NULL;
}

/** 
 * Add a graph word to a cluster holder.
 * 
 * @param c [out] cluster holder
 * @param wg [in] graph word to be added
 */
static void
cn_add_wg(CN_CLUSTER *c, WordGraph *wg)
{
  if (c->wgnum >= c->wgnum_alloc) {
    c->wgnum_alloc += CN_CLUSTER_WG_STEP;
    c->wg = (WordGraph **)myrealloc(c->wg, sizeof(WordGraph *) * c->wgnum_alloc);
  }
  c->wg[c->wgnum] = wg;
  c->wgnum++;
}

/** 
 * Merge a cluster holder into another.
 * 
 * @param dst [i/o] target cluster holder
 * @param src [in] source cluster holder.
 */
static void
cn_merge(RecogProcess *r, CN_CLUSTER *dst, CN_CLUSTER *src)
{
  WordGraph *wg;
  int i, j, n;

  /* update order matrix */
  for(i=0;i<src->wgnum;i++) {
    wg = src->wg[i];
    for(j=0;j<dst->wgnum;j++) {
      for(n=0;n<wg->leftwordnum;n++) {
	r->order_matrix[m2i(wg->leftword[n]->id, dst->wg[j]->id)] = 1;
      }
      for(n=0;n<wg->rightwordnum;n++) {
	r->order_matrix[m2i(dst->wg[j]->id, wg->rightword[n]->id)] = 1;
      }
    }
  }
  graph_update_order(r);
  /* add words in the source cluster to target cluster */
  for(i=0;i<src->wgnum;i++) {
    cn_add_wg(dst, src->wg[i]);
  }
}

/** 
 * Erase a cluster holder and remove it from the list.
 * 
 * @param target [i/o] a cluster holder to be erased
 * @param root [i/o] pointer to root pointer of cluster holder list
 */
static void
cn_destroy(CN_CLUSTER *target, CN_CLUSTER **root)
{
  CN_CLUSTER *c, *cprev;

  cprev = NULL;
  for(c = *root; c; c = c->next) {
    if (c == target) {
      if (cprev) {
	cprev->next = c->next;
      } else {
	*root = c->next;
      }
      cn_free(c);
      break;
    }
    cprev = c;
  }
}

/** 
 * Build / update word list from graph words for a cluster holder.
 * 
 * @param c [i/o] cluster holder to process
 * @param winfo [in] word dictionary 
 */
static void
cn_build_wordlist(CN_CLUSTER *c, WORD_INFO *winfo)
{
  int i, j;

  if (c->words) {
    free(c->words);
  }
  c->words = (WORD_ID *)mymalloc(sizeof(WORD_ID) * (c->wgnum + 1));
  c->wordsnum = 0;
  for(i=0;i<c->wgnum;i++) {
    for(j=0;j<c->wordsnum;j++) {
      if (is_same_word(c->words[j], c->wg[i]->wid, winfo)) break;
    }
    if (j>=c->wordsnum) {
      c->words[c->wordsnum] = c->wg[i]->wid;
      c->wordsnum++;
    }
  }
}

/** 
 * qsort_reentrant callback to sort clusters by their time order.
 * 
 * @param x [in] element 1
 * @param y [in] element 2
 * @param r [in] recognition process instance
 * 
 * @return order value
 */
static int
compare_cluster(CN_CLUSTER **x, CN_CLUSTER **y, RecogProcess *r)
{
  //int i, min1, min2;
/* 
 * 
 *   for(i=0;i<(*x)->wgnum;i++) {
 *     if (i == 0 || min1 > (*x)->wg[i]->lefttime) min1 = (*x)->wg[i]->lefttime;
 *   }
 *   for(i=0;i<(*y)->wgnum;i++) {
 *     if (i == 0 || min2 > (*y)->wg[i]->lefttime) min2 = (*y)->wg[i]->lefttime;
 *   }
 *   if (min1 < min2) return -1;
 *   else if (min1 > min2) return 1;
 *   else return 0;
 */
  int i, j;

  if (x == y) return 0;
  for(i=0;i<(*x)->wgnum;i++) {
    for(j=0;j<(*y)->wgnum;j++) {
      //if (graph_ordered((*x)->wg[i]->id, (*y)->wg[j]->id)) dir = 1;
      if (r->order_matrix[m2i((*x)->wg[i]->id, (*y)->wg[j]->id)] == 1) {
	return -1;
      }
    }
  }
  return 1;
}


/** 
 * Compute intra-word similarity of two graph words for confusion network
 * generation.
 * 
 * @param w1 [in] graph word 1
 * @param w2 [in] graph word 2
 * 
 * @return the similarity value.
 */
static PROB
get_intraword_similarity(WordGraph *w1, WordGraph *w2)
{
  PROB overlap;
  int overlap_frame, sum_len;
  PROB sim;

  /* compute overlap_frame */
  if (w1->lefttime < w2->lefttime) {
    if (w1->righttime < w2->lefttime) {
      overlap_frame = 0;
    } else if (w1->righttime > w2->righttime) {
      overlap_frame = w2->righttime - w2->lefttime + 1;
    } else {
      overlap_frame = w1->righttime - w2->lefttime + 1;
    }
  } else if (w1->lefttime > w2->righttime) {
    overlap_frame = 0;
  } else {
    if (w1->righttime > w2->righttime) {
      overlap_frame = w2->righttime - w1->lefttime + 1;
    } else {
      overlap_frame = w1->righttime - w1->lefttime + 1;
    }
  }
  sum_len = (w1->righttime - w1->lefttime + 1) + (w2->righttime - w2->lefttime + 1);
  overlap = (PROB)overlap_frame / (PROB)sum_len;
#ifdef CDEBUG2
  printf("[%d..%d] [%d..%d]  overlap = %d / %d = %f",
	 w1->lefttime, w1->righttime, w2->lefttime, w2->righttime,
	 overlap_frame, sum_len, overlap);
#endif

#ifdef PREFER_GRAPH_CM
#ifdef CDEBUG2
  printf("  cm=%f, %f", w1->graph_cm, w2->graph_cm);
#endif
  sim = overlap * w1->graph_cm * w2->graph_cm;
#else 
#ifdef CDEBUG2
  printf("  cm=%f, %f", w1->cmscore, w2->cmscore);
#endif
  sim = overlap * w1->cmscore * w2->cmscore;
#endif

#ifdef CDEBUG2
  printf("  similarity=%f\n", sim);
#endif

  return sim;
}

/** 
 * Compute intra-word similarity of two clusters.
 * 
 * @param c1 [in] cluster 1
 * @param c2 [in] cluster 2
 * @param winfo [in] word dictionary
 * 
 * @return the maximum similarity.
 */
static PROB
get_cluster_intraword_similarity(CN_CLUSTER *c1, CN_CLUSTER *c2, WORD_INFO *winfo)
{
  int i1, i2;
  PROB simmax, sim;

  simmax = 0.0;
  for(i1 = 0; i1 < c1->wgnum; i1++) {
    for(i2 = 0; i2 < c2->wgnum; i2++) {
      if (is_same_word(c1->wg[i1]->wid, c2->wg[i2]->wid, winfo)) {
	//if (graph_ordered(c1->wg[i1]->id, c2->wg[i2]->id)) continue;
	sim = get_intraword_similarity(c1->wg[i1], c2->wg[i2]);
	if (simmax < sim) simmax = sim;
      }
    }
  }
  return(simmax);
}

#ifdef CDEBUG
/** 
 * Output a cluster information.
 *
 * @param fp [in] file pointer to output
 * @param c [in] cluster to output
 * @param winfo [in] word dictionary
 */
static void
put_cluster(FILE *fp, CN_CLUSTER *c, WORD_INFO *winfo)
{
  int i;

  for(i=0;i<c->wgnum;i++) {
    fprintf(fp, "[%d:%s:%d..%d]", c->wg[i]->id, winfo->woutput[c->wg[i]->wid], c->wg[i]->lefttime, c->wg[i]->righttime);
  }
  printf("\n");
}
#endif

/** 
 * Return minimum value of the three arguments.
 * 
 * @param a [in] value 1
 * @param b [in] value 2
 * @param c [in] value 3
 * 
 * @return the minumum value.
 */
static int
minimum(int a, int b, int c)
{
  int min;

  min = a;
  if (b < min)
    min = b;
  if (c < min)
    min = c;
  return min;
}

/** 
 * Calculate Levenstein distance (edit distance) of two words.
 * 
 * @param w1 [in] word ID 1
 * @param w2 [in] word ID 2
 * @param winfo [in] word dictionary
 * 
 * @return the distance.
 */
static int
edit_distance(WORD_ID w1, WORD_ID w2, WORD_INFO *winfo, char *b1, char *b2)
{
  int i1, i2;
  int *d;
  int len1, len2;
  int j;
  int cost;
  int distance;

  len1 = winfo->wlen[w1] + 1;
  len2 = winfo->wlen[w2] + 1;
  d = (int *)mymalloc(sizeof(int) * len1 * len2);
  for(j=0;j<len1;j++) d[j] = j;
  for(j=0;j<len2;j++) d[j*len1] = j;

  for(i1=1;i1<len1;i1++) {
    center_name(winfo->wseq[w1][i1-1]->name, b1);
    for(i2=1;i2<len2;i2++) {
      center_name(winfo->wseq[w2][i2-1]->name, b2);
      if (strmatch(b1, b2)) {
	cost = 0;
      } else {
	cost = 1;
      }
      d[i2 * len1 + i1] = minimum(d[(i2-1) * len1 + i1] + 1, d[i2 * len1 + (i1-1)] + 1, d[(i2-1) * len1 + (i1-1)] + cost);
    }
  }

  distance = d[len1 * len2 - 1];

  free(d);
  return(distance);
}

/** 
 * Compute inter-word similarity of two clusters.
 * 
 * @param c1 [in] cluster 1
 * @param c2 [in] cluster 2
 * @param winfo [in] word dictionary
 * 
 * @return the average similarity.
 */
static PROB
get_cluster_interword_similarity(RecogProcess *r, CN_CLUSTER *c1, CN_CLUSTER *c2, WORD_INFO *winfo, char *buf1, char *buf2)
{
  int i1, i2, j;
  WORD_ID w1, w2;
  PROB p1, p2;
  PROB sim, simsum;
  int simsum_count;
  int dist;

  /* order check */
  for(i1 = 0; i1 < c1->wgnum; i1++) {
    for(i2 = 0; i2 < c2->wgnum; i2++) {
      if (graph_ordered(r, c1->wg[i1]->id, c2->wg[i2]->id)) {
	/* ordered clusters should not be merged */
	//printf("Ordered:\n");
	//printf("c1:\n"); put_cluster(stdout, c1, winfo);
	//printf("c2:\n"); put_cluster(stdout, c2, winfo);
	return 0.0;
      }
    }
  }

#ifdef CDEBUG2
  printf("-----\n");
  printf("c1:\n"); put_cluster(stdout, c1, winfo);
  printf("c2:\n"); put_cluster(stdout, c2, winfo);
#endif

  /* compute similarity */
  simsum = 0.0;
  simsum_count = 0;
  for(i1 = 0; i1 < c1->wordsnum; i1++) {
    w1 = c1->words[i1];
    p1 = 0.0;
    for(j = 0; j < c1->wgnum; j++) {
      if (is_same_word(c1->wg[j]->wid, w1, winfo)) {
#ifdef PREFER_GRAPH_CM
	p1 += c1->wg[j]->graph_cm;
#else
	p1 += c1->wg[j]->cmscore;
#endif
      }
    }
    for(i2 = 0; i2 < c2->wordsnum; i2++) {
      w2 = c2->words[i2];
      p2 = 0.0;
      for(j = 0; j < c2->wgnum; j++) {
	if (is_same_word(c2->wg[j]->wid, w2, winfo)) {
#ifdef PREFER_GRAPH_CM
	  p2 += c2->wg[j]->graph_cm;
#else
	  p2 += c2->wg[j]->cmscore;
#endif
	}
      }
      dist = edit_distance(w1, w2, winfo, buf1, buf2);
#ifdef CDEBUG2
      for(j=0;j<winfo->wlen[w1];j++) {
	printf("%s ", winfo->wseq[w1][j]->name);
      }
      printf("\n");
      for(j=0;j<winfo->wlen[w2];j++) {
	printf("%s ", winfo->wseq[w2][j]->name);
      }
      printf("\n");
      printf("distance=%d\n", dist);
#endif

      sim = 1.0 - (float)dist / (float)(winfo->wlen[w1] + winfo->wlen[w2]);

#ifdef CDEBUG2
      printf("(%s) - (%s): sim = %f, p1 = %f, p2 = %f\n", winfo->woutput[w1], winfo->woutput[w2], sim, p1, p2);
#endif

      simsum += sim * p1 * p2;
      simsum_count++;
    }
  }

#ifdef CDEBUG2
  printf("SIM=%f\n", simsum / simsum_count);
  printf("-----\n");
#endif

  return(simsum / simsum_count);
}


/** 
 * @brief  Create a confusion network from word graph.
 *
 * @param root [in] root pointer of word graph
 * @param r [in] recognition process instance
 *
 * @return root pointer to the cluster list.
 *
 * @callgraph
 * @callergraph
 * 
 */
CN_CLUSTER *
confnet_create(WordGraph *root, RecogProcess *r)
{
  CN_CLUSTER *croot;
  CN_CLUSTER *c, *cc, *cmax1, *cmax2;
  WordGraph *wg;
  PROB sim, max_sim;
  int wg_totalnum, n, i;
  char *buf1, *buf2;

  buf1 = (char *)mymalloc(MAX_HMMNAME_LEN);
  buf2 = (char *)mymalloc(MAX_HMMNAME_LEN);

  /* make initial confnet instances from word graph */
  croot = NULL;
  wg_totalnum = 0;
  for(wg=root;wg;wg=wg->next) {
    c = cn_new();
    cn_add_wg(c, wg);
    c->next = croot;
    croot = c;
    wg_totalnum++;
  }

  /* intraword clustering iteration */
  do {
    /* find most similar pair */
    max_sim = 0.0;
    for(c=croot;c;c=c->next) {
      for(cc=c->next;cc;cc=cc->next) {
	sim = get_cluster_intraword_similarity(c, cc, r->lm->winfo);
	if (max_sim < sim) {
	  max_sim = sim;
	  cmax1 = c;
	  cmax2 = cc;
	}
      }
    }
    /* merge the maximum one if exist */
    if (max_sim != 0.0) {
#ifdef CDEBUG
      printf(">>> max_sim = %f\n", max_sim);
      put_cluster(stdout, cmax1, r->lm->winfo);
      put_cluster(stdout, cmax2, r->lm->winfo);
#endif
      cn_merge(r, cmax1, cmax2);
      cn_destroy(cmax2, &croot);
    }
  } while (max_sim != 0.0); /* loop until no more similar pair exists */

  n = 0;
  for(c=croot;c;c=c->next) n++;
  if (verbose_flag) jlog("STAT: confnet: %d words -> %d clusters by intra-word clustering\n", wg_totalnum, n);

#ifdef CDEBUG
  printf("---- result of intra-word clustering ---\n");
  i = 0;
  for(c=croot;c;c=c->next) {
    printf("%d :", i);
    put_cluster(stdout, c, r->lm->winfo);
#ifdef CDEBUG2
    for(i=0;i<c->wgnum;i++) {
      printf("    ");
      put_wordgraph(stdout, c->wg[i], r->lm->winfo);
    }
#endif
    i++;
  }
  printf("----------------------------\n");
#endif

  /* inter-word clustering */
  do {
    /* build word list for each cluster */
    for(c=croot;c;c=c->next) cn_build_wordlist(c, r->lm->winfo);
    /* find most similar pair */
    max_sim = 0.0;
    for(c=croot;c;c=c->next) {
      for(cc=c->next;cc;cc=cc->next) {
	sim = get_cluster_interword_similarity(r, c, cc, r->lm->winfo, buf1, buf2);
	if (max_sim < sim) {
	  max_sim = sim;
	  cmax1 = c;
	  cmax2 = cc;
	}
      }
    }
    /* merge the maximum one if exist */
    if (max_sim != 0.0) {
#ifdef CDEBUG
      printf(">>> max_sim = %f\n", max_sim);
      put_cluster(stdout, cmax1, r->lm->winfo);
      put_cluster(stdout, cmax2, r->lm->winfo);
#endif
      cn_merge(r, cmax1, cmax2);
      cn_destroy(cmax2, &croot);
    }
  } while (max_sim != 0.0); /* loop until no more similar pair exists */

  n = 0;
  for(c=croot;c;c=c->next) n++;
  if (verbose_flag) jlog("STAT: confnet: -> %d clusters by inter-word clustering\n", n);

  /* compute posterior probabilities and insert NULL entry */
  {
    PROB p, psum;
    int j;

    for(c=croot;c;c=c->next) {
      psum = 0.0;
      c->pp = (LOGPROB *)mymalloc(sizeof(LOGPROB) * (c->wordsnum + 1));
      for(i=0;i<c->wordsnum;i++) {
	p = 0.0;
	for(j = 0; j < c->wgnum; j++) {
	  if (is_same_word(c->wg[j]->wid, c->words[i], r->lm->winfo)) {
#ifdef PREFER_GRAPH_CM
	    p += c->wg[j]->graph_cm;
#else
	    p += c->wg[j]->cmscore;
#endif
	  }
	}
	c->pp[i] = p;
	psum += p;
      }
      if (psum < 1.0) {
	c->words[c->wordsnum] = WORD_INVALID;
	c->pp[c->wordsnum] = 1.0 - psum;
	c->wordsnum++;
      }
    }
  }

  /* sort the words in each cluster by their posterior probabilities */
  {
    int j;
    WORD_ID wtmp;
    LOGPROB ltmp;
    for(c=croot;c;c=c->next) {
      for(i=0;i<c->wordsnum;i++) {
	for(j=c->wordsnum - 1;j>i;j--) {
	  if (c->pp[j-1] < c->pp[j]) {
	    ltmp = c->pp[j-1];
	    c->pp[j-1] = c->pp[j];
	    c->pp[j] = ltmp;
	    wtmp = c->words[j-1];
	    c->words[j-1] = c->words[j];
	    c->words[j] = wtmp;
	  }
	}
      }
    }
  }

  /* re-order clusters by their beginning frames */
  {
    CN_CLUSTER **clist;
    int k;

    /* sort cluster list by the left frame*/
    clist = (CN_CLUSTER **)mymalloc(sizeof(CN_CLUSTER *) * n);
    for(i=0,c=croot;c;c=c->next) {
      clist[i++] = c;
    }
    qsort_reentrant(clist, n, sizeof(CN_CLUSTER *), (int (*)(const void *, const void *, void *))compare_cluster, r);
    croot = NULL;
    for(k=0;k<n;k++) {
      if (k == 0) croot = clist[k];
      if (k == n - 1) clist[k]->next = NULL;
      else clist[k]->next = clist[k+1];
    }
    free(clist);
  }

#if 0
  /* output */
  printf("---- begin confusion network ---\n");
  for(c=croot;c;c=c->next) {
    for(i=0;i<c->wordsnum;i++) {
      printf("(%s:%.3f)", (c->words[i] == WORD_INVALID) ? "-" : r->lm->winfo->woutput[c->words[i]], c->pp[i]);
      if (i == 0) printf("  ");
    }
    printf("\n");
  }
  printf("---- end confusion network ---\n");
#endif

  free(buf2);
  free(buf1);

  return(croot);
}

/* end of file */
