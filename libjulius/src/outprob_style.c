/**
 * @file   outprob_style.c
 * 
 * <JA>
 * @brief  状態の出力確率計算（第1パス）
 *
 * 第1パスで，木構造化辞書上のノードの，入力ベクトルに対する HMM の
 * 出力対数確率を計算する. 
 *
 * トライフォン使用時は，単語内の音素環境依存については辞書読み込み時に
 * 考慮されて対応するトライフォンがすでに割り当てられているので，ここで
 * 特別な処理は行われない. 単語先頭および末尾の音素は，木構造化辞書上では
 * pseudo triphone が割り当たっており，これらについては，以下のように
 * 単語間を音素環境依存性を考慮した計算が行われる. 
 *  -# 単語内音素: 通常通り計算する. 
 *  -# 単語の先頭音素: 直前単語の情報から，pseudo triphone を正しい
 *     トライフォンに動的に切り替えて計算. 
 *  -# 単語の末尾音素: その pseudo triphone に含まれる（同じ左コンテキストを
 *     持つトライフォンの）状態集合中のすべての状態について尤度を計算し，
 *      - "-iwcd1 max" 指定時は最大値
 *      - "-iwcd1 avg" 指定時は平均値(default)
 *      - "-iwcd1 best N" 指定時は上位N個の平均値
 *     をその状態の尤度として採用する. (これは outprob_cd() 内で自動的に選択
 *     され計算される. 
 *  -# 1音素からなる単語の場合: 上記を両方とも考慮する. 
 *
 * 上記の処理を行うには，木構造化辞書の状態ごとに，それぞれが単語内でどの
 * 位置の音素に属する状態であるかの情報が必要である. 木構造化辞書では，
 * 状態ごとに上記のどの処理を行えば良いかを AS_Style であらかじめ保持している. 
 *
 * また，上記の 2 と 4 の状態では，コンテキストに伴うtriphone変化を，
 * 直前単語ID とともに状態ごとにフレーム単位でキャッシュしている. これにより
 * 計算量の増大を防ぐ. 
 * </JA>
 * 
 * <EN>
 * @brief  Compute output probability of a state (1st pass)
 *
 * These functions compute the output probability of an input vector
 * from a state on the lexicon tree.
 *
 * When using triphone acoustic model, the cross-word triphone handling is
 * done here.  The head and tail phoneme of every words has corresponding
 * pseudo phone set on the tree lexicon, so the actual likelihood computation
 * will be done as the following:
 *   -# word-internal: compute as normal.
 *   -# Word head phone: the correct triphone phone, according to the last
 *      word information on the passing token, will be dynamically assigned
 *      to compute the cross-word dependency.
 *   -# Word tail phone: all the states in the pseudo phone set (they are
 *      states of triphones that has the same left context as the word end)
 *      will be computed, and use
 *       - maximum value if "-iwcd1 max" specified, or
 *       - average value if "-iwcd1 avg" specified, or
 *       - average of best N states if "-iwcd1 best N" specified (default: 3)
 *      the actual pseudo phoneset computation will be done in outprob_cd().
 *   -# word with only one state: both of above should be considered.
 *
 *  To denote which operation to do for a state, AS_Style ID is assigned
 *  to each state.
 *
 *  The triphone transformation, that will be performed on the state
 *  of 2 and 4 above, will be cached on the tree lxicon by each state
 *  per frame, to suppress computation overhead.
 *   
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Mon Aug 22 17:14:26 2005
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

#ifdef PASS1_IWCD

/** 
 * <JA>
 * 語頭トライフォン変化用キャッシュの初期化
 * 
 * @param wchmm [i/o] 木構造化辞書
 * </JA>
 * <EN>
 * Initialize cache for triphone changing on every word head.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 * @callgraph
 * @callergraph
 */
void
outprob_style_cache_init(WCHMM_INFO *wchmm)
{
  int n;
  for(n=0;n<wchmm->n;n++) {
    if (wchmm->state[n].out.state == NULL) continue;
    if (wchmm->outstyle[n] == AS_RSET) {
      (wchmm->state[n].out.rset)->cache.state = NULL;
    } else if (wchmm->outstyle[n] == AS_LRSET) {
      (wchmm->state[n].out.lrset)->cache.state = NULL;
    }
  }
}

/**********************************************************************/

