/**
 * @file   search_bestfirst_v2.c
 * 
 * <JA>
 * @brief  第2パスのViterbi演算および仮説スコア計算 (通常版)
 *
 * ここでは，第2パスにおいて探索中の仮説のViterbiスコアの更新演算，
 * 次単語とのトレリス接続，および仮説のスコア計算を行う関数が定義されて
 * います. 
 *
 * 単語接続部の単語間音素環境依存性は，正確な nextscan アルゴリズムを用います. 
 * このファイルで定義されている関数は，config.h において PASS2_STRICT_IWCD
 * が define であるときに使用されます. 逆に上記が undef であるときは，
 * search_bestfirst_v1.c の関数が用いられます. 
 *
 * Backscan では，デコーディングの精度を重視して，次単語とその前の単語に
 * おける単語間音素コンテキストは仮説展開時にすべて厳密に計算されます. 
 * Backscan を行なう search_bestfirst_v1.c が，仮説の POP 時に行なうのに
 * 比べて，ここでは仮説生成の時点で正確なスコアを計算するため，
 * スコア精度は高い. ただし，生成されるすべての仮説に対して
 * (たとえスタックに入らない仮説であっても)トライフォンの再計算を行なうため，
 * 計算量は backscan に比べて増大します. 
 * </JA>
 * 
 * <EN>
 * @brief  Viterbi path update and scoring on the second pass (standard version)
 *
 * This file has functions for score calculations on the 2nd pass.
 * It includes Viterbi path update calculation of a hypothesis, calculations
 * of scores and word trellis connection at word expansion.
 * 
 * The cross-word triphone will be computed just at word expansion time,
 * for precise scoring.  This is called "nextscan" altgorithm. These
 * functions are enabled when PASS2_STRICT_IWCD is DEFINED in config.h.
 * If undefined, the "backscan" functions in search_bestfirst_v1.c will be
 * used instead.
 *
 * Here in nextscan algorithm, all cross-word context dependencies between
 * next word and source hypothesis are computed as soon as a new hypotheses
 * is expanded.  As the precise cross-word triphone score is applied on
 * hypothesis generation with no delay, more accurate search-time score can
 * be obtained than the delayed backscan method in search_bestfirst_v1.c.
 * On the other hand, the computational cost grows much by re-calculating
 * forward score of cross-word triphones for all the generated hypothethes,
 * even non-promising ones.
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Mon Sep 12 00:58:50 2005
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

/* By "fast" setting (default), search_bestfirst_v1.c is used for faster
   decoding.  Please specify option "--enable-setup=standard" or
   "--enable-strict-iwcd2" at "./configure" to activate this. */

#include <julius/julius.h>

#ifdef PASS2_STRICT_IWCD

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
  if (src->region->ccd_flag) {
    dst->last_ph = src->last_ph;
    dst->last_ph_sp_attached = src->last_ph_sp_attached;
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
    tmp = (NODE *)mymalloc(sizeof(NODE));
    tmp->g = (LOGPROB *)mymalloc(sizeof(LOGPROB) * peseqlen);
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
    tmp->totallscore = LOG_ZERO;
  }
  tmp->endflag = FALSE;
  tmp->seqnum = 0;
  for(i = 0; i < peseqlen; i++) {
    tmp->g[i] = LOG_ZERO;
  }
  tmp->final_g = LOG_ZERO;
#ifdef VISUALIZE
  tmp->popnode = NULL;
#endif
  if (r->graphout) {
    tmp->prevgraph = NULL;
    tmp->lastcontext = NULL;
  }

  tmp->region = r;

  return(tmp);
}


/**********************************************************************/
/************ 前向きトレリス展開と尤度計算             ****************/
/************ Expand trellis and update forward score *****************/
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

  maxwn = r->lm->winfo->maxwn + 10;	/* CCDによる変動を考慮 */
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

  dwrk->wef = NULL;
  dwrk->wes = NULL;
  dwrk->wend_token_frame[0] = NULL;
  dwrk->wend_token_frame[1] = NULL;
  dwrk->wend_token_gscore[0] = NULL;
  dwrk->wend_token_gscore[1] = NULL;
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  if (r->graphout) {
    dwrk->wef = (short *)mymalloc(sizeof(short) * r->peseqlen);
    dwrk->wes = (LOGPROB *)mymalloc(sizeof(LOGPROB) * r->peseqlen);
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
  free(dwrk->wordtrellis[0]);
  free(dwrk->wordtrellis[1]);
  free(dwrk->g);
  free(dwrk->phmmseq);
  if (dwrk->has_sp) {
    free(dwrk->has_sp);
    dwrk->has_sp = NULL;
  }
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  if (dwrk->wef) {
    free(dwrk->wef);
    free(dwrk->wes);
    free(dwrk->wend_token_frame[0]);
    free(dwrk->wend_token_frame[1]);
    free(dwrk->wend_token_gscore[0]);
    free(dwrk->wend_token_gscore[1]);
    dwrk->wef = NULL;
  }
#endif
}


