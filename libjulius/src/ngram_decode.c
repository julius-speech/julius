/**
 * @file   ngram_decode.c
 * 
 * <JA>
 * @brief  N-gram確率に基づく次単語予測（第2パス） 
 *
 * Julius のN-gramを用いたスタックデコーディング(第2パス)において，
 * 次に接続しうる単語の集合を決定する. 
 * 
 * 与えられた展開元仮説の始端フレームを予測し，単語トレリス上で
 * その予測フレーム周辺に終端が存在する単語の集合を，
 * そのN-gram出現確率とともに返す. 
 *
 * Julius では ngram_firstwords(), ngram_nextwords(), ngram_acceptable() が
 * それぞれ第2パスのメイン関数 wchmm_fbs() から呼び出される. なお，
 * Julian ではこれらの関数の代わりに dfa_decode.c の関数が用いられる. 
 * </JA>
 * 
 * <EN>
 * @brief N-gram based word prediction for the 2nd pass.
 *
 * These functions returns next word candidates in the 2nd recognition
 * pass of Julius, i.e. N-gram based stack decoding.
 *
 * Given a partial sentence hypothesis, it first estimate the beginning frame
 * of the hypothesis based on the word trellis.  Then the words in the word
 * trellis around the estimated frame are extracted from the word trellis.
 * They will be returned with their N-gram probabilities.
 *
 * In Julius, ngram_firstwords(), ngram_nextwords() and ngram_acceptable()
 * are called from main search function wchmm_fbs().  In Julian,
 * corresponding functions in dfa_decode.c will be used instead.
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Fri Jul  8 14:57:51 2005
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
   
#include <julius/julius.h>

/** 
 * <JA>
 * 次単語候補ソート用 qsort コールバック関数. 
 * 
 * @param a [in] 要素1
 * @param b [in] 要素2
 * 
 * @return aの単語ID > bの単語ID なら1, 逆なら -1, 同じなら 0 を返す. 
 * </JA>
 * <EN>
 * qsort callback function to sort next word candidates by their word ID.
 * 
 * @param a [in] element 1
 * @param b [in] element 2
 * 
 * @return 1 if word id of a > that of b, -1 if negative, 0 if equal.
 * </EN>
 */
static int
compare_nw(NEXTWORD **a, NEXTWORD **b)
{
  if ((*a)->id > (*b)->id) return 1;
  if ((*a)->id < (*b)->id) return -1;
  return 0;
}

/** 
 * <JA>
 * 次単語候補リスト内から単語を検索する. 
 * 
 * @param nw [in] 次単語候補リスト
 * @param w [in] 検索する単語のID
 * @param num [in] 次単語候補リストの長さ
 * 
 * @return 見つかった場合その次単語候補構造体へのポインタ，見つからなければ
 * NULL を返す. 
 * </JA>
 * <EN>
 * Find a word from list of next word candidates.
 * 
 * @param nw [in] list of next word candidates
 * @param w [in] word id to search for
 * @param num [in] length of @a nw
 * 
 * @return the pointer to the NEXTWORD data if found, or NULL if not found.
 * </EN>
 */
