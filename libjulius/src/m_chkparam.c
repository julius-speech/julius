/**
 * @file   m_chkparam.c
 * 
 * <JA>
 * @brief  パラメータ設定の後処理.
 *
 * jconf ファイルおよびコマンドオプションによって与えられた
 * パラメータについて後処理を行い，最終的に認識処理で使用する値を確定する. 
 * </JA>
 * 
 * <EN>
 * @brief  Post processing of parameters for recognition.
 *
 * These functions will finalize the parameter values for recognition.
 * They check for parameters given from jconf file or command line,
 * set default values if needed, and prepare for recognition.
 * 
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Fri Mar 18 16:31:45 2005
 *
 * $Revision: 1.9 $
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
 * ファイルが存在して読み込み可能かチェックする. 
 * 
 * @param filename [in] ファイルパス名
 * </JA>
 * <EN>
 * Check if a file actually exist and is readable.
 * 
 * @param filename [in] file path name
 * </EN>
 *
 */
boolean
checkpath(char *filename)
{
  if (access(filename, R_OK) == -1) {
    jlog("ERROR: m_chkparam: cannot access %s\n", filename);
    return FALSE;
  }
  return TRUE;
}

/** 
 * <JA>
 * @brief  jconf設定パラメータを最終的に決定する
 *
 * この関数は，jconf ファイルやコマンドオプションによって与えられた
 * jconf 内のパラメータについて精査を行う. 具体的には，値の範囲のチェッ
 * クや，競合のチェック，設定から算出される各種パラメータの計算，使用
 * するモデルに対する指定の有効性などをチェックする. 
 *
 * この関数は，アプリケーションによって jconf の各値の指定が終了した直後，
 * エンジンインスタンスの作成やモデルのロードが行われる前に呼び出される
 * べきである. 
 * 
 * </JA>
 * <EN>
 * @brief  Check and finalize jconf parameters.
 *
 * This functions parse through the global jconf configuration parameters.
 * This function checks for value range of variables, file existence,
 * competing specifications among variables or between variables and models,
 * calculate some parameters from the given values, etc.
 *
 * This function should be called just after all values are set by
 * jconf, command argument or by user application, and before creating
 * engine instance and loading models.
 * 
 * </EN>
 *
 * @param jconf [i/o] global jconf configuration structure
 *
 * @return TRUE when all check has been passed, or FALSE if not passed.
 *
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
boolean
j_jconf_finalize(Jconf *jconf)
{
  boolean ok_p;
  JCONF_LM *lm;
  JCONF_AM *am;
  JCONF_SEARCH *s, *hs;

  ok_p = TRUE;

  /* update and tailor configuration */
  /* if a search config has progout_flag enabled, set it to all config */
  hs = NULL;
  for(s=jconf->search_root;s;s=s->next) {
    if (s->output.progout_flag) {
      hs = s;
      break;
    }
  }
  if (hs != NULL) {
    for(s=jconf->search_root;s;s=s->next) {
      s->output.progout_flag = hs->output.progout_flag;
      s->output.progout_interval = hs->output.progout_interval;
    }
  }
      
  /* if an instance has short-pause segmentation enabled,
     set it to global opt for parameter handling
     (only a recognizer with this option will decide the segmentation,
      but the segmentation should be synchronized for all the recognizer)
  */
  for(s=jconf->search_root;s;s=s->next) {
    if (s->successive.enabled) {
      jconf->decodeopt.segment = TRUE;
      break;
    }
  }
#ifdef GMM_VAD
  /* if GMM VAD enabled, set it to global */
  if (jconf->reject.gmm_filename) {
    jconf->decodeopt.segment = TRUE;
  }
