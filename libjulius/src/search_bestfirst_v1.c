/**
 * @file   search_bestfirst_v1.c
 * 
 * <JA>
 * @brief  第2パスのViterbi演算および仮説スコア計算 (高速版)
 *
 * ここでは，第2パスにおいて探索中の仮説のViterbiスコアの更新演算，
 * 次単語とのトレリス接続，および仮説のスコア計算を行う関数が定義されて
 * います. 
 *
 * 単語接続部の単語間音素環境依存性は，高速な backscan アルゴリズムに
 * よって行われます. このファイルで定義されている関数は，config.h において
 * PASS2_STRICT_IWCD が undef であるときに使用されます. 逆に上記が define
 * されているときは，search_bestfirst_v2.c の関数が用いられます. 
 *
 * Backscan アルゴリズムでは，デコーディングの高速化のため，
 * 次単語とその前の単語の接続点について，「単語間音素コンテキストの遅延処理」
 * を行ないます：
 * 
 *  -# 新仮説の生成(next_word())では，次単語の最後の音素の右コンテキスト
 *     のみが考慮される. 
 *  -# その単語間の完全な音素環境依存性は，その仮説がいったんスタックに
 *     入った後もう一度 POP されたときに scan_word() にて改めて計算する. 
 *
 * 仮説生成時にはすべての生成仮説に対して依存計算を行なず，あとでスコアが
 * 高く POP された仮説についてのみ再計算を行ないます. このため処理が
 * 高速化されますが，仮説スコア計算(next_word())において次単語接続部分の
 * 環境依存性が考慮されないので, 探索中のスコアに誤差が生じる場合があります. 
 *
 * 実装について:
 * 
 *  -# next_word() では，次単語の最後の音素のみを右コンテキスト(=展開元
 *     単語の最初の音素)を考慮して変化させ，トレリス接続点の出力確率を求める. 
 *  -# scan_word() では，新単語部分ともう１つ前の単語の最初の音素を変化
 *     させ，scan する. そのため新単語部分だけでなく，そのもう一音素前まで
 *     scan の対象となる. この "1-phoneme backscan" を行なうため,
 *     各仮説ノードは最終HMM状態の前向きスコア (NODEにおける g[]) だけでなく，
 *     その backscan 開始点(もう１つ前の単語の最初の音素の手前)のスコア
 *     も保存しておく必要がある (NODE における g_prev[]). 
 *
 * なお，１音素のみからなる単語では backscan 開始点と単語境界が重なることを
 * 考慮する必要があるため，実装はもう少し複雑になる. 
 * </JA>
 * 
 * <EN>
 * @brief  Viterbi path update and scoring on the second pass (fast version)
 *
 * This file has functions for score calculations on the 2nd pass.
 * It includes Viterbi path update calculation of a hypothesis, calculations
 * of scores and word trellis connection at word expansion.
 * 
 * The cross-word triphone will be computed not at word expansion time,
 * but at later pop up for rapid decoding. This is called "backscan"
 * altgorithm. These functions are enabled when PASS2_STRICT_IWCD is
 * UNDEFINED in config.h.  If defined, "nextscan" functions in
 * search_bestfirst_v2.c are used instead.
 *
 * Here we use "delayed cross-word context handling" method
 * for connection of next word and last word of the hypothesis for
 * speeding up decoding:
 *
 *  -# Only right context of the tail phone in the next word is considered
 *     when generating a new hypothesis (next_word()).
 *     
 *  -# The whole context dependency will be fully computed when the
 *     hypothesis is once pushed to stack and later popped in scan_word().
 *
 * This method avoid computing full context-dependency handling for all
 * generated hypothesis in next_word(), and only re-compute it after
 * primising ones are popped from stack later.  This speeds up decoding.
 * But since the context dependency is not considered in the total hypothesis
 * score (computed in next_word()).
 *
 * The actual implimentation:
 * 
 *  -# In nextword(), the tail phone in the new word is modified considering
 *     the right context (= head phone in the last word of source hypothesis),
 *     and the outprob on the connection point between backtrellis and forward
 *     trellis is computed using the triphone.
 *     
 *  -# In scan_word(), not only the new word but also the head phone in the
 *     previous word should be modified and re-scanned.
 *     To realize this '1-phoneme backscan' procedure, hypothesis nodes
 *     have to keep forward scores not only at the last HMM state (g[] in
 *     NODE), but also at the backscan restart point (= before the head
 *     phone in the previous word, g_prev[] in NODE).
 *
 * Note that the actual implementation becomes a little more complicated
 * to handle 1-phoneme words...
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Sun Sep 11 23:54:53 2005
 *
 * $Revision: 1.10 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius/julius.h>

#ifndef PASS2_STRICT_IWCD

#undef TCD			///< Define if want triphone debug messages

/**********************************************************************/
/************ 仮説ノードの基本操作                         ************/
/************ Basic functions for hypothesis node handling ************/
/**********************************************************************/

#undef STOCKER_DEBUG

#ifdef STOCKER_DEBUG
static int stocked_num = 0;
static int reused_num = 0;
static int new_num = 0;
static int request_num = 0;
#endif

/** 
 * <JA>
 * 仮説ノードを実際にメモリ上から解放する. 
 * 
 * @param node [in] 仮説ノード
 * </JA>
 * <EN>
 * Free a hypothesis node actually.
 * 
 * @param node [in] hypothesis node
 * </EN>
 */
static void
free_node_exec(NODE *node)
{
  if (node == NULL) return;

  free(node->g);
  if (node->g_prev != NULL) free(node->g_prev);
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  if (node->region->graphout) {
    free(node->wordend_frame);
    free(node->wordend_gscore);
  }
#endif

  free(node);
}

/** 
 * <JA>
 * 仮説ノードの利用を終了してリサイクル用にストックする
 * 
 * @param node [in] 仮説ノード
 * </JA>
 * <EN>
 * Stock an unused hypothesis node for recycle.
 * 
 * @param node [in] hypothesis node
 * </EN>
 *
 * @callgraph
 * @callergraph
 */
void
free_node(NODE *node)
{
  if (node == NULL) return;

  if (node->region->graphout) {
    if (node->prevgraph != NULL && node->prevgraph->saved == FALSE) {
      wordgraph_free(node->prevgraph);
    }
  }

  /* save to stocker */
  node->next = node->region->pass2.stocker_root;
  node->region->pass2.stocker_root = node;

#ifdef STOCKER_DEBUG
  stocked_num++;
#endif
}

/** 
 * <JA>
 * リサイクル用ノード格納庫を空にする.
 *
 * @param s [in] stack decoding work area
 * 
 * </JA>
 * <EN>
 * Clear the node stocker for recycle.
 *
 * @param s [in] stack decoding work area
 * 
 * </EN>
 * @callgraph
 * @callergraph
 */
void
clear_stocker(StackDecode *s)
{
  NODE *node, *tmp;
  node = s->stocker_root;
  while(node) {
    tmp = node->next;
    free_node_exec(node);
    node = tmp;
  }
  s->stocker_root = NULL;

#ifdef STOCKER_DEBUG
  jlog("DEBUG: %d times requested, %d times newly allocated, %d times reused\n", request_num, new_num, reused_num);
  stocked_num = 0;
  reused_num = 0;
  new_num = 0;
  request_num = 0;
#endif
}

/** 
 * <JA>
 * 仮説をコピーする. 
 * 
 * @param dst [out] コピー先の仮説
 * @param src [in] コピー元の仮説
 * 
 * @return @a dst を返す. 
 * </JA>
 * <EN>
 * Copy the content of node to another.
 * 
 * @param dst [out] target hypothesis
 * @param src [in] source hypothesis
 * 
 * @return the value of @a dst.
 * </EN>
 * @callgraph
 * @callergraph
 */