/**********************************************************************/
/************ 仮説の前向き尤度計算                  *******************/
/************ Compute forward score of a hypothesis *******************/
/**********************************************************************/

/* 与えられた音素のならび phmmseq[0..phmmlen-1]に対してviterbi計算を行う. 
   g[0..framelen-1] のスコアを初期値として g_new[0..framelen-1]に更新値を代入. 
   最低 least_frame まではscanする. */
/* Viterbi computation for the given phoneme sequence 'phmmseq[0..phmmlen-1]'
   with g[0..framelen-1] as initial values.  The results are stored in
   g_new[0..framelen-1].  Scan should not terminate at least it reaches
   'least_frame'. */
/** 
 * <JA>
 * 与えられた音素の並びに対して Viterbi 計算を行い，前向きスコアを
 * 更新する汎用関数. 
 * 
 * @param g [in] 現在の時間ごとの前向きスコア
 * @param g_new [out] 更新後の新たな前向きスコアを格納するバッファ
 * @param phmmseq [in] 音素HMMの並び
 * @param has_sp [in] short-pause location
 * @param phmmlen [in] @a phmmseq の長さ
 * @param param [in] 入力パラメータ
 * @param framelen [in] 入力フレーム長
 * @param least_frame [in] ビーム設定時，このフレーム数以上は Viterbi計算する
 * @param final_g [in] final g scores
 * @param wordend_frame_src [in] 現在の単語終端フレームトークン
 * @param wordend_frame_dst [out] 更新後の新たな単語終端フレームトークン
 * @param wordend_gscore_src [in] 現在の単語終端スコアトークン
 * @param wordend_gscore_dst [out] 更新後の新たな単語終端スコアトークン
 * @param r [in] recognition process instance
 * </JA>
 * <EN>
 * Generic function to perform Viterbi path updates for given phoneme
 * sequence.
 * 
 * @param g [in] current forward scores at each input frame
 * @param g_new [out] buffer to save the resulting score updates
 * @param phmmseq [in] phoneme sequence to perform Viterbi
 * @param has_sp [in] short-pause location
 * @param phmmlen [in] length of @a phmmseq.
 * @param param [in] input parameter vector
 * @param framelen [in] input frame length to compute
 * @param least_frame [in] Least frame length to force viterbi even with beam
 * @param final_g [in] final g scores
 * @param wordend_frame_src [in] current word-end frame tokens
 * @param wordend_frame_dst [out] buffer to store updated word-end frame tokens
 * @param wordend_gscore_src [in] current word-end score tokens
 * @param wordend_gscore_dst [out] buffer to store updated word-end score tokens
 * @param r [in] recognition process instance
 * 
 * </EN>
 */
