/**
 * @file   factoring_sub.c
 * 
 * <JA>
 * @brief  言語スコアのfactoring計算（第1パス）
 *
 * このファイルには，第１パスにおいて言語スコアの factoring を行うための
 * 関数が含まれています. 木構造化辞書上でのサブツリー内の単語リスト
 * (successor list) の構築，および認識中の言語スコア計算ルーチンが
 * 含まれます. 
 *
 * successor list は，木構造化辞書の各ノードに割り付けられる，
 * そのノードを共有する単語のリストです. 木構造化辞書において，
 * 枝部分の次のノードがこのリストを保持します. 実際にはリストが変化する
 * 場所，すなわち木構造化辞書の枝の分岐点に割り付けられます. 
 * 例えば，以下のような木構造化辞書の場合，数字の書いてあるノードに
 * successor list が割り付けられます. 
 * <pre>
 *
 *        2-o-o - o-o-o - o-o-o          word "A" 
 *       /
 *  1-o-o
 *       \       4-o-o                   word "B"
 *        \     /   
 *         3-o-o - 5-o-o - 7-o-o         word "C"
 *              \        \ 
 *               \        8-o-o          word "D"
 *                6-o-o                  word "E"
 * </pre>
 *
 * 各 successor list はそのサブツリーに含まれる単語のリストです. 
 * この例では以下のようになります. 
 *
 * <pre>
 *   node  | successor list (wchmm->state[node].sc)
 *   =======================
 *     1   | A B C D E
 *     2   | A
 *     3   |   B C D E
 *     4   |   B
 *     5   |     C D
 *     6   |         E
 *     7   |     C
 *     8   |       D
 * </pre>
 *
 * ある successor list に含まれる単語が１つになったとき，その時点で
 * 単語が確定する. 上記の場合，単語 "A" はノード 2 の位置ですでに
 * その後続単語として "A" 以外無いので，そこで確定する. 
 * すなわち，単語 A の正確な言語スコアは，単語終端を待たずノード 2 で決まる. 
 *
 * 第１パスにおける factoring の計算は，実際には beam.c で行なわれる. 
 * 2-gram factoringの場合，次ノードに successor list が存在すれば,
 * その successor list の単語の 2-gram の最大値を求め, 伝搬してきている
 * factoring 値を更新する. successor list に単語が1つのノードでは，
 * 正しい2-gramが自動的に割り当てられる. 
 * 1-gram factoringの場合，次ノードに successor list が存在する場合，
 * その successor list の単語の 1-gram の最大値を求め，伝搬してきている
 * factoring 値を更新する. successor list に単語が１つのノードで，はじめて
 * 2-gram を計算する. 
 *
 * 実際では 1-gram factoring では各 successor list における factoring 値
 * は単語履歴に非依存なので，successor list 構築時に全てあらかじめ計算して
 * おく. すなわち，エンジン起動時に木構造化辞書を構築後，successor list
 * を構築したら，単語を2個以上含む successor list についてはその 1-gram の
 * 最大値を計算して，それをそのノードの fscore メンバに格納しておき，その
 * successor list は free してしまえばよい. 単語が１つのみの successor list
 * についてはその単語IDを残しておき，探索時にパスがそこに到達したら
 * 正確な2-gramを計算すれば良い. 
 *
 * DFA文法使用時は，デフォルトでは言語制約(カテゴリ対制約)を
 * カテゴリ単位で木を構築することで静的に表現する. このため，
 * これらの factoring 機構は用いられない. ただし，
 * CATEGORY_TREE が undefined であれば，決定的 factoring を用いた言語制約
 * 適用を行うことも可能である. 
 * すなわち，次ノードに successor list が存在すれば,
 * その successor list 内の各単語と直前単語の単語対制約を調べ,
 * そのうち一つでも接続可能な単語があれば，その遷移を許し，一つも
 * なければ遷移させない. この機能は技術参考のために残されているのみである. 
 * </JA>
 * 
 * <EN>
 * @brief  LM factoring on 1st pass.
 * </EN>
 *
 * This file contains functions to do language score factoring on the 1st
 * pass.  They build a successor lists which holds the successive words in
 * each sub tree on the tree lexicon, and also provide a factored LM
 * probability on each nodes on the tree lexicon.
 *
 * The "successor list" will be assigned for each lexicon tree node to
 * represent a list of words that exist in the sub-tree and share the node.
 * Actually they will be assigned to the branch node.
 * Below is the example of successor lists on a tree lexicon, in which
 * the lists is assigned to the numbered nodes.
 * 
 * <pre>
 *         2-o-o - o-o-o - o-o-o          word "A" 
 *        /
 *   1-o-o
 *        \       4-o-o                   word "B"
 *         \     /   
 *          3-o-o - 5-o-o - 7-o-o         word "C"
 *           \            \ 
 *            \            8-o-o          word "D"
 *             6-o-o                      word "E"
 * </pre>
 *
 * The contents of the successor lists are the following:
 *
 * <pre>
 *  node  | successor list (wchmm->state[node].sc)
 *  =======================
 *    1   | A B C D E
 *    2   | A
 *    3   |   B C D E
 *    4   |   B
 *    5   |     C D
 *    6   |         E
 *    7   |     C
 *    8   |       D
 * </pre>
 *
 * When the 1st pass proceeds, if the next going node has a successor list,
 * all the word 2-gram scores in the successor list on the next node
 * will be computed, and the propagating LM value in the token on
 * the current node will be replaced by the maximum value of the scores
 * when copied to the next node.  Appearently, if the successor list has
 * only one word, it means that the word can be determined on that point,
 * and the precise 2-gram value will be assigned as is.
 *
 * When using 1-gram factoring, the computation will be slightly different.
 * Since the factoring value (maximum value of 1-gram scores on each successor
 * list) is independent of the word context, they can be computed statically
 * before the search.  Thus, for all the successor lists that have more than
 * two words, the maximum 1-gram value is computed and stored to
 * "fscore" member in tree lexicon, and the successor lists will be freed.
 * The successor lists with only one word should still remain in the
 * tree lexicon, to compute the precise 2-gram scores for the words.
 *
 *
 * When using DFA grammar, Julian builds separated lexicon trees for every
 * word categories, to statically express the catergory-pair constraint.
 * Thus these factoring scheme is not used by default.
 * However you can still force Julian to use the grammar-based
 * deterministic factoring scheme by undefining CATEGORY_TREE.
 * If CATEGORY_TREE is undefined, the word connection constraint will be
 * performed based on the successor list at the middle of tree lexicon.
 * This enables single tree search on Julian.  This function is left
 * only for technical reference.
 * 
 * @author Akinobu LEE
 * @date   Mon Mar  7 23:20:26 2005
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

/*----------------------------------------------------------------------*/