/** 
 * <JA>
 * @brief  単語末尾のトライフォンセット (pseudo phone set) を検索する. 
 *
 * 文法認識では，各カテゴリごとに独立した pseudo phone set を用いる. 
 * ここでは単語末用カテゴリ付き pseudo phone set を検索する. 
 * 
 * @param wchmm [in] 木構造化辞書
 * @param hmm [in] 単語の末尾の HMM
 * @param category [in] 単語の属するカテゴリ
 * 
 * @return 該当 set が見つかればそこへのポインタ，あるいは見つからなければ
 * NULL を返す. 
 * </JA>
 * <EN>
 * Lookup a word-end triphone set (aka pseudo phone set) with
 * category id for grammar recognition.
 * 
 * @param wchmm [in] word lexicon tree
 * @param hmm [in] logical HMM of word end phone
 * @param category [in] belonging category id of the word
 * 
 * @return pointer to the corresponding phone set if found, or NULL if
 * not found.
 * </EN>
 * @callgraph
 * @callergraph
 */
CD_Set *
lcdset_lookup_with_category(WCHMM_INFO *wchmm, HMM_Logical *hmm, WORD_ID category)
{
  CD_Set *cd;

  leftcenter_name(hmm->name, wchmm->lccbuf);
  sprintf(wchmm->lccbuf2, "%s::%04d", wchmm->lccbuf, category);
  if (wchmm->lcdset_category_root != NULL) {
    cd = aptree_search_data(wchmm->lccbuf2, wchmm->lcdset_category_root);
    if (cd == NULL) return NULL;
    if (strmatch(wchmm->lccbuf2, cd->name)) {
      return cd;
    }
  }
  return NULL;
}

/** 
 * <JA>
 * @brief  単語末用カテゴリ付き pseudo phone set を生成する. 
 *
 * Julian では，ある単語に後続可能な単語集合は文法によって制限される. よって，
 * 単語末尾から次に後続しうる単語先頭音素の種類も文法によって限定
 * される. そこで，与えられた辞書上で，単語のカテゴリごとに，後続しうる先頭音素
 * をカテゴリ対情報から作成し，それらをカテゴリ付き pseudo phone set として
 * 定義して単語終端に用いることで，Julian における単語間トライフォンの
 * 近似誤差を小さくすることができる. 
 * 
 * この phone set の名前は通常の "a-k" などと異なり "a-k::38" となる
 * (数字はカテゴリID). ここでは，辞書を検索して可能なすべてのカテゴリ付き
 * pseudo phone set を，生成する. これは通常の pseudo phone set とは別に
 * 保持され，単語末端のみで使用される. 
 * 
 * @param wchmm [i/o] 木構造化辞書
 * @param hmm [in] これから登録する単語の終端の論理HMM
 * @param category [in] これから登録する単語の文法カテゴリID
 * 
 * </JA>
 * <EN>
 * @brief  Make a category-indexed context-dependent (pseudo) state set
 * for word ends.
 *
 * In Julian, the word-end pseudo triphone set can be shrinked by using the
 * category-pair constraint, since the number of possible right-context
 * phones on the word end will be smaller than all phone.  This shrinking not
 * only saves computation time but also improves recognition since the
 * approximated value will be closer to the actual value.
 * 
 * For example, if a word belongs to category ID 38 and has a phone "a-k"
 * at word end, CD_Set "a-k::38" is generated and assigned to the
 * phone instead of normal CD_Set "a-k".  The "a-k::38" set consists
 * of triphones whose right context are the beginning phones within
 * possibly fllowing categories.  These will be separated from the normal
 * pseudo phone set.
 * 
 * @param wchmm [i/o] tree lexicon
 * @param hmm [in] logical HMM at the end of a word, of which the
 * category-indexed pseudo state set will be generated.
 * @param category [in] category ID of the word.
 * 
 * </EN>
 */
