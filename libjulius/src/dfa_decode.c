/**
 * @file   dfa_decode.c
 * 
 * <JA>
 * @brief  記述文法に基づく次単語予測（第２パス）
 *
 * 与えられた仮説に対して，DFA 文法上接続可能な次単語の集合を決定する. 
 * ただし実際には, 展開元仮説の予測される始端フレーム周辺の単語トレリス
 * 上に残っている単語のみが展開される. 
 *
 * 文法制約ではショートポーズは単語として記述し，そのショートポーズ単語の
 * 出現位置を文法で指定する. ただし，実際の入力ではその想定した位置に
 * 必ずしもポーズが入らないため，単語展開においては，
 * 次単語集合にショートポーズがある場合は，さらにその次の単語集合まで見て
 * 次単語集合に含める. 実際にそこにショートポーズが挿入されるかどうかは，
 * search_bestfirst_main.c で両者のスコアを比較して判断する. 
 *
 * 文法を用いる認識処理インスタンスでは，dfa_firstwords(), dfa_nextwords(),
 * dfa_acceptable(), dfa_eosscore() が第2パスのメイン関数 wchmm_fbs() から
 * 使用される. なお N-gram を用いる認識処理インスタンスでは，
 * 代わりに ngram_decode.c 内の関数が使われる. 
 * </JA>
 * 
 * <EN>
 * @brief  Grammar-based word prediction (2nd pass)
 *
 * Given a part-of-sentence hypothesis, these function determine a set of next
 * words allowed to be connected by the grammar.  Actually, only words in the
 * word trellis, which exist around the estimated word-end frame will be
 * expanded.
 *
 * When using DFA grammar, the possible (short) pause insertion point
 * should be explicitly specified in grammar, by defining "short-pause
 * word" in vocabulary and write its appearance in grammar.  Since a
 * short pause will not always appear on the specified point, Julius
 * will consider the skipping of such short pause word for the next
 * word prediction in these functions.  Whether short pause was
 * actually inserted or not in the user input will be determined by
 * score in search_bestfirst_main.c.
 *
 * In recognition process instance with DFA grammar, dfa_firstwords(),
 * dfa_nextwords(), dfa_acceptable() and dfa_eosscore() will be called
 * from main search function wchmm_fbs().  When using N-gram, on the
 * other hand, the corresponding functions in ngram_decode.c will be
 * used instead.  </EN>
 * 
 * @author Akinobu LEE
 * @date   Mon Mar  7 15:31:00 2005
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
 * <JA>
 * 文法にしたがって，文頭に接続しうる単語を最初の予測単語群として返す. 
 * 
 * @param nw [out] 次単語集合の格納先へのポインタ
 * @param peseqlen [in] 入力フレーム長
 * @param maxnw [in] @a nw の許される最大長
 * @param r [in] 認識プロセスインスタンス
 * 
 * @return 予測された単語数 (ただしエラー時は -1 を返す)
 * </JA>
 * <EN>
 * Return initial word set from grammar.
 * 
 * @param nw [out] pointer to hold the resulting next word set
 * @param peseqlen [in] input frame length
 * @param maxnw [in] maximum number of words that can be set in @a nw
 * @param r [in] recognition process instance
 * 
 * @return the number of predicted words, or -1 on error.
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
int
dfa_firstwords(NEXTWORD **nw, int peseqlen, int maxnw, RecogProcess *r)
{
  DFA_INFO *dfa;
  DFA_ARC *arc;
  MULTIGRAM *m;
  int s, sb, se;
  int cate, iw, ns;
  int num = 0;

  dfa = r->lm->dfa;

  for (m = r->lm->grammars; m; m = m->next) {
    if (m->active) {
      sb = m->state_begin;
      se = sb + m->dfa->state_num;
      for(s=sb;s<se;s++) {
	if ((dfa->st[s].status & INITIAL_S) != 0) { /* from initial state */
	  for (arc = dfa->st[s].arc; arc; arc = arc->next) {	/* for all arc */
	    cate = arc->label;	/* category ID */
	    ns = arc->to_state;	/* next DFA state ID */
	    /* all words within the category is expanded */
	    for (iw=0;iw<dfa->term.wnum[cate];iw++) {
	      nw[num]->id = dfa->term.tw[cate][iw]; /* word ID */
	      nw[num]->next_state = ns; /* next state */
	      nw[num]->can_insert_sp = FALSE; /* short pause should not inserted before this word */
#ifdef FIX_PENALTY
	      nw[num]->lscore = 0.0;
#else
	      nw[num]->lscore = r->config->lmp.penalty2;
#ifdef CLASS_NGRAM
	      /* add per-word penalty */
	      nw[num]->lscore += r->wchmm->winfo->cprob[nw[num]->id];
#endif
#endif
	      num++;
	      if (num >= maxnw) return -1; /* buffer overflow */
	    }
	  }
	}
      }
    }
  }

  return num;
}