/** 
 * <JA>
 * 木構造化辞書上の全ノードに successor list を構築するメイン関数
 * 
 * @param wchmm [i/o] 木構造化辞書
 * </JA>
 * <EN>
 * Main function to build whole successor list to lexicon tree.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
void
make_successor_list(WCHMM_INFO *wchmm)
{
  int node;
  WORD_ID w;
  int i, j;
  int s;
  WORD_ID *scnumlist;
  WORD_ID *sclen;
  int scnum, new_scnum;
  int *scidmap;
  boolean *freemark;

  jlog("STAT: make successor lists for factoring\n");

  /* 1. initialize */
  /* initialize node->sclist index on wchmm tree */
  for (node=0;node<wchmm->n;node++) wchmm->state[node].scid = 0;

  /* parse the tree to assign unique scid and get the maximum size of
     successor list */
  scnum = 1;
  for (w=0;w<wchmm->winfo->num;w++) {
    for (i=0;i<wchmm->winfo->wlen[w];i++) {
      if (wchmm->state[wchmm->offset[w][i]].scid == 0) {
	wchmm->state[wchmm->offset[w][i]].scid = scnum;
	scnum++;
      }
    }
    if (wchmm->state[wchmm->wordend[w]].scid == 0) {
      wchmm->state[wchmm->wordend[w]].scid = scnum;
      scnum++;
    }
  }
  if (debug2_flag) {
    jlog("DEBUG: initial successor list size = %d\n", scnum);
  }

  /* 2. count number of each successor */
  sclen = (WORD_ID *)mymalloc(sizeof(WORD_ID) * scnum);
  for (i=1;i<scnum;i++) sclen[i] = 0;
  for (w=0;w<wchmm->winfo->num;w++) {
    for (i=0;i<wchmm->winfo->wlen[w];i++) {
      sclen[wchmm->state[wchmm->offset[w][i]].scid]++;
    }
    sclen[wchmm->state[wchmm->wordend[w]].scid]++;
  }

  /* 3. delete bogus successor lists */
  freemark = (boolean *)mymalloc(sizeof(boolean) * scnum);
  for (i=1;i<scnum;i++) freemark[i] = FALSE;
  for (w=0;w<wchmm->winfo->num;w++) {
    node = wchmm->wordend[w];	/* begin from the word end node */
    i = wchmm->winfo->wlen[w]-1;
    while (i >= 0) {		/* for each phoneme start node */
      if (node == wchmm->offset[w][i]) {
	/* word with only 1 state: skip */
	i--;
	continue;
      }
      if (wchmm->state[node].scid == 0) break; /* already parsed */
      if (sclen[wchmm->state[node].scid] == sclen[wchmm->state[wchmm->offset[w][i]].scid]) {
	freemark[wchmm->state[node].scid] = TRUE;	/* mark the node */
	wchmm->state[node].scid = 0;
      }
      node = wchmm->offset[w][i];
      i--;
    }
  }
  /* build compaction map */
  scidmap = (int *)mymalloc(sizeof(int) * scnum);
  scidmap[0] = 0;
  j = 1;
  for (i=1;i<scnum;i++) {
    if (freemark[i]) {
      scidmap[i] = 0;
    } else {
      scidmap[i] = j;
      j++;
    }
  }
  new_scnum = j;
  if (debug2_flag) {
    jlog("DEBUG: compacted successor list size = %d\n", new_scnum);
  }

  /* 4. rewrite scid and do compaction for new sclen */
  for (node=0;node<wchmm->n;node++) {
    if (wchmm->state[node].scid > 0) {
      wchmm->state[node].scid = scidmap[wchmm->state[node].scid];
    }
  }
  wchmm->sclen = (WORD_ID *)mybmalloc2(sizeof(WORD_ID) * new_scnum, &(wchmm->malloc_root));
  for (i=1;i<scnum;i++) {
    if (scidmap[i] != 0) wchmm->sclen[scidmap[i]] = sclen[i];
  }
  wchmm->scnum = new_scnum;

  free(scidmap);
  free(freemark);
  free(sclen);

  /* 5. now index completed, make word list for each list */
  wchmm->sclist = (WORD_ID **)mybmalloc2(sizeof(WORD_ID *) * wchmm->scnum, &(wchmm->malloc_root));
  scnumlist = (WORD_ID *)mymalloc(sizeof(WORD_ID) * wchmm->scnum);
  for(i=1;i<wchmm->scnum;i++) {
    wchmm->sclist[i] = (WORD_ID *)mybmalloc2(sizeof(WORD_ID) * wchmm->sclen[i], &(wchmm->malloc_root));
    scnumlist[i] = 0;
  }
  {
    int scid;
    for (w=0;w<wchmm->winfo->num;w++) {
      for (i=0;i<wchmm->winfo->wlen[w];i++) {
	scid = wchmm->state[wchmm->offset[w][i]].scid;
	if (scid != 0) {
	  wchmm->sclist[scid][scnumlist[scid]] = w;
	  scnumlist[scid]++;
	  if (scnumlist[scid] > wchmm->sclen[scid]) {
	    jlog("hogohohoho\n");
	    exit(1);
	  }
	}
      }
      /* at word end */
      scid = wchmm->state[wchmm->wordend[w]].scid;
      if (scid != 0) {
	wchmm->sclist[scid][scnumlist[scid]] = w;
	scnumlist[scid]++;
	  if (scnumlist[scid] > wchmm->sclen[scid]) {
	    jlog("hogohohoho\n");
	    exit(1);
	  }
      }
    }
  }
  free(scnumlist);

  jlog("STAT: done\n");
}