NODE *
cpy_node(NODE *dst, NODE *src)
{
  int peseqlen;

  peseqlen = src->region->peseqlen;
  
  dst->next = src->next;
  dst->prev = src->prev;
  memcpy(dst->g, src->g, sizeof(LOGPROB) * peseqlen);
  memcpy(dst->seq, src->seq, sizeof(WORD_ID) * MAXSEQNUM);
#ifdef CM_SEARCH
#ifdef CM_MULTIPLE_ALPHA
  {
    int w;
    for(w=0;w<src->seqnum;w++) {
      memcpy(dst->cmscore[w], src->cmscore[w], sizeof(LOGPROB) * src->region->config->annotate.cm_alpha_num);
    }
  }     
#else
  memcpy(dst->cmscore, src->cmscore, sizeof(LOGPROB) * MAXSEQNUM);
#endif
#endif /* CM_SEARCH */
  dst->seqnum = src->seqnum;
  dst->score = src->score;
  dst->bestt = src->bestt;
  dst->estimated_next_t = src->estimated_next_t;
  dst->endflag = src->endflag;
  dst->state = src->state;
  dst->tre = src->tre;
  if (src->g_prev != NULL) { // ccd_flag == TRUE
    memcpy(dst->g_prev, src->g_prev, sizeof(LOGPROB) * peseqlen);
    dst->last_ph = src->last_ph;
    dst->last_ph_sp_attached = src->last_ph_sp_attached;
    dst->lscore = src->lscore;
  }
  dst->totallscore = src->totallscore;
  dst->final_g = src->final_g;
#ifdef VISUALIZE
  dst->popnode = src->popnode;
#endif

  if (src->region->graphout) {
#ifdef GRAPHOUT_PRECISE_BOUNDARY
    memcpy(dst->wordend_frame, src->wordend_frame, sizeof(short) * peseqlen);
    memcpy(dst->wordend_gscore, src->wordend_gscore, sizeof(LOGPROB) * peseqlen);
#endif
    dst->prevgraph = src->prevgraph;
    dst->lastcontext = src->lastcontext;
#ifndef GRAPHOUT_PRECISE_BOUNDARY
    dst->tail_g_score = src->tail_g_score;
#endif
  }

  return(dst);
}

/** 
 * <JA>
 * 新たな仮説ノードを割り付ける. もし格納庫に以前試用されなくなった
 * ノードがある場合はそれを再利用する. なければ新たに割り付ける.
 *
 * @param r [in] 認識処理インスタンス
 * 
 * @return 新たに割り付けられた仮説ノードへのポインタを返す. 
 * </JA>
 * <EN>
 * Allocate a new hypothesis node.  If the node stocker is not empty,
 * the one in the stocker is re-used.  Otherwise, allocate as new.
 *
 * @param r [in] recognition process instance
 * 
 * @return pointer to the newly allocated node.
 * </EN>
 * @callgraph
 * @callergraph
 */
NODE *
newnode(RecogProcess *r)
{
  NODE *tmp;
  int i;
  int peseqlen;

  peseqlen = r->peseqlen;

#ifdef STOCKER_DEBUG
  request_num++;
#endif
  if ((tmp = r->pass2.stocker_root) != NULL) {
    /* re-use ones in the stocker */
    r->pass2.stocker_root = tmp->next;
#ifdef STOCKER_DEBUG
    stocked_num--;
    reused_num++;
#endif
  } else {
    /* allocate new */
    tmp =(NODE *)mymalloc(sizeof(NODE));
    tmp->g = (LOGPROB *)mymalloc(sizeof(LOGPROB) * peseqlen);
    if (r->ccd_flag) {
      tmp->g_prev = (LOGPROB *)mymalloc(sizeof(LOGPROB) * peseqlen);
    } else {
      tmp->g_prev = NULL;
    }

#ifdef GRAPHOUT_PRECISE_BOUNDARY
    if (r->graphout) {
      tmp->wordend_frame = (short *)mymalloc(sizeof(short) * peseqlen);
      tmp->wordend_gscore = (LOGPROB *)mymalloc(sizeof(LOGPROB) * peseqlen);
    }
#endif
#ifdef STOCKER_DEBUG
    new_num++;
#endif
  }

  /* clear the data */
  /*bzero(tmp,sizeof(NODE));*/
  tmp->next=NULL;
  tmp->prev=NULL;
  tmp->last_ph = NULL;
  tmp->last_ph_sp_attached = FALSE;
  if (r->ccd_flag) {
    if (r->lmtype == LM_PROB) {
      tmp->lscore = LOG_ZERO;
      tmp->totallscore = LOG_ZERO;
    } else if (r->lmtype == LM_DFA) {
      tmp->lscore = 0.0;
      tmp->totallscore = 0.0;
    }
  }
  tmp->endflag = FALSE;
  tmp->seqnum = 0;
  for(i=0;i<peseqlen;i++) {
    tmp->g[i] = LOG_ZERO;
  }
  if (r->ccd_flag) {
    for(i=0;i<peseqlen;i++) {
      tmp->g_prev[i] = LOG_ZERO;
    }
  }
  tmp->final_g = LOG_ZERO;
#ifdef VISUALIZE
  tmp->popnode = NULL;
#endif
  tmp->tre = NULL;

  if (r->graphout) {
    tmp->prevgraph = NULL;
    tmp->lastcontext = NULL;
  }

  tmp->region = r;

#ifdef USE_MBR
  tmp->score_mbr = 0.0;
#endif

  return(tmp);
}


/**********************************************************************/
/************ 前向きトレリス展開と尤度計算              ***************/
/************ Expand trellis and update forward viterbi ***************/
/**********************************************************************/

/** 
 * <JA>
 * 1単語分のトレリス計算用のワークエリアを確保.
 * 
 * @param r [in] 認識処理インスタンス
 * 
 * </JA>
 * <EN>
 * Allocate work area for trellis computation of a word.
 * 
 * @param r [in] recognition process instance
 * 
 * </EN>
 * @callgraph
 * @callergraph
 */
void
malloc_wordtrellis(RecogProcess *r)
{
  int maxwn;
  StackDecode *dwrk;

  maxwn = r->lm->winfo->maxwn + 10;
  dwrk = &(r->pass2);

  dwrk->wordtrellis[0] = (LOGPROB *)mymalloc(sizeof(LOGPROB) * maxwn);
  dwrk->wordtrellis[1] = (LOGPROB *)mymalloc(sizeof(LOGPROB) * maxwn);

  dwrk->g = (LOGPROB *)mymalloc(sizeof(LOGPROB) * r->peseqlen);

  dwrk->phmmlen_max = r->lm->winfo->maxwlen + 2;
  dwrk->phmmseq = (HMM_Logical **)mymalloc(sizeof(HMM_Logical *) * dwrk->phmmlen_max);
  if (r->lm->config->enable_iwsp && r->am->hmminfo->multipath) {
    dwrk->has_sp = (boolean *)mymalloc(sizeof(boolean) * dwrk->phmmlen_max);
  } else {
    dwrk->has_sp = NULL;
  }

  dwrk->wend_token_frame[0] = NULL;
  dwrk->wend_token_frame[1] = NULL;
  dwrk->wend_token_gscore[0] = NULL;
  dwrk->wend_token_gscore[1] = NULL;
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  if (r->graphout) {
    dwrk->wend_token_frame[0] = (short *)mymalloc(sizeof(short) * maxwn);
    dwrk->wend_token_frame[1] = (short *)mymalloc(sizeof(short) * maxwn);
    dwrk->wend_token_gscore[0] = (LOGPROB *)mymalloc(sizeof(LOGPROB) * maxwn);
    dwrk->wend_token_gscore[1] = (LOGPROB *)mymalloc(sizeof(LOGPROB) * maxwn);
  }
#endif
  
}

/** 
 * <JA>
 * 1単語分のトレリス計算用のワークエアリアを解放
 * 
 * </JA>
 * <EN>
 * Free the work area for trellis computation of a word.
 * 
 * </EN>
 * @callgraph
 * @callergraph
 */
