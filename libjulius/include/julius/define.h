/**
 * @file   define.h
 * 
 * <JA>
 * @brief  内部処理選択のためのシンボル定義
 *
 * configure スクリプトは，システム/ユーザごとの設定を config.h に書き出し
 * ます. このファイルでは，その configure で設定された config.h 内の定義を
 * 元に，Julius/Julian のための内部シンボルの定義を行います. 
 * これらは実験用のコードの切り替えや古いオプションとの互換性のために
 * 定義されているものがほとんどです. 通常のユーザはこの定義を書き換える
 * 必要はありません. 
 * </JA>
 * 
 * <EN>
 * @brief  Internal symbol definitions
 *
 * The "configure" script will output the system- and user-dependent
 * configuration in "config.h".  This file defines some symboles
 * according to the generated config.h, to switch internal functions.
 * Most of the definitions here are for disabling experimental or debug
 * code for development, or to keep compatibility with old Julius.  These
 * definitions are highly internal, and normal users should not alter
 * these definitions without knowning what to do.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Mon Mar  7 15:17:26 2005
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

#ifndef __J_DEFINE_H__
#define __J_DEFINE_H__


/*****************************************************************************/
/** DO NOT MODIFY MANUALLY DEFINES BELOW UNLESS YOU KNOW WHAT YOU ARE DOING **/
/*****************************************************************************/

/* type of language model */
#define LM_UNDEF 0		///< not specified
#define LM_PROB 1		///< Statistical (N-gram - Julius)
#define LM_DFA 2		///< DFA (Julian)

/* LM variation specification */
#define LM_NGRAM 0		///< N-gram
#define LM_DFA_GRAMMAR 1	///< DFA grammar
#define LM_DFA_WORD 2		///< Isolated word recognition
#define LM_NGRAM_USER 3		///< User-defined statistical LM

/* recognition status */
#define J_RESULT_STATUS_REJECT_LONG -8 ///< Input rejected by long input
#define J_RESULT_STATUS_BUFFER_OVERFLOW -7 ///< Input buffer overflow
#define J_RESULT_STATUS_REJECT_POWER -6 ///< Input rejected by power
#define J_RESULT_STATUS_TERMINATE -5 ///< Input was terminated by app. request
#define J_RESULT_STATUS_ONLY_SILENCE -4 ///< Input contains only silence
#define J_RESULT_STATUS_REJECT_GMM -3 ///< Input rejected by GMM
#define J_RESULT_STATUS_REJECT_SHORT -2 ///< Input rejected by short input
#define J_RESULT_STATUS_FAIL -1	///< Recognition ended with no candidate
#define J_RESULT_STATUS_SUCCESS 0 ///< Recognition output some result

/* delete incoherent option */
/* CATEGORY_TREE: DFA=always on, NGRAM=always off */
/* switch with recog->category_tree */
/* UNIGRAM_FACTORING: DFA=always off, NGRAM=option */
/* enclose UNIGRAM_FACTORING section with "if (lmtype == LM_NGRAM)" */

/* abbreviations for verbose message output */
#define VERMES if (verbose_flag) jlog

/** 
 * define this to report memory usage on exit (Linux only)
 * 
 */
#undef REPORT_MEMORY_USAGE

/*** N-gram tree construction ***/
/* With 1-best approximation, Constructing a single tree from all words
   causes much error by factoring.  Listing each word flatly with no
   tree-organization will not cause this error, but the network becomes
   much larger and, especially, the inter-word LM handling becomes much more
   complex (O(n^2)).  The cost may be eased by LM caching, but it needs much
   memory. */