static void
lcdset_register_with_category(WCHMM_INFO *wchmm, HMM_Logical *hmm, WORD_ID category)
{
  WORD_ID c2, i, w;
  HMM_Logical *ltmp;

  int cnt_c, cnt_w, cnt_p;

  if (lcdset_lookup_with_category(wchmm, hmm, category) == NULL) {
    leftcenter_name(hmm->name, wchmm->lccbuf);
    sprintf(wchmm->lccbuf2, "%s::%04d", wchmm->lccbuf, category);
    if (debug2_flag) {
      jlog("DEBUG: category-aware lcdset {%s}...", wchmm->lccbuf2);
    }
    cnt_c = cnt_w = cnt_p = 0;
    /* search for category that can connect after this category */
    for(c2=0;c2<wchmm->dfa->term_num;c2++) {
      if (! dfa_cp(wchmm->dfa, category, c2)) continue;
      /* for each word in the category, register triphone whose right context
	 is the beginning phones  */
      for(i=0;i<wchmm->dfa->term.wnum[c2];i++) {
	w = wchmm->dfa->term.tw[c2][i];
	ltmp = get_right_context_HMM(hmm, wchmm->winfo->wseq[w][0]->name, wchmm->hmminfo);
	if (ltmp == NULL) {
	  ltmp = hmm;
	  if (ltmp->is_pseudo) {
	    error_missing_right_triphone(hmm, wchmm->winfo->wseq[w][0]->name);
	  }
	}
	if (! ltmp->is_pseudo) {
	  if (regist_cdset(&(wchmm->lcdset_category_root), ltmp->body.defined, wchmm->lccbuf2, &(wchmm->lcdset_mroot))) {
	    cnt_p++;
	  }
	}
      }
      cnt_c++;
      cnt_w += wchmm->dfa->term.wnum[c2];
    }
    if (debug2_flag) {
      jlog("%d categories (%d words) can follow, %d HMMs registered\n", cnt_c, cnt_w, cnt_p);
    }
  }
}

/** 
 * <JA>
 * 全ての単語末用カテゴリ付き pseudo phone set を生成する. 
 * 辞書上のすべての単語について，その末尾に登場しうるカテゴリ付き pseudo phone
 * set を生成する（文法認識用）. 
 * 
 * @param wchmm [i/o] 木構造化辞書情報
 * </JA>
 * <EN>
 * Generate all possible category-indexed pseudo phone sets for
 * grammar recognition.
 * 
 * @param wchmm [i/o] tree lexicon
 * </EN>
 * @callgraph
 * @callergraph
 */
void
lcdset_register_with_category_all(WCHMM_INFO *wchmm)
{
  WORD_INFO *winfo;
  WORD_ID c1, w, w_prev;
  int i;
  HMM_Logical *ltmp;

  winfo = wchmm->winfo;

  /* (1) 単語終端の音素について */
  /*     word end phone */
  for(w=0;w<winfo->num;w++) {
    ltmp = winfo->wseq[w][winfo->wlen[w]-1];
    lcdset_register_with_category(wchmm, ltmp, winfo->wton[w]);
  }
  /* (2)１音素単語の場合, 先行しうる単語の終端音素を考慮 */
  /*    for one-phoneme word, possible left context should be also considered */
  for(w=0;w<winfo->num;w++) {
    if (winfo->wlen[w] > 1) continue;
    for(c1=0;c1<wchmm->dfa->term_num;c1++) {
      if (! dfa_cp(wchmm->dfa, c1, winfo->wton[w])) continue;
      for(i=0;i<wchmm->dfa->term.wnum[c1];i++) {
	w_prev = wchmm->dfa->term.tw[c1][i];
	ltmp = get_left_context_HMM(winfo->wseq[w][0], winfo->wseq[w_prev][winfo->wlen[w_prev]-1]->name, wchmm->hmminfo);
	if (ltmp == NULL) continue; /* 1音素自身のlcd_setは(1)で作成済 */
	if (ltmp->is_pseudo) continue; /* pseudo phone ならlcd_setはいらない */
	lcdset_register_with_category(wchmm, ltmp, winfo->wton[w]);
      }
    }
  }
}

/** 
 * <JA>
 * カテゴリ付き pseudo phone set をすべて消去する. この関数は Julian で文法が
 * 変更された際に，カテゴリ付き pseudo phone set を再構築するのに用いられる. 
 * 
 * @param wchmm [i/o] 木構造化辞書
 * </JA>
 * <EN>
 * Remove all the registered category-indexed pseudo state sets.
 * This function will be called when a grammar is changed to re-build the
 * state sets.
 * 
 * @param wchmm [i/o] lexicon tree information
 * </EN>
 * @callgraph
 * @callergraph
 */
void
lcdset_remove_with_category_all(WCHMM_INFO *wchmm)
{
  free_cdset(&(wchmm->lcdset_category_root), &(wchmm->lcdset_mroot));
}

#endif /* PASS1_IWCD */

/** 
 * <JA>
 * 木構造化辞書上の状態の出力確率を計算する. 
 * 
 * @param wchmm [in] 木構造化辞書情報
 * @param node [in] ノード番号
 * @param last_wid [in] 直前単語（単語先頭のトライフォン計算に用いる）
 * @param t [in] 時間フレーム
 * @param param [in] 特徴量パラメータ構造体 (@a t 番目のベクトルについて計算する)
 * 
 * @return 出力確率の対数値を返す. 
 * </JA>
 * <EN>
 * Calculate output probability on a tree lexion node.  This function
 * calculates log output probability of an input vector on time frame @a t
 * in input paramter @a param at a node on tree lexicon.
 * 
 * @param wchmm [in] tree lexicon structure
 * @param node [in] node ID to compute the output probability
 * @param last_wid [in] word ID of last word hypothesis (used when the node is
 * within the word beginning phone and triphone is used.
 * @param t [in] time frame of input vector in @a param to compute.
 * @param param [in] input parameter structure
 * 
 * @return the computed log probability.
 * </EN>
 * @callgraph
 * @callergraph
 */