#endif

  for(lm = jconf->lm_root; lm; lm = lm->next) {
    if (lm->lmtype == LM_UNDEF) {
      /* determine LM type from the specified LM files */
      if (lm->ngram_filename_lr_arpa || lm->ngram_filename_rl_arpa || lm->ngram_filename) {
	/* n-gram specified */
	lm->lmtype = LM_PROB;
	lm->lmvar  = LM_NGRAM;
      }
      if (lm->gramlist_root) {
	/* DFA grammar specified */
	if (lm->lmtype != LM_UNDEF) {
	  jlog("ERROR: m_chkparam: LM conflicts: several LM of different type specified?\n");
	  return FALSE;
	}
	lm->lmtype = LM_DFA;
	lm->lmvar  = LM_DFA_GRAMMAR;
      }
      if (lm->dfa_filename) {
	/* DFA grammar specified by "-dfa" */
	if (lm->lmtype != LM_UNDEF && lm->lmvar != LM_DFA_GRAMMAR) {
	  jlog("ERROR: m_chkparam: LM conflicts: several LM of different type specified?\n");
	  return FALSE;
	}
	lm->lmtype = LM_DFA;
	lm->lmvar  = LM_DFA_GRAMMAR;
      }
      if (lm->wordlist_root) {
	/* word list specified */
	if (lm->lmtype != LM_UNDEF) {
	  jlog("ERROR: m_chkparam: LM conflicts: several LM of different type specified?\n");
	  return FALSE;
	}
	lm->lmtype = LM_DFA;
	lm->lmvar  = LM_DFA_WORD;
      }
    }
    if (lm->lmtype == LM_UNDEF) { /* an LM is not specified */
      jlog("ERROR: m_chkparam: you should specify at least one LM to run Julius!\n");
      return FALSE;
    }
    if (lm->lmtype == LM_PROB) {
      if (lm->dictfilename == NULL) {
	jlog("ERROR: m_chkparam: needs dictionary file (-v dict_file)\n");
	ok_p = FALSE;
      }
    }
    /* file existence check */
    if (lm->dictfilename != NULL) 
      if (!checkpath(lm->dictfilename)) ok_p = FALSE;
    if (lm->ngram_filename != NULL) 
      if (!checkpath(lm->ngram_filename)) ok_p = FALSE;
    if (lm->ngram_filename_lr_arpa != NULL)
      if (!checkpath(lm->ngram_filename_lr_arpa)) ok_p = FALSE;
    if (lm->ngram_filename_rl_arpa != NULL)
      if (!checkpath(lm->ngram_filename_rl_arpa)) ok_p = FALSE;
    if (lm->dfa_filename != NULL) 
      if (!checkpath(lm->dfa_filename)) ok_p = FALSE;
  }

  for(am = jconf->am_root; am; am = am->next) {
    /* check if needed files are specified */
    if (am->hmmfilename == NULL) {
      jlog("ERROR: m_chkparam: needs HMM definition file (-h hmmdef_file)\n");
      ok_p = FALSE;
    }
    /* file existence check */
    if (am->hmmfilename != NULL) 
      if (!checkpath(am->hmmfilename)) ok_p = FALSE;
    if (am->mapfilename != NULL) 
      if (!checkpath(am->mapfilename)) ok_p = FALSE;
    if (am->hmm_gs_filename != NULL) 
      if (!checkpath(am->hmm_gs_filename)) ok_p = FALSE;
    /* cmn{save,load}_filename allows missing file (skipped if missing) */
    if (am->frontend.ssload_filename != NULL) 
      if (!checkpath(am->frontend.ssload_filename)) ok_p = FALSE;
  }
  if (jconf->reject.gmm_filename != NULL) 
    if (!checkpath(jconf->reject.gmm_filename)) ok_p = FALSE;
  if (jconf->input.inputlist_filename != NULL) {
    if (jconf->input.speech_input != SP_RAWFILE && jconf->input.speech_input != SP_MFCFILE && jconf->input.speech_input != SP_OUTPROBFILE) {
      jlog("WARNING: m_chkparam: not file input, \"-filelist %s\" ignored\n", jconf->input.inputlist_filename);
    } else {
      if (!checkpath(jconf->input.inputlist_filename)) ok_p = FALSE;
    }
  }

  /* set default realtime flag according to input mode */
  if (jconf->decodeopt.force_realtime_flag) {
    if (jconf->input.type == INPUT_VECTOR) {
      jlog("WARNING: m_chkparam: real-time concurrent processing is not needed on feature vector input\n");
      jlog("WARNING: m_chkparam: real-time flag has turned off\n");
      jconf->decodeopt.realtime_flag = FALSE;
    } else {
      jconf->decodeopt.realtime_flag = jconf->decodeopt.forced_realtime;
    }
  }

  /* check for cmn */
  if (jconf->decodeopt.realtime_flag) {
    for(am = jconf->am_root; am; am = am->next) {
      if (am->analysis.cmn_update == FALSE && am->analysis.cmnload_filename == NULL) {
	jlog("ERROR: m_chkparam: when \"-cmnnoupdate\", initial cepstral normalisation data should be given by \"-cmnload\"\n");
	ok_p = FALSE;
      }
    }
  }

  /* set values for search config */
  for(s=jconf->search_root;s;s=s->next) {
    lm = s->lmconf;
    am = s->amconf;

    /* force context dependency handling flag for word-recognition mode */
    if (lm->lmtype == LM_DFA && lm->lmvar == LM_DFA_WORD) {
      /* disable inter-word context dependent handling ("-no_ccd") */
      s->ccd_handling = FALSE;
      s->force_ccd_handling = TRUE;
      /* force 1pass ("-1pass") */
      s->compute_only_1pass = TRUE;
    }

    /* set default iwcd1 method from lm */
    /* WARNING: THIS WILL BEHAVE WRONG IF MULTIPLE LM TYPE SPECIFIED */
    /* RECOMMEND USING EXPLICIT OPTION */
    if (am->iwcdmethod == IWCD_UNDEF) {
      switch(lm->lmtype) {
      case LM_PROB:
	am->iwcdmethod = IWCD_NBEST; break;
      case LM_DFA:
	am->iwcdmethod = IWCD_AVG; break;
      }
    }

  }

  /* check option validity with the current lm type */
  /* just a warning message for user */
  for(s=jconf->search_root;s;s=s->next) {
    lm = s->lmconf;
    am = s->amconf;
    if (lm->lmtype != LM_PROB) {
      /* in case not a probabilistic model */
      if (s->lmp.lmp_specified) {
	jlog("WARNING: m_chkparam: \"-lmp\" only for N-gram, ignored\n");
      }
      if (s->lmp.lmp2_specified) {
	jlog("WARNING: m_chkparam: \"-lmp2\" only for N-gram, ignored\n");
      }
      if (s->lmp.lm_penalty_trans != 0.0) {
	jlog("WARNING: m_chkparam: \"-transp\" only for N-gram, ignored\n");
      }
      if (lm->head_silname && !strmatch(lm->head_silname, BEGIN_WORD_DEFAULT)) {
	jlog("WARNING: m_chkparam: \"-silhead\" only for N-gram, ignored\n");
      }
      if (lm->tail_silname && !strmatch(lm->tail_silname, END_WORD_DEFAULT)) {
	jlog("WARNING: m_chkparam: \"-siltail\" only for N-gram, ignored\n");
      }
      if (lm->enable_iwspword) {
	jlog("WARNING: m_chkparam: \"-iwspword\" only for N-gram, ignored\n");
      }
      if (lm->iwspentry && !strmatch(lm->iwspentry, IWSPENTRY_DEFAULT)) {
	jlog("WARNING: m_chkparam: \"-iwspentry\" only for N-gram, ignored\n");
      }
#ifdef HASH_CACHE_IW
      if (s->pass1.iw_cache_rate != 10) {
	jlog("WARNING: m_chkparam: \"-iwcache\" only for N-gram, ignored\n");
      }
#endif
#ifdef SEPARATE_BY_UNIGRAM
      if (lm->separate_wnum != 150) {
	jlog("WARNING: m_chkparam: \"-sepnum\" only for N-gram, ignored\n");
      }
#endif
    }  
    if (lm->lmtype != LM_DFA) {
      /* in case not a deterministic model */
      if (s->pass2.looktrellis_flag) {
	jlog("WARNING: m_chkparam: \"-looktrellis\" only for grammar, ignored\n");
      }
      if (s->output.multigramout_flag) {
	jlog("WARNING: m_chkparam: \"-multigramout\" only for grammar, ignored\n");
      }
      if (s->lmp.penalty1 != 0.0) {
	jlog("WARNING: m_chkparam: \"-penalty1\" only for grammar, ignored\n");
      }
      if (s->lmp.penalty2 != 0.0) {
	jlog("WARNING: m_chkparam: \"-penalty2\" only for grammar, ignored\n");
      }
    }
  }



  if (!ok_p) {
    jlog("ERROR: m_chkparam: could not pass parameter check\n");
  } else {
    jlog("STAT: jconf successfully finalized\n");
  }

  if (debug2_flag) {
    print_jconf_overview(jconf);
  }

  return ok_p;
}