#ifdef UNIGRAM_FACTORING

/** 
 * <JA>
 * 木構造化辞書上の全ノードに successor list を構築するメイン関数(unigram factoring 用
 * 
 * @param wchmm [i/o] 木構造化辞書
 * </JA>
 * <EN>
 * Main function to build whole successor list to lexicon tree for unigram factoring
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
void
make_successor_list_unigram_factoring(WCHMM_INFO *wchmm)
{

#ifndef FAST_FACTOR1_SUCCESSOR_LIST

  /* old way */
  make_successor_list(wchmm);
  calc_all_unigram_factoring_values(wchmm);

#else  /* ~FAST_FACTOR1_SUCCESSOR_LIST */

  /* new way */

  int node, node2;
  WORD_ID w, w2;
  int i, j, n, f;
  int s;
  LOGPROB tmpprob;
  WORD_ID *mtmp;

  jlog("STAT: make successor lists for unigram factoring\n");

  /* 1. initialize */
  /* initialize node->sclist index on wchmm tree */
  for (node=0;node<wchmm->n;node++) wchmm->state[node].scid = 0;

  /* in unigram factoring, number of successor = vocabulary size */
  wchmm->scnum = wchmm->winfo->num + 1;
  if (debug2_flag) {
    jlog("DEBUG: successor list size = %d\n", wchmm->scnum);
  }

  /* allocate successor list for 1-gram factoring */
  wchmm->scword = (WORD_ID *)mybmalloc2(sizeof(WORD_ID) * wchmm->scnum, &(wchmm->malloc_root));

  /* 2. make successor list, and count needed fscore num */
  f = 1;
  s = 1;
  for (w=0;w<wchmm->winfo->num;w++) {
    for (i=0;i<wchmm->winfo->wlen[w] + 1;i++) {
      if (i < wchmm->winfo->wlen[w]) {
	node = wchmm->offset[w][i];
      } else {
	node = wchmm->wordend[w];
      }
      if (wchmm->state[node].scid == 0) { /* not assigned */
	/* new node found, assign new and exit here */
	wchmm->state[node].scid = s;
	wchmm->scword[s] = w;
	s++;
	if (s > wchmm->scnum) {
	  jlog("InternalError: make_successor_list_unigram_factoring: scid num exceeded?\n");
	  return;
	}
	break;
      } else if (wchmm->state[node].scid > 0) {
	/* that node has successor */
	/* move it to the current first isolated node in that word */
	w2 = wchmm->scword[wchmm->state[node].scid];
	for(j=i+1;j<wchmm->winfo->wlen[w2] + 1;j++) {
	  if (j < wchmm->winfo->wlen[w2]) {
	    node2 = wchmm->offset[w2][j];
	  } else {
	    node2 = wchmm->wordend[w2];
	  }
	  if (wchmm->state[node2].scid == 0) { /* not assigned */
	    /* move successor to there */
	    wchmm->state[node2].scid = wchmm->state[node].scid;
	    break;
	  }
	}
	if (j >= wchmm->winfo->wlen[w2] + 1) {
	  /* not found? */
	  jlog("InternalError: make_successor_list_unigram_factoring: no isolated word for %d\n", w2);
	  return;
	}
	/* make current node as fscore node */
	n = f++;
	wchmm->state[node].scid = -n;
	/* not compute unigram factoring value yet */
      }

    }
  }

  /* 2. allocate fscore buffer */
  wchmm->fsnum = f;
  wchmm->fscore = (LOGPROB *)mymalloc(sizeof(LOGPROB) * wchmm->fsnum);
  for(n=0;n<wchmm->fsnum;n++) wchmm->fscore[n] = LOG_ZERO;

  /* 3. parse again to assign fscore values */
  for (w=0;w<wchmm->winfo->num;w++) {
    for (i=0;i<wchmm->winfo->wlen[w] + 1;i++) {
      if (i < wchmm->winfo->wlen[w]) {
	node = wchmm->offset[w][i];
      } else {
	node = wchmm->wordend[w];
      }
      if (wchmm->state[node].scid < 0) {
	/* update max */
	if (wchmm->ngram) {
	  tmpprob = uni_prob(wchmm->ngram, wchmm->winfo->wton[w])
#ifdef CLASS_NGRAM
	    + wchmm->winfo->cprob[w]
#endif
	    ;
	} else {
	  tmpprob = LOG_ZERO;
	}
	if (wchmm->lmvar == LM_NGRAM_USER) {
	  tmpprob = (*(wchmm->uni_prob_user))(wchmm->winfo, w, tmpprob);
	}
	n = - wchmm->state[node].scid;
	if (wchmm->fscore[n] < tmpprob) {
	  wchmm->fscore[n] = tmpprob;
	}
      }

    }
  }