/* This is a trade-off of accuracy and cost */
#define SHORT_WORD_LEN 2
#ifdef LOWMEM
/* don't separate, construct a single tree from all words */
/* root nodes are about 50 in monophone, cache size will be 5MB on max */
#define NO_SEPARATE_SHORT_WORD
#else
#ifdef LOWMEM2
/* experimental: separate words frequently appears in corpus (1-gram) */
/* root nodes will be "-sepnum num" + 50, cache size will be 10MB or so */
#define NO_SEPARATE_SHORT_WORD
#define SEPARATE_BY_UNIGRAM
#else
/* separate all short words (<= 2 phonemes) */
/* root nodes are about 1100 in 20k (proportional to vocabulary),
   cache size will be about 100MB on max */
#endif /* LOWMEM2 */
#endif /* LOWMEM */

/*#define HASH_CACHE_IW*/
/* "./configure --enable-lowmem" defines NO_SEPARATE_SHORT_WORD instead */

/* default language model weight and insertion penalty for pass1 and pass2 */
/* these values come from the best parameters in IPA evaluation result */
#define DEFAULT_LM_WEIGHT_MONO_PASS1   5.0
#define DEFAULT_LM_PENALTY_MONO_PASS1 -1.0
#define DEFAULT_LM_WEIGHT_MONO_PASS2   6.0
#define DEFAULT_LM_PENALTY_MONO_PASS2  0.0
#ifdef PASS1_IWCD
#define DEFAULT_LM_WEIGHT_TRI_PASS1   8.0
#define DEFAULT_LM_PENALTY_TRI_PASS1 -2.0
#define DEFAULT_LM_WEIGHT_TRI_PASS2   8.0
#define DEFAULT_LM_PENALTY_TRI_PASS2 -2.0
#else
#define DEFAULT_LM_WEIGHT_TRI_PASS1   9.0
#define DEFAULT_LM_PENALTY_TRI_PASS1  8.0
#define DEFAULT_LM_WEIGHT_TRI_PASS2  11.0
#define DEFAULT_LM_PENALTY_TRI_PASS2 -2.0
#endif /* PASS1_IWCD */

/* Switch head/tail word insertion penalty to be inserted */
#undef FIX_PENALTY

/* some definitions for short-pause segmentation */
#undef SP_BREAK_EVAL		/* output messages for evaluation */
#undef SP_BREAK_DEBUG		/* output messages for debug */
#undef SP_BREAK_RESUME_WORD_BEGIN /* resume word = maxword at beginning of sp area */

#ifdef GMM_VAD
#define DEFAULT_GMM_MARGIN 20	/* backstep margin / determine buffer length */
#define GMM_VAD_AUTOSHRINK_LIMIT 500
#undef GMM_VAD_DEBUG		/* output debug message */
#endif

/* default values for spseg_naist */
#ifdef SPSEGMENT_NAIST
#define DEFAULT_SP_MARGIN 40
#define DEFAULT_SP_DELAY 4
#define SPSEGMENT_NAIST_AUTOSHRINK_LIMIT 500
#endif

/* '01/10/18 by ri: enable fix for trellis lookup order */
#define PREFER_CENTER_ON_TRELLIS_LOOKUP

/* '01/11/28 by ri: malloc step for startnode for multipath mode */
#define STARTNODE_STEP 300

/* default dict entry for IW-sp word that will be added to dict with -iwspword */
#define IWSPENTRY_DEFAULT "<UNK> [sp] sp sp"

/* confidence scoring method */
#ifdef CONFIDENCE_MEASURE
# ifndef CM_NBEST	/* use conventional N-best CM, will be defined if "--enable-cm-nbest" specified */
#  define CM_SEARCH	/* otherwise, use on-the-fly CM scoring */
# endif
#endif

/* dynamic word graph generation */
#undef GRAPHOUT_SEARCH_CONSIDER_RIGHT /* if defined, only hypothesis whose
					 left/right contexts is already
					 included in popped hypo will be merged.
					 EXPERIMENTAL, should not be defined.
				       */
#ifdef CM_SEARCH_LIMIT
#undef CM_SEARCH_LIMIT_AFTER	/* enable above only after 1 sentence found */
#undef CM_SEARCH_LIMIT_POP	/* terminate hypo of low CM on pop */
#endif