void
free_wordtrellis(StackDecode *dwrk)
{
  int i;

  free(dwrk->wordtrellis[0]);
  free(dwrk->wordtrellis[1]);
  free(dwrk->g);
  free(dwrk->phmmseq);
  if (dwrk->has_sp) {
    free(dwrk->has_sp);
    dwrk->has_sp = NULL;
  }
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  for(i=0;i<2;i++) {
    if (dwrk->wend_token_frame[i]) {
      free(dwrk->wend_token_frame[i]);
      dwrk->wend_token_frame[i] = NULL;
    }
    if (dwrk->wend_token_gscore[i]) {
      free(dwrk->wend_token_gscore[i]);
      dwrk->wend_token_gscore[i] = NULL;
    }
  }
#endif
}


/**********************************************************************/
/************ 仮説の前向き尤度計算                  *******************/
/************ Compute forward score of a hypothesis *******************/
/**********************************************************************/

/** 
 * <JA>
 * 最終状態への遷移確率の最大値を求める (multipath)
 * 
 * @param tr [in] 遷移行列
 * @param state_num [in] 状態数
 * 
 * @return 最終状態への遷移確率への最大値を返す. 
 * </JA>
 * <EN>
 * Get the maximum transition log probability to final state. (multipath)
 * 
 * @param tr [in] transition matrix
 * @param state_num [in] number of states
 * 
 * @return the maximum log probability of transition to the final state.
 * </EN>
 */
static LOGPROB
get_max_out_arc(HTK_HMM_Trans *tr, int state_num)
{
  LOGPROB max_a;
  int afrom;
  LOGPROB a;
  
  max_a = LOG_ZERO;
  for (afrom = 0; afrom < state_num - 1; afrom++) {
    a = tr->a[afrom][state_num-1];
    if (max_a < a) max_a = a;
  }
  return(max_a);
}

/** 
 * <JA>
 * 音素の出力状態への遷移確率の最大値を求める.  (multipath)
 * 
 * @param l [in] 音素
 * 
 * @return 出力状態への遷移確率の最大値を返す. 
 * </JA>
 * <EN>
 * Get the maximum transition log probability outside a phone. (multipath)
 * 
 * @param l [in] phone
 * 
 * @return the maximum transition log probability outside a phone.
 * </EN>
 */
static LOGPROB
max_out_arc(HMM_Logical *l)
{
  return(get_max_out_arc(hmm_logical_trans(l), hmm_logical_state_num(l)));
}

/** 
 * <JA>
 * 最後の1単語の前向きトレリスを計算して，文仮説の前向き尤度を更新する. 
 * 
 * @param now [i/o] 文仮説
 * @param param [in] 入力パラメータ列
 * @param r [in] 認識処理インスタンス
 * 
 * </JA>
 * <EN>
 * Compute the forward viterbi for the last word to update forward scores
 * and ready for word connection.
 * 
 * @param now [i/o] hypothesis
 * @param param [in] input parameter vectors
 * @param r [in] recognition process instance
 * 
 * </EN>
 * @callgraph
 * @callergraph
 */