#endif  /* ~FAST_FACTOR1_SUCCESSOR_LIST */

  jlog("STAT: done\n");
}

#endif /* UNIGRAM_FACTORING */


/** 
 * <JA>
 * 構築された factoring 情報を multipath 用に調整する. factoring 情報を,
 * モデル全体をスキップする遷移がある場合はその先の音素へコピーする. 
 * また，(出力を持たない)文頭文法ノードに単語先頭ノードからコピーする. 
 * 
 * @param wchmm [in] 木構造化辞書
 * </JA>
 * <EN>
 * Adjust factoring data in tree lexicon for multipath transition handling.
 * 
 * @param wchmm [in] tree lexicon
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
void
adjust_sc_index(WCHMM_INFO *wchmm)
{
  WORD_ID w;
  int i,j,k;
  HMM_Logical *ltmp;
  int ltmp_state_num;
  int ato;
  LOGPROB prob;
  int node, scid;
  A_CELL2 *ac;
  
  /* duplicate scid for HMMs with more than one arc from initial state */
  for(w=0;w<wchmm->winfo->num;w++) {
    for(k=0;k<wchmm->winfo->wlen[w];k++) {
      node = wchmm->offset[w][k];
      scid = wchmm->state[node].scid;
      if (scid == 0) continue;
      ltmp = wchmm->winfo->wseq[w][k];
      ltmp_state_num = hmm_logical_state_num(ltmp);
      if ((hmm_logical_trans(ltmp))->a[0][ltmp_state_num-1] != LOG_ZERO) {
	j = k + 1;
	if (j == wchmm->winfo->wlen[w]) {
	  if (wchmm->state[wchmm->wordend[w]].scid == 0) {
	    jlog("STAT: word %d: factoring node copied for skip phone\n", w);
	    wchmm->state[wchmm->wordend[w]].scid = scid;
	  }
	} else {
	  if (wchmm->state[wchmm->offset[w][j]].scid == 0) {
	    jlog("STAT: word %d: factoring node copied for skip phone\n", w);
	    wchmm->state[wchmm->offset[w][j]].scid = scid;
	  }
	}
      }
      for(ato=1;ato<ltmp_state_num;ato++) {
	prob = (hmm_logical_trans(ltmp))->a[0][ato];
	if (prob != LOG_ZERO) {
	  wchmm->state[node+ato-1].scid = scid;
	}
      }
    }
  }

  /* move scid and fscore on the head state to the head grammar state */
  for(i=0;i<wchmm->startnum;i++) {
    node = wchmm->startnode[i];
    if (wchmm->state[node].out.state != NULL) {
      j_internal_error("adjust_sc_index: outprob exist in word-head node??\n");
    }
    if (wchmm->next_a[node] != LOG_ZERO) {
      if (wchmm->state[node+1].scid != 0) {
	if (wchmm->state[node].scid != 0 && wchmm->state[node].scid != wchmm->state[node+1].scid) {
	  j_internal_error("adjust_sc_index: different successor list within word-head phone?\n");
	}
	wchmm->state[node].scid = wchmm->state[node+1].scid;
	wchmm->state[node+1].scid = 0;
      }
    }
    for(ac=wchmm->ac[node];ac;ac=ac->next) {
      for(k=0;k<ac->n;k++) {
	if (wchmm->state[ac->arc[k]].scid != 0) {
	  if (wchmm->state[node].scid != 0 && wchmm->state[node].scid != wchmm->state[ac->arc[k]].scid) {
	    j_internal_error("adjust_sc_index: different successor list within word-head phone?\n");
	  }
	  wchmm->state[node].scid = wchmm->state[ac->arc[k]].scid;
	  wchmm->state[ac->arc[k]].scid = 0;
	}
      }
    }
  }
}


/* -------------------------------------------------------------------- */
/* factoring computation */

/** 
 * <JA>
 * 木構造化辞書用の factoring キャッシュをメモリ割り付けして初期化する. 
 * この関数はプログラム開始時に一度だけ呼ばれる. 
 * 
 * @param wchmm [i/o] 木構造化辞書
 * </JA>
 * <EN>
 * Initialize factoring cache for a tree lexicon, allocating memory for
 * cache.  This should be called only once on start up.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
void
max_successor_cache_init(WCHMM_INFO *wchmm)
{
  int i;
  LM_PROB_CACHE *l;
  WORD_ID wnum;

  /* for word-internal */
  l = &(wchmm->lmcache);

  l->probcache = (LOGPROB *) mymalloc(sizeof(LOGPROB) * wchmm->scnum);
  l->lastwcache = (WORD_ID *) mymalloc(sizeof(WORD_ID) * wchmm->scnum);
  for (i=0;i<wchmm->scnum;i++) {
    l->lastwcache[i] = WORD_INVALID;
  }
  /* for cross-word */
  if (wchmm->ngram) {
    wnum = wchmm->ngram->max_word_num;
  } else {
    wnum = wchmm->winfo->num;
  }
