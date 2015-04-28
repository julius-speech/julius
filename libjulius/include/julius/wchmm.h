/**
 * @file   wchmm.h
 * 
 * <JA>
 * @brief  木構造化辞書の構造体定義. 
 *
 * このファイルでは，第1パスで用いられる木構造化辞書（あるいは単語連結
 * HMM (wchmm) とも呼ばれる）の構造体を定義しています. 起動時に，単語辞書の
 * 前単語が並列に並べられ，ツリー上に結合されて木構造化辞書が構築されます. 
 * HMMの状態単位で構築され，各状態は，対応するHMM出力確率，ツリー内での遷移先
 * のリスト，および探索のための様々な情報（言語スコアファクタリングのための
 * successor word list や uni-gram 最大値，単語始終端マーカー，音素開始
 * マーカーなど）を含みます. 
 * </JA>
 * 
 * <EN>
 * @brief  Structure Definition of tree lexicon
 *
 * This file defines structure for word-conjunction HMM, aka tree lexicon
 * for recognition of 1st pass.  Words in the dictionary are gathered to
 * build a tree lexicon.  The lexicon is built per HMM state basis,
 * with their HMM output probabilities, transition arcs, and other
 * informations for search such as successor word lists and maximum
 * uni-gram scores for LM factoring, word head/tail marker, phoneme
 * start marker, and so on.
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Sun Sep 18 21:31:32 2005
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

#ifndef __J_WORD_CONJ_HMM_H__
#define __J_WORD_CONJ_HMM_H__

//#define		MAXWCNSTEP  40000 ///< Number of states to be allocated at once

#ifdef PASS1_IWCD

/* Cross-word triphone handling */

/**
 * State output probability data for head phone of a word.  The phoneme HMM
 * should change while search according to the last context word.
 * 
 */
typedef struct {
  HMM_Logical  *hmm;		///< Original HMM state on the dictionary
  short		state_loc;	///< State location within the phoneme (1-)
  /* Context cache */
  boolean	last_is_lset;	///< TRUE if last assigned model was context-dependent state set
  union {
    HTK_HMM_State *state;	///< Last assigned state (last_is_lset = FALSE)
    CD_State_Set  *lset;	///< Last assigned lset (last_is_lset = TRUE)
  } cache;
  WORD_ID	lastwid_cache;	///< Last context word ID
} RC_INFO;

/**
 * State output probability data for 1-phone word.  The phoneme HMM should
 * change according to the last context word.
 * 
 */
typedef struct {
  HMM_Logical  *hmm;		///< Original HMM state on the dictionary
  short		state_loc;	///< State location within the phoneme (1-)
  /* Context cache */
  boolean	last_is_lset;	///< TRUE if last assigned model was context-dependent state set
  WORD_ID	category;	///< Last context word's category ID
  union {
    HTK_HMM_State *state;	///< Last assigned state
    CD_State_Set  *lset;	///< Last assigned lset
  } cache;
  WORD_ID	lastwid_cache;	///< Last context word ID
} LRC_INFO;

/* For word tail phoneme, pseudo phone on the dictionary will be directly
   used as context-dependent state set */

/**
 * State output probability container on lexicon tree.  Each state
 * should have one of them.
 * 
 */
typedef union {
  HTK_HMM_State *state;		///< For AS_STATE (word-internal phone)
  CD_State_Set  *lset;		///< For AS_LSET (word tail phone)
  RC_INFO	*rset;		///< For AS_RSET (word head phone)
  LRC_INFO	*lrset;		///< For AS_LRSET (phone in 1-phoneme word)
} ACOUSTIC_SPEC;

/**
 * ID to indicate which data is in the ACOUSTIC_SPEC container.
 * 
 */
typedef enum {
  AS_STATE,			///< This state is in word-internal phone
  AS_LSET,			///< This state is in word tail phone
  AS_RSET,			///< This state is in word head phone
  AS_LRSET			///< This state is in 1-phone word
} AS_Style;
#endif
  
/*************************************************************************/
/**
 * LM cache for the 1st pass
 * 
 */
typedef struct {
  /// Word-internal factoring cache indexed by scid, holding last score
  LOGPROB *probcache;
  /// Word-internal factoring cache indexed by scid, holding last N-gram entry ID
  WORD_ID *lastwcache;
/**
 * @brief  Cross-word factoring cache to hold last-word-dependent factoring
 * score toward word head nodes.
 *
 * Cached values will be stored as [last_nword][n], where n is the number of
 * word-head node on which the last_nword-dependent N-gram factoring value
 * should be computed on cross-word transition.  In 1-gram factoring,
 * n equals to wchmm->isolatenum, the number of isolated (not shared)
 * word-head nodes.
 * In 2-gram factoring, n simply equals to wchmm->startnum, the number of
 * all word-head nodes.
 *
 * The cache area will be allocated per the previous word when they appeared
 * while search.
 * It will retain across the speech stream, so the cache area will grow
 * to an extent as recognition was done for many files.
 */
  LOGPROB **iw_sc_cache;
  /**
   * Maximum size of cross-word factoring cache @a iw_sc_cache per last word.
   * The value is set in max_successor_cache_init().
   */
  int iw_cache_num;
#ifdef HASH_CACHE_IW
  WORD_ID *iw_lw_cache; ///< Maps hash cache id [x] to corresponding last word
#endif
  
} LM_PROB_CACHE;

/*************************************************************************/
/**
 * Number of arcs in an arc cell.
 * 
 */
#define A_CELL2_ALLOC_STEP 4

/**
 * Transition arc holding cell
 * 
 */