static void
do_viterbi(LOGPROB *g, LOGPROB *g_new, HMM_Logical **phmmseq, boolean *has_sp, int phmmlen, HTK_Param *param, int framelen, int least_frame, LOGPROB *final_g, short *wordend_frame_src, short *wordend_frame_dst, LOGPROB *wordend_gscore_src, LOGPROB *wordend_gscore_dst, RecogProcess *r) /* has_sp and final_g is for multipath only */
{
  HMM *whmm;			/* HMM */
  int wordhmmnum;		/* length of above */
  int startt;			/* scan start frame */
  LOGPROB tmpmax,tmpscore;	/* variables for Viterbi process */
  A_CELL *ac;
  int t,i,j;
  boolean node_exist_p;
  int tn;		       ///< Temporal pointer to current buffer
  int tl;		       ///< Temporal pointer to previous buffer

  /* store global values to local for rapid access */
  StackDecode *dwrk;
  WORD_INFO *winfo;
  HTK_HMM_INFO *hmminfo;
  LOGPROB *framemaxscore;
#ifdef SCAN_BEAM
  LOGPROB scan_beam_thres;
#endif

  dwrk = &(r->pass2);
  winfo = r->lm->winfo;
  hmminfo = r->am->hmminfo;
  framemaxscore = r->pass2.framemaxscore;
#ifdef SCAN_BEAM
  scan_beam_thres = r->config->pass2.scan_beam_thres;
#endif


#ifdef TCD
  jlog("DEBUG: scan for:");
  for (i=0;i<phmmlen;i++) {
    jlog(" %s", phmmseq[i]->name);
  }
  jlog("\n");
#endif
  
  /* 単語HMMを作る */
  /* make word HMM */
  whmm = new_make_word_hmm(hmminfo, phmmseq, phmmlen, has_sp);
  if (whmm == NULL) {
    j_internal_error("Error: failed to make word hmm\n");
  }
  wordhmmnum = whmm->len;
  if (wordhmmnum >= winfo->maxwn + 10) {
    j_internal_error("do_viterbi: word too long (>%d)\n", winfo->maxwn + 10);
  }

  /* scan開始点を検索 -> starttへ*/
  /* search for the start frame -> set to startt */
  for(t = framelen-1; t >=0 ; t--) {
    if (
#ifdef SCAN_BEAM
	g[t] > framemaxscore[t] - scan_beam_thres &&
#endif
	g[t] > LOG_ZERO) {
      break;
    }
  }
  if (t < 0) {			/* no node has score > LOG_ZERO */
    /* reset all scores and end */
    for(t=0;t<framelen;t++) {
      g_new[t] = LOG_ZERO;
#ifdef GRAPHOUT_PRECISE_BOUNDARY
      if (r->graphout) {
	wordend_frame_dst[t] = -1;
	wordend_gscore_dst[t] = LOG_ZERO;
      }
#endif
    }
    free_hmm(whmm);
    return;
  }
  startt = t;
  
  /* 開始点以降[startt+1..framelen-1] の g_new[] をリセット */
  /* clear g_new[] for [startt+1..framelen-1] */
  for(t=framelen-1;t>startt;t--) {
    g_new[t] = LOG_ZERO;
#ifdef GRAPHOUT_PRECISE_BOUNDARY
    if (r->graphout) {
      wordend_frame_dst[t] = -1;
      wordend_gscore_dst[t] = LOG_ZERO;
    }
#endif
  }

  /*****************/
  /* viterbi start */
  /*****************/

  /* set initial swap buffer */
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
    /* 時間 [startt] 上の値を初期化 */
    /* initialize scores on frame [startt] */
    for(i=0;i<wordhmmnum-1;i++) dwrk->wordtrellis[tn][i] = LOG_ZERO;
    dwrk->wordtrellis[tn][wordhmmnum-1] = g[startt] + outprob(&(r->am->hmmwrk), startt, &(whmm->state[wordhmmnum-1]), param);
    g_new[startt] = dwrk->wordtrellis[tn][0];
#ifdef GRAPHOUT_PRECISE_BOUNDARY
    if (r->graphout) {
      dwrk->wend_token_frame[tn][wordhmmnum-1] = wordend_frame_src[startt];
      dwrk->wend_token_gscore[tn][wordhmmnum-1] = wordend_gscore_src[startt];
      wordend_frame_dst[startt] = dwrk->wend_token_frame[tn][0];
      wordend_gscore_dst[startt] = dwrk->wend_token_gscore[tn][0];
    }
#endif
  }
  
  /* メインループ: startt から始まり 0 に向かって Viterbi 計算 */
  /* main loop: start from [startt], and compute Viterbi toward [0] */
  for(t = hmminfo->multipath ? startt : startt - 1; t >= 0; t--) {
    
    /* wordtrellisのワークエリアをスワップ */
    /* swap workarea of wordtrellis */
    i = tn; tn = tl; tl = i;

    node_exist_p = FALSE;	/* TRUE if there is at least 1 survived node in this frame */

    if (! hmminfo->multipath) {
    
      /* 端のノード [t][wordhmmnum-1]は，内部遷移 か g[]の高い方になる */
      /* the edge node [t][wordhmmnum-1] is either internal transitin or g[] */
      tmpscore = LOG_ZERO;
      for (ac=whmm->state[wordhmmnum-1].ac;ac;ac=ac->next) {
	if (tmpscore < dwrk->wordtrellis[tl][ac->arc] + ac->a) {
	  j = ac->arc;
	  tmpscore = dwrk->wordtrellis[tl][ac->arc] + ac->a;
	}
      }
      if (g[t] > tmpscore) {
	tmpmax = g[t];
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	if (r->graphout) {
	  dwrk->wend_token_frame[tn][wordhmmnum-1] = wordend_frame_src[t];
	  dwrk->wend_token_gscore[tn][wordhmmnum-1] = wordend_gscore_src[t];
	}
#endif
      } else {
	tmpmax = tmpscore;
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

    }

    /* node[wordhmmnum-2..0]についてトレリスを展開 */
    /* expand trellis for node [t][wordhmmnum-2..0] */
    for(i=wordhmmnum-2;i>=0;i--) {
      
      /* 最尤パスと最尤スコア tmpmax を見つける */
      /* find most likely path and the max score 'tmpmax' */
      tmpmax = LOG_ZERO;
      for (ac=whmm->state[i].ac;ac;ac=ac->next) {
	if (hmminfo->multipath) {
	  if (ac->arc == wordhmmnum-1) tmpscore = g[t];
	  else if (t + 1 > startt) tmpscore = LOG_ZERO;
	  else tmpscore = dwrk->wordtrellis[tl][ac->arc];
	  tmpscore += ac->a;
	} else {
	  tmpscore = dwrk->wordtrellis[tl][ac->arc] + ac->a;
	}
	if (tmpmax < tmpscore) {
	  tmpmax = tmpscore;
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
 	dwrk->wordtrellis[tn][i] = tmpmax;
	if (! hmminfo->multipath || i > 0) {
	  dwrk->wordtrellis[tn][i] += outprob(&(r->am->hmmwrk), t, &(whmm->state[i]), param);
	}
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	if (r->graphout) {
	  if (hmminfo->multipath) {
	    if (j == wordhmmnum-1) {
	      dwrk->wend_token_frame[tn][i] = wordend_frame_src[t];
	      dwrk->wend_token_gscore[tn][i] = wordend_gscore_src[t];
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
      }
    } /* end of node loop */

    /* 時間 t のViterbi計算終了. 新たな前向きスコア g_new[t] をセット */
    /* Viterbi end for frame [t].  set the new forward score g_new[t] */
    g_new[t] = dwrk->wordtrellis[tn][0];
#ifdef GRAPHOUT_PRECISE_BOUNDARY
    if (r->graphout) {
    /* new wordend */
      wordend_frame_dst[t] = dwrk->wend_token_frame[tn][0];
      wordend_gscore_dst[t] = dwrk->wend_token_gscore[tn][0];
    }
#endif
    /* 指定された least_frame より先まで t が進んでおり，かつこの t において
       スコアエンベロープによって生き残ったノードが一つも無かった場合,
       このフレームで計算を打ち切りそれ以上先([0..t-1])は計算しない */
    /* if frame 't' already reached the 'least_frame' and no node was
       survived in this frame (all nodes pruned by score envelope),
       terminate computation at this frame and do not computer further
       frame ([0..t-1]). */
    if (t < least_frame && (!node_exist_p)) {
      /* crear the rest scores */
      for (i=t-1;i>=0;i--) {
	g_new[i] = LOG_ZERO;
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	if (r->graphout) {
	  wordend_frame_dst[i] = -1;
	  wordend_gscore_dst[i] = LOG_ZERO;
	}
#endif
      }
      /* terminate loop */
      break;
    }
    
  } /* end of time loop */

  if (hmminfo->multipath) {
    /* 前向きスコアの最終値を計算 (状態 0 から時間 0 への遷移) */
    /* compute the total forward score (transition from state 0 to frame 0 */
    if (t < 0) {			/* computed till the end */
      tmpmax = LOG_ZERO;
      for(ac=whmm->state[0].ac;ac;ac=ac->next) {
	tmpscore = dwrk->wordtrellis[tn][ac->arc] + ac->a;
	if (tmpmax < tmpscore) tmpmax = tmpscore;
      }
      *final_g = tmpmax;
    } else {
      *final_g = LOG_ZERO;
    }
  }

  /* free work area */
  free_hmm(whmm);
}

/** 
 * <JA>
 * 最後の1音素に対して Viterbi 計算を進める. 
 * 
 * @param now [in] 展開元の文仮説. 一音素前の前向きスコアが g[] にあるとする. 
 * @param new [out] 計算後の前向きスコアが g[] に格納される. 
 * @param lastphone [in] Viterbi計算を行う音素HMM
 * @param sp [in] short-pause insertion
 * @param param [in] 入力ベクトル列
 * @param r [in] 認識処理インスタンス
 * </JA>
 * <EN>
 * Proceed Viterbi for the last one phoneme.
 * 
 * @param now [in] source hypothesis where the forward scores prior to the
 * last one phone is stored at g[]
 * @param new [out] the resulting updated forward scores will be saved to g[]
 * @param lastphone [in] phone HMM for the Viterbi processing
 * @param sp [in] short-pause insertion
 * @param param [in] input vectors
 * @param r [in] recognition process instance
 * </EN>
 */
static void
do_viterbi_next_word(NODE *now, NODE *new, HMM_Logical *lastphone, boolean sp, HTK_Param *param, RecogProcess *r) /* sp is for multipath only */
{
  int t, n;
  LOGPROB a_value;		/* for non multi-path */
  int peseqlen;
  boolean multipath;
  StackDecode *dwrk;

  dwrk = &(r->pass2);

  multipath = r->am->hmminfo->multipath;

  peseqlen = r->peseqlen;
  
  if (! multipath) {

    /* もし展開元仮説の最後の単語の音素長が 1 であれば，その音素は
       直前の scan_word で計算されていない. この場合, now->g[] に以前の
       初期値が格納されている. 
       もし音素長が１以上であれば，now->g[] はその手前まで計算した状態
       のスコアが入っているので,now->g[t] から初期値を設定する必要がある */
    /* If the length of last word is 1, it means the last phone was not
       scanned in the last call of scan_word().  In this case, now->g[]
       keeps the previous initial value, so start viterbi with the old scores.
       If the length is more than 1, the now->g[] keeps the values of the
       scan result till the previous phone, so make initial value
       considering last transition probability. */
    if (r->lm->winfo->wlen[now->seq[now->seqnum-1]] > 1) {
      n = hmm_logical_state_num(lastphone);
      a_value = (hmm_logical_trans(lastphone))->a[n-2][n-1];
      for(t=0; t<peseqlen-1; t++) dwrk->g[t] = now->g[t+1] + a_value;
      dwrk->g[peseqlen-1] = LOG_ZERO;
    } else {
      for(t=0; t<peseqlen; t++) dwrk->g[t] = now->g[t];
    }

  } else {
  
    for(t=0; t<peseqlen; t++) dwrk->g[t] = now->g[t];
    dwrk->phmmseq[0] = lastphone;
    if (r->lm->config->enable_iwsp) dwrk->has_sp[0] = sp;

  }
  
  do_viterbi(dwrk->g, new->g,
	     multipath ? dwrk->phmmseq : &lastphone,
	     (r->lm->config->enable_iwsp && multipath) ? dwrk->has_sp : NULL,
	     1, param, peseqlen, now->estimated_next_t, &(new->final_g)
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	     , now->wordend_frame, new->wordend_frame
	     , now->wordend_gscore, new->wordend_gscore
#else
	     , NULL, NULL
	     , NULL, NULL
#endif
	     , r
	     );

#ifdef GRAPHOUT_PRECISE_BOUNDARY
  if (! multipath) {
    if (r->graphout) {
      /* 次回の next_word 用に境界情報を調整 */
      /* proceed word boundary for one step for next_word */
      new->wordend_frame[r->peseqlen-1] = new->wordend_frame[0];
      new->wordend_gscore[r->peseqlen-1] = new->wordend_gscore[0];
      for (t=0;t<r->peseqlen-1;t++) {
	new->wordend_frame[t] = new->wordend_frame[t+1];
	new->wordend_gscore[t] = new->wordend_gscore[t+1];
      }
    }
  }
#endif
}

/** 
 * <JA>
 * 最後の1単語の前向きトレリスを計算して，文仮説の前向き尤度を更新する. 
 * 
 * @param now [i/o] 文仮説
 * @param param [in] 入力パラメータ列
 * @param r [in] 認識処理インスタンス
 * </JA>
 * <EN>
 * Compute the forward viterbi for the last word to update forward scores
 * and ready for word connection.
 * 
 * @param now [i/o] hypothesis
 * @param param [in] input parameter vectors
 * @param r [in] recognition process instance
 * </EN>
 * @callgraph
 * @callergraph
 */
void
scan_word(NODE *now, HTK_Param *param, RecogProcess *r)
{
  int   i,t;
  WORD_ID word;
  int phmmlen;
  HMM_Logical *tailph;

  /* store global values to local for rapid access */
  WORD_INFO *winfo;
  HTK_HMM_INFO *hmminfo;
  int peseqlen;
  boolean ccd_flag;
  boolean enable_iwsp;		/* multipath */
  StackDecode *dwrk;

  dwrk = &(r->pass2);
  winfo = r->lm->winfo;
  hmminfo = r->am->hmminfo;
  peseqlen = r->peseqlen;
  ccd_flag = r->ccd_flag;
  if (hmminfo->multipath) {
    enable_iwsp = r->lm->config->enable_iwsp;
  }
  
#ifndef GRAPHOUT_PRECISE_BOUNDARY
  if (r->graphout) {
    if (ccd_flag) {
      now->tail_g_score = now->g[now->bestt];
    }
  }
#endif

  /* ----------------------- prepare phoneme sequence ------------------ */
  /* triphoneなら先頭の1音素はここでは対象外(あとでnext_wordでやる) */
  /*             末尾の1音素はコンテキストにしたがって置換 */
  /* with triphone, modify the tail phone of the last word according to the
     previous word, and do not compute the head phone here (that will be
     computed later in next_word() */
  word = now->seq[now->seqnum-1];
  
#ifdef TCD
    jlog("DEBUG: w=");
    for(i=0;i<winfo->wlen[word];i++) {
      jlog(" %s",(winfo->wseq[word][i])->name);
    }
    if (ccd_flag) {
      if (now->last_ph != NULL) {
	jlog(" | %s", (now->last_ph)->name);
      }
    }
    jlog("\n");
#endif /* TCD */
    
  if (ccd_flag) {
    
    /* the tail triphone of the last word varies by context */
    if (now->last_ph != NULL) {
      tailph = get_right_context_HMM(winfo->wseq[word][winfo->wlen[word]-1], now->last_ph->name, hmminfo);
      if (tailph == NULL) {
	/* fallback to the original bi/mono-phone */
	/* error if the original is pseudo phone (not explicitly defined
	   in hmmdefs/hmmlist) */
	/* exception: word with 1 phone (triphone may exist in the next expansion */
	if (winfo->wlen[word] > 1 && winfo->wseq[word][winfo->wlen[word]-1]->is_pseudo){
	  error_missing_right_triphone(winfo->wseq[word][winfo->wlen[word]-1], now->last_ph->name);
	}

	tailph = winfo->wseq[word][winfo->wlen[word]-1];
      }
    } else {
      tailph = winfo->wseq[word][winfo->wlen[word]-1];
    }
    /* 長さ1の単語は次のnextwordでさらに変化するのでここではscanしない */
    /* do not scan word if the length is 1, as it further varies in the
       following next_word() */
    if (winfo->wlen[word] == 1) {
      now->last_ph = tailph;
      if (enable_iwsp && hmminfo->multipath) now->last_ph_sp_attached = TRUE;
#ifdef GRAPHOUT_PRECISE_BOUNDARY
      if (r->graphout) {
	/* 単語境界伝搬情報を初期化 */
	/* initialize word boundary propagation info */
	for (t=0;t<peseqlen;t++) {
	  now->wordend_frame[t] = t;
	  now->wordend_gscore[t] = now->g[t];
	}
      }
#endif
#ifdef TCD
      jlog("DEBUG: suspended as %s\n", (now->last_ph)->name);
#endif
      return;
    }

    /* scan範囲の音素列を準備 */
    /* prepare HMM of the scan range */
    phmmlen = winfo->wlen[word] - 1;
    if (phmmlen > dwrk->phmmlen_max) {
      j_internal_error("scan_word: num of phonemes in a word exceed phmmlenmax (%d) ?\n", dwrk->phmmlen_max);
    }
    for (i=0;i<phmmlen-1;i++) {
      dwrk->phmmseq[i] = winfo->wseq[word][i+1];
    }
    dwrk->phmmseq[phmmlen-1] = tailph;
    if (enable_iwsp && hmminfo->multipath) {
      for (i=0;i<phmmlen-1;i++) dwrk->has_sp[i] = FALSE;
      dwrk->has_sp[phmmlen-1] = TRUE;
    }

  } else {			/* ~ccd_flag */

    phmmlen = winfo->wlen[word];
    for (i=0;i<phmmlen;i++) dwrk->phmmseq[i] = winfo->wseq[word][i];
    if (enable_iwsp && hmminfo->multipath) {
      for (i=0;i<phmmlen;i++) dwrk->has_sp[i] = FALSE;
      dwrk->has_sp[phmmlen-1] = TRUE;
    }

  }

  /* 元のg[]をいったん待避しておく */
  /* temporally keeps the original g[] */
  for (t=0;t<peseqlen;t++) dwrk->g[t] = now->g[t];

#ifdef GRAPHOUT_PRECISE_BOUNDARY
  if (r->graphout) {
    /* 単語境界伝搬情報を初期化 */
    /* initialize word boundary propagation info */
    for (t=0;t<peseqlen;t++) {
      dwrk->wef[t] = t;
      dwrk->wes[t] = now->g[t];
    }
  }
#endif

  /* viterbiを実行して g[] から now->g[] を更新する */
  /* do viterbi computation for phmmseq from g[] to now->g[] */
  do_viterbi(dwrk->g, now->g, dwrk->phmmseq, (enable_iwsp && hmminfo->multipath) ? dwrk->has_sp : NULL, 
	     phmmlen, param, peseqlen, now->estimated_next_t, &(now->final_g)
#ifdef GRAPHOUT_PRECISE_BOUNDARY
	     /* 単語境界情報 we[] から now->wordend_frame[] を更新する */
	     /* propagate word boundary info from we[] to now->wordend_frame[] */
	     , dwrk->wef, now->wordend_frame
	     , dwrk->wes, now->wordend_gscore
#else
	     , NULL, NULL
	     , NULL, NULL
#endif
	     , r
	     );
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

  if (ccd_flag) {
    /* 次回のために now->last_ph を更新 */
    /* update 'now->last_ph' for future scan_word() */
    now->last_ph = winfo->wseq[word][0];
    if (enable_iwsp && hmminfo->multipath) now->last_ph_sp_attached = FALSE; /* wlen > 1 here */
#ifdef TCD
    jlog("DEBUG: last_ph = %s\n", (now->last_ph)->name);
#endif
  }
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
 * </EN>
 * @callgraph
 * @callergraph
 */
void
next_word(NODE *now, NODE *new, NEXTWORD *nword, HTK_Param *param, RecogProcess *r)
{
  HMM_Logical *lastphone, *newphone;
  LOGPROB *g_src;
  int   t;
  int lastword;
  int   i;
  LOGPROB a_value;
  LOGPROB tmpp;
  int   startt;
  int word;
  TRELLIS_ATOM *tre;
  LOGPROB totalscore;
  BACKTRELLIS *backtrellis;
  WORD_INFO *winfo;
  HTK_HMM_INFO *hmminfo;
  int peseqlen;
  boolean ccd_flag;
  StackDecode *dwrk;

  dwrk = &(r->pass2);
  backtrellis = r->backtrellis;
  winfo = r->lm->winfo;
  hmminfo = r->am->hmminfo;
  peseqlen = r->peseqlen;
  ccd_flag = r->ccd_flag;

  word = nword->id;
  lastword = now->seq[now->seqnum-1];

  /* lastphone (直前単語の先頭音素) を準備 */
  /* prepare lastphone (head phone of previous word) */
  if (ccd_flag) {
    /* 最終音素 triphone を接続単語に会わせて変化 */
    /* modify triphone of last phone according to the next word */
    lastphone = get_left_context_HMM(now->last_ph, winfo->wseq[word][winfo->wlen[word]-1]->name, hmminfo);
    if (lastphone == NULL) {
      /* fallback to the original bi/mono-phone */
      /* error if the original is pseudo phone (not explicitly defined
	 in hmmdefs/hmmlist) */
      /* exception: word with 1 phone (triphone may exist in the next expansion */
      if (now->last_ph->is_pseudo){
	error_missing_left_triphone(now->last_ph, winfo->wseq[word][winfo->wlen[word]-1]->name);
      }
      lastphone = now->last_ph;
    }
  }

  /* newphone (接続単語の末尾音素) を準備 */
  /* prepare newphone (tail phone of next word) */
  if (ccd_flag) {
    newphone = get_right_context_HMM(winfo->wseq[word][winfo->wlen[word]-1], now->last_ph->name, hmminfo);
    if (newphone == NULL) {
      /* fallback to the original bi/mono-phone */
      /* error if the original is pseudo phone (not explicitly defined
	 in hmmdefs/hmmlist) */
      /* exception: word with 1 phone (triphone may exist in the next expansion */
      if (winfo->wlen[word] > 1 && winfo->wseq[word][winfo->wlen[word]-1]->is_pseudo){
	error_missing_right_triphone(winfo->wseq[word][winfo->wlen[word]-1], now->last_ph->name);
      }
      newphone = winfo->wseq[word][winfo->wlen[word]-1];
    }
  } else {
    newphone = winfo->wseq[word][winfo->wlen[word]-1];
  }
  
  /* 単語並び、DFA状態番号、言語スコアを new へ継承・更新 */
  /* inherit and update word sequence, DFA state and total LM score to 'new' */
  new->score = LOG_ZERO;
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
  if (ccd_flag) {
    /* 次仮説の履歴情報として保存 */
    /* keep the lastphone for next scan_word() */
    new->last_ph = lastphone;
    new->last_ph_sp_attached = now->last_ph_sp_attached;
  }

  if (ccd_flag) {
    /* 最後の1音素(lastphone)分をscanし，更新したスコアを new に保存 */
    /* scan the lastphone and set the updated score to new->g[] */
    do_viterbi_next_word(now, new, lastphone,
			 hmminfo->multipath ? now->last_ph_sp_attached : FALSE,
			 param, r);
    g_src = new->g;
  } else {
    g_src = now->g;
#ifdef GRAPHOUT_PRECISE_BOUNDARY
    if (r->graphout) {
      memcpy(new->wordend_frame, now->wordend_frame, sizeof(short)*peseqlen);
      memcpy(new->wordend_gscore, now->wordend_gscore, sizeof(LOGPROB)*peseqlen);
    }
#endif
  }
      
  /* 次回の scan_word に備えて new->g[] を変更しておく */
  /* prepare new->g[] for next scan_word() */
  if (hmminfo->multipath) {
    startt = peseqlen-1;
  } else {
    startt = peseqlen-2;
  }
  i = hmm_logical_state_num(newphone);
  a_value = (hmm_logical_trans(newphone))->a[i-2][i-1];
  if (hmminfo->multipath) {
    for(t=0; t <= startt; t++) {
      new->g[t] = g_src[t] + nword->lscore;
    }
  } else {
    for(t=0; t <= startt; t++) {
      new->g[t] = g_src[t+1] + a_value + nword->lscore;
    }
  }

  /***************************************************************************/
  /* 前向き(第２パス),後ろ向き(第１パス)トレリスを接続し最尤接続点を見つける */
  /* connect forward/backward trellis to look for the best connection time   */
  /***************************************************************************/
  /*-----------------------------------------------------------------*/
  /* 単語トレリスを探して, 次単語の最尤接続点を発見する */
  /* determine the best connection time of the new word, seeking the word
     trellis */
  /*-----------------------------------------------------------------*/

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
	new->tre = tre;
      }
    }

    return;
  }

  /* 最後に参照したTRELLIS_ATOMの終端時間の前後 */
  /* newの推定時間は，上記で採用したTRELLIS_ATOMの始端時間 */

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
  /* 2. search bckward */
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

  /* set current LM score */
  new->lscore = nword->lscore;
  
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
  TRELLIS_ATOM *tre = NULL;
  LOGPROB tmpp;
  int t;

  BACKTRELLIS *backtrellis;
  WORD_INFO *winfo;

  int peseqlen;
  boolean ccd_flag;
  boolean multipath;

  backtrellis = r->backtrellis;
  winfo = r->lm->winfo;
  peseqlen = r->peseqlen;
  ccd_flag = r->ccd_flag;
  multipath = r->am->hmminfo->multipath;

  /* initialize data */
  word = nword->id;
  new->score = LOG_ZERO;
  new->seqnum = 1;
  new->seq[0] = word;

  new->state = nword->next_state;
  new->totallscore = nword->lscore;

  /* set current LM score */
  new->lscore = nword->lscore;

  /* cross-word triphone need not be handled on startup */
  newphone = winfo->wseq[word][winfo->wlen[word]-1];
  if (ccd_flag) {
    new->last_ph = NULL;
  }
  
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
      if (! multipath) {
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
 * </JA>
 * <EN>
 * Hypothesis termination: set the final sentence scores of hypothesis
 * that has already reached to the end.
 * 
 * @param now [in] hypothesis that has already reached to the end
 * @param new [out] pointer to save the final sentence information
 * @param param [in] input parameter vectors
 * @param r [in] recognition process instance
 * </EN>
 * @callgraph
 * @callergraph
 */
void
last_next_word(NODE *now, NODE *new, HTK_Param *param, RecogProcess *r)
{
  cpy_node(new, now);
  if (r->ccd_flag) {
    /* 最終音素分を viterbi して最終スコアを設定 */
    /* scan the last phone and update the final score */
    if (r->am->hmminfo->multipath) {
      do_viterbi_next_word(now, new, now->last_ph, now->last_ph_sp_attached, param, r);
      new->score = new->final_g;
    } else {
      do_viterbi_next_word(now, new, now->last_ph, FALSE, param, r);
      new->score = new->g[0];
    }
  } else {
    if (r->am->hmminfo->multipath) {
      new->score = now->final_g;
    } else {
      new->score = now->g[0];
    }
#ifdef GRAPHOUT_PRECISE_BOUNDARY
    if (r->graphout) {
      /* last boundary has moved to [peseqlen-1] in last scan_word() */
      memcpy(new->wordend_frame, now->wordend_frame, sizeof(short)*r->peseqlen);
      memcpy(new->wordend_gscore, now->wordend_gscore, sizeof(LOGPROB)*r->peseqlen);
    }
#endif
  }
}

#endif /* PASS2_STRICT_IWCD */

/* end of file */
