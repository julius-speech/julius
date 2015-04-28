/**
 * @file   beam.h
 * 
 * <JA>
 * @brief  第１パスのフレーム同期ビーム探索用定義
 * </JA>
 * 
 * <EN>
 * @brief  Definitions for frame-synchronous beam search on 1st pass.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Mon Mar  7 15:12:29 2005
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

#ifndef __J_BEAM_H__
#define __J_BEAM_H__

/// token id for the 1st pass
typedef int TOKENID;

/// id for undefined token
#define TOKENID_UNDEFINED -1

/// Token to hold viterbi pass history
typedef struct {
  TRELLIS_ATOM *last_tre;	///< Previous word candidate in word trellis
  WORD_ID last_cword;		///< Previous context-aware (not transparent) word for N-gram
  LOGPROB last_lscore;		///< Currently assigned word-internal LM score for factoring for N-gram
  LOGPROB score;		///< Current accumulated score (AM+LM)
  int node;			///< Lexicon node ID to which this token is assigned
#ifdef WPAIR
  TOKENID next;			///< ID pointer to next token at same node, for word-pair approx.
#endif
} TOKEN2;

#define FILLWIDTH 70		///< Word-wrap character length for progressive output

#endif /* __J_BEAM_H__ */