void
scan_word(NODE *now, HTK_Param *param, RecogProcess *r)
{
  int   i,t, j;
  HMM *whmm;
  A_CELL *ac;
  WORD_ID word;
  LOGPROB tmpmax, tmptmp, score1;
  int startt = 0, endt;
  int wordhmmnum;
  LOGPROB tmpmax_store, store_point_maxarc; /* multipath */
  LOGPROB tmpmax2 = LOG_ZERO;
  int phmmlen;
  HMM_Logical *ret, *wend;
  int store_point;
  int crossword_point = 0;
  boolean back_rescan = FALSE;
  boolean node_exist_p;
  int tn;		       ///< Temporal pointer to current buffer
  int tl;		       ///< Temporal pointer to previous buffer

  /* store global values to local for rapid access */
  WORD_INFO *winfo;
  HTK_HMM_INFO *hmminfo;
  LOGPROB *framemaxscore;
  int peseqlen;
  boolean ccd_flag;
  boolean enable_iwsp;
#ifdef SCAN_BEAM
  LOGPROB scan_beam_thres;
#endif
  StackDecode *dwrk;

  winfo = r->lm->winfo;
  hmminfo = r->am->hmminfo;
  dwrk = &(r->pass2);
  peseqlen = r->peseqlen;
  framemaxscore = r->pass2.framemaxscore;
  ccd_flag = r->ccd_flag;
  enable_iwsp = r->lm->config->enable_iwsp; /* multipath */
#ifdef SCAN_BEAM
  scan_beam_thres = r->config->pass2.scan_beam_thres;
#endif

  if (hmminfo->multipath) {
    store_point = -1;
  } else {
    store_point = 0;
  }
  
  /* ----------------------- prepare HMM ----------------------- */

  if (ccd_flag) {
    /* 直前の音素があれば，そこまでさかのぼって scan する */
    /* if there are any last phone, enable backscan */
    if (now->last_ph == NULL) {
      /* initial score: now->g[] */
      /* scan range: phones in now->seq[now->seqnum-1] */
      back_rescan = FALSE;
    } else {
      /* initial score: now->g_prev[] (1-phone before)*/
      /* scan range: phones in now->seq[now->seqnum-1] + now->last_ph */
      back_rescan = TRUE;
    }
  }
#ifdef TCD
  if (now->last_ph != NULL) {
    jlog("DEBUG: inherited last_ph: %s\n", (now->last_ph)->name);
    if (now->last_ph_sp_attached) jlog("DEBUG: (sp attached)\n"); /* multipath */
  } else {
    jlog("DEBUG: no last_ph inherited\n");
  }
#endif

  /* scan 範囲分のHMMを準備 */
  /* prepare HMM of the scan range */
  word = now->seq[now->seqnum-1];

  if (ccd_flag) {

    if (back_rescan) {
      
      /* scan range: phones in now->seq[now->seqnum-1] + now->last_ph */
      
      phmmlen = winfo->wlen[word] + 1;
      if (phmmlen > dwrk->phmmlen_max) {
	j_internal_error("scan_word: num of phonemes in a word exceed phmmlenmax (%d) ?\n", dwrk->phmmlen_max);
      }
      for (i=0;i<phmmlen - 2;i++) dwrk->phmmseq[i] = winfo->wseq[word][i];
      if (enable_iwsp && hmminfo->multipath) {
	for (i=0;i<phmmlen - 2;i++) dwrk->has_sp[i] = FALSE;
      }

      /* 最終単語と last_ph 間の単語間triphoneを考慮 */
      /* consider cross-word context dependency between the last word and now->last_ph */
      wend = winfo->wseq[word][winfo->wlen[word]-1];
      ret = get_right_context_HMM(wend, now->last_ph->name, hmminfo);
      if (ret == NULL) {	/* triphone not found */
	/* fallback to the original bi/mono-phone */
	/* error if the original is pseudo phone (not explicitly defined
	   in hmmdefs/hmmlist) */
	/* exception: word with 1 phone (triphone may exist in the next expansion */
	if (winfo->wlen[word] > 1 && wend->is_pseudo) {
	  error_missing_right_triphone(wend, now->last_ph->name);
	}
	dwrk->phmmseq[phmmlen-2] = wend;
      } else {
	dwrk->phmmseq[phmmlen-2] = ret;
      }
      ret = get_left_context_HMM(now->last_ph, wend->name, hmminfo);
      if (ret == NULL) {
	/* fallback to the original bi/mono-phone */
	/* error if the original is pseudo phone (not explicitly defined
	   in hmmdefs/hmmlist) */
	if (now->last_ph->is_pseudo) {
	  error_missing_left_triphone(now->last_ph, wend->name);
	}
	dwrk->phmmseq[phmmlen-1] = now->last_ph;
      } else {
	dwrk->phmmseq[phmmlen-1] = ret;
      }

      if (enable_iwsp && hmminfo->multipath) {
	dwrk->has_sp[phmmlen-2] = TRUE;
	dwrk->has_sp[phmmlen-1] = now->last_ph_sp_attached;
      }
 
#ifdef TCD
      jlog("DEBUG: w=");
      for(i=0;i<winfo->wlen[word];i++) {
	jlog(" %s",(winfo->wseq[word][i])->name);
	if (enable_iwsp && hmminfo->multipath && dwrk->has_sp[i]) jlog("(sp)");
      }
      jlog(" | %s\n", (now->last_ph)->name);
      if (hmminfo->multipath && now->last_ph_sp_attached) jlog("DEBUG:   (sp)\n");
      jlog("DEBUG: scan for:");
      
      for (i=0;i<phmmlen;i++) {
	jlog(" %s", dwrk->phmmseq[i]->name);
 	if (enable_iwsp && hmminfo->multipath && dwrk->has_sp[i]) jlog("(sp)");
      }
      jlog("\n");
#endif

      /* 単語HMMを作る */
      /* make word HMM */
      whmm = new_make_word_hmm(hmminfo, dwrk->phmmseq, phmmlen, (enable_iwsp && hmminfo->multipath) ? dwrk->has_sp : NULL);
      if (whmm == NULL) {
	j_internal_error("Error: failed to make word hmm for word #%d \"%s [%s]\"\n", word, winfo->wname[word], winfo->woutput[word]);
      }
      
      /* backscan なので，計算前の g[] 初期値は now->g_prev[] を使用 */
      /* As backscan enabled, the initial forward score g[] is set by
	 now->g_prev[] */
      for (t=0;t<peseqlen;t++) {
	dwrk->g[t]=now->g_prev[t];

      }
      
      /* 次段用のg_prevを格納するノード位置を設定 */
      /* set where to store scores as new g_prev[] for the next backscan
	 in the HMM */
      if (hmminfo->multipath) {
	store_point = hmm_logical_state_num(dwrk->phmmseq[0]) - 2;
	store_point_maxarc = max_out_arc(dwrk->phmmseq[0]);
	if (enable_iwsp && dwrk->has_sp[0]) {
	  store_point += hmm_logical_state_num(hmminfo->sp) - 2;
	  if (store_point_maxarc < max_out_arc(hmminfo->sp)) {
	    store_point_maxarc = max_out_arc(hmminfo->sp);
	  }
	}
      } else {
	store_point = hmm_logical_state_num(dwrk->phmmseq[0]) - 2 - 1;
      }
      /* scan中に直前単語とこの単語をまたぐ場所を設定 */
      /* set where is the connection point of the last word in the HMM */
      if (hmminfo->multipath) {
	crossword_point = whmm->len - hmm_logical_state_num(dwrk->phmmseq[phmmlen-1]);
	if (enable_iwsp && dwrk->has_sp[phmmlen-1]) {
	  crossword_point -= hmm_logical_state_num(hmminfo->sp) - 2;
	}
      } else {
	crossword_point = whmm->len - (hmm_logical_state_num(dwrk->phmmseq[phmmlen-1]) - 2) - 1;
      }
      
    } else {			/* not backscan mode */
      
      /* scan range: phones in now->seq[now->seqnum-1] */
      
#ifdef TCD
      jlog("DEBUG: scan(org):");
      for (i=0;i<winfo->wlen[word];i++) {
	jlog(" %s", (winfo->wseq[word][i])->name);
      }
      jlog("\n");
#endif

      if (enable_iwsp && hmminfo->multipath) {
	/* 必要ならばショートポーズを挟み込む位置を指定する */
	for(i=0;i<winfo->wlen[word];i++) {
	  dwrk->has_sp[i] = FALSE;
	}
	dwrk->has_sp[winfo->wlen[word]-1] = TRUE;
      }
      
      /* 単語HMMを作る */
      /* make word HMM */
      whmm = new_make_word_hmm(hmminfo, winfo->wseq[word], winfo->wlen[word], (enable_iwsp && hmminfo->multipath) ? dwrk->has_sp : NULL);
      if (whmm == NULL) {
	j_internal_error("Error: failed to make word hmm for word #%d \"%s [%s]\"\n", word, winfo->wname[word], winfo->woutput[word]);
      }
      
      /* 計算前の g[] 初期値は now->g[] を使用 */
      /* the initial forward score g[] is set by now->g[] */
      for (t=0;t<peseqlen;t++) {
	dwrk->g[t]=now->g[t];
      }
      
      /* 次段用のg_prevを格納するノード位置を設定 */
      /* set where to store scores as new g_prev[] for the next backscan
	 in the HMM */
      if (hmminfo->multipath) {
	store_point = hmm_logical_state_num(winfo->wseq[word][0]) - 2;
	store_point_maxarc = max_out_arc(winfo->wseq[word][0]);
	if (enable_iwsp && dwrk->has_sp[0]) {
	  store_point += hmm_logical_state_num(hmminfo->sp) - 2;
	  if (store_point_maxarc < max_out_arc(hmminfo->sp)) {
	    store_point_maxarc = max_out_arc(hmminfo->sp);
	  }
	}
      } else {
	store_point = hmm_logical_state_num(winfo->wseq[word][0]) - 2 - 1;
      }

      /* scan中に直前単語とこの単語をまたぐ場所は，なし */
      /* the connection point of the last word is not exist in the HMM */
      crossword_point = -1;
    }
    
  } else {			/* ccd_flag == FALSE */

    if (enable_iwsp && hmminfo->multipath) {
      /* 必要ならばショートポーズを挟み込む位置を指定する */
      for(i=0;i<winfo->wlen[word];i++) {
	dwrk->has_sp[i] = FALSE;
      }
      dwrk->has_sp[winfo->wlen[word]-1] = TRUE;
    }

    /* 音素環境非依存の場合は単純に最終単語分の HMM を作成 */
    /* for monophone: simple make HMM for the last word */
    whmm = new_make_word_hmm(hmminfo, winfo->wseq[word], winfo->wlen[word], (enable_iwsp && hmminfo->multipath) ? dwrk->has_sp : NULL);
    if (whmm == NULL) {
      j_internal_error("Error: failed to make word hmm for word #%d \"%s [%s]\"\n", word, winfo->wname[word], winfo->woutput[word]);
    }

    /* 計算前の g[] 初期値は now->g[] を使用 */
    /* the initial forward score g[] is set by now->g[] */
    for (t=0;t<peseqlen;t++) {
      dwrk->g[t]=now->g[t];
    }
    
  }

#ifdef TCD
  jlog("DEBUG: whmm len	  = %d\n",whmm->len);
  jlog("DEBUG: crossword_point = %d\n", crossword_point);
  jlog("DEBUG: g[] store point = %d\n", store_point);
#endif

  wordhmmnum = whmm->len;
  if (wordhmmnum >= winfo->maxwn + 10) {
    j_internal_error("scan_word: word too long (>%d)\n", winfo->maxwn + 10);
  }

#ifndef GRAPHOUT_PRECISE_BOUNDARY
  if (r->graphout) {
    if (ccd_flag) {
      now->tail_g_score = now->g[now->bestt];
    }
  }
#endif

  /* ----------------------- do scan ----------------------- */
  
  /* scan開始点を検索 -> starttへ*/
  /* search for the start frame -> set to startt */
  for(t = peseqlen-1; t >=0 ; t--) {
    if (
#ifdef SCAN_BEAM
	dwrk->g[t] > framemaxscore[t] - scan_beam_thres &&
#endif
	dwrk->g[t] > LOG_ZERO) {
      break;
    }
  }
  if (t < 0) {			/* no node has score > LOG_ZERO */
    for(t=0;t<peseqlen;t++) {
      if (ccd_flag) now->g_prev[t] = LOG_ZERO;
      now->g[t] = LOG_ZERO;
    }
#ifdef GRAPHOUT_PRECISE_BOUNDARY
    if (r->graphout) {
      for(t=0;t<peseqlen;t++) {
	now->wordend_frame[t] = -1;
	now->wordend_gscore[t] = LOG_ZERO;
      }
    }
#endif
    goto end_of_scan;
  }
  startt = t;
  
  /* clear [startt+1..peseqlen-1] */
  for(t=peseqlen-1;t>startt;t--) {
    if (ccd_flag) now->g_prev[t] = LOG_ZERO;
    now->g[t] = LOG_ZERO;
#ifdef GRAPHOUT_PRECISE_BOUNDARY
    if (r->graphout) {
      now->wordend_frame[t] = -1;
      now->wordend_gscore[t] = LOG_ZERO;
    }
#endif
  }

  /* バッファポインタ初期化 */
  tn = 0; tl = 1;

#ifdef GRAPHOUT_PRECISE_BOUNDARY
  if (r->graphout) {
    for(i=0;i<wordhmmnum;i++) {
      dwrk->wend_token_frame[tn][i] = -1;
      dwrk->wend_token_gscore[tn][i] = LOG_ZERO;
    }
  }
#endif
  
  if (! hmminfo->multipath) {
    /* Below initialization is not needed on multipath version, since
       the actual viterbi will begin at frame 0 in multipath mode in main loop */
    
    /* 時間 [startt] 上の値を初期化 */
    /* initialize scores on frame [startt] */
    for(i=0;i<wordhmmnum-1;i++) dwrk->wordtrellis[tn][i] = LOG_ZERO;
    dwrk->wordtrellis[tn][wordhmmnum-1] = dwrk->g[startt] + outprob(&(r->am->hmmwrk), startt, &(whmm->state[wordhmmnum-1]), param);
    if (ccd_flag) {
      now->g_prev[startt] = dwrk->wordtrellis[tn][store_point];
    }
    now->g[startt] = dwrk->wordtrellis[tn][0];
    
#ifdef GRAPHOUT_PRECISE_BOUNDARY
    if (r->graphout) {
      if (ccd_flag) {
	if (back_rescan) {
	  if (wordhmmnum-1 == crossword_point) {
	    dwrk->wend_token_frame[tn][wordhmmnum-1] = startt;
	    dwrk->wend_token_gscore[tn][wordhmmnum-1] = dwrk->g[startt];
	  } else {
	    dwrk->wend_token_frame[tn][wordhmmnum-1] = -1;
	    dwrk->wend_token_gscore[tn][wordhmmnum-1] = LOG_ZERO;
	  }
	} else {
	  dwrk->wend_token_frame[tn][wordhmmnum-1] = startt;
	  dwrk->wend_token_gscore[tn][wordhmmnum-1] = dwrk->g[startt];
	}
      } else {
	dwrk->wend_token_frame[tn][wordhmmnum-1] = startt;
	dwrk->wend_token_gscore[tn][wordhmmnum-1] = dwrk->g[startt];
      }
      now->wordend_frame[startt] = dwrk->wend_token_frame[tn][0];
      now->wordend_gscore[startt] = dwrk->wend_token_gscore[tn][0];
    }
#endif
  } /* end of hmminfo->multipath */
  
  endt = startt;

  /* メインループ: startt から始まり 0 に向かって Viterbi 計算 */
  /* main loop: start from [startt], and compute Viterbi toward [0] */
  for(t = hmminfo->multipath ? startt : startt - 1; t >= 0; t--) {
    
    /* wordtrellisのワークエリアをスワップ */
    i = tn; tn = tl; tl = i;
    
    node_exist_p = FALSE;	/* TRUE if there is at least 1 survived node in this frame */

    if (hmminfo->multipath) {

      /* 端のノード [t][wordhmmnum-1]は g[] を参照する */
      /* the edge node [t][wordhmmnum-1] is equal to g[] */

      /* ノード [t][wordhmmnum-2..0] についてトレリスを計算 */
      /* expand trellis for node [t][wordhmmnum-2..0] */
      tmpmax_store = LOG_ZERO;

    } else {
    
      /* 端のノード [t][wordhmmnum-1]は，内部遷移 か g[]の高い方になる */
      /* the edge node [t][wordhmmnum-1] is either internal transitin or g[] */
      tmptmp = LOG_ZERO;
      for (ac=whmm->state[wordhmmnum-1].ac;ac;ac=ac->next) {
	score1 = dwrk->wordtrellis[tl][ac->arc] + ac->a;
	if (tmptmp < score1) {
	  j = ac->arc;
	  tmptmp = score1;
	}
      }
      if (dwrk->g[t] > tmptmp) {
	tmpmax = dwrk->g[t];
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	if (r->graphout) {
	  if (!back_rescan || wordhmmnum-1 == crossword_point) {
	    dwrk->wend_token_frame[tn][wordhmmnum-1] = t;
	    dwrk->wend_token_gscore[tn][wordhmmnum-1] = dwrk->g[t];
	  } else {
	    dwrk->wend_token_frame[tn][wordhmmnum-1] = dwrk->wend_token_frame[tl][j];
	    dwrk->wend_token_gscore[tn][wordhmmnum-1] = dwrk->wend_token_gscore[tl][j];
	  }
	}
#endif
      } else {
	tmpmax = tmptmp;
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	if (r->graphout) {
	  dwrk->wend_token_frame[tn][wordhmmnum-1] = dwrk->wend_token_frame[tl][j];
	  dwrk->wend_token_gscore[tn][wordhmmnum-1] = dwrk->wend_token_gscore[tl][j];
	}
#endif
      }

      /* 端のノードのスコアエンベロープチェック: 一定幅外なら落とす */
      /* check if the edge node is within score envelope */
      if (
#ifdef SCAN_BEAM
	  tmpmax <= framemaxscore[t] - scan_beam_thres ||
#endif
	  tmpmax <= LOG_ZERO
	  ) {
	dwrk->wordtrellis[tn][wordhmmnum-1] = LOG_ZERO;
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	if (r->graphout) {
	  dwrk->wend_token_frame[tn][wordhmmnum-1] = -1;
	  dwrk->wend_token_gscore[tn][wordhmmnum-1] = LOG_ZERO;
	}
#endif
      } else {
	node_exist_p = TRUE;
	dwrk->wordtrellis[tn][wordhmmnum-1] = tmpmax + outprob(&(r->am->hmmwrk), t, &(whmm->state[wordhmmnum-1]), param);
      }

    } /* end of ~multipath */

    /* ノード [t][wordhmmnum-2..0] についてトレリスを計算 */
    /* expand trellis for node [t][wordhmmnum-2..0] */
    for(i=wordhmmnum-2;i>=0;i--) {

      if (ccd_flag) {

	/* 最尤パスと最尤スコア tmpmax を見つける */
	/* tmpmax2 は次回用 g_prev[] のための最大値(自己遷移を除いた最大値) */
	/* find most likely path and the max score 'tmpmax' */
	/* 'tmpmax2' is max score excluding self transition, for next g_prev[] */
	if (! hmminfo->multipath) {
	  if (i == store_point) {
	    tmpmax2 = LOG_ZERO;
	  }
	}
	tmpmax = LOG_ZERO;
	for (ac=whmm->state[i].ac;ac;ac=ac->next) {
	  if (hmminfo->multipath) {
	    if (ac->arc == wordhmmnum-1) score1 = dwrk->g[t];
	    else if (t + 1 > startt) score1 = LOG_ZERO;
	    else score1 = dwrk->wordtrellis[tl][ac->arc];
	    score1 += ac->a;
	  } else {
	    score1 = dwrk->wordtrellis[tl][ac->arc] + ac->a;
	  }
	  if (i <= crossword_point && ac->arc > crossword_point) {
	    /* これは単語を越える遷移 (backscan 実行時) */
	    /* this is a transition across word (when backscan is enabled) */
	    score1 += now->lscore; /* add LM score */
	  }

	  if (hmminfo->multipath) {
	    if (i <= store_point && ac->arc > store_point) {
	      if (tmpmax_store < score1) tmpmax_store = score1;
	    }
	  } else {
	    if (i == store_point && i != ac->arc) {
	      if (tmpmax2 < score1) tmpmax2 = score1;
	    }
	  }

	  if (tmpmax < score1) {
	    tmpmax = score1;
	    j = ac->arc;
	  }
	}

	/* スコアエンベロープチェック: 一定幅外なら落とす */
	/* check if score of this node is within the score envelope */
	if (
#ifdef SCAN_BEAM
	    tmpmax <= framemaxscore[t] - scan_beam_thres ||
#endif
	    tmpmax <= LOG_ZERO
	    ) {  /* invalid node */
	  dwrk->wordtrellis[tn][i] = LOG_ZERO;
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	  if (r->graphout) {
	    dwrk->wend_token_frame[tn][i] = -1;
	    dwrk->wend_token_gscore[tn][i] = LOG_ZERO;
	  }
#endif
	  if (! hmminfo->multipath) {
	    if (i == store_point) now->g_prev[t] = LOG_ZERO;
	  }
	} else { /* survived node */
	  if (! hmminfo->multipath) {
	    if (i == store_point) now->g_prev[t] = tmpmax2;
	  }
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	  if (r->graphout) {

	    if (hmminfo->multipath) {
	      if ((back_rescan && i <= crossword_point && j > crossword_point)
		  || j == wordhmmnum-1) {
		dwrk->wend_token_frame[tn][i] = t;
		dwrk->wend_token_gscore[tn][i] = tmpmax;
	      } else {
		dwrk->wend_token_frame[tn][i] = dwrk->wend_token_frame[tl][j];
		dwrk->wend_token_gscore[tn][i] = dwrk->wend_token_gscore[tl][j];
	      }
	    } else {
	      if (i <= crossword_point && j > crossword_point) {
		dwrk->wend_token_frame[tn][i] = t;
		dwrk->wend_token_gscore[tn][i] = tmpmax;
	      } else {
		dwrk->wend_token_frame[tn][i] = dwrk->wend_token_frame[tl][j];
		dwrk->wend_token_gscore[tn][i] = dwrk->wend_token_gscore[tl][j];
	      }
	    }
	  }
#endif
	  node_exist_p = TRUE;	/* at least one node survive in this frame */

	  dwrk->wordtrellis[tn][i] = tmpmax;
	  if (! hmminfo->multipath || i > 0) {
	    /* compute output probability */
	    dwrk->wordtrellis[tn][i] += outprob(&(r->am->hmmwrk), t, &(whmm->state[i]), param);
	  }
	}
	
      } else {			/* not triphone */

	/* backscan 無し: store_point, crossword_point は無関係 */
	/* no backscan: store_point, crossword_point ignored */
	tmpmax = LOG_ZERO;
	if (hmminfo->multipath) {
	  for (ac=whmm->state[i].ac;ac;ac=ac->next) {
	    if (ac->arc == wordhmmnum-1) score1 = dwrk->g[t];
	    else if (t + 1 > startt) score1 = LOG_ZERO;
	    else score1 = dwrk->wordtrellis[tl][ac->arc];
	    score1 += ac->a;
	    if (tmpmax < score1) {
	      tmpmax = score1;
	      j = ac->arc;
	    }
	  }
	} else {
	  for (ac=whmm->state[i].ac;ac;ac=ac->next) {
	    score1 = dwrk->wordtrellis[tl][ac->arc] + ac->a;
	    if (tmpmax < score1) {
	      tmpmax = score1;
	      j = ac->arc;
	    }
	  }
	}

	/* スコアエンベロープチェック: 一定幅外なら落とす */
	/* check if score of this node is within the score envelope */
	if (
#ifdef SCAN_BEAM
	    tmpmax <= framemaxscore[t] - scan_beam_thres ||
#endif
	    tmpmax <= LOG_ZERO
	    ) {
	  /* invalid node */
	  dwrk->wordtrellis[tn][i] = LOG_ZERO;
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	  if (r->graphout) {
	    dwrk->wend_token_frame[tn][i] = -1;
	    dwrk->wend_token_gscore[tn][i] = LOG_ZERO;
	  }
#endif
	} else {
	  /* survived node */
	  node_exist_p = TRUE;
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	  if (r->graphout) {
	    if (hmminfo->multipath) {
	      if (j == wordhmmnum-1) {
		dwrk->wend_token_frame[tn][i] = t;
		dwrk->wend_token_gscore[tn][i] = tmpmax;
	      } else {
		dwrk->wend_token_frame[tn][i] = dwrk->wend_token_frame[tl][j];
		dwrk->wend_token_gscore[tn][i] = dwrk->wend_token_gscore[tl][j];
	      }
	    } else {
	      dwrk->wend_token_frame[tn][i] = dwrk->wend_token_frame[tl][j];
	      dwrk->wend_token_gscore[tn][i] = dwrk->wend_token_gscore[tl][j];
	    }
	  }
#endif
	  /* score of node [t][i] has been determined here */
	  dwrk->wordtrellis[tn][i] = tmpmax;
	  if (! hmminfo->multipath || i > 0) {
	    dwrk->wordtrellis[tn][i] += outprob(&(r->am->hmmwrk), t, &(whmm->state[i]), param);
	  }
	}
	
      }
    } /* end of node loop */

    /* 時間 t のViterbi計算終了. 前向きスコアはscanした単語の始端 */
    /* Viterbi end for frame [t].  the forward score is the score of word
       beginning scanned */
    now->g[t] = dwrk->wordtrellis[tn][0];
#ifdef GRAPHOUT_PRECISE_BOUNDARY
    if (r->graphout) {
      now->wordend_frame[t] = dwrk->wend_token_frame[tn][0];
      now->wordend_gscore[t] = dwrk->wend_token_gscore[tn][0];
    }
#endif

    if (hmminfo->multipath) {
      /* triphone 時, 次段のために store_point のデータをg_prevに保存 */
      /* store the scores crossing the store_point to g_prev, for next scan */
      if (ccd_flag) {
	/* the max arc crossing the store_point always selected as tmpmax_score */ 
	tmpmax_store -= store_point_maxarc;
	if (tmpmax_store < LOG_ZERO) tmpmax_store = LOG_ZERO;
	now->g_prev[t] = tmpmax_store;
      }
    }

    /* store the number of last computed frame */
    if (node_exist_p) endt = t;
    
    /* scanした単語の第１パスでの始端時刻より先まで t が進んでおり，かつ
       この t においてスコアエンベロープによって生き残ったノードが一つも
       無かったならば,このフレームで計算を打ち切りそれ以上先([0..t-1])は
       計算しない */
    /* if frame 't' already reached the beginning frame of scanned word
       in 1st pass and no node was survived in this frame (all nodes pruned
       by score envelope), terminate computation at this frame and
       do not computer further frame ([0..t-1]). */
    if (t < now->estimated_next_t && (!node_exist_p)) {
      /* clear the rest scores */
      for (i=t-1;i>=0;i--) {
	now->g[i] = LOG_ZERO;
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	if (r->graphout) {
	  now->wordend_frame[i] = -1;
	  now->wordend_gscore[i] = LOG_ZERO;
	}
#endif
	if (ccd_flag) now->g_prev[i] = LOG_ZERO;
      }
      /* terminate loop */
      break;
    }
    
  } /* end of time loop */
  
  if (debug2_flag) jlog("DEBUG: scanned: [%3d-%3d]\n", endt, startt);

 end_of_scan:

  if (hmminfo->multipath) {
    /* 前向きスコアの最終値を計算 (状態 0 から時間 0 への遷移) */
    /* compute the total forward score (transition from state 0 to frame 0 */
    if (endt == 0) {
      tmpmax = LOG_ZERO;
      for(ac=whmm->state[0].ac;ac;ac=ac->next) {
	score1 = dwrk->wordtrellis[tn][ac->arc] + ac->a;
	if (tmpmax < score1) tmpmax = score1;
      }
      now->final_g = score1;
    } else {
      now->final_g = LOG_ZERO;
    }
  }
  
  /* 次回 backscan のための情報格納 */
  /* store data for next backscan */
  if (ccd_flag) {
    if (store_point == (hmminfo->multipath ? wordhmmnum - 2 : wordhmmnum - 1)) {
      /* last_ph無し，かつ単語の音素長=1の場合、次回の scan_word() で
	 単語全体がもう一度再計算される. この場合,
	 g_prev は，このscan_wordを開始する前のスコアを入れておく必要がある */
      /* if there was no 'last_ph' and the scanned word consists of only
	 1 phone, the whole word should be re-computed in the future scan_word().
	 So the next 'g_prev[]' should be the initial forward scores
	 before we begin Viterbi (= g[t]). */
      for (t = startt; t>=0; t--) {
	now->g_prev[t] = dwrk->g[t];
      }
    }
#ifndef GRAPHOUT_PRECISE_BOUNDARY
    if (r->graphout) {
      if (now->tail_g_score != LOG_ZERO) {
	if (now->prevgraph != NULL) {
	  (now->prevgraph)->leftscore = now->tail_g_score;
	}
      }
    }
#endif
    /* 次回のために now->last_ph を更新 */
    /* update 'now->last_ph' for future scan_word() */
    if (back_rescan) {
      now->last_ph = dwrk->phmmseq[0];
    } else {
      now->last_ph = winfo->wseq[word][0];
    }
    if (enable_iwsp && hmminfo->multipath) {
      now->last_ph_sp_attached = dwrk->has_sp[0];
    }
  }

#ifdef GRAPHOUT_PRECISE_BOUNDARY
  if (! hmminfo->multipath) {
    if (r->graphout) {
      /* 次回の next_word 用に境界情報を調整 */
      /* proceed word boundary for one step for next_word */
      now->wordend_frame[peseqlen-1] = now->wordend_frame[0];
      now->wordend_gscore[peseqlen-1] = now->wordend_gscore[0];
      for (t=0;t<peseqlen-1;t++) {
	now->wordend_frame[t] = now->wordend_frame[t+1];
	now->wordend_gscore[t] = now->wordend_gscore[t+1];
      }
    }
  }
#endif

  /* free work area */
  free_hmm(whmm);
#ifdef TCD
  if (hmminfo->multipath) {
    if (ccd_flag) {
      jlog("DEBUG: last_ph = %s", (now->last_ph)->name);
      if (now->last_ph_sp_attached) jlog(" (sp attached)");
      jlog("\n");
    }
  } else {
    jlog("DEBUG: last_ph = %s\n", (now->last_ph)->name);
  }
#endif
}