#ifdef HASH_CACHE_IW
  l->iw_cache_num = wnum * jconf.search.pass1.iw_cache_rate / 100;
  if (l->iw_cache_num < 10) l->iw_cache_num = 10;
#else
  l->iw_cache_num = wnum;
#endif /* HASH_CACHE_IW */
  l->iw_sc_cache = (LOGPROB **)mymalloc(sizeof(LOGPROB *) * l->iw_cache_num);
  for (i=0;i<l->iw_cache_num;i++) {
    l->iw_sc_cache[i] = NULL;
  }
#ifdef HASH_CACHE_IW
  l->iw_lw_cache = (WORD_ID *)mymalloc(sizeof(WORD_ID) * l->iw_cache_num);
  for (i=0;i<l->iw_cache_num;i++) {
    l->iw_lw_cache[i] = WORD_INVALID;
  }
#endif
}

/** 
 * <JA>
 * 単語間の factoring cache のメモリ領域を解放する. 
 * 
 * @param wchmm [i/o] 木構造化辞書
 * </JA>
 * <EN>
 * Free cross-word factoring cache.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 */
static void
max_successor_prob_iw_free(WCHMM_INFO *wchmm)
{
  int i;
  LM_PROB_CACHE *l;
  l = &(wchmm->lmcache);
  for (i=0;i<l->iw_cache_num;i++) {
    if (l->iw_sc_cache[i] != NULL) free(l->iw_sc_cache[i]);
    l->iw_sc_cache[i] = NULL;
  }
}

/** 
 * <JA>
 * factoring 用 cache のメモリ領域を全て解放する. 
 * 
 * @param wchmm [i/o] 木構造化辞書
 * </JA>
 * <EN>
 * Free all memory for  factoring cache.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
void
max_successor_cache_free(WCHMM_INFO *wchmm)
{
  free(wchmm->lmcache.probcache);
  free(wchmm->lmcache.lastwcache);
  max_successor_prob_iw_free(wchmm);
  free(wchmm->lmcache.iw_sc_cache);
#ifdef HASH_CACHE_IW
  free(wchmm->lmcache.iw_lw_cache);
#endif
}

#ifdef UNIGRAM_FACTORING

/** 
 * <JA>
 * @brief  単語先頭ノードのうちFactoring においてキャッシュが必要なノードの
 * リストを作成する. 
 *
 * 1-gram factoring は，枝ノードにおいて直前単語に依存しない固定値
 * (unigramの最大値)を与える. このため，単語間の factoring 計算において，
 * 木構造化辞書上で複数の単語で共有されている単語先頭ノードについては，
 * その値は直前単語によらず固定値であり，認識時に単語間キャッシュを保持
 * する必要はない. 
 * 
 * この関数では，単語先頭ノードのリストからそのような factoring キャッシュが
 * 不要なノードを除外して，1-gram factoring 時に単語間キャッシュが必要な
 * 単語先頭ノード（＝他の単語と共有されていない独立した単語先頭ノード）の
 * リストを作成し，wchmm->start2isolate および wchmm->isolatenum に格納する. 
 * 
 * @param wchmm [i/o] 木構造化辞書
 * </JA>
 * <EN>
 * @brief  Make a list of word head nodes on which cross-word factoring cache
 * is needed.
 *
 * On 1-gram factoring, the branch nodes on tree lexicon has a fixed
 * factoring value (maximum 1-gram score of all sub-tree words).  Thus, when
 * computing cross-word factoring at word head nodes on inter-word
 * transition, such 1-gram factoring nodes on word head, shared by several
 * words, need not be cached in inter-word factoring cache.
 *
 * This function make a list of word-head nodes which requires inter-word
 * factoring caching (i.e. isolated word head nodes, does not shared by other
 * words) from the existing list of word head nodes, and set it to
 * wchmm->start2isolate and wchmm->isolatenum.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
void
make_iwcache_index(WCHMM_INFO *wchmm)
{
  int i, node, num;

  wchmm->start2isolate = (int *)mymalloc(sizeof(int) * wchmm->startnum);
  num = 0;
  for(i=0;i<wchmm->startnum;i++) {
    node = wchmm->startnode[i];
    if (wchmm->state[node].scid >= 0) {	/* not a factoring node (isolated node, has no 1-gram factoring value) */
      wchmm->start2isolate[i] = num;
      num++;
    } else {			/* factoring node (shared) */
      wchmm->start2isolate[i] = -1;
    }
  }
  wchmm->isolatenum = num;
}

#ifndef FAST_FACTOR1_SUCCESSOR_LIST

