/**
 * @file   default.c
 * 
 * <JA>
 * @brief  設定のデフォルト値のセット
 *
 * 設定可能なパラメータの初期値をセットします. 
 * </JA>
 * 
 * <EN>
 * @brief  Set system default values for configuration parameters
 *
 * This file contains a function to set system default values for all the
 * configuration parameters.  This will be called at initialization phase.
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Fri Feb 16 15:05:43 2007
 *
 * $Revision: 1.22 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius/julius.h>

/** 
 * <JA>
 * @brief  パラメータ構造体 Jconf に初期値を代入する. 
 *
 * ここで値が初期化されるのは，Jconf 自身に格納される値のみである. 
 * 下位の構造（AM, LM, SEARCH）のパラメータはセットしないので，
 * それぞれ別の関数で初期化する必要が有る. 
 * 
 * @param j [in] パラメータ構造体
 * </JA>
 * <EN>
 * @brief   Fill in the system default values to a parameter structure Jconf.
 *
 * Only values of the Jconf will be set.  The parameters in sub-structures
 * (AM, LM, SEARCH) will not be set in this function: they should be
 * initialized separatedly at each corresponding functions.
 * 
 * @param j [in] parameter structure
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
void
jconf_set_default_values(Jconf *j)
{
  j->input.type				= INPUT_VECTOR;
  j->input.speech_input			= SP_MFCFILE;
  j->input.device			= SP_INPUT_DEFAULT;
  j->input.plugin_source		= -1;
  j->input.sfreq			= 16000;
  j->input.period			= 625;
  j->input.framesize			= DEF_FRAMESIZE;
  j->input.frameshift			= DEF_FRAMESHIFT;
  j->input.use_ds48to16			= FALSE;
  j->input.inputlist_filename		= NULL;
  j->input.adinnet_port			= ADINNET_PORT;
#ifdef USE_NETAUDIO
  j->input.netaudio_devname		= NULL;
#endif
  j->input.paramtype_check_flag		= TRUE;

  j->detect.level_thres			= 2000;
  j->detect.head_margin_msec		= 300;
  j->detect.tail_margin_msec		= 400;
  j->detect.zero_cross_num		= 60;
  j->detect.silence_cut			= 2; /* accept device default */
  j->detect.chunk_size			= 1000;
#ifdef GMM_VAD
  j->detect.gmm_margin			= DEFAULT_GMM_MARGIN;
  j->detect.gmm_uptrigger_thres		= 0.7;
  j->detect.gmm_downtrigger_thres	= -0.2;
#endif

  j->preprocess.strip_zero_sample	= TRUE;
  j->preprocess.use_zmean		= FALSE;
  j->preprocess.level_coef		= 1.0;

  j->reject.gmm_filename		= NULL;
  j->reject.gmm_gprune_num		= 10;
  j->reject.gmm_reject_cmn_string	= NULL;
  j->reject.rejectshortlen		= 0;
  j->reject.rejectlonglen		= -1;
#ifdef POWER_REJECT
  j->reject.powerthres			= POWER_REJECT_DEFAULT_THRES;
#endif

  j->decodeopt.forced_realtime		= FALSE;
  j->decodeopt.force_realtime_flag	= FALSE;
  j->decodeopt.segment			= FALSE;

  j->optsection				= JCONF_OPT_DEFAULT;
  j->optsectioning			= TRUE;
  j->outprob_outfile			= NULL;
}

/** 
 * <EN>
 * Fill in system default values to an AM parameter structure.
 * @param j [in] AM configuration parameter structure
 * </EN>
 * <JA>
 * AMパラメータ構造体に初期値を代入する.
 * 
 * @param j [in] AMパラメータ構造体
 * </JA>
 * 
 *
 * @callgraph
 * @callergraph
 * 
 */