/**************************************************************************/
/*** 新仮説の展開とヒューリスティックを繋いだ全体スコアを計算           ***/
/*** Expand new hypothesis and compute the total score (with heuristic) ***/
/**************************************************************************/

/** 
 * <JA>
 * 展開元仮説に次単語を接続して新しい仮説を生成する. 次単語の単語トレリス上の
 * スコアから最尤接続点を求め，仮説スコアを計算する. 
 * 
 * @param now [in] 展開元仮説
 * @param new [out] 新たに生成された仮説が格納される
 * @param nword [in] 接続する次単語の情報
 * @param param [in] 入力パラメータ列
 * @param r [in] 認識処理インスタンス
 *
 * </JA>
 * <EN>
 * Connect a new word to generate a next hypothesis.  The optimal connection
 * point and new sentence score of the new hypothesis will be estimated by
 * looking up the corresponding words on word trellis.
 * 
 * @param now [in] source hypothesis
 * @param new [out] pointer to save the newly generated hypothesis
 * @param nword [in] next word to be connected
 * @param param [in] input parameter vector
 * @param r [in] recognition process instance
 * 
 * </EN>
 * @callgraph
 * @callergraph
 */
void
next_word(NODE *now, NODE *new,	NEXTWORD *nword, HTK_Param *param, RecogProcess *r)
{
  int   t;
  HMM_Logical *newphone;
  int lastword;
  int   i;
  LOGPROB tmpp;
  LOGPROB a_value;
  int   startt;
  int word;
  LOGPROB totalscore;
  TRELLIS_ATOM *tre;

  BACKTRELLIS *backtrellis;
  WORD_INFO *winfo;
  HTK_HMM_INFO *hmminfo;
  int peseqlen;
  boolean ccd_flag;

  backtrellis = r->backtrellis;
  winfo = r->lm->winfo;
  hmminfo = r->am->hmminfo;
  peseqlen = r->peseqlen;
  ccd_flag = r->ccd_flag;

  new->score = LOG_ZERO;

  word = nword->id;
  lastword=now->seq[now->seqnum-1];

  /* 単語並び、DFA状態番号、言語スコアを継承・更新 */
  /* inherit and update word sequence, DFA state and total LM score */
  for (i=0;i< now->seqnum;i++){	
    new->seq[i] = now->seq[i];
#ifdef CM_SEARCH
#ifdef CM_MULTIPLE_ALPHA
    memcpy(new->cmscore[i], now->cmscore[i], sizeof(LOGPROB) * r->config->annotate.cm_alpha_num);
#else
    new->cmscore[i] = now->cmscore[i];
#endif
#endif /* CM_SEARCH */
  }
  new->seq[i] = word;
  new->seqnum = now->seqnum+1;
  new->state = nword->next_state;
  new->totallscore = now->totallscore + nword->lscore;
  if (hmminfo->multipath) new->final_g = now->final_g;
  
  if (ccd_flag) {
    
    /* 展開単語の接続点の音素HMMをnewphoneにセットする. 
       元仮説 now との単語間の音素環境依存性を考慮する */
    /* set the triphone at the connection point to 'newphone', considering
       cross-word context dependency to 'now' */
    newphone = get_right_context_HMM(winfo->wseq[word][winfo->wlen[word]-1], now->last_ph->name, hmminfo);
    if (newphone == NULL) {	/* triphone not found */
      /* fallback to the original bi/mono-phone */
      /* error if the original is pseudo phone (not explicitly defined
	 in hmmdefs/hmmlist) */
      /* exception: word with 1 phone (triphone may exist in the next expansion */
      if (winfo->wlen[word] > 1 && winfo->wseq[word][winfo->wlen[word]-1]->is_pseudo){
	error_missing_right_triphone(winfo->wseq[word][winfo->wlen[word]-1], now->last_ph->name);
      }
      newphone = winfo->wseq[word][winfo->wlen[word]-1];
    }
    
    /* 元仮説をscanした時の末端音素HMM -> 新仮説の直前音素HMM */
    /* inherit last_ph */
    new->last_ph = now->last_ph;
    if (hmminfo->multipath) {
      new->last_ph_sp_attached = now->last_ph_sp_attached;
    }
    
    /* backscan用接続ポイントのスコア g_prev[] をコピー */
    /* copy g_prev[] that are scores at backscan connection point */
    for (t=0;t<peseqlen;t++) {
      new->g_prev[t] = now->g_prev[t];
    }
    
  } else {			/* not triphone */
    
    /* 展開単語の接続(=終端)の音素HMMをnewphoneにセット */
    /* set the phone at the connection point to 'newphone' */
    newphone = winfo->wseq[word][winfo->wlen[word]-1];
  }


  /* 接続確率を与える */
  new->lscore = nword->lscore;

  if (! hmminfo->multipath) {
    /* a_value: 接続点の遷移確率 */
    /* a_value: transition probability of connection point */
    i = hmm_logical_state_num(newphone);
    a_value = (hmm_logical_trans(newphone))->a[i-2][i-1];
  }

  /***************************************************************************/
  /* 前向き(第２パス),後ろ向き(第１パス)トレリスを接続し最尤接続点を見つける */
  /* connect forward/backward trellis to look for the best connection time   */
  /***************************************************************************/

  if (hmminfo->multipath) {
    startt = peseqlen-1;
  } else {
    startt = peseqlen-2;
    new->g[startt+1] = LOG_ZERO;
  }

  /*-----------------------------------------------------------------*/
  /* 単語トレリスを探して, 次単語の最尤接続点を発見する */
  /* determine the best connection time of the new word, seeking the word
     trellis */
  /*-----------------------------------------------------------------*/

  /* update new->g[t] */
  if (hmminfo->multipath) {
    for(t=startt;t>=0;t--) {
      new->g[t] = now->g[t] + nword->lscore;
    }
  } else {
    for(t=startt;t>=0;t--) {
      new->g[t] = now->g[t+1] + a_value + nword->lscore;
    }
  }

  new->tre = NULL;

  if (r->lmtype == LM_DFA && !r->config->pass2.looktrellis_flag) {
    /* すべてのフレームにわたって最尤を探す */
    /* search for best trellis word throughout all frame */
    for(t = startt; t >= 0; t--) {
      tre = bt_binsearch_atom(backtrellis, t, (WORD_ID) word);
      if (tre == NULL) continue;
      totalscore = new->g[t] + tre->backscore;
      if (! hmminfo->multipath) {
	if (newphone->is_pseudo) {
	  tmpp = outprob_cd(&(r->am->hmmwrk), t, &(newphone->body.pseudo->stateset[newphone->body.pseudo->state_num-2]), param);
	} else {
	  tmpp = outprob_state(&(r->am->hmmwrk), t, newphone->body.defined->s[newphone->body.defined->state_num-2], param);
	}
	totalscore += tmpp;
      }
      if (new->score < totalscore) {
	new->score = totalscore;
	new->bestt = t;
	new->estimated_next_t = tre->begintime - 1;
	new->tre  =  tre;
      }
    }

    return;

  }
      
  /* この展開単語のトレリス上の終端時間の前後のみスキャンする
     前後に連続して存在するフレームについてのみ計算 */
  /* search for best trellis word only around the estimated time */
  /* 1. search forward */
  for(t = (nword->tre)->endtime; t >= 0; t--) {
    tre = bt_binsearch_atom(backtrellis, t, (WORD_ID) word);
    if (tre == NULL) break;	/* go to 2 if the trellis word disappear */
    totalscore = new->g[t] + tre->backscore;
    if (! hmminfo->multipath) {
      if (newphone->is_pseudo) {
	tmpp = outprob_cd(&(r->am->hmmwrk), t, &(newphone->body.pseudo->stateset[newphone->body.pseudo->state_num-2]), param);
      } else {
	tmpp = outprob_state(&(r->am->hmmwrk), t, newphone->body.defined->s[newphone->body.defined->state_num-2], param);
      }
      totalscore += tmpp;
    }
    if (new->score < totalscore) {
      new->score = totalscore;
      new->bestt = t;
      new->estimated_next_t = tre->begintime - 1;
      new->tre = tre;
    }
  }
  /* 2. search backward */
  for(t = (nword->tre)->endtime + 1; t <= startt; t++) {
    tre = bt_binsearch_atom(backtrellis, t, (WORD_ID) word);
    if (tre == NULL) break;	/* end if the trellis word disapper */
    totalscore = new->g[t] + tre->backscore;
    if (! hmminfo->multipath) {
      if (newphone->is_pseudo) {
	tmpp = outprob_cd(&(r->am->hmmwrk), t, &(newphone->body.pseudo->stateset[newphone->body.pseudo->state_num-2]), param);
      } else {
	tmpp = outprob_state(&(r->am->hmmwrk), t, newphone->body.defined->s[newphone->body.defined->state_num-2], param);
      }
      totalscore += tmpp;
    }
    if (new->score < totalscore) {
      new->score = totalscore;
      new->bestt = t;
      new->estimated_next_t = tre->begintime - 1;
      new->tre = tre;
    }
  }

}