/** 
 * <JA>
 * @brief  木構造化辞書上の 1-gram factoring 値を計算して格納する. 
 *
 * 1-gram factoring では単語間で共有されている枝ノードでは 1-gram の最大値
 * を与える. 単語履歴によらないため，その値は認識開始前に
 * 計算しておくことができる. この関数は木構造化辞書
 * 全体について，共有されている（successor list に２つ以上の単語を持つノード）
 * ノードの 1-gram factoring 値を計算して格納する. 1-gram factoring値を
 * 計算後は，そのノードの successor list はもはや不要であるため，ここで
 * 削除する. 
 *
 * 実際には，factoring 値は wchmm->fscore に順次保存され，ノードの
 * scid にその保存値へのインデックス(1-)の負の値が格納される. 不要になった
 * successor list は，実際には compaction_successor 内で，対応するノードの
 * scid が負になっている successor list を削除することで行なわれる. 
 * 
 * @param wchmm [i/o] 木構造化辞書
 * </JA>
 * <EN>
 * @brief  Calculate all the 1-gram factoring values on tree lexicon.
 *
 * On 1-gram factoring, the shared nodes on branch has fixed factoring score
 * from 1-gram values, independent of the word context on recognition.  So
 * the values are fixed for all recognition and can be calculated before
 * search.  This function stores all the neede 1-gram factoring value by
 * traversing tree lexicon with successor lists and compute maximum 1-gram
 * for each successor lists that has more than two words (=shared).
 * Since a successor list is no more neede after the 1-gram value is computed,
 * they will be freed.
 *
 * Actually, computed factoring scores will be stored in wchmm->fscore
 * sequencially, and the index value, starting from 1,
 * to the fscore list is stored in scid of each nodes as a negative value.
 * The free will be performed in compaction_successor() by checking if a
 * successor's corresponding scid on tree lexicon has negative value.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
void
calc_all_unigram_factoring_values(WCHMM_INFO *wchmm)
{
  S_CELL *sc, *sctmp;
  LOGPROB tmpprob, maxprob;
  int i, n;

  /* count needed number of 1-gram factoring nodes */
  n = 0;
  for (i=1;i<wchmm->scnum;i++) {
    sc = wchmm->sclist[i];
    if (sc == NULL) {
      j_internal_error("call_all_unigram_factoring_values: sclist has no sc?\n");
    }
    if (sc->next != NULL) {
      /* more than two words, so compute maximum 1-gram probability */
      n++;
    }
  }
  wchmm->fsnum = n + 1;
  /* allocate area */
  wchmm->fscore = (LOGPROB *)mymalloc(sizeof(LOGPROB) * wchmm->fsnum);
  /* assign values */
  n = 1;
  for (i=1;i<wchmm->scnum;i++) {
    sc = wchmm->sclist[i];
    if (sc->next != NULL) {
      maxprob = LOG_ZERO;
      for (sctmp = sc; sctmp; sctmp = sctmp->next) {
	if (wchmm->ngram) {
	  tmpprob = uni_prob(wchmm->ngram, wchmm->winfo->wton[sctmp->word])
#ifdef CLASS_NGRAM
	    + wchmm->winfo->cprob[sctmp->word] 
#endif
	    ;
	} else {
	  tmpprob = LOG_ZERO;
	}
	if (wchmm->lmvar == LM_NGRAM_USER) {
	  tmpprob = (*(wchmm->uni_prob_user))(wchmm->winfo, sctmp->word, tmpprob);
	}
	if (maxprob < tmpprob) maxprob = tmpprob;
      }
      wchmm->fscore[n] = maxprob;
      free_successor(wchmm, i);
      wchmm->state[wchmm->sclist2node[i]].scid = - n;
      n++;
    }
  }
  /* garbage collection of factored sclist */
  compaction_successor(wchmm);
}

#endif

#else  /* ~UNIGRAM_FACTORING */

/** 
 * <JA>
 * 木構造化辞書上のあるノードについて，与えられた単語履歴に対する2-gram
 * スコアを計算する. 
 * 
 * @param wchmm [in] 木構造化辞書
 * @param lastword [in] 直前単語
 * @param node [in] @a wchmm 上のノード番号
 * 
 * @return 2-gram 確率. 
 * </JA>
 * <EN>
 * Compute 2-gram factoring value for the node and return the probability.
 * 
 * @param wchmm [in] tree lexicon
 * @param lastword [in] the last context word
 * @param node [in] node ID on @a wchmm
 * 
 * @return the log probability of 2-gram on that node.
 * </EN>
 * 
 */
static LOGPROB
calc_successor_prob(WCHMM_INFO *wchmm, WORD_ID lastword, int node)
{
  LOGPROB tmpprob, maxprob;
  WORD_ID lw, w;
  int i;
  int scid;

  maxprob = LOG_ZERO;
  if (wchmm->ngram) {
    lw = wchmm->winfo->wton[lastword];
  }

  scid = wchmm->state[node].scid;

  for (i = 0; i < wchmm->sclen[scid]; i++) {
    w = wchmm->sclist[scid][i];
    if (wchmm->ngram) {
      tmpprob = (*(wchmm->ngram->bigram_prob))(wchmm->ngram, lw , wchmm->winfo->wton[w])
#ifdef CLASS_NGRAM
	+ wchmm->winfo->cprob[w]
#endif
	;
    } else {
      tmpprob = LOG_ZERO;
    }
    if (wchmm->lmvar == LM_NGRAM_USER) {
      tmpprob = (*(wchmm->bi_prob_user))(wchmm->winfo, lastword, w, tmpprob);
    }
    if (maxprob < tmpprob) maxprob = tmpprob;
  }

  return(maxprob);
}

#endif  /* ~UNIGRAM_FACTORING */

