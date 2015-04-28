/**
 * @file   version.c
 * 
 * <JA>
 * @brief  バージョンおよびコンパイル時設定の出力
 * 
 * </JA>
 * 
 * <EN>
 * @brief  Output version and compilation-time configuration.
 *
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Mon Sep 12 01:34:15 2005
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

/* Generated automatically from version.c.in by configure. */

#include <julius/julius.h>

#define CC "MSVC" ///< Used compiler
#define CFLAGS "" ///< Used flags for compilation

/** 
 * <JA>
 * ヘッダを出力する
 * 
 * @param strm [in] 出力ストリーム
 * </JA>
 * <EN>
 * Output application header.
 * 
 * @param strm [in] output stream
 * </EN>
 */
void
j_put_header(FILE *strm){
  if (strm == NULL) return;
  fprintf(strm,"%s rev.%s (%s)\n\n", JULIUS_PRODUCTNAME, JULIUS_VERSION, JULIUS_SETUP);
}

/** 
 * <JA>
 * バージョン情報を出力する
 * 
 * @param strm [in] 出力ストリーム
 * </JA>
 * <EN>
 * Output version information.
 * 
 * @param strm [in] output stream
 * </EN>
 */
void
j_put_version(FILE *strm){
  if (strm == NULL) return;
  fprintf(strm,"\n%s rev.%s (%s)  built for %s\n\n",
	  JULIUS_PRODUCTNAME, JULIUS_VERSION, JULIUS_SETUP, JULIUS_HOSTINFO);
  fprintf(strm,"Copyright (c) 1991-2013 Kawahara Lab., Kyoto University\n");
  fprintf(strm,"Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan\n");
  fprintf(strm,"Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology\n");
  fprintf(strm,"Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology\n\n");
}

/** 
 * <JA>
 * コンパイル時の設定を出力する．
 * 
 * @param strm [in] 入力ストリーム
 * </JA>
 * <EN>
 * Output compile-time settings.
 * 
 * @param strm [in] input stream
 * </EN>
 */
void
j_put_compile_defs(FILE *strm){
  if (strm == NULL) return;
  fprintf(strm,"Engine specification:\n");
  fprintf(strm," -  Base setup   : %s\n", JULIUS_SETUP);
  fprintf(strm," -  Supported LM : DFA, N-gram, Word\n");
  fprintf(strm," -  Extension    :");
#ifndef UNIGRAM_FACTORING
  fprintf(strm, ", 2gramFactoring");
#endif

# ifdef GRAPHOUT_DYNAMIC
#  ifdef GRAPHOUT_SEARCH
  /* this is default */
  //fprintf(strm, " GraphOutSearch");
#  else
  fprintf(strm, " GraphOutNonSearchTermination");
#  endif
# else 
  fprintf(strm, " GraphOutFromNBest");
# endif
# ifndef GRAPHOUT_PRECISE_BOUNDARY
  fprintf(strm, " DisableGraphOutPostFitting");
# endif

#ifdef CM_SEARCH_LIMIT
# ifdef CM_SEARCH_LIMIT_AFTER
  fprintf(strm, " CMPruning_OnlyAfterReached");
# else
  fprintf(strm, " CMPruning");
# endif
# ifdef CM_SEARCH_LIMIT_POP
  fprintf(strm, " CMPruningOnPOP");
# endif
#endif  

# ifndef LM_FIX_DOUBLE_SCORING
  fprintf(strm, " NoLMFix");
# endif

# ifndef CLASS_NGRAM
  fprintf(strm, " NoClassNGram");
# endif

#ifdef WORDS_INT
  fprintf(strm, " WordsInt");
#endif

# ifdef LOWMEM
  fprintf(strm, " SingleTree");
# else
#  ifdef LOWMEM2
  /* fprintf(strm, " HiFreqLinearTree");*/
#  else
  fprintf(strm, " ShortWordTree");
#  endif
# endif

# ifndef CATEGORY_TREE
  //fprintf(strm, " NoCategoryTree");
# endif

#ifdef MONOTREE
  fprintf(strm, " MonoTree1");
#endif
#ifndef SCAN_BEAM
  fprintf(strm, " NoScoreEnvelope");
#endif
#ifndef PASS1_IWCD
  fprintf(strm, " NoIWCD1");
#endif
#ifdef PASS2_STRICT_IWCD
  fprintf(strm, " StrictIWCD2");
#endif

#ifdef WPAIR
# ifdef WPAIR_KEEP_NLIMIT
  fprintf(strm, " WordPairNApprox");
# else
  fprintf(strm, " WordPairApprox");
# endif
#endif

#ifdef WORD_GRAPH
  fprintf(strm, " 1stPassWordGraph");
#endif

#ifndef CONFIDENCE_MEASURE
  fprintf(strm, " NoCM");
#else
# ifdef CM_NBEST
  fprintf(strm, " N-bestCM");
# endif
# ifdef CM_MULTIPLE_ALPHA
  fprintf(strm, " MultiCMOutput");
# endif
#endif /* CONFIDENCE_MEASURE */

#ifndef USE_MIC
  fprintf(strm, " NoMic");
#endif
#ifdef USE_NETAUDIO
  fprintf(strm, " NetAudio");
#endif
#ifndef HAVE_PTHREAD
  fprintf(strm, " NoPThread");
#endif
#ifdef HAVE_LIBSNDFILE
  fprintf(strm, " LibSndFile");
#endif

#ifdef VISUALIZE
  fprintf(strm, " Visualize");
#endif

#ifdef FORK_ADINNET
  fprintf(strm, " ForkOnAdinnet");
#endif

#ifndef MFCC_SINCOS_TABLE
  fprintf(strm, " DisableMFCCTable");
#endif

#ifndef LM_FIX_DOUBLE_SCORING
  fprintf(strm, " DisableLMFix3.4");
#endif

#ifdef USE_LIBJCODE
  fprintf(strm, " Libjcode");
#endif
  
#ifdef HAVE_ICONV
  fprintf(strm, " IconvOutput");
#endif

#ifdef GMM_VAD
  fprintf(strm, " GMMVAD");
#endif

#ifdef SPSEGMENT_NAIST
  fprintf(strm, " DecoderVAD");
#endif

#ifdef POWER_REJECT
  fprintf(strm, " PowerReject");
#endif

  fprintf(strm, "\n");
  fprintf(strm," -  Compiled by  : %s %s\n", CC, CFLAGS);
}

/** 
 * <JA>
 * ライブラリの設定を出力する
 * 
 * @param strm [in] 出力ストリーム
 * </JA>
 * <EN>
 * Output library configuration.
 * 
 * @param strm [in] output stream
 * </EN>
 */
void
j_put_library_defs(FILE *strm) {
  if (strm == NULL) return;
  fprintf(strm, "Library configuration: ");
  confout(strm);
  fprintf(strm, "\n");
}

/* end of file */