/** 
 * <JA>
 * @brief  あらかじめ定められた第1パスのデフォルトビーム幅を返す. 
 *
 * デフォルトのビーム幅は，認識エンジンのコンパイル時設定や
 * 使用する音響モデルに従って選択される. これらの値は，20k の
 * IPA 評価セットで得られた最適値（精度を保ちつつ最大速度が得られる値）
 * である. 
 * 
 * @return 実行時の条件によって選択されたビーム幅
 * </JA>
 * <EN>
 * @brief  Returns the pre-defined default beam width on 1st pass of
 * beam search.
 * 
 * The default beam width will be selected from the pre-defined values
 * according to the compilation-time engine setting and the type of
 * acoustic model.  The pre-defined values were determined from the
 * development experiments on IPA evaluation testset of Japanese 20k-word
 * dictation task.
 * 
 * @return the selected default beam width.
 * </EN>
 */
static int
default_width(HTK_HMM_INFO *hmminfo)
{
  if (strmatch(JULIUS_SETUP, "fast")) { /* for fast setup */
    if (hmminfo->is_triphone) {
      if (hmminfo->is_tied_mixture) {
	/* tied-mixture triphones (PTM etc.) */
	return(600);
      } else {
	/* shared-state triphone */
#ifdef PASS1_IWCD
	return(800);
#else
	/* v2.1 compliant (no IWCD on 1st pass) */
	return(1000);		
#endif
      }
    } else {
      /* monophone */
      return(400);
    }
  } else {			/* for standard / v2.1 setup */
    if (hmminfo->is_triphone) {
      if (hmminfo->is_tied_mixture) {
	/* tied-mixture triphones (PTM etc.) */
	return(800);
      } else {
	/* shared-state triphone */
#ifdef PASS1_IWCD
	return(1500);
#else
	return(1500);		/* v2.1 compliant (no IWCD on 1st pass) */
#endif
      }
    } else {
      /* monophone */
      return(700);
    }
  }
}