/** 
 * <JA>
 * @brief  単語内のあるノードについて factoring 値を計算する. 
 *
 * 1-gram factoring で固定factoring値がある場合はその値が即座に返される. 
 * 他の場合は，そのノードのサブツリー内の単語の 2-gram確率（の最大値）が
 * 計算される. 
 *
 * 単語内 factoring キャッシュが考慮される. すなわち各ノードについて
 * 直前単語が前回アクセスされたときと同じであれば，
 * 前回の値が返され，そうでなければ値を計算し，キャッシュが更新される. 
 * 
 * @param wchmm [in] 木構造化辞書
 * @param lastword [in] 直前単語のID
 * @param node [in] ノード番号
 * 
 * @return 言語モデルスコア
 * </JA>
 * <EN>
 * @brief  compute factoring LM score for the given word-internal node.
 *
 * If it is a shared branch node and 1-gram factoring is used, the
 * constant factoring value which has already been assigned before search
 * will be returned immediately.  Else, the maximum 2-gram probability
 * of corresponding successor words are computed.
 *
 * The word-internal factoring cache is consulted within this function.
 * If the given last word is the same as the last call on that node,
 * the last computed value will be returned, else the maximum value
 * will be computed update the cache with the last word and value.
 * 
 * @param wchmm [in] tree lexicon
 * @param lastword [in] word ID of last context word
 * @param node [in] node ID
 * 
 * @return the LM factoring score.
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
LOGPROB
max_successor_prob(WCHMM_INFO *wchmm, WORD_ID lastword, int node)
{
  LOGPROB maxprob;
  WORD_ID last_nword, w;
  int scid;
  LM_PROB_CACHE *l;

  l = &(wchmm->lmcache);

  if (lastword != WORD_INVALID) { /* return nothing if no previous word */
    if (wchmm->ngram) {
      last_nword = wchmm->winfo->wton[lastword];
    } else {
      last_nword = lastword;
    }
    scid = wchmm->state[node].scid;
#ifdef UNIGRAM_FACTORING
    if (scid < 0) {
      /* return 1-gram factoring value already calced */
      return(wchmm->fscore[(- scid)]);
    } else {
      /* this node has only one successor */
      /* return precise 2-gram score */
      if (last_nword != l->lastwcache[scid]) {
	/* calc and cache */
	w = wchmm->scword[scid];
	if (wchmm->ngram) {
	  maxprob = (*(wchmm->ngram->bigram_prob))(wchmm->ngram, last_nword, wchmm->winfo->wton[w])
#ifdef CLASS_NGRAM
	    + wchmm->winfo->cprob[w]
#endif
	    ;
	} else {
	  maxprob = LOG_ZERO;
	}
	if (wchmm->lmvar == LM_NGRAM_USER) {
	  maxprob = (*(wchmm->bi_prob_user))(wchmm->winfo, lastword, w, maxprob);
	}
	l->lastwcache[scid] = last_nword;
	l->probcache[scid] = maxprob;
	return(maxprob);
      } else {
	/* return cached */
	return (l->probcache[scid]);
      }
    }
#else  /* UNIGRAM_FACTORING */
    /* 2-gram */
    if (last_nword != l->lastwcache[scid]) {
      maxprob = calc_successor_prob(wchmm, lastword, node);
      /* store to cache */
      l->lastwcache[scid] = last_nword;
      l->probcache[scid] = maxprob;
      return(maxprob);
    } else {
      return (l->probcache[scid]);
    }
#endif /* UNIGRAM_FACTORING */
  } else {
    return(0.0);
#if 0
    maxprob = LOG_ZERO;
    for (sc=wchmm->state[node].sc;sc;sc=sc->next) {
      tmpprob = uni_prob(wchmm->ngram, sc->word);
      if (maxprob < tmpprob) maxprob = tmpprob;
    }
    return(maxprob);
#endif
  }

}

/** 
 * <JA>
 * @brief  単語間の factoring 値のリストを返す. 
 *
 * 与えられた直前単語に対して，factoring値を計算すべき全ての単語先頭への
 * factoring 値を計算し，そのリストを返す. このfactoring値は
 * 直前単語ごとにリスト単位でキャッシュされる. すなわち，その直前単語が
 * それまでに一度でも直前単語として出現していた場合，そのリストをそのまま
 * 返す. 
 * 
 * @param wchmm [in] 木構造化辞書
 * @param lastword [in] 直前単語
 * 
 * @return 全単語先頭ノードへの factoring スコアのリスト
 * </JA>
 * <EN>
 * @brief  Compute cross-word facgtoring values for word head nodes and return
 * the list.
 *
 * Given a last word, this function compute the factoring LM scores for all
 * the word head node to which the context-dependent (not 1-gram) factoring
 * values should be computed.  The resulting list of factoring values are
 * cached within this function per the last word.
 * 
 * @param wchmm [in] tree lexicon
 * @param lastword [in] last word
 * 
 * @return the list of factoring LM scores for all the needed word-head nodes.
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
LOGPROB *
max_successor_prob_iw(WCHMM_INFO *wchmm, WORD_ID lastword)
{
  int i, j, x, node;
  int last_nword;
  WORD_ID w;
  LM_PROB_CACHE *l;
  LOGPROB p;

  l = &(wchmm->lmcache);

  if (wchmm->ngram) {
    last_nword = wchmm->winfo->wton[lastword];
  } else {
    last_nword = lastword;
  }

#ifdef HASH_CACHE_IW
  x = last_nword % l->iw_cache_num;
  if (l->iw_lw_cache[x] == last_nword) { /* cache hit */
    return(l->iw_sc_cache[x]);
  }
#else  /* full cache */
  if (l->iw_sc_cache[last_nword] != NULL) { /* cache hit */
    return(l->iw_sc_cache[last_nword]);
  }
  x = last_nword;
  /* cache mis-hit, calc probs and cache them as new */
