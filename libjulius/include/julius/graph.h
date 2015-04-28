/**
 * @file   graph.h
 * 
 * <JA>
 * @brief  単語グラフの構造体定義
 *
 * 単語グラフ中の単語を表す構造体，および confusion network 中の
 * 単語を表す構造体が定義されています. 
 * </JA>
 * 
 * <EN>
 * @brief  Structure definition for word graph.
 *
 * This file defines instances for word graph and confusion network.
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Thu Aug 16 00:30:54 2007
 *
 * $Revision: 1.5 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __J_GRAPH_H__
#define __J_GRAPH_H__

#define FANOUTSTEP 7		///< Memory allocation step for connection words in WordGraph

/**
 * <JA>
 * 単語グラフ上の単語候補. 
 * </JA>
 * <EN>
 * Word arc on the word graph.
 * </EN>
 */
typedef struct __word_graph__ {
  WORD_ID wid;			///< Word ID
  int lefttime;			///< Head frame where this word begins
  int righttime;		///< Tail frame where this word ends
  LOGPROB fscore_head;		///< Partial sentence score 'f' when the next (left) word of this word was expanded at 2nd pass.  f = g(thisword) + h(nextword)
  LOGPROB fscore_tail;		///< Partial sentence score when this word was expanded in 2nd pass.  f = g(rightword) + h(thisword)
  LOGPROB gscore_head;		///< Accumulated viterbi score at the head state of this word on lefttime.  This value includes both accumulated AM score and LM score of this word.
  LOGPROB gscore_tail;		///< Accumultaed viterbi score at the head state of previous (right) word.
  LOGPROB lscore_tmp;		///< Temporally holds LM score
  LOGPROB forward_score;	///< Forward score at right edge, incl. LM, obtained by forward-backward process
  LOGPROB backward_score;	///< Backward score at left edge, incl. LM, obtained by forward-backward process
#ifdef CM_SEARCH
  LOGPROB cmscore;		///< Confidence score obtained while search
#endif
  LOGPROB amavg;		///< average acoustic score of matched frame
  HMM_Logical *headphone;	///< Applied phone HMM at the head of the word 
  HMM_Logical *tailphone;	///< Applied phone HMM at the end of the word 
  struct __word_graph__ **leftword; ///< List of left context
  LOGPROB *left_lscore;		///< List of LM score for left contexts
  int leftwordnum;		///< Stored num of @a leftword
  int leftwordmaxnum;		///< Allocated size of @a leftword
  struct __word_graph__ **rightword; ///< List of right context
  LOGPROB *right_lscore;        ///< List of LM score for right contexts
  int rightwordnum;		///< Stored num of @a leftword
  int rightwordmaxnum;		///< Allocated size of @a letfword
  struct __word_graph__ *next;	///< Pointer to next wordgraph for throughout access
  boolean mark;			///< Delete mark for compaction operation
  int id;			///< Unique ID within the graph
  boolean saved;		///< Save mark for graph generation
#ifdef GRAPHOUT_DYNAMIC
  boolean purged;		///< Purged mark for graph generation
#endif
  LOGPROB graph_cm;		///< Confidense score computed from the graph
} WordGraph;

/**
 * Word Cluster for confusion network generation
 * 
 */
typedef struct __confnet_cluster__ {
  WordGraph **wg;		///< List of graph words in this cluster
  int wgnum;			///< Number of @a wg;
  int wgnum_alloc;		///< Allocated size of @a wg;
  WORD_ID *words;		///< List of words in this cluster (WORD_INVALID) means skip ("-")
  LOGPROB *pp;			///< Posterior probability of each word
  int wordsnum;			///< Number of @a words
  struct __confnet_cluster__ *next; ///< Pointer to next structure
} CN_CLUSTER;

/**
 * Number of allocation step for CN_CLUSTER
 * 
 */
#define CN_CLUSTER_WG_STEP 10

#endif /* __J_GRAPH_H__ */