/** 
 * <JA>
 * 部分文仮説に対して，文法に従って次に接続しうる単語群を返す. 
 * 
 * @param hypo [in] 展開元の部分文仮説
 * @param nw [out] 次単語集合の格納先へのポインタ
 * @param maxnw [in] @a nw の許される最大長
 * @param r [in] 認識プロセスインスタンス
 * 
 * @return 予測された単語数 (ただしエラー時は -1 を返す)
 * </JA>
 * <EN>
 * Given a part-of-sentence hypothesis, returns the next word set defined
 * by DFA grammar.
 * 
 * @param hypo [in] the source part-of-sentene hypothesis
 * @param nw [out] pointer to hold the resulting next word set
 * @param maxnw [in] maximum number of words that can be set in @a nw
 * @param r [in] recognition process instance
 * 
 * @return the number of predicted words, or -1 on error.
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
int
dfa_nextwords(NODE *hypo, NEXTWORD **nw, int maxnw, RecogProcess *r)
{
  DFA_INFO *dfa;
  DFA_ARC *arc, *arc2;
  int iw,cate,ns,cate2,ns2;
  int num = 0;

  dfa = r->lm->dfa;

  /* hypo->state: current DFA state ID */
  for (arc = dfa->st[hypo->state].arc; arc; arc = arc->next) {/* for all arc */
    cate = arc->label;
    ns = arc->to_state;
    if (dfa->is_sp[cate]) {	/* short pause */
      /* expand one more next (not expand the short pause word itself) */
      for (arc2 = dfa->st[ns].arc; arc2; arc2 = arc2->next) {
	cate2 = arc2->label;
	ns2 = arc2->to_state;
	for (iw=0;iw<dfa->term.wnum[cate2];iw++) {
	  nw[num]->id = dfa->term.tw[cate2][iw];
	  nw[num]->next_state = ns2;
	  nw[num]->can_insert_sp = TRUE;
	  nw[num]->lscore = r->config->lmp.penalty2;
#ifdef CLASS_NGRAM
	  /* add per-word penalty */
	  nw[num]->lscore += r->wchmm->winfo->cprob[nw[num]->id];
#endif
	  num++;
	  if (num >= maxnw) return -1; /* buffer overflow */
	}
      }
    } else {			/* not short pause */
      /* all words within the category is expanded */
      for (iw=0;iw<dfa->term.wnum[cate];iw++) {
	nw[num]->id = dfa->term.tw[cate][iw];
	nw[num]->next_state = ns;
	nw[num]->can_insert_sp = FALSE;
	nw[num]->lscore = r->config->lmp.penalty2;
#ifdef CLASS_NGRAM
	/* add per-word penalty */
	nw[num]->lscore += r->wchmm->winfo->cprob[nw[num]->id];
#endif
	num++;
	if (num >= maxnw) return -1; /* buffer overflow */
      }
    }
  }
  return num;
}


/** 
 * <JA>
 * 部分文仮説が文法上文として最終(受理可能)状態にあるかどうかを返す. 
 * 
 * @param hypo [in] 部分文仮説
 * @param r [in] 認識プロセスインスタンス
 * 
 * @return 受理可能状態にあるとき TRUE 受理不可能なとき FALSE
 * </JA>
 * <EN>
 * Return whether the hypothesis is currently on final state
 * 
 * @param hypo [in] sentence hypothesis
 * @param r [in] recognition process instance
 * 
 * @return TRUE when on final state, or FALSE if not acceptable.
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
boolean
dfa_acceptable(NODE *hypo, RecogProcess *r)
{
  if (r->lm->dfa->st[hypo->state].status & ACCEPT_S) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/* patch by kashima */
/** 
 * <JA>
 * 次単語候補がその推定された接続予測点の前後の単語トレリス上に
 * あるかどうかをチェックし，もしあればそのトレリス単語へのポインタをセット
 * する. なお最尤の接続点はあとで決まるので，ここでは最適なトレリス単語
 * でなくてよい. 
 * 
 * @param nword [i/o] 次単語候補 (対応するトレリス単語へのポインタが
 * セットされる)
 * @param hypo [in] 展開元仮説
 * @param r [in] 認識プロセスインスタンス
 * 
 * @return 単語トレリス上の予測位置付近に次単語が存在すれば TRUE，存在
 * しなければ FALSE を返す. 
 * </JA>
 * <EN>
 * Check if the given nextword exists in the word trellis around the
 * estimated connection time.  If exist, set the pointer to the corresponding
 * trellis word to the nextword.  Since the best connection time will be
 * re-computed later, it need not to be an optimal one.
 * 
 * @param nword [i/o] next word candidate (pointer to the found trellis word
 * will be set)
 * @param hypo [in] source part-of-sentence hypothesis
 * @param r [in] recognition process instance
 * 
 * @return TRUE if the nextword exists on the word trellis around the estimated
 * connection point, or FALSE if not exist.
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
boolean
dfa_look_around(NEXTWORD *nword, NODE *hypo, RecogProcess *r)
{
  int t,tm;
  int i;
  WORD_ID w;
  BACKTRELLIS *bt;
  int lookup_range;

  bt = r->backtrellis;
  lookup_range = r->config->pass2.lookup_range;
  
  tm = hypo->estimated_next_t;	/* estimated connection time */

  /* look aound [tm-lookup_range..tm+lookup_range] frame */
  /* near the center is better:
     1. the first half (backward)   2. the second half (forward) */
  /* 1. backward */
  for(t = tm; t >= tm - lookup_range; t--) {
    if (t < 0) break;
     for (i=0;i<bt->num[t];i++) {
       w = (bt->rw[t][i])->wid;
       if(w == nword->id){	/* found */
         nword->tre = bt->rw[t][i];
         return TRUE;
       }
     }
  }
  /* 2. forward */
  for(t = tm + 1; t < tm + lookup_range; t++) {
    if (t > bt->framelen - 1) break;
    if (t >= hypo->bestt) break;
    for (i=0;i<bt->num[t];i++) {
      w = (bt->rw[t][i])->wid;
      if(w == nword->id){	/* found */
        nword->tre = bt->rw[t][i];
        return TRUE;
      }
    }
  }

  return FALSE;			/* not found */
}

/* end of file */