#endif
  /* allocate cache memory */
  if (l->iw_sc_cache[x] == NULL) {
#ifdef UNIGRAM_FACTORING
    l->iw_sc_cache[x] = (LOGPROB *)mymalloc(sizeof(LOGPROB)*wchmm->isolatenum);
#else
    l->iw_sc_cache[x] = (LOGPROB *)mymalloc(sizeof(LOGPROB)*wchmm->startnum);
#endif
    if (l->iw_sc_cache[x] == NULL) { /* malloc failed */
      /* clear existing cache, and retry */
      max_successor_prob_iw_free(wchmm);
      jlog("STAT: inter-word LM cache (%dMB) rehashed\n",
	       (l->iw_cache_num * 
#ifdef UNIGRAM_FACTORING
		wchmm->isolatenum
#else
		wchmm->startnum
#endif
		) / 1000 * sizeof(LOGPROB) / 1000);
#ifdef UNIGRAM_FACTORING
      l->iw_sc_cache[x] = (LOGPROB *)mymalloc(sizeof(LOGPROB)*wchmm->isolatenum);
#else
      l->iw_sc_cache[x] = (LOGPROB *)mymalloc(sizeof(LOGPROB)*wchmm->startnum);
#endif
      if (l->iw_sc_cache[x] == NULL) { /* malloc failed again? */
	j_internal_error("max_successor_prob_iw: cannot malloc\n");
      }
    }
  }

  /* calc prob for all startid */
#ifdef UNIGRAM_FACTORING
  for (j=0;j<wchmm->startnum;j++) {
    i = wchmm->start2isolate[j];
    if (i == -1) continue;
    node = wchmm->startnode[j];
    if (wchmm->state[node].scid <= 0) {
      /* should not happen!!! below is just for debugging */
      j_internal_error("max_successor_prob_iw: isolated (not shared) tree root node has unigram factoring value??\n");
    } else {
      w = wchmm->scword[wchmm->state[node].scid];
      if (wchmm->ngram) {
	p = (*(wchmm->ngram->bigram_prob))(wchmm->ngram, last_nword, wchmm->winfo->wton[w])
#ifdef CLASS_NGRAM
	  + wchmm->winfo->cprob[w]
#endif
	  ;
      } else {
	p = LOG_ZERO;
      }
      if (wchmm->lmvar == LM_NGRAM_USER) {
	p = (*(wchmm->bi_prob_user))(wchmm->winfo, lastword, w, p);
      }
      l->iw_sc_cache[x][i] = p;
    }
  }
#else  /* ~UNIGRAM_FACTORING */
  for (i=0;i<wchmm->startnum;i++) {
    node = wchmm->startnode[i];
    l->iw_sc_cache[x][i] = calc_successor_prob(wchmm, lastword, node);
  }
#endif
#ifdef HASH_CACHE_IW
  l->iw_lw_cache[x] = last_nword;
#endif

  return(l->iw_sc_cache[x]);
}

/** 
 * <JA>
 * @brief  文法による単語内決定的 factoring
 *
 * Julian において CATEGORY_TREE が定義されているとき（デフォルト），
 * 木構造化辞書はカテゴリ単位（すなわち構文制約の記述単位）で構築されるため，
 * 第1パスでの言語モデルであるカテゴリ対制約は単語の始終端で適用できる. 
 * 
 * この CATEGORY_TREE が定義されていない場合，木構造化辞書は
 * 辞書全体で単一の木が作られるため，カテゴリ対制約は N-gram (Julius) と
 * 同様に単語内で factoring と同様の機構で適用される必要がある. 
 *
 * この関数は CATEGORY_TREE が定義されていないときに，上記の factoring
 * （決定的 factoring と呼ばれる）を行なうために提供されている. 
 * 
 * @param wchmm [in] 木構造化辞書
 * @param lastword [in] 直前単語
 * @param node [in] ノード番号
 *
 * @return カテゴリ制約上その枝への遷移が許されれば TRUE, 不可能であれば FALSE
 * </JA>
 * <EN>
 * @brief  Deterministic factoring for grammar-based recognition (Julian)
 *
 * If CATEGORY_TREE is defined (this is default) on Julian, the tree lexicon
 * will be organized per category and the category-pair constraint used
 * in the 1st pass can be applied statically at cross-word transition.
 *
 * If the CATEGORY_TREE is not defined, a single tree lexicon will be
 * constucted for a whole dictionary.  In this case, the category-pair
 * constraint should be applied dynamically in the word-internal transition,
 * like the factoring scheme with N-gram (Julius).
 *
 * This function provides such word-internal factoring for grammar-based
 * recognition (called deterministic factoring) when CATEGORY_TREE is
 * undefined in Julian.
 * 
 * @param wchmm [in] tree lexicon
 * @param lastword [in] last word
 * @param node [in] node ID to check the constraint
 * 
 * @return TRUE if the transition to the branch is allowed on the category-pair
 * constraint, or FALSE if not allowed.
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
boolean
can_succeed(WCHMM_INFO *wchmm, WORD_ID lastword, int node)
{
  int lc;
  int i;
  int s;

  /* return TRUE if at least one subtree word can connect */

  s = wchmm->state[node].scid;

  if (lastword == WORD_INVALID) { /* case at beginning-of-word */
    for (i = 0; i < wchmm->sclen[s]; i++) {
      if (dfa_cp_begin(wchmm->dfa, wchmm->sclist[s][i]) == TRUE) return(TRUE);
    }
    return(FALSE);
  } else {
    lc = wchmm->winfo->wton[lastword];
    for (i = 0; i < wchmm->sclen[s]; i++) {
      if (dfa_cp(wchmm->dfa, lc, wchmm->sclist[s][i]) == TRUE) return(TRUE);
    }
    return(FALSE);
  }
}

/* end of file */