LOGPROB
outprob_style(WCHMM_INFO *wchmm, int node, int last_wid, int t, HTK_Param *param)
{
  char rbuf[MAX_HMMNAME_LEN]; ///< Local workarea for HMM name conversion

#ifndef PASS1_IWCD
  
  /* if cross-word triphone handling is disabled, we simply compute the
     output prob of the state */
  return(outprob_state(wchmm->hmmwrk, t, wchmm->state[node].out, param));
  
#else  /* PASS1_IWCD */

  /* state type and context cache is considered */
  HMM_Logical *ohmm, *rhmm;
  RC_INFO *rset;
  LRC_INFO *lrset;
  CD_Set *lcd;
  WORD_INFO *winfo = wchmm->winfo;
  HTK_HMM_INFO *hmminfo = wchmm->hmminfo;

  /* the actual computation is different according to their context dependency
     handling */
  switch(wchmm->outstyle[node]) {
  case AS_STATE:
    /* normal state (word-internal or context-independent )*/
    /* compute as usual */
    return(outprob_state(wchmm->hmmwrk, t, wchmm->state[node].out.state, param));
  case AS_LSET:
    /* node in word end phone */
    /* compute approximated value using the state set in pseudo phone */
    return(outprob_cd(wchmm->hmmwrk, t, wchmm->state[node].out.lset, param));
  case AS_RSET:
    /* note in the beginning phone of word */
    /* depends on the last word hypothesis to compute the actual triphone */
    rset = wchmm->state[node].out.rset;
    /* consult cache */
    if (rset->cache.state == NULL || rset->lastwid_cache != last_wid) {
      /* cache miss...calculate */
      /* rset contains either defined biphone or pseudo biphone */
      if (last_wid != WORD_INVALID) {
	/* lookup triphone with left-context (= last phoneme) */
	if ((ohmm = get_left_context_HMM(rset->hmm, (winfo->wseq[last_wid][winfo->wlen[last_wid]-1])->name, hmminfo)) != NULL) {
	  rhmm = ohmm;
	} else {
	  /* if triphone not found, try to use the bi-phone itself */
	  rhmm = rset->hmm;
	  /* If the bi-phone is explicitly specified in hmmdefs/HMMList,
	     use it.  if both triphone and biphone not found in user-given
	     hmmdefs/HMMList, use "pseudo" phone, as same as the end of word */
	  if (debug2_flag) {
	    if (rhmm->is_pseudo) {
	    error_missing_left_triphone(rset->hmm, (winfo->wseq[last_wid][winfo->wlen[last_wid]-1])->name);
	    }
	  }
	}
      } else {
	/* if last word is WORD_INVALID try to use the bi-phone itself */
	rhmm = rset->hmm;
	/* If the bi-phone is explicitly specified in hmmdefs/HMMList,
	   use it.  if not, use "pseudo" phone, as same as the end of word */
	if (debug2_flag) {
	  if (rhmm->is_pseudo) {
	    error_missing_left_triphone(rset->hmm, (winfo->wseq[last_wid][winfo->wlen[last_wid]-1])->name);
	  }
	}
      }
      /* rhmm may be a pseudo phone */
      /* store to cache */
      if (rhmm->is_pseudo) {
	rset->last_is_lset  = TRUE;
	rset->cache.lset    = &(rhmm->body.pseudo->stateset[rset->state_loc]);
      } else {
	rset->last_is_lset  = FALSE;
	rset->cache.state   = rhmm->body.defined->s[rset->state_loc];
      }
      rset->lastwid_cache = last_wid;
    }
    /* calculate outprob and return */
    if (rset->last_is_lset) {
      return(outprob_cd(wchmm->hmmwrk, t, rset->cache.lset, param));
    } else {
      return(outprob_state(wchmm->hmmwrk, t, rset->cache.state, param));
    }
  case AS_LRSET:
    /* node in word with only one phoneme --- both beginning and end */
    lrset = wchmm->state[node].out.lrset;
    if (lrset->cache.state == NULL || lrset->lastwid_cache != last_wid) {
      /* cache miss...calculate */
      rhmm = lrset->hmm;
      /* lookup cdset for given left context (= last phoneme) */
      strcpy(rbuf, rhmm->name);
      if (last_wid != WORD_INVALID) {
	add_left_context(rbuf, (winfo->wseq[last_wid][winfo->wlen[last_wid]-1])->name);
      }
      if (wchmm->category_tree) {
#ifdef USE_OLD_IWCD
	lcd = lcdset_lookup_by_hmmname(hmminfo, rbuf);
#else
	/* use category-indexed cdset */
	if (last_wid != WORD_INVALID &&
	    (ohmm = get_left_context_HMM(rhmm, (winfo->wseq[last_wid][winfo->wlen[last_wid]-1])->name, hmminfo)) != NULL) {
	  lcd = lcdset_lookup_with_category(wchmm, ohmm, lrset->category);
	} else {
	  lcd = lcdset_lookup_with_category(wchmm, rhmm, lrset->category);
	}
#endif
      } else {
	lcd = lcdset_lookup_by_hmmname(hmminfo, rbuf);
      }
      if (lcd != NULL) {	/* found, set to cache */
	lrset->last_is_lset  = TRUE;
        lrset->cache.lset    = &(lcd->stateset[lrset->state_loc]);
        lrset->lastwid_cache = last_wid;
      } else {
	/* no relating lcdset found, falling to normal state */
	if (rhmm->is_pseudo) {
	  lrset->last_is_lset  = TRUE;
	  lrset->cache.lset    = &(rhmm->body.pseudo->stateset[lrset->state_loc]);
	  lrset->lastwid_cache = last_wid;
	} else {
	  lrset->last_is_lset  = FALSE;
	  lrset->cache.state   = rhmm->body.defined->s[lrset->state_loc];
	  lrset->lastwid_cache = last_wid;
	}
      }
      /*printf("[%s->%s]\n", lrset->hmm->name, rhmm->name);*/
    }
    /* calculate outprob and return */
    if (lrset->last_is_lset) {
      return(outprob_cd(wchmm->hmmwrk, t, lrset->cache.lset, param));
    } else {
      return(outprob_state(wchmm->hmmwrk, t, lrset->cache.state, param));
    }
  default:
    /* should not happen */
    j_internal_error("outprob_style: no outprob style??\n");
    return(LOG_ZERO);
  }

#endif  /* PASS1_IWCD */

}