/* compute exact boundary instead of using 1st pass result */
/* also propagate exact time boundary to the right context after generation */
/* this may produce precise word boundary, but cause bigger word graph output */
#define GRAPHOUT_PRECISE_BOUNDARY

#undef GDEBUG			/* enable debug message in graphout.c */

/* some decoding fix candidates */
#undef FIX_35_PASS2_STRICT_SCORE /* fix hypothesis scores by enabling
				      bt_discount_pescore() in standard mode
				      with PASS2_STRICT_IWCD, 
				   */
#define FIX_35_INHIBIT_SAME_WORD_EXPANSION /* privent connecting the same trellis word in 2nd pass */


/* below are new since 3.5.2 */

/** 
 * Allow overwriting existing graph word if score is higher.
 * By default, while search, Julius merges the same graph words appeared at the
 * same location as previously stored word, and terminate search.  This
 * option make Julius to continue search in that case if fscore_head of
 * current hypo. is greater than the already existing one.  In that case,
 * the score of existing one will be overridden by the new higher one.
 * (from 3.5.2)
 * 
 */
#define GRAPHOUT_OVERWRITE

/* with GRAPHOUT_OVERWRITE, use gscore_head instead of fscore_head */
/**
 * (EXPERIMENTAL) With GRAPHOUT_OVERWRITE, use gscore_head for the
 * comparison instead of fscore_head.
 * 
 */
#undef GRAPHOUT_OVERWRITE_GSCORE

/**
 * At post-processing of graph words, this option limits the number of
 * "fit boundary" loop up to this value.  This option is made to avoid
 * long loop by the "boundary oscillation" of short words. (from 3.5.2)
 * 
 */
#define GRAPHOUT_LIMIT_BOUNDARY_LOOP

/**
 * This option enables "-graphsearchdelay" and "-nographsearchdelay" option.
 * When "-graphsearchdelay" option is set, Julius modifies its alogrithm of
 * graph generation on the 2nd pass not to apply search termination by graph
 * merging until the first sentence candidate is found.
 *
 * This option may result in slight improvement of graph accuracy only
 * when you are going to generate a huge word graph by setting broad search.
 * Namely, it may result in better graph accuracy when you set wide beams on
 * both 1st pass "-b" and 2nd pass "-b2", and large number for "-n".
 * 
 */
#define GRAPHOUT_SEARCH_DELAY_TERMINATION

/**
 * This option enables word graph cutting by word depth at post-processing.
 * This option will erase many short words to explode at a wide beam width.
 * 
 */
#define GRAPHOUT_DEPTHCUT

/**
 * Mimimal beam width that will be auto-determined for the 1st pass.
 * See set_beam_width() and default_width() for details.
 *
 */
#define MINIMAL_BEAM_WIDTH 200

/**
 * (DEBUG) Use old full lcdset instead of category-pair-aware lcdset
 * on Julian (-oldiwcd on 3.5.3 and previous)
 */
#undef USE_OLD_IWCD

/**
 * (EXPERIMENTAL) early word determination on isolated word recognition mode.
 * Results will be shown via CALLBACK_RESULT_PASS1_DETERMINED.
 * 
 */
#undef DETERMINE

#define FWD_NGRAM

#define MAX_SPEECH_ALLOC_STEP 320000


#define POWER_REJECT_DEFAULT_THRES 9.0

/**
 * A test to find optimal warping factor for VTLN (EXPERIMENTAL)
 * 
 */
#undef DEBUG_VTLN_ALPHA_TEST
#define VTLN_RANGE 0.2
#define VTLN_STEP  0.02

/**
 * Use fast successor composition at 1-gram factoring.
 * 
 */
#define FAST_FACTOR1_SUCCESSOR_LIST

/**
 * Enable score based pruning at the 1st pass.
 * 
 */
#define SCORE_PRUNING

#endif /* __J_DEFINE_H__ */