/** 
 * <JA>
 * @brief  第1パスのビーム幅を決定する. 
 *
 * ユーザが "-b" オプションでビーム幅を指定しなかった場合は，
 * 下記のうち小さい方がビーム幅として採用される. 
 *   - default_width() の値
 *   - sqrt(語彙数) * 15
 * 
 * @param wchmm [in] 木構造化辞書
 * @param specified [in] ユーザ指定ビーム幅(0: 全探索 -1: 未指定)
 * 
 * @return 採用されたビーム幅. 
 * </JA>
 * <EN>
 * @brief  Determine beam width on the 1st pass.
 * 
 * @param wchmm [in] tree lexicon data
 * @param specified [in] user-specified beam width (0: full search,
 * -1: not specified)
 * 
 * @return the final beam width to be used.
 * </EN>
 *
 * @callgraph
 * @callergraph
 */
int
set_beam_width(WCHMM_INFO *wchmm, int specified)
{
  int width;
  int standard_width;
  
  if (specified == 0) { /* full search */
    jlog("WARNING: doing full search (can be extremely slow)\n");
    width = wchmm->n;
  } else if (specified == -1) { /* not specified */
    standard_width = default_width(wchmm->hmminfo); /* system default */
    width = (int)(sqrt(wchmm->winfo->num) * 15.0); /* heuristic value!! */
    if (width > standard_width) width = standard_width;
    /* 2007/1/20 bgn */
    if (width < MINIMAL_BEAM_WIDTH) {
      width = MINIMAL_BEAM_WIDTH;
    }
    /* 2007/1/20 end */
  } else {			/* actual value has been specified */
    width = specified;
  }
  if (width > wchmm->n) width = wchmm->n;

  return(width);
}

/* end of file */