/** 
 * <JA>
 * @brief  トライフォンエラーメッセージ：右コンテキスト用
 * 
 * 指定した右コンテキストを持つトライフォンが
 * 見つからなかった場合にエラーメッセージを出力する関数. 
 * 
 * @param base [in] ベースのトライフォン
 * @param rc_name [in] 右コンテキストの音素名
 * </JA>
 * <EN>
 * @brief  Triphone error message for right context.
 * 
 * Output error message when a triphone with the specified right context is
 * not defined.
 * 
 * @param base [in] base triphone
 * @param rc_name [in] name of right context phone 
 * </EN>
 * @callgraph
 * @callergraph
 */
void
error_missing_right_triphone(HMM_Logical *base, char *rc_name)
{
  char rbuf[MAX_HMMNAME_LEN]; ///< Local workarea for HMM name conversion
  /* only output message */
  strcpy(rbuf, base->name);
  add_right_context(rbuf, rc_name);
  jlog("WARNING: IW-triphone for word end \"%s\" not found, fallback to pseudo {%s}\n", rbuf, base->name);
}

/** 
 * <JA>
 * @brief  トライフォンエラーメッセージ：左コンテキスト用
 * 
 * 指定した左コンテキストを持つトライフォンが
 * 見つからなかった場合にエラーメッセージを出力する関数. 
 * 
 * @param base [in] ベースのトライフォン
 * @param lc_name [in] 左コンテキストの音素名
 * </JA>
 * <EN>
 * @brief  Triphone error message for left context.
 * 
 * Output error message when a triphone with the specified right context is
 * not defined.
 * 
 * @param base [in] base triphone
 * @param lc_name [in] name of left context phone 
 * </EN>
 * @callgraph
 * @callergraph
 */
void
error_missing_left_triphone(HMM_Logical *base, char *lc_name)
{
  char rbuf[MAX_HMMNAME_LEN]; ///< Local workarea for HMM name conversion
  /* only output message */
  strcpy(rbuf, base->name);
  add_left_context(rbuf, lc_name);
  jlog("WARNING: IW-triphone for word head \"%s\" not found, fallback to pseudo {%s}\n", rbuf, base->name);
}

/* end of file */
