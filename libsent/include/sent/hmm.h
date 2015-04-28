/**
 * @file   hmm.h
 *
 * <EN>
 * @brief  Hidden Marcov Model for recognition.
 *
 * This file defines %HMM instance structure for recognition.
 * When recognition, the required word %HMM or tree lexicon will be built
 * using these structures referencing word dictionary and HTK %HMM Acoustic
 * Model (defined in htk_hmm.h), and actual likelihood computation.
 * </EN>
 * <JA>
 * @brief  認識計算用の Hidden Marcov Model
 *
 * このファイルでは, 認識における%HMMの構造体を定義しています．
 * 認識時には, 辞書および音響モデル構造体 (htk_hmm.h) を元に
 * 必要な単語%HMMや木構造化辞書がこの構造体を用いて構築され,
 * 音響尤度計算はこの上で行われます．
 * </JA>
 *
 * @author Akinobu LEE
 * @date   Thu Feb 10 14:54:06 2005
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

#ifndef __SENT_HMM_NEW2_H__
#define __SENT_HMM_NEW2_H__

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>
#include <sent/hmm_calc.h>

/// Transition arc of %HMM state
typedef struct _a_cell {
  LOGPROB		a;	///< Transition probability in log10
  int			arc;	///< Transition destination in state ID
  struct _a_cell	*next;  ///< Pointer to next transition on the same state
} A_CELL;

/// %HMM State
typedef struct {
  A_CELL		*ac;	///< List of transition arcs from this state
  /**
   * @brief Pointer to corresponding output state definition
   *
   * When a triphone model is used, if this state is located as a
   * part of phoneme %HMM on @em word-edge, the corresponding
   * @em pseudo-triphone, i.e. "*-e+g", "k-a+*", should be assigned.
   * Otherwise, i.e. if this state is located at @em internal position
   * of a word, pointer to the corresponding @em physical
   * (actually defined) %HMM will be assigned.
   * When a triphone model is used, phoneme %HMMs on @em word-edge should
   * be @em pseudo-triphone, i.e. "*-e+g", "k-a+*".  In this case,  the
   * belonging state should hold the output probability function
   * in CD_State_Set.
   * Otherwise, i.e. if this state is located at @em internal position
   * of a word, pointer to the corresponding @em physical
   * (actually defined) %HMM will be assigned.
   */
  union {
    HTK_HMM_State *state;	///< Pointer to the mapped physical %HMM
    CD_State_Set  *cdset;	///< Pointer to the pseudo %HMM
  } out;
  boolean is_pseudo_state;	///< TRUE if pseudo %HMM is assigned, FALSE if physical %HMM is assigned
} HMM_STATE;

/**
 * @brief %HMM state sequence
 *
 * @note This assumes that there is only one transition that goes outside
 * this %HMM state sequence.
 */
typedef struct {
  int			len;	///< Length of state
  HMM_STATE		*state;	///< Array of state
  LOGPROB		accept_ac_a; ///< Transition probability outside this sequence (fixed to one) for normal mode
} HMM;


/**
 * Token definition for viterbi segmentation.
 * 
 */
typedef struct _seg_token {
  int last_id;			///< ID of last unit
  int last_end_frame;		///< Frame at which the last unit ends
  LOGPROB last_end_score;	///< Score at which the last unit ends
  struct _seg_token *next;	///< Pointer to previous token context, NULL if no context
  struct _seg_token *list;	///< Link to next token, NULL if last
} SEGTOKEN;

#ifdef __cplusplus
extern "C" {
#endif

/* mkwhmm.c */
HMM *new_make_word_hmm(HTK_HMM_INFO *, HMM_Logical  **, int, boolean *);
HMM *new_make_word_hmm_with_lm(HTK_HMM_INFO *, HMM_Logical  **, int, boolean *, LOGPROB *);
void free_hmm(HMM *);
/* vsegment.c */
LOGPROB viterbi_segment(HMM *hmm, HTK_Param *param, HMMWork *wrk, boolean multipath, int *endstates, int ulen, int **id_ret, int **seg_ret, LOGPROB **uscore_ret, int *retlen);

/* hmminfo/outprob.c */
LOGPROB outprob(HMMWork *wrk, int t, HMM_STATE *hmmstate, HTK_Param *param);

/* hmminfo/put_htkdata_info */
void put_hmm_arc(FILE *fp, HMM *d);
void put_hmm_outprob(FILE *fp, HMM *d);
void put_hmm(FILE *fp, HMM *d);

#ifdef __cplusplus
}
#endif

#endif /* __SENT_HMM_NEW2_H__ */