void
jconf_set_default_values_am(JCONF_AM *j)
{
  j->name[0] = '\0';

  j->hmmfilename			= NULL;
  j->mapfilename			= NULL;
  j->gprune_method			= GPRUNE_SEL_UNDEF;
  j->mixnum_thres			= 2;
  j->spmodel_name			= NULL;
  j->hmm_gs_filename			= NULL;
  j->gs_statenum			= 24;
  j->iwcdmethod				= IWCD_UNDEF;
  j->iwcdmaxn				= 3;
  j->iwsp_penalty			= -1.0;
  j->force_multipath			= FALSE;
  undef_para(&(j->analysis.para));
  undef_para(&(j->analysis.para_hmm));
  undef_para(&(j->analysis.para_default));
  undef_para(&(j->analysis.para_htk));
  make_default_para(&(j->analysis.para_default));
  make_default_para_htk(&(j->analysis.para_htk));
  j->analysis.cmnload_filename		= NULL;
  j->analysis.cmn_update		= TRUE;
  j->analysis.cmnsave_filename		= NULL;
  j->analysis.cmn_map_weight		= 100.0;
  j->frontend.ss_alpha			= DEF_SSALPHA;
  j->frontend.ss_floor			= DEF_SSFLOOR;
  j->frontend.sscalc			= FALSE;
  j->frontend.sscalc_len		= 300;
  j->frontend.ssload_filename		= NULL;
}

/** 
 * <EN>
 * Fill in system default values to an LM parameter structure.
 * 
 * @param j [in] LM configuration parameter structure
 * </EN>
 * <JA>
 * LMパラメータ構造体に初期値を代入する.
 * 
 * @param j [in] LMパラメータ構造体
 * </JA>
 *
 * @callgraph
 * @callergraph
 * 
 */
void
jconf_set_default_values_lm(JCONF_LM *j)
{
  j->name[0] = '\0';

  j->lmtype = LM_UNDEF;
  j->lmvar  = LM_UNDEF;
  j->dictfilename			= NULL;
  j->head_silname			= NULL;
  j->tail_silname			= NULL;
  j->forcedict_flag			= FALSE;
  j->ngram_filename			= NULL;
  j->ngram_filename_lr_arpa		= NULL;
  j->ngram_filename_rl_arpa		= NULL;
  j->dfa_filename			= NULL;
  j->gramlist_root			= NULL;
  j->wordlist_root			= NULL;
  j->enable_iwsp			= FALSE;
  j->enable_iwspword			= FALSE;
  j->iwspentry				= NULL;
#ifdef SEPARATE_BY_UNIGRAM
  j->separate_wnum			= 150;
#endif
  strcpy(j->wordrecog_head_silence_model_name, "silB");
  strcpy(j->wordrecog_tail_silence_model_name, "silE");
  j->wordrecog_silence_context_name[0] = '\0';
  strcpy(j->unknown_name, UNK_WORD_DEFAULT); // or UNK_WORD_DEFAULT2
  j->additional_dict_files		= NULL;
  j->additional_dict_entries		= NULL;
}

/** 
 * <EN>
 * Fill in system default values to a search parameter structure.
 * 
 * @param j [in] search configuration parameter structure
 * </EN>
 * <JA>
 * 探索(SEARCH)パラメータ構造体に初期値を代入する.
 * 
 * @param j [in] 探索パラメータ構造体
 * </JA>
 * 
 * @callgraph
 * @callergraph
 * 
 */