typedef struct __A_CELL2__ {
  /**
   * Number of arcs currently stored in this cell.
   * If this reaches A_CELL2_ALLOC_STEP, next cell will be allocated.
   * 
   */
  unsigned short n;
  int arc[A_CELL2_ALLOC_STEP];	///< Transition destination node numbers
  LOGPROB a[A_CELL2_ALLOC_STEP]; ///< Transitino probabilities
  struct __A_CELL2__ *next;	///< Pointer to next cell
} A_CELL2;

/**
 * HMM state on tree lexicon.
 * 
 */
typedef struct wchmm_state {
#ifdef PASS1_IWCD
  ACOUSTIC_SPEC out;		///< State output probability container
  /* below has been moved to WCHMM (04/06/22 by ri) */
  /*unsigned char	outstyle;	output type (one of AS_Style) */
#else  /* ~PASS1_IWCD */
  HTK_HMM_State *out;		///< HMM State
#endif /* ~PASS1_IWCD */
  /**
   * LM factoring parameter:
   * If scid > 0, it will points to the successor list index.
   * If scid = 0, the node is not on branch.
   * If scid < 0, it will points to the unigram factoring value index.
   */
  int scid;
} WCHMM_STATE;

/**
 * wchmm-specific work area
 * 
 */
typedef struct {
  int *out_from;
  int *out_from_next;
  LOGPROB *out_a;
  LOGPROB *out_a_next;
  int out_from_len;
} WCHMM_WORK;

/**
 * Whole lexicon tree structure holding all information.
 * 
 */
typedef struct wchmm_info {
  int lmtype;			///< LM type
  int lmvar;			///< LM variant
  boolean category_tree;	///< TRUE if category_tree is used
  HTK_HMM_INFO *hmminfo;	///< HMM definitions used to construct this lexicon
  NGRAM_INFO *ngram;		///< N-gram used to construct this lexicon
  DFA_INFO *dfa;		///< Grammar used to construct this lexicon
  WORD_INFO *winfo;		///< Word dictionary used to construct this lexicon
  boolean ccd_flag;		///< TRUE if handling context dependency
  int	maxwcn;			///< Memory assigned maximum number of nodes
  int	n;			///< Num of nodes in this lexicon
  WCHMM_STATE	*state;		///< HMM state on tree lexicon [nodeID]
  LOGPROB *self_a;		///< Transition probability to self node
  LOGPROB *next_a;		///< Transition probabiltiy to next (now+1) node
  A_CELL2 **ac;			///< Transition arc information other than self and next.
  WORD_ID	*stend;		///< Word ID that ends at the state [nodeID]
  int	**offset;		///< Node ID of a phone [wordID][0..phonelen-1]
  int	*wordend;		///< Node ID of word-end state [wordID]
  int	startnum;		///< Number of root nodes
  int	*startnode;		///< Root node index [0..startnum-1] -> node ID
  int	*wordbegin;		///< Node ID of word-beginning state [wordID] for multipath mode
  int	maxstartnum;		///< Allocated number of startnodes for multipath mode
  WORD_ID *start2wid;		///< Root node index [0..startnum-1] -> word ID for multipath mode
#ifdef UNIGRAM_FACTORING
  int	*start2isolate;		///< Root node index -> isolated root node ID
  int	isolatenum;		///< Number of isolated root nodes
#endif
  LOGPROB	*wordend_a;	///< Transition prob. outside word [wordID] for non-multipath mode
#ifdef PASS1_IWCD
  unsigned char *outstyle;	///< ID to indicate type of output probability container (one of AS_Style)
#endif
  /* Successor lists on the tree are stored on sequencial list at @a sclist,
     and each node has index to the list */
  /* sclist and sclen are used at 2-gram factoring only */
  /* scword is used at 1-gram factoring only */
#ifdef UNIGRAM_FACTORING
  WORD_ID *scword;		///< successor word[scid]
  LOGPROB *fscore;		///< List of 1-gram factoring score [-scid]
  int fsnum;			///< Number of @a fscore
#endif
  WORD_ID **sclist;		///< List of successor list [scid]
  WORD_ID *sclen;		///< Length of each succcessor list [scid]
  int   scnum;			///< Total number of factoring nodes that has successor list
  BMALLOC_BASE *malloc_root;	///< Pointer for block memory allocation
#ifdef PASS1_IWCD
  APATNODE *lcdset_category_root; ///< Index of lexicon-dependent category-aware pseudo phone set when used on Julian
  BMALLOC_BASE *lcdset_mroot;
#endif /* PASS1_IWCD */

  HMMWork *hmmwrk;		///< Work area for HMM computation in wchmm

  LM_PROB_CACHE lmcache;	///< LM score cache for 1st pass

  WCHMM_WORK wrk;		///< Other work area for 1st pass transition computation

  int separated_word_count; ///< Number of words actually separated (linearlized) from the tree

  char lccbuf[MAX_HMMNAME_LEN+7]; ///< Work area for HMM name conversion
  char lccbuf2[MAX_HMMNAME_LEN+7]; ///< Work area for HMM name conversion

  /* user-defined functions, used when this->lmvar == LM_NGRAM_USER */
  /* they are local copy from parent Recog instance */
  LOGPROB (*uni_prob_user)(WORD_INFO *, WORD_ID, LOGPROB); ///< Pointer to function returning word occurence probability

  LOGPROB (*bi_prob_user)(WORD_INFO *, WORD_ID, WORD_ID, LOGPROB); ///< Pointer to function returning a word probability given a word context (corresponds to bi-gram)

} WCHMM_INFO;

#endif /* __J_WORD_CONJ_HMM_H__ */