/* find next word candiate whose id 'w' */
static NEXTWORD *
search_nw(NEXTWORD **nw, WORD_ID w, int num)
{
  int left,right,mid;
  NEXTWORD *tmp;

  if (num == 0) return NULL;
  left = 0;
  right = num - 1;
  while (left < right) {
    mid = (left + right) / 2;
    if ((nw[mid])->id < w) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  tmp = nw[left];
  if (tmp->id == w) {
    return tmp;
  } else {
    return NULL;
  }
}

/** 
 * <EN>
 * Compute backward N-gram score from forward N-gram.
 * </EN>
 * <JA>
 * 後向きの N-gram スコアを前向き N-gram から算出する. 
 * </JA>
 * 
 * @param ngram [in] N-gram data structure
 * @param w [in] word sequence
 * @param wlen [in] length of @a w
 * 
 * @return the backward probability of the word w[0].
 * 
 */
static LOGPROB
ngram_forw2back(NGRAM_INFO *ngram, WORD_ID *w, int wlen)
{
  int i;
  LOGPROB p1, p2;

  p1 = 0.0;
  for(i = 1; i < ngram->n; i++) {
    if (i >= wlen) break;
    p1 += ngram_prob(ngram, i, &(w[1]));
  }
  p2 = 0.0;
  for(i = 0; i < ngram->n; i++) {
    if (i >= wlen) break;
    p2 += ngram_prob(ngram, i+1, w);
  }

  return(p2 - p1);
}

/** 
 * <JA>
 * @brief  単語トレリスから次単語候補を抽出する. 
 *
 * 単語トレリス上の指定したフレーム上に終端が存在するトレリス単語
 * のリストを抽出し，それらの次単語としての N-gram 接続確率を計算する. 
 * そのリストを次単語情報構造体に追加して返す. 
 * 
 * @param r [in] 認識処理インスタンス
 * @param nw [i/o] 次単語候補リスト（抽出結果は @a oldnum 以降に追加される）
 * @param oldnum [in] @a nw にすでに格納されている次単語の数
 * @param hypo [in] 展開元の文仮説
 * @param t [in] 指定フレーム
 * 
 * @return 抽出リストを追加したあとの @a nw に含まれる次単語の総数. 
 * </JA>
 * <EN>
 * @brief  Extract next word candidates from word trellis.
 *
 * This function extracts the list of trellis words whose word end
 * has survived in the word trellis at the specified frame.
 * The N-gram probabilities of them are then computed and added to
 * the current next word candidates data.
 * 
 * @param r [in] recognition process instance
 * @param nw [in] list of next word candidates (new words will be appended at @a oldnum)
 * @param oldnum [in] number of words already stored in @a nw
 * @param hypo [in] the source sentence hypothesis
 * @param t [in] specified frame
 * 
 * @return the total number of words currently stored in the @a nw.
 * </EN>
 */
static int
pick_backtrellis_words(RecogProcess *r, NEXTWORD **nw, int oldnum, NODE *hypo, short t)
{
  int i;
  WORD_ID w;
  LOGPROB rawscore;
#ifdef WPAIR
  int w_old = WORD_INVALID;
#endif
  int num;
  int cnnum;		///< Num of found non-transparent words (<=2)
  int last_trans;		///< Num of skipped transparent words
  StackDecode *dwrk;

  BACKTRELLIS *bt;
  WORD_INFO *winfo;
  NGRAM_INFO *ngram;
  LOGPROB lm_weight2, lm_penalty2, lm_penalty_trans;

  num = oldnum;
  bt = r->backtrellis;
  winfo = r->lm->winfo;
  ngram = r->lm->ngram;
  lm_weight2 = r->config->lmp.lm_weight2;
  lm_penalty2 = r->config->lmp.lm_penalty2;
  lm_penalty_trans = r->config->lmp.lm_penalty_trans;
  dwrk = &(r->pass2);

  /* set word contexts to cnword[] from 1 considering transparent words */
  if (ngram) {
    cnnum = 0;
    last_trans = 0;
    for(i=hypo->seqnum-1;i>=0;i--) {
      if (! winfo->is_transparent[hypo->seq[i]]) {
	dwrk->cnword[cnnum+1] = hypo->seq[i];
	cnnum++;
	if (cnnum >= ngram->n - 1) break;
      } else {
	last_trans++;
      }
    }
    if (ngram->dir == DIR_RL) {
      for(i=0;i<cnnum;i++) {
	dwrk->cnwordrev[cnnum-1-i] = dwrk->cnword[i+1];
      }
    }
    /* use ngram id */
    if (ngram->dir == DIR_RL) {
      for(i=0;i<cnnum;i++) dwrk->cnwordrev[i] = winfo->wton[dwrk->cnwordrev[i]];
    } else {
      for(i=0;i<cnnum;i++) dwrk->cnword[i+1] = winfo->wton[dwrk->cnword[i+1]];
    }
  }

  /* lookup survived words in backtrellis on time frame 't' */
  for (i=0;i<bt->num[t];i++) {
    w = (bt->rw[t][i])->wid;
#ifdef WORD_GRAPH
    /* only words on the word graphs are expanded */
    if (!(bt->rw[t][i])->within_wordgraph) continue;
#endif /* not WORD_GRAPH */
#ifdef WPAIR
    /* some word have same word ID with different previous word, so
       only one will be opened (best word will be selected later
       by next_word() */
    if (w == w_old) continue;	/* backtrellis is sorted by word ID */
    else w_old = w;
#endif /* WPAIR */
    /* skip if already exist */
    if (search_nw(nw, w, oldnum) != NULL) continue;

    /* compute LM probability of the word */
    if (ngram) {
      /* compute N-gram probability */
      if (ngram->dir == DIR_RL) {
	/* just compute N-gram prob of the word candidate */
	dwrk->cnwordrev[cnnum] = winfo->wton[w];
	rawscore = ngram_prob(ngram, cnnum + 1, dwrk->cnwordrev);
      } else {
	dwrk->cnword[0] = winfo->wton[w];
	rawscore = ngram_forw2back(ngram, dwrk->cnword, cnnum + 1);
      }
#ifdef CLASS_NGRAM
      rawscore += winfo->cprob[w];
#endif
    }
    if (r->lmvar == LM_NGRAM_USER) {
      /* call user-defined function */
      /* be careful that the word context is ordered in backward direction */
      rawscore = (*(r->lm->lmfunc.lmprob))(winfo, hypo->seq, hypo->seqnum, w, rawscore);
    }

    nw[num]->tre   = bt->rw[t][i];
    nw[num]->id    = w;
    nw[num]->lscore = rawscore * lm_weight2 + lm_penalty2;
    if (winfo->is_transparent[w]) {
      /*nw[num]->lscore -= (LOGPROB)last_trans * TRANS_RENZOKU_PENALTY;*/
      if (winfo->is_transparent[hypo->seq[hypo->seqnum-1]]) {
	nw[num]->lscore += lm_penalty_trans;
      }
    }
    
    /* j_printf("%d: %s added\n", num, winfo->wname[nw[num]->id]); */
    num++;
  }

  return num;
}

/** 
 * <JA>
 * @brief  単語トレリスから次単語集合を決定する. 
 *
 * 指定フレームの前後 lookup_range 分に終端があるトレリス上の単語を集め，
 * 次単語構造体を構築する. 同じ単語が上記の範囲内に複数ある場合，
 * 指定フレームにもっとも近いトレリス上の単語が選択される. 
 * 
 * @param r [in] 認識処理インスタンス
 * @param nw [out] 次単語集合を格納する構造体へのポインタ
 * @param hypo [in] 展開元の部分文仮説
 * @param tm [in] 単語を探す中心となる指定フレーム
 * @param t_end [in] 単語を探すフレームの右端
 * 
 * @return @a nw に格納された次単語候補の数を返す. 
 * </JA>
 * <EN>
 * @brief  Determine next word candidates from the word trellis.
 *
 * This function builds a list of next word candidates by looking up
 * the word trellis at specified frame, with lookup_range frame margin.
 * If the same words exists in the near frames, only the one nearest to the
 * specified frame will be chosen.
 * 
 * @param r [in] recognition process instance
 * @param nw [out] pointer to hold the extracted words as list of next word candidates
 * @param hypo [in] partial sentence hypothesis from which the words will be expanded
 * @param tm [in] center time frame to look up the words
 * @param t_end [in] right frame boundary for the lookup.
 * 
 * @return the number of next words candidates stored in @a nw.
 * </EN>
 */
static int
get_backtrellis_words(RecogProcess *r, NEXTWORD **nw, NODE *hypo, short tm, short t_end)
{
  int num = 0;
  int t, t_step;
  int oldnum=0;

  BACKTRELLIS *bt;
  int lookup_range;

  if (tm < 0) return(0);

  bt = r->backtrellis;
  lookup_range = r->config->pass2.lookup_range;

#ifdef PREFER_CENTER_ON_TRELLIS_LOOKUP
  /* fix for 3.2 (01/10/18 by ri) */
  /* before and after (one near center frame has high priority) */
  for (t_step = 0; t_step < lookup_range; t_step++) {
    /* before or center */
    t = tm - t_step;
    if (t < 0 || t > bt->framelen - 1 || t >= t_end) continue;
    num = pick_backtrellis_words(r, nw, oldnum, hypo, t);
    if (num > oldnum) {
      qsort(nw, num, sizeof(NEXTWORD *),
	    (int (*)(const void *,const void *))compare_nw);
      oldnum = num;
    }
    if (t_step == 0) continue;	/* center */
    /* after */
    t = tm + t_step;
    if (t < 0 || t > bt->framelen - 1 || t >= t_end) continue;
    num = pick_backtrellis_words(r, nw, oldnum, hypo, t);
    if (num > oldnum) {
      qsort(nw, num, sizeof(NEXTWORD *),
	    (int (*)(const void *,const void *))compare_nw);
      oldnum = num;
    }
  }

#else

  /* before the center frame */
  for(t = tm; t >= tm - lookup_range; t--) {
    if (t < 0) break;
    num = pick_backtrellis_words(r, nw, oldnum, hypo, t);
    if (num > oldnum) {
      qsort(nw, num, sizeof(NEXTWORD *),
	    (int (*)(const void *,const void *))compare_nw);
      oldnum = num;
    }
  }
  /* after the center frame */
  for(t = tm + 1; t < tm + lookup_range; t++) {
    if (t > bt->framelen - 1) break;
    if (t >= t_end) break;
    num = pick_backtrellis_words(r, nw, oldnum, hypo, t);
    if (num > oldnum) {
      qsort(nw, num, sizeof(NEXTWORD *),
	    (int (*)(const void *,const void *))compare_nw);
      oldnum = num;
    }
  }
#endif

  return num;
}

/** 
 * <JA>
 * @brief  非展開単語を除去. 
 * 
 * 制約により展開対象とならない単語をリストから消去する. 
 * 
 * @param nw [i/o] 次単語集合（集合中の展開できない単語が消去される）
 * @param hypo [in] 展開元の部分文仮説
 * @param num [in] @a nw に現在格納されている単語数
 * @param winfo [in] 単語辞書
 * 
 * @return 新たに nw に含まれる次単語数
 * </JA>
 * <EN>
 * @brief  Remove non-expansion word from list.
 * 
 * Remove words in the nextword list which should not be expanded.
 * 
 * @param nw [i/o] list of next word candidates (will be shrinked by removing some words)
 * @param hypo [in] partial sentence hypothesis from which the words will be expanded
 * @param num [in] current number of next words in @a nw
 * @param winfo [in] word dictionary
 * 
 * @return the new number of words in @a nw
 * </EN>
 */
static int
limit_nw(NEXTWORD **nw, NODE *hypo, int num, WORD_INFO *winfo)
{
  int src,dst;
  int newnum;

  /* <s>からは何も展開しない */
  /* no hypothesis will be generated after "<s>" */
  if (hypo->seq[hypo->seqnum-1] == winfo->head_silwid) {
    return(0);
  }

  dst = 0;
  for (src=0; src<num; src++) {
    if (nw[src]->id == winfo->tail_silwid) {
      /* </s> は展開しない */
      /* do not expand </s> (it only appears at start) */
      continue;
    }
#ifdef FIX_35_INHIBIT_SAME_WORD_EXPANSION
    /* 直前単語と同じトレリス単語は展開しない */
    /* inhibit expanding the exactly the same trellis word twice */
    if (nw[src]->tre == hypo->tre) continue;
#endif
    
    if (src != dst) memcpy(nw[dst], nw[src], sizeof(NEXTWORD));
    dst++;
  }
  newnum = dst;

  return newnum;
}
	

/** 
 * <JA>
 * @brief  初期単語仮説集合を求める. 
 *
 * N-gramベースの探索では，初期仮説は単語末尾の無音単語に固定されている. 
 * ただし，ショートポーズセグメンテーション時は，第1パスで最終フレームに終端が
 * 残った単語の中で尤度最大の単語となる. 
 * 
 * @param nw [out] 次単語候補リスト（得られた初期単語仮説を格納する）
 * @param peseqlen [in] 入力フレーム長
 * @param maxnw [in] @a nw に格納できる単語の最大数
 * @param r [in] 認識処理インスタンス
 * 
 * @return @a nw に格納された単語候補数を返す. 
 * </JA>
 * <EN>
 * @brief  Get initial word hypotheses at the beginning.
 *
 * on N-gram based recogntion, the initial hypothesis is fixed to the tail
 * silence word.  Exception is that, in short-pause segmentation mode, the
 * initial hypothesis will be chosen from survived words on the last input
 * frame in the first pass.
 * 
 * @param nw [out] pointer to hold the initial word candidates
 * @param peseqlen [in] input frame length
 * @param maxnw [in] maximum number of words that can be stored in @a nw
 * @param r [in] recognition process instance
 * 
 * @return the number of words extracted and stored to @a nw.
 * </EN>
 *
 * @callgraph
 * @callergraph
 */
int
ngram_firstwords(NEXTWORD **nw, int peseqlen, int maxnw, RecogProcess *r)
{

  if (r->config->successive.enabled) {
    /* in sp segment mode  */
    if (r->sp_break_2_begin_word != WORD_INVALID) {
      /* 初期仮説は 最終フレームに残った単語トレリス上の最尤単語 */
      /* the initial hypothesis is the best word survived on the last frame of
	 the segment */
      nw[0]->id = r->sp_break_2_begin_word;
    } else {
      /* 最終セグメント: 初期仮説は 単語の末尾の無音単語(=winfo->tail_silwid) */
      /* we are in the last of sentence: initial hypothesis is word-end silence word */
      nw[0]->id = r->lm->winfo->tail_silwid;
    }
  } else {
    /* initial hypothesis should be word-end silence word */
    nw[0]->id = r->lm->winfo->tail_silwid;
  }

  nw[0]->lscore = uni_prob(r->wchmm->ngram, r->wchmm->winfo->wton[nw[0]->id]);
#ifdef CLASS_NGRAM
  nw[0]->lscore += r->wchmm->winfo->cprob[nw[0]->id];
#endif
  nw[0]->lscore *= r->config->lmp.lm_weight2;
#ifndef FIX_PENALTY
  nw[0]->lscore += r->config->lmp.lm_penalty2;
#endif

  return 1;			/* number of words = 1 */
}

/** 
 * <JA>
 * @brief 次単語仮説集合を返す. 
 *
 * 与えられた部分文仮説から，次に接続しうる単語の集合を返す. 実際には，
 * 第1パスの結果であるトレリス単語集合 bt 上で，展開元の部分文仮説の最終単語の
 * （推定された）始端フレーム hypo->estimated_next_t の前後に存在する
 * 単語集合を取出し，それらの N-gram 接続確率を計算して返す. 
 * 取り出された次単語仮説は，あらかじめ maxnm の長さだけ
 * 領域が確保されている nw に格納される. 
 * 
 * @param hypo [in] 展開元の文仮説
 * @param nw [out] 次単語候補リストを格納する領域へのポインタ
 * @param maxnw [in] @a nw の最大長
 * @param r [in] 認識処理インスタンス
 * 
 * @return 抽出され nw に格納された次単語仮説の数を返す. 
 * </JA>
 * <EN>
 * @brief  Return the list of next word candidate.
 *
 * Given a partial sentence hypothesis "hypo", it returns the list of
 * next word candidates.  Actually, it extracts from word trellis the
 * list of words whose word-end node has survived near the estimated
 * beginning-of-word frame of last word "hypo->estimated_next_t", and store
 * them to "nw" with their N-gram probabilities. 
 * 
 * @param hypo [in] source partial sentence hypothesis
 * @param nw [out] pointer to store the list of next word candidates (should be already allocated)
 * @param maxnw [in] maximum number of words that can be stored to @a nw
 * @param r [in] recognition process instance
 * 
 * @return the number of extracted next word candidates in @a nw.
 * </EN>
 * @callgraph
 * @callergraph
 */
int
ngram_nextwords(NODE *hypo, NEXTWORD **nw, int maxnw, RecogProcess *r)
{
  int num, num2;

  if (hypo->seqnum == 0) {
    j_internal_error("ngram_nextwords: hypo contains no word\n");
  }

  /* 仮説の推定終端時刻において backtrellis内に残っている単語を得る */
  /* get survived words on backtrellis at the estimated end frame */
  num = get_backtrellis_words(r, nw, hypo, hypo->estimated_next_t, hypo->bestt);

  /* 展開できない単語をチェックして外す */
  /* exclude unallowed words */
  num2 = limit_nw(nw, hypo, num, r->lm->winfo);

  if (debug2_flag) jlog("DEBUG: ngram_decode: %d-%d=%d unfolded\n",num, num-num2,num2);

  return(num2);
}

/** 
 * <JA>
 * @brief  受理判定
 * 
 * 与えられた部分文仮説が，文（すなわち探索終了）として
 * 受理可能であるかどうかを返す. N-gram では文頭に対応する無音単語
 * (silhead) であれば受理する. 
 * 
 * @param hypo [in] 部分文仮説
 * @param r [in] 認識処理インスタンス
 * 
 * @return 文として受理可能であれば TRUE，不可能なら FALSE を返す. 
 * </JA>
 * <EN>
 * @brief  Acceptance check.
 * 
 * Return whether the given partial hypothesis is acceptable as a sentence
 * and can be treated as a final search candidate.  In N-gram mode, it checks
 * whether the last word is the beginning-of-sentence silence (silhead).
 * 
 * @param hypo [in] partial sentence hypothesis to be examined
 * @param r [in] recognition process instance
 * 
 * @return TRUE if acceptable as a sentence, or FALSE if not.
 * </EN>
 * @callgraph
 * @callergraph
 */
boolean
ngram_acceptable(NODE *hypo, RecogProcess *r)
{

  if (r->config->successive.enabled) {
    /* 最後の仮説が第１パス最尤仮説の最初の単語と一致しなければならない */
    /* the last word should be equal to the first word on the best hypothesis on 1st pass */
    if (hypo->seq[hypo->seqnum-1] == r->sp_break_2_end_word) {
      return TRUE;
    }
  } else {
    /* 最後の仮説が文頭無音単語でなければならない */
    /* the last word should be head silence word */
    if (hypo->seq[hypo->seqnum-1] == r->lm->winfo->head_silwid) {
      return TRUE;
    }
  }
  return FALSE;
}

/* end of file */