/**********************************************************************/
/********** 初期仮説の生成                 ****************************/
/********** Generate an initial hypothesis ****************************/
/**********************************************************************/

/** 
 * <JA>
 * 与えられた単語から初期仮説を生成する. 
 * 
 * @param new [out] 新たに生成された仮説が格納される
 * @param nword [in] 初期仮説単語の情報
 * @param param [in] 入力パラメータ列
 * @param r [in] 認識処理インスタンス
 * 
 * </JA>
 * <EN>
 * Generate an initial hypothesis from given word.
 * 
 * @param new [out] pointer to save the newly generated hypothesis
 * @param nword [in] words of the first candidates
 * @param param [in] input parameter vector
 * @param r [in] recognition process instance
 *
 * </EN>
 * @callgraph
 * @callergraph
 */
void
start_word(NODE *new, NEXTWORD *nword, HTK_Param *param, RecogProcess *r)
{
  HMM_Logical *newphone;
  WORD_ID word;
  LOGPROB tmpp;
  int t;
  TRELLIS_ATOM *tre = NULL;

  BACKTRELLIS *backtrellis;
  WORD_INFO *winfo;
  int peseqlen;
  boolean ccd_flag;

  backtrellis = r->backtrellis;
  winfo = r->lm->winfo;
  peseqlen = r->peseqlen;
  ccd_flag = r->ccd_flag;

  /* initialize data */
  word = nword->id;
  new->score = LOG_ZERO;
  new->seqnum = 1;
  new->seq[0] = word;

  new->state = nword->next_state;
  new->totallscore = nword->lscore;

  /* cross-word triphone handling is not needed on startup */
  newphone = winfo->wseq[word][winfo->wlen[word]-1];
  if (ccd_flag) {
    new->last_ph = NULL;
    new->last_ph_sp_attached = FALSE;
  }
  new->lscore = nword->lscore;

  new->g[peseqlen-1] = nword->lscore;
  
  for (t=peseqlen-1; t>=0; t--) {
    tre = bt_binsearch_atom(backtrellis, t, word);
    if (tre != NULL) {
      if (r->graphout) {
	new->bestt = peseqlen-1;
      } else {
	new->bestt = t;
      }
      new->score = new->g[peseqlen-1] + tre->backscore;
      if (! r->am->hmminfo->multipath) {
	if (newphone->is_pseudo) {
	  tmpp = outprob_cd(&(r->am->hmmwrk), peseqlen-1, &(newphone->body.pseudo->stateset[newphone->body.pseudo->state_num-2]), param);
	} else {
	  tmpp = outprob_state(&(r->am->hmmwrk), peseqlen-1, newphone->body.defined->s[newphone->body.defined->state_num-2], param);
	}
	new->score += tmpp;
      }
      new->estimated_next_t = tre->begintime - 1;
      new->tre = tre;
      break;
    }
  }
  if (tre == NULL) {		/* no word in backtrellis */
    new->score = LOG_ZERO;
  }

}


/** 
 * <JA>
 * 終端処理：終端まで達した文仮説の最終的なスコアをセットする. 
 * 
 * @param now [in] 終端まで達した仮説
 * @param new [out] 最終的な文仮説のスコアを格納する場所へのポインタ
 * @param param [in] 入力パラメータ列
 * @param r [in] 認識処理インスタンス
 * 
 * </JA>
 * <EN>
 * Hypothesis termination: set the final sentence scores of hypothesis
 * that has already reached to the end.
 * 
 * @param now [in] hypothesis that has already reached to the end
 * @param new [out] pointer to save the final sentence information
 * @param param [in] input parameter vectors
 * @param r [in] recognition process instance
 * 
 * </EN>
 * @callgraph
 * @callergraph
 */
void
last_next_word(NODE *now, NODE *new, HTK_Param *param, RecogProcess *r)
{
  cpy_node(new, now);
  /* 最終スコアを設定 */
  /* update the final score */
  if (r->am->hmminfo->multipath) {
    new->score = now->final_g;
  } else {
    new->score = now->g[0];
  }
}


#endif /* PASS2_STRICT_IWCD */

/* end of file */
