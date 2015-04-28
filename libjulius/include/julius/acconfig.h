/**
 * @file   acconfig.h
 * 
 * <JA>
 * @brief  config.h.in を configure.in から生成するための autoconf 用ヘッダ
 *
 * このファイルはソースからインクルードされることはありません. 
 * 実際にはこの内容は config.h.in に埋め込まれており，
 * configure によって config.h.in から生成された config.h が
 * プログラムによって使用されます. 
 *
 * @sa config.h, config.h.in, configure, configure.in
 * </JA>
 * 
 * <EN>
 * @brief  Autoconf header to generate config.h.in from configure.in
 *
 * This file is not included by any source file.  The contents of this file
 * is already included in config.h.in, and the configuration script "configure"
 * will generate config.h from config.h.in.  It sets definitions according to
 * the running environment and user-specified setting.  The final config.h
 * will be included by the sources.
 * 
 * @sa config.h, config.h.in, configure, configure.in
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Sat Feb 19 12:53:54 2005
 *
 * $Revision: 1.2 $
 * 
 */

/// Name of the Product.
#undef PRODUCTNAME

/// Version string
#undef VERSION

/// Engine setting (value of "--enable-setup=...").
#undef SETUP

/// Compilation host information 
#undef HOSTINFO

@TOP@

/// For Julius, defined if using 1-gram factoringon the 1st pass instead of 2-gram factoring.
#undef UNIGRAM_FACTORING

/**
 * For Julius, defined if dictionary forms a single tree lexicon, sharing
 * only a single root node.  This saves memory of inter-word LM cache.
 * 
 */
#undef LOWMEM

/**
 * For Julius, defined if frequent words should be separated from the lexicon
 * tree.  This will improve accuracy on small beam, and default of "fast"
 * setting.  If none of LOWMEM and LOWMEM2 is defined, separation of short
 * words from lexicon tree will be performed to get the better accuracy, at
 * a cost of LM cache area on word head.
 * 
 */
#undef LOWMEM2

/**
 * If defined, use word-pair approximation on the 1st pass instead of
 * 1-best approximation.
 * 
 */
#undef WPAIR

/**
 * When WPAIR is defined, only up to N tokens will be kept for each node
 * instead of keeping tokens depending on the previous word.  This may
 * improve memory efficiency when word-pair approximation is used.
 * 
 */
#undef WPAIR_KEEP_NLIMIT

/**
 * If defined, generate a simple word graph instead of word trellis on
 * the 1st pass.  This limits word expansion on the 2nd pass
 * to only the words on the word graph, and the final recognition accuracy
 * can be decreased.  You should enable this with WPAIR to get reasonable
 * output.  Please note that this is different
 * from Word Graph Output of the 2nd pass which can be enabled by GRAPHOUT.
 * 
 */
#undef WORD_GRAPH

/** 
 * If defined, use monophone tree lexicon on the 1st pass for speed up
 * the search.  This is EXPERIMENTAL, and should not be used.
 * 
 */
#undef MONOTREE

/** 
 * Handle inter-word triphone on the 1st pass.  This should be defined
 * if using context-dependent acoustic model.  If not defined, the context
 * will not be considered any more.  This is defined by default.
 * 
 */
#undef PASS1_IWCD

/**
 * On word expansion of the 2nd pass, Julius and Julian by default does
 * not handle inter-word context dependency of the newly expanded words
 * on the expansion time, and they will be computed when the hypothesis
 * is popped from the stack at the later processing.  If PASS2_STRICT_IWCD
 * is defined, a strict inter-word triphone will be computed just on the
 * word expansion time, by re-computing word edge phones on the connection
 * point for all the word candidates.
 *
 * This option will results in a better
 * recognition accuracy.  However, the 2nd pass will become slower by the
 * increasing acoustic matching cost.
 * 
 */
#undef PASS2_STRICT_IWCD

/**
 * Enable score envelope beaming on the hypothesis scoring in the 2nd pass.
 * This will be defined by default.
 * 
 */
#undef SCAN_BEAM

/// Set the default method of Gaussian pruning for tied-mixture model to safe algorithm
#undef GPRUNE_DEFAULT_SAFE

/// Set the default method of Gaussian pruning for tied-mixture model to heuristic algorithm
#undef GPRUNE_DEFAULT_HEURISTIC

/// Set the default method of Gaussian pruning for tied-mixture model to beam algorithm
#undef GPRUNE_DEFAULT_BEAM

/**
 * Enables confidence scoring for the output words.  This will be defined
 * by default.
 * 
 */
#undef CONFIDENCE_MEASURE

/* use N-best confidence measure instead of search-time computation */
/**
 * By default, Julius/Julian uses search-time heuristic scores to get the
 * posterior probability based word confidence measures on the search time.
 * This default algorithm can output word confidence scores with a little
 * additional computation without searching for much sentences.
 *
 * If you still use a trivial method of computing the word confidence scores
 * from the N-best sentence list, you can define this.
 * 
 */
#undef CM_NBEST

/**
 * If defined, compute confidence scores for multiple alpha values.
 * 
 */
#undef CM_MULTIPLE_ALPHA

/**
 * Enable search space visualization feature.  You need X11 and GTK to
 * use this.
 * 
 */
#undef VISUALIZE

/** 
 * When VISUALIZE is defind, this defines a command to play the recorded
 * sound on the visualization window.
 */
#undef PLAYCOMMAND

/**
 * On Julius, if defined, fix some language model scoring bug on the 2nd pass.
 * 
 */
#undef LM_FIX_DOUBLE_SCORING

/**
 * Use dynamic word graph generation on the 2nd pass.
 * The word candidates are fixed as soon as the word boundary is fixed
 * in search, and as soon as same word appears in the same position,
 * they will be merged.  It results in much more words to be
 * remained in the graph.
 * 
 */
#undef GRAPHOUT_DYNAMIC

/**
 * If defined with GRAPHOUT_DYNAMIC, use modified stack
 * decoding algorithm for efficient word graph generation.
 *
 */
#undef GRAPHOUT_SEARCH

/**
 * If defined, avoid expansion of low CM word on search.  This may
 * speed up 
 * 
 */
#undef CM_SEARCH_LIMIT

/**
 * If defined, enable decoder-oriented VAD using short-pause segmentation
 * scheme developed by NAIST team
 * 
 */

#undef SPSEGMENT_NAIST

/**
 * If defined, enable a simple GMM-based VAD.  Both frontend VAD and
 * postprocessing rejection will be performed using the same GMM.
 * 
 */

#undef GMM_VAD

/**
 * This will be defined internally when in-decoder type VAD is enabled.
 * 
 */

#undef BACKEND_VAD

/**
 * If enabled, do post-rejection by power
 * 
 */
#undef POWER_REJECT
