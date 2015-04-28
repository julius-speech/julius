/**
 * @file   search.h
 * 
 * <JA>
 * @brief  第2パスで使用する仮説候補を扱う構造体
 *
 * ここでは，第2パスのスタックデコーディングで用いられる仮説候補の構造体
 * が定義されています. NODE は部分文候補を保持し，合計スコアや現在のViterbi
 * スコア，言語スコア，信頼度スコア，推定された終端フレームなどの様々な仮説
 * 情報を保持します. WordGraph は単語グラフ生成時にグラフ中の単語をあらわす
 * のに用いられます. NEXTWORD は単語展開時に次単語候補を表現します. POPNODE
 * は探索空間可視化機能 (--enable-visualize) 指定時に，探索の過程を残しておく
 * のに使われます. 
 * </JA>
 * 
 * <EN>
 * @brief  Strucures for handling hypotheses on the 2nd pass.
 * </EN>
 *
 * This file includes definitions for handling hypothesis used on the 2nd
 * pass stack decoding.  Partial sentence hypotheses are stored in NODE
 * structure, with its various information about total scores, viterbi scores,
 * language scores, confidence scores, estimated end frame, and so on.
 * WordGraph express a word in graph, generated through the 2nd pass.
 * NEXTWORD is used to hold next word information at
 * hypothesis expantion stage. POPNODE will be used when Visualization is
 * enabled to store the search trail.
 * 
 * @author Akinobu Lee
 * @date   Wed Sep 07 07:40:11 2005
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

#ifndef __J_SEARCH_H__
#define __J_SEARCH_H__

/**
 * <JA>
 * 第2パスの次単語候補. ある仮説から次に接続しうる単語の集合をあらわすのに
 * 用いられる. 
 * </JA>
 * <EN>
 * Next word candidate in the 2nd pass.  This will be used to hold word
 * candidates that can be connected to a given hypothesis.
 * </EN>
 */
typedef struct __nextword__ {
  WORD_ID id;			///< Word ID
  LOGPROB lscore;		///< Language score of this word (always 0 for dfa)
  int next_state;		///< (dfa) Next DFA grammar state ID
  boolean can_insert_sp;	///< (dfa) TRUE if a short pause can insert between source hypothesis and this word
  TRELLIS_ATOM *tre;		///< Pointer to the corresponding word in trellis
} NEXTWORD;

#ifdef VISUALIZE
/**
 * <JA>
 * 可視化機能用に，第2パスでpopされたトレリス単語の情報を保持する. 
 * </JA>
 * <EN>
 * Store popped trellis words on the 2nd pass for visualization.
 * </EN>
 */
typedef struct __popnode__ {
  TRELLIS_ATOM *tre;		///< Last referred trellis word
  LOGPROB score;		///< Total score when expanded (g(x)+h(x))
  struct __popnode__ *last;	///< Link to previous word context
  struct __popnode__ *next;	///< List pointer to next data
} POPNODE;
#endif /* VISUALIZE */

/**
 * <JA>
 * 第2パスの文仮説
 * </JA>
 * <EN>
 * Sentence hypothesis at 2nd pass
 * </EN>
 */
typedef struct __node__ {
  struct __node__    *next;	///< Link to next hypothesis, used in stack
  struct __node__    *prev;	///< Link to previous hypothesis, used in stack
  boolean endflag;              ///< TRUE if this is a final sentence result
  WORD_ID seq[MAXSEQNUM];	///< Word sequence
  short seqnum;                 ///< Length of @a seq
  LOGPROB score;		///< Total score (forward+backward, LM+AM)
  short bestt;                  ///< Best connection frame of last word in word trellis
  short estimated_next_t;	///< Estimated next connection time frame (= beginning of last word on word trellis): next word hypothesis will be looked up near this frame on word trellis
  LOGPROB *g;			///< Current forward viterbi score in each frame
  LOGPROB final_g;		///< Extra forward score on end of frame for multipath mode
  int state;			///< (dfa) Current DFA state ID
  TRELLIS_ATOM *tre;		///< Trellis word of last word
  
#ifndef PASS2_STRICT_IWCD
  /* for inter-word context dependency, the last phone on previous word
     need to be calculated later */
  LOGPROB *g_prev;		///< Viterbi score back to last 1 phoneme
#endif
  HMM_Logical *last_ph;		///< Last applied triphone
  boolean last_ph_sp_attached;  ///< Last phone which the inter-word sp has been attached for multipath mode
  LOGPROB lscore;		///< N-gram score of last word (will be used for 1-phoneme backscan and graph output, always 0 for dfa
  LOGPROB totallscore;		///< (n-gram) Accumulated language score (LM only)
#ifdef CONFIDENCE_MEASURE
#ifdef CM_MULTIPLE_ALPHA
  LOGPROB cmscore[MAXSEQNUM][100]; ///< Confidence score of each word (multiple)
#else
  LOGPROB cmscore[MAXSEQNUM];	///< Confidence score of each word
#endif /* CM_MULTIPLE_ALPHA */
#endif /* CONFIDENCE_MEASURE */
#ifdef VISUALIZE
  POPNODE *popnode;		///< Pointer to last popped node 
#endif
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  short *wordend_frame;		///< Buffer to store propagated word end score for word boundary adjustment
  LOGPROB *wordend_gscore;	///< Buffer to store propagated scores at word end for word boundary adjustment
#endif
  WordGraph *prevgraph;		///< Graph word corresponding to the last word
  WordGraph *lastcontext;	///< Graph word of next previous word
#ifndef GRAPHOUT_PRECISE_BOUNDARY
  LOGPROB tail_g_score;		///< forward g score for later score adjustment
#endif

  struct __recogprocess__ *region;		///> Where this node belongs to

#ifdef USE_MBR
  float score_mbr; ///< MBR score
#endif
} NODE;

/*
  HOW SCORES ARE CALCULATED:
  
              0         bestt                            T-1
              |-h(n)---->|<------------g(n)--------------|
==============================================================
              |\                                         |
	      .....                                  .....  
                                                            <word trellis>
              |    \estimated_next_t                     |  =backward trellis
--------------------\------------------------------------|  (1st pass)
              |      \                                   |   
seq[seqnum-1] |       \_                                 |   
              |         \bestt                           |   
=========================+====================================================
              |           \                              |<-g[0..T-1]
              |            \                             |   
seq[seqnum-2] |             \__                          |
              |                \                         |
--------------------------------\------------------------|
     (last_ph)|                  \__                     |
              |_ _ _ _ _ _ _ _ _ _ _\ _ _ _ _ _ _ _ _ _ _|
seq[seqnum-3] |                      \______             |<--g_prev[0..T-1]
              |                             \___         |  
              |                                 \        |  
-------------------------------------------------\-------|  <forward trellis>
              ......                                ......  (2nd pass)

              |                                        \_|
===============================================================	      
*/

#ifdef USE_MBR
/**
 * <JA>
 * DPマッチングで使うノード
 * </JA>
 * <EN>
 * Nodes for DP matching
 * </EN>
 */
typedef struct {

  int d; // 最短距離
  int r; // 遷移元 1=Ins. 2=Del. 3=Sub. or Cor.
  int c; // r=3とした場合の遷移コスト 1=Sub. 0=Cor.
} DP;
#endif

#endif /* __J_SEARCH_H__ */