void
jconf_set_default_values_search(JCONF_SEARCH *j)
{
  j->name[0] = '\0';

  j->amconf = NULL;
  j->lmconf = NULL;
  j->compute_only_1pass			= FALSE;
  j->force_ccd_handling			= FALSE;
  j->ccd_handling			= FALSE;
  /* 
    default values below are assigned later using HMM information:
	j->lmp.*
  */
  j->lmp.lm_penalty_trans		= 0.0;
  j->lmp.penalty1			= 0.0;
  j->lmp.penalty2			= 0.0;
  j->lmp.lmp2_specified			= FALSE;
  j->lmp.lmp_specified			= FALSE;

  j->pass1.specified_trellis_beam_width	= -1;
#ifdef SCORE_PRUNING
  j->pass1.score_pruning_width		= -1.0;
#endif
#if defined(WPAIR) && defined(WPAIR_KEEP_NLIMIT)
  j->pass1.wpair_keep_nlimit		= 3;
#endif
#ifdef HASH_CACHE_IW
  j->pass1.iw_cache_rate		= 10;
#endif
  j->pass1.old_tree_function_flag = FALSE;
#ifdef DETERMINE
  j->pass1.determine_score_thres = 10.0;
  j->pass1.determine_duration_thres = 6;
#endif
  if (strmatch(JULIUS_SETUP, "fast")) {
    j->pass2.nbest		= 1;
    j->pass2.enveloped_bestfirst_width = 30;
  } else {
    j->pass2.nbest		= 10;
    j->pass2.enveloped_bestfirst_width = 100;
  }
#ifdef SCAN_BEAM
  j->pass2.scan_beam_thres	= 80.0;
#endif
  j->pass2.hypo_overflow		= 2000;
  j->pass2.stack_size		= 500;
  j->pass2.lookup_range		= 5;
  j->pass2.looktrellis_flag	= FALSE; /* dfa */

  j->graph.enabled			= FALSE;
  j->graph.lattice			= FALSE;
  j->graph.confnet			= FALSE;
  j->graph.graph_merge_neighbor_range	= 0;
#ifdef   GRAPHOUT_DEPTHCUT
  j->graph.graphout_cut_depth		= 80;
#endif
#ifdef   GRAPHOUT_LIMIT_BOUNDARY_LOOP
  j->graph.graphout_limit_boundary_loop_num = 20;
#endif
#ifdef   GRAPHOUT_SEARCH_DELAY_TERMINATION
  j->graph.graphout_search_delay	= FALSE;
#endif
  j->successive.enabled			= FALSE;
  j->successive.sp_frame_duration	= 10;
  j->successive.pausemodelname		= NULL;
#ifdef SPSEGMENT_NAIST
  j->successive.sp_margin		= DEFAULT_SP_MARGIN;
  j->successive.sp_delay		= DEFAULT_SP_DELAY;
#endif
#ifdef CONFIDENCE_MEASURE
  j->annotate.cm_alpha			= 0.05;
#ifdef   CM_MULTIPLE_ALPHA
  j->annotate.cm_alpha_bgn		= 0.03;
  j->annotate.cm_alpha_end		= 0.15;
  j->annotate.cm_alpha_num		= 5;
  j->annotate.cm_alpha_step		= 0.03;
#endif
#ifdef   CM_SEARCH_LIMIT
  j->annotate.cm_cut_thres		= 0.03;
#endif
#ifdef   CM_SEARCH_LIMIT_POPO
  j->annotate.cm_cut_thres_pop		= 0.1;
#endif
#endif /* CONFIDENCE_MEASURE */
  j->annotate.align_result_word_flag	= FALSE;
  j->annotate.align_result_phoneme_flag	= FALSE;
  j->annotate.align_result_state_flag	= FALSE;

  j->output.output_hypo_maxnum		= 1;
  j->output.progout_flag		= FALSE;
  j->output.progout_interval		= 300;
  j->output.multigramout_flag		= FALSE; /* dfa */
  
  j->sw.trellis_check_flag		= FALSE;
  j->sw.triphone_check_flag		= FALSE;
  j->sw.wchmm_check_flag		= FALSE;
  j->sw.start_inactive			= FALSE;
  j->sw.fallback_pass1_flag		= FALSE;

#ifdef USE_MBR
  j->mbr.use_mbr = FALSE;
  j->mbr.use_word_weight = FALSE;
  j->mbr.score_weight = 0.1;
  j->mbr.loss_weight = 1.0;
#endif
}

/* end of file */
