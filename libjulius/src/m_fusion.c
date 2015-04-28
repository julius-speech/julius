/**
 * @file   m_fusion.c
 * 
 * <JA>
 * @brief  認識の最終準備をする. 
 *
 * 設定に従い，モデルの読み込み・木構造化辞書などのデータ構造の構築・
 * ワークエリアの確保など，認識開始に必要な環境の構築を行なう. 
 * </JA>
 * 
 * <EN>
 * @brief  Final set up for recognition.
 *
 * These functions build everything needed for recognition: load
 * models into memory, build data structures such as tree lexicon, and
 * allocate work area for computation.
 *
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Thu May 12 13:31:47 2005
 *
 * $Revision: 1.29 $
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
 * @brief  音響HMMをファイルから読み込み，認識用にセットアップする. 
 *
 * ファイルからのHMM定義の読み込み，HMMList ファイルの読み込み，
 * パラメータ型のチェック，マルチパス扱いの on/off, ポーズモデルの設定など
 * が行われ，認識のための準備が行われる. 
 *
 * この音響モデルの入力となる音響パラメータの種類やパラメータもここで
 * 最終決定される. 決定には，音響HMMのヘッダ，（バイナリHMMの場合，存
 * 在すれば）バイナリHMMに埋め込まれた特徴量情報，jconf の設定（ばらば
 * らに，あるいは -htkconf 使用時）などの情報が用いられる. 
 * </JA>
 * <EN>
 * @brief  Read in an acoustic HMM from file and setup for recognition.
 *
 * This functions reads HMM definitions from file, reads also a
 * HMMList file, makes logical-to-physical model mapping, determine
 * required parameter type, determine whether multi-path handling is needed,
 * and find pause model in the definitions.
 *
 * The feature vector extraction parameters are also finally
 * determined in this function.  Informations used for the
 * determination is (1) the header values in hmmdefs, (2) embedded
 * parameters in binary HMM if you are reading a binary HMM made with
 * recent mkbinhmm, (3) user-specified parameters in jconf
 * configurations (either by separatedly specified or by -htkconf
 * options).
 *
 * </EN>
 * 
 * @param amconf [in] AM configuration variables
 * @param jconf [i/o] global configuration variables
 * 
 * @return the newly created HMM information structure, or NULL on failure.
 * 
 */
static HTK_HMM_INFO *
initialize_HMM(JCONF_AM *amconf, Jconf *jconf)
{
  HTK_HMM_INFO *hmminfo;

  /* at here, global variable "para" holds values specified by user or
     by user-specified HTK config file */
  if (amconf->analysis.para_hmm.loaded == 1) {
    jlog("Warning: you seems to read more than one acoustic model for recognition, but\n");
    jlog("Warning: previous one already has header-embedded acoustic parameters\n");
    jlog("Warning: if you have different parameters, result may be wrong!\n");
  }
  
  /* allocate new hmminfo */
  hmminfo = hmminfo_new();
  /* load hmmdefs */
  if (init_hmminfo(hmminfo, amconf->hmmfilename, amconf->mapfilename, &(amconf->analysis.para_hmm)) == FALSE) {
    hmminfo_free(hmminfo);
    return NULL;
  }
  if (debug2_flag) {
    HTK_HMM_Data *dtmp;
    int i;
    for (dtmp = hmminfo->start; dtmp; dtmp = dtmp->next) {
      printf("***\nname: %s\n", dtmp->name);
      for (i=0;i<dtmp->state_num;i++) {
	if (dtmp->s[i] == NULL) continue;
	printf("state %d: id=%d   %s\n", i + 1, dtmp->s[i]->id, (dtmp->s[i]->name) ? dtmp->s[i]->name : "");
      }
    }
  }

  /* set multipath mode flag */
  if (amconf->force_multipath) {
    jlog("STAT: m_fusion: force multipath HMM handling by user request\n");
    hmminfo->multipath = TRUE;
  } else {
    hmminfo->multipath = hmminfo->need_multipath;
  }

  /* only MFCC is supported for audio input */
  /* MFCC_{0|E}[_D][_A][_Z][_N] is supported */
  /* check parameter type of this acoustic HMM */
  if (jconf->input.type == INPUT_WAVEFORM) {
    /* Decode parameter extraction type according to the training
       parameter type in the header of the given acoustic HMM */
    switch(hmminfo->opt.param_type & F_BASEMASK) {
    case F_MFCC:
    case F_FBANK:
    case F_MELSPEC:
      break;
    default:
      jlog("ERROR: m_fusion: for direct speech input, only HMM trained by MFCC ior filterbank is supported\n");
      hmminfo_free(hmminfo);
      return NULL;
    }
    /* set acoustic analysis parameters from HMM header */
    calc_para_from_header(&(amconf->analysis.para), hmminfo->opt.param_type, hmminfo->opt.vec_size);
  }
  /* check if tied_mixture */
  if (hmminfo->is_tied_mixture && hmminfo->codebooknum <= 0) {
    jlog("ERROR: m_fusion: this tied-mixture model has no codebook!?\n");
    hmminfo_free(hmminfo);
    return NULL;
  }

#ifdef PASS1_IWCD
  /* make state clusters of same context for inter-word triphone approx. */
  if (hmminfo->is_triphone) {
    if (hmminfo->cdset_root == NULL) {
      jlog("STAT: making pseudo bi/mono-phone for IW-triphone\n");
      if (make_cdset(hmminfo) == FALSE) {
	jlog("ERROR: m_fusion: failed to make context-dependent state set\n");
	hmminfo_free(hmminfo);
	return NULL;
      }
    } else {
      jlog("STAT: pseudo phones are loaded from binary hmmlist file\n");
    }

    /* add those `pseudo' biphone and monophone to the logical HMM names */
    /* they points not to the defined HMM, but to the CD_Set structure */
    hmm_add_pseudo_phones(hmminfo);
  }
#endif

  /* find short pause model and set to hmminfo->sp */
  htk_hmm_set_pause_model(hmminfo, amconf->spmodel_name);


  hmminfo->cdset_method = amconf->iwcdmethod;
  hmminfo->cdmax_num = amconf->iwcdmaxn;

  if (amconf->analysis.para_htk.loaded == 1) apply_para(&(amconf->analysis.para), &(amconf->analysis.para_htk));
  if (amconf->analysis.para_hmm.loaded == 1) apply_para(&(amconf->analysis.para), &(amconf->analysis.para_hmm));
  apply_para(&(amconf->analysis.para), &(amconf->analysis.para_default));

  return(hmminfo);
  
}

/** 
 * <JA>
 * Gaussian Mixture Selection のための状態選択用モノフォンHMMを読み込む. 
 * </JA>
 * <EN>
 * Initialize context-independent HMM for state selection with Gaussian
 * Mixture Selection.
 * </EN>
 *
 * @param amconf [in] AM configuratino variables
 *
 * @return the newly created HMM information structure, or NULL on failure.
 */
static HTK_HMM_INFO *
initialize_GSHMM(JCONF_AM *amconf)
{
  HTK_HMM_INFO *hmm_gs;
  Value para_dummy;

  jlog("STAT: Reading GS HMMs:\n");
  hmm_gs = hmminfo_new();
  undef_para(&para_dummy);
  if (init_hmminfo(hmm_gs, amconf->hmm_gs_filename, NULL, &para_dummy) == FALSE) {
    hmminfo_free(hmm_gs);
    return NULL;
  }
  return(hmm_gs);
}

/** 
 * <JA>
 * 発話検証・棄却用の1状態 GMM を読み込んで初期化する. 
 * 
 * </JA>
 * <EN>
 * Read and initialize an 1-state GMM for utterance verification and
 * rejection.
 * 
 * </EN>
 *
 * @param jconf [in] global configuration variables
 *
 * @return the newly created GMM information structure in HMM format,
 * or NULL on failure.
 */
static HTK_HMM_INFO *
initialize_GMM(Jconf *jconf)
{
  HTK_HMM_INFO *gmm;
  
  jlog("STAT: reading GMM: %s\n", jconf->reject.gmm_filename);

  if (jconf->gmm == NULL) {
    /* no acoustic parameter setting was given for GMM using -AM_GMM, 
       copy the first AM setting */
    jlog("STAT: -AM_GMM not used, use parameter of the first AM\n");
    jconf->gmm = j_jconf_am_new();
    memcpy(jconf->gmm, jconf->am_root, sizeof(JCONF_AM));
    jconf->gmm->hmmfilename = NULL;
    jconf->gmm->mapfilename = NULL;
    jconf->gmm->spmodel_name = NULL;
    jconf->gmm->hmm_gs_filename = NULL;
    if (jconf->am_root->analysis.cmnload_filename) {
      jconf->gmm->analysis.cmnload_filename = strcpy((char *)mymalloc(strlen(jconf->am_root->analysis.cmnload_filename)+ 1), jconf->am_root->analysis.cmnload_filename);
    }
    if (jconf->am_root->analysis.cmnsave_filename) {
      jconf->gmm->analysis.cmnsave_filename = strcpy((char *)mymalloc(strlen(jconf->am_root->analysis.cmnsave_filename)+ 1), jconf->am_root->analysis.cmnsave_filename);
    }
    if (jconf->am_root->frontend.ssload_filename) {
      jconf->gmm->frontend.ssload_filename = strcpy((char *)mymalloc(strlen(jconf->am_root->frontend.ssload_filename)+ 1), jconf->am_root->frontend.ssload_filename);
    }
  }

  gmm = hmminfo_new();
  if (init_hmminfo(gmm, jconf->reject.gmm_filename, NULL, &(jconf->gmm->analysis.para_hmm)) == FALSE) {
    hmminfo_free(gmm);
    return NULL;
  }
  /* check parameter type of this acoustic HMM */
  if (jconf->input.type == INPUT_WAVEFORM) {
    /* Decode parameter extraction type according to the training
       parameter type in the header of the given acoustic HMM */
    switch(gmm->opt.param_type & F_BASEMASK) {
    case F_MFCC:
    case F_FBANK:
    case F_MELSPEC:
      break;
    default:
      jlog("ERROR: m_fusion: for direct speech input, only GMM trained by MFCC ior filterbank is supported\n");
      hmminfo_free(gmm);
      return NULL;
    }
  }

  /* set acoustic analysis parameters from HMM header */
  calc_para_from_header(&(jconf->gmm->analysis.para), gmm->opt.param_type, gmm->opt.vec_size);

  if (jconf->gmm->analysis.para_htk.loaded == 1) apply_para(&(jconf->gmm->analysis.para), &(jconf->gmm->analysis.para_htk));
  if (jconf->gmm->analysis.para_hmm.loaded == 1) apply_para(&(jconf->gmm->analysis.para), &(jconf->gmm->analysis.para_hmm));
  apply_para(&(jconf->gmm->analysis.para), &(jconf->gmm->analysis.para_default));

  return(gmm);
}

/** 
 * <JA>
 * @brief  単語辞書をファイルから読み込んでセットアップする. 
 *
 * 辞書上のモノフォン表記からトライフォンへの計算は init_voca() で
 * 読み込み時に行われる. このため，辞書読み込み時には，認識で使用する
 * 予定のHMM情報を与える必要がある. 
 *
 * N-gram 使用時は，文頭無音単語およぶ文末無音単語をここで設定する. 
 * また，"-iwspword" 指定時は，ポーズ単語を辞書の最後に挿入する. 
 * 
 * </JA>
 * <EN>
 * @brief  Read in word dictionary from a file and setup for recognition.
 *
 * Monophone-to-triphone conversion will be performed inside init_voca().
 * So, an HMM definition data that will be used with the LM should also be
 * specified as an argument.
 * 
 * When reading dictionary for N-gram, sentence head silence word and
 * tail silence word will be determined in this function.  Also,
 * when an option "-iwspword" is specified, this will insert a pause
 * word at the last of the given dictionary.
 * 
 * </EN>
 *
 * @param lmconf [in] LM configuration variables
 * @param hmminfo [in] HMM definition of each phone in dictionary, for
 * phone checking and monophone-to-triphone conversion.
 *
 * @return the newly created word dictionary structure, or NULL on failure.
 * 
 */
static WORD_INFO *
initialize_dict(JCONF_LM *lmconf, HTK_HMM_INFO *hmminfo)
{
  WORD_INFO *winfo;
  JCONF_LM_NAMELIST *nl;
  char buf[MAXLINELEN];
  int n;

  /* allocate new word dictionary */
  winfo = word_info_new();
  /* read in dictinary from file */
  if ( ! 
#ifdef MONOTREE
      /* leave winfo monophone for 1st pass lexicon tree */
       init_voca(winfo, lmconf->dictfilename, hmminfo, TRUE, lmconf->forcedict_flag)
#else 
       init_voca(winfo, lmconf->dictfilename, hmminfo, FALSE, lmconf->forcedict_flag)
#endif
       ) {
    jlog("ERROR: m_fusion: failed to read dictionary, terminated\n");
    word_info_free(winfo);
    return NULL;
  }

  /* load additional entries */
  for (nl = lmconf->additional_dict_files; nl; nl=nl->next) {
    FILE *fp;
    if ((fp = fopen(nl->name, "rb")) == NULL) {
      jlog("ERROR: m_fusion: failed to open %s\n",nl->name);
      word_info_free(winfo);
      return NULL;
    }
    n = winfo->num;
    while (getl_fp(buf, MAXLINELEN, fp) != NULL) {
      if (voca_load_line(buf, winfo, hmminfo) == FALSE) break;
    }
    if (voca_load_end(winfo) == FALSE) {
      if (lmconf->forcedict_flag) {
	jlog("Warning: m_fusion: the error words above are ignored\n");
      } else {
	jlog("ERROR: m_fusion: error in reading dictionary %s\n", nl->name);
	fclose(fp);
	word_info_free(winfo);
	return NULL;
      }
    }
    if (fclose(fp) == -1) {
      jlog("ERROR: m_fusion: failed to close %s\n", nl->name);
      word_info_free(winfo);
      return NULL;
    }
    jlog("STAT: + additional dictionary: %s (%d words)\n", nl->name, winfo->num - n);
  }
  n = winfo->num;
  for (nl = lmconf->additional_dict_entries; nl; nl=nl->next) {
    if (voca_load_line(nl->name, winfo, hmminfo) == FALSE) {
      jlog("ERROR: m_fusion: failed to set entry: %s\n", nl->name);
    }
  }
  if (lmconf->additional_dict_entries) {
    if (voca_load_end(winfo) == FALSE) {
      jlog("ERROR: m_fusion: failed to read additinoal word entry\n");
      word_info_free(winfo);
      return NULL;
    }
    jlog("STAT: + additional entries: %d words\n", winfo->num - n);
  }

  if (lmconf->lmtype == LM_PROB) {
    /* if necessary, append a IW-sp word to the dict if "-iwspword" specified */
    if (lmconf->enable_iwspword) {
      if (
#ifdef MONOTREE
	  voca_append_htkdict(lmconf->iwspentry, winfo, hmminfo, TRUE)
#else 
	  voca_append_htkdict(lmconf->iwspentry, winfo, hmminfo, FALSE)
#endif
	  == FALSE) {
	jlog("ERROR: m_fusion: failed to make IW-sp word entry \"%s\"\n", lmconf->iwspentry);
	word_info_free(winfo);
	return NULL;
      } else {
	jlog("STAT: 1 IW-sp word entry added\n");
      }
    }
    /* set {head,tail}_silwid */
    winfo->head_silwid = voca_lookup_wid(lmconf->head_silname, winfo);
    if (winfo->head_silwid == WORD_INVALID) { /* not exist */
      jlog("ERROR: m_fusion: head sil word \"%s\" not exist in voca\n", lmconf->head_silname);
      word_info_free(winfo);
      return NULL;
    }
    winfo->tail_silwid = voca_lookup_wid(lmconf->tail_silname, winfo);
    if (winfo->tail_silwid == WORD_INVALID) { /* not exist */
      jlog("ERROR: m_fusion: tail sil word \"%s\" not exist in voca\n", lmconf->tail_silname);
      word_info_free(winfo);
      return NULL;
    }
  }
  
  return(winfo);
  
}


/** 
 * <JA>
 * @brief  単語N-gramをファイルから読み込んでセットアップする. 
 *
 * ARPA フォーマットで指定時は，LRファイルと RL ファイルの組合せで
 * 動作が異なる. LR のみ，あるいは RL のみ指定時は，それをそのまま読み込む. 
 * 双方とも指定されている場合は，RLをまず主モデルとして読み込んだ後，
 * LR の 2-gram だけを第1パス用に主モデルに追加読み込みする. 
 *
 * また，読み込み終了後，辞書上のN-gramエントリとのマッチングを取る. 
 * 
 * </JA>
 * <EN>
 * @brief  Read in word N-gram from file and setup for recognition.
 *
 * When N-gram is specified in ARPA format, the behavior relies on whether
 * N-grams are specified in "-nlr" and "-nrl".  When either of them was
 * solely specified,  this function simply read it.  If both are specified,
 * it will read the RL model fully as a primary model, and additionally
 * read only the 2-gram part or the LR model as the first pass LM.
 *
 * Also, this function create mapping from dictionary words to LM entry.
 * 
 * </EN>
 *
 * @param lmconf [in] LM configuration variables
 * @param winfo [i/o] word dictionary that will be used with this N-gram.
 * each word in the dictionary will be assigned to an N-gram entry here.
 *
 * @return the newly created N-gram information data, or NULL on failure.
 * 
 */
static NGRAM_INFO *
initialize_ngram(JCONF_LM *lmconf, WORD_INFO *winfo)
{
  NGRAM_INFO *ngram;
  boolean ret;

  /* allocate new */
  ngram = ngram_info_new();
  /* load LM */
  if (lmconf->ngram_filename != NULL) {	/* binary format */
    ret = init_ngram_bin(ngram, lmconf->ngram_filename);
  } else {			/* ARPA format */
    /* if either forward or backward N-gram is specified, read it */
    /* if both specified, use backward N-gram as main and
       use forward 2-gram only for 1st pass (this is an old behavior) */
    if (lmconf->ngram_filename_rl_arpa) {
      ret = init_ngram_arpa(ngram, lmconf->ngram_filename_rl_arpa, DIR_RL);
      if (ret == FALSE) {
	ngram_info_free(ngram);
	return NULL;
      }
      if (lmconf->ngram_filename_lr_arpa) {
	ret = init_ngram_arpa_additional(ngram, lmconf->ngram_filename_lr_arpa);
	if (ret == FALSE) {
	  ngram_info_free(ngram);
	  return NULL;
	}
      }
    } else if (lmconf->ngram_filename_lr_arpa) {
      ret = init_ngram_arpa(ngram, lmconf->ngram_filename_lr_arpa, DIR_LR);
    }
  }
  if (ret == FALSE) {
    ngram_info_free(ngram);
    return NULL;
  }

  /* set unknown (=OOV) word id */
  if (strcmp(lmconf->unknown_name, UNK_WORD_DEFAULT)) {
    set_unknown_id(ngram, lmconf->unknown_name);
  }

  /* map dict item to N-gram entry */
  if (make_voca_ref(ngram, winfo) == FALSE) {
    ngram_info_free(ngram);
    return NULL;
  }

  /* post-fix EOS / BOS uni prob for SRILM */
  fix_uniprob_srilm(ngram, winfo);

  return(ngram);
}

/** 
 * <EN>
 * @brief  Load an acoustic model.
 *
 * This function will create an AM process instance using the given AM
 * configuration, and load models specified in the configuration into
 * the instance.  Then the created instance will be installed to the
 * engine instance.  The amconf should be registered to the global
 * jconf before calling this function.
 *
 * </EN>
 *
 * <JA>
 * @brief 音響モデルを読み込む．
 *
 * この関数は，与えられた AM 設定に従って AM 処理インスタンスを生成し，
 * その中に音響モデルをロードします．その後，そのAM処理インスタンスは
 * 新たにエンジンインスタンスに登録されます．AM設定はこの関数を
 * 呼ぶ前にあらかじめ全体設定recog->jconfに登録されている必要があります．
 * 
 * </JA>
 * 
 * @param recog [i/o] engine instance
 * @param amconf [in] AM configuration to load
 * 
 * @return TRUE on success, or FALSE on error.
 *
 * @callgraph
 * @callergraph
 * @ingroup instance
 * 
 */
boolean
j_load_am(Recog *recog, JCONF_AM *amconf)
{
  PROCESS_AM *am;

  jlog("STAT: *** loading AM%02d %s\n", amconf->id, amconf->name);

  /* create AM process instance */
  am = j_process_am_new(recog, amconf);
  
  /* HMM */
  if ((am->hmminfo = initialize_HMM(amconf, recog->jconf)) == NULL) {
    jlog("ERROR: m_fusion: failed to initialize AM\n");
    return FALSE;
  }
  if (amconf->hmm_gs_filename != NULL) {
    if ((am->hmm_gs = initialize_GSHMM(amconf)) == NULL) {
      jlog("ERROR: m_fusion: failed to initialize GS HMM\n");
      return FALSE;
    }
  }

  /* fixate model-specific params */
  /* set params whose default will change by models and not specified in arg */
  /* select Gaussian pruning function */
  if (am->config->gprune_method == GPRUNE_SEL_UNDEF) {/* set default if not specified */
    if (am->hmminfo->is_tied_mixture) {
      /* enabled by default for tied-mixture models */
#if defined(GPRUNE_DEFAULT_SAFE)
      am->config->gprune_method = GPRUNE_SEL_SAFE;
#elif defined(GPRUNE_DEFAULT_HEURISTIC)
      am->config->gprune_method = GPRUNE_SEL_HEURISTIC;
#elif defined(GPRUNE_DEFAULT_BEAM)
      am->config->gprune_method = GPRUNE_SEL_BEAM;
#endif
    } else {
      /* disabled by default for non tied-mixture model */
      am->config->gprune_method = GPRUNE_SEL_NONE;
    }
  }
  
  /* fixated analysis.para not uses loaded flag any more, so
     reset it for binary matching */
  amconf->analysis.para.loaded = 0;

  jlog("STAT: *** AM%02d %s loaded\n", amconf->id, amconf->name);

  return TRUE;
}

/** 
 * <EN>
 * @brief  Load a language model.
 *
 * This function will create an LM process instance using the given LM
 * configuration, and load models specified in the configuration into
 * the instance.  Then the created instance will be installed to the
 * engine instance.  The lmconf should be registered to the 
 * recog->jconf before calling this function.
 *
 * To convert phoneme sequence to triphone at loading, you should
 * specify which AM to use with this LM by the argument am.
 *
 * </EN>
 *
 * <JA>
 * @brief 言語モデルを読み込む．
 *
 * この関数は，与えられた LM 設定に従って LM 処理インスタンスを生成し，
 * その中に言語モデルをロードします．その後，そのLM処理インスタンスは
 * 新たにエンジンインスタンスに登録されます．LM設定はこの関数を
 * 呼ぶ前にあらかじめ全体設定recog->jconfに登録されている必要があります．
 *
 * 辞書の読み込み時にトライフォンへの変換および音響モデルとのリンクが
 * 同時に行われます．このため，この言語モデルが使用する音響モデルの
 * インスタンスを引数 am として指定する必要があります．
 * 
 * </JA>
 * 
 * @param recog [i/o] engine instance
 * @param lmconf [in] LM configuration to load
 * 
 * @return TRUE on success, or FALSE on error.
 * 
 * @callgraph
 * @callergraph
 * @ingroup instance
 * 
 */
boolean
j_load_lm(Recog *recog, JCONF_LM *lmconf)
{
  JCONF_SEARCH *sh;
  PROCESS_LM *lm;
  PROCESS_AM *am, *atmp;

  jlog("STAT: *** loading LM%02d %s\n", lmconf->id, lmconf->name);

  /* find which am process instance to assign to each LM */
  am = NULL;
  for(sh=recog->jconf->search_root;sh;sh=sh->next) {
    if (sh->lmconf == lmconf) {
      for(atmp=recog->amlist;atmp;atmp=atmp->next) {
	if (sh->amconf == atmp->config) {
	  am = atmp;
	}
      }
    }
  }
  if (am == NULL) {
    jlog("ERROR: cannot find corresponding AM for LM%02d %s\n", lmconf->id, lmconf->name);
    jlog("ERROR: you should write all AM/LM combinations to be used for recognition with \"-SR\"\n");
    return FALSE;
  }

  /* create LM process instance */
  lm = j_process_lm_new(recog, lmconf);

  /* assign AM process instance to the LM instance */
  lm->am = am;

  /* load language model */
  if (lm->lmtype == LM_PROB) {
    /* LM (N-gram) */
    if ((lm->winfo = initialize_dict(lm->config, lm->am->hmminfo)) == NULL) {
      jlog("ERROR: m_fusion: failed to initialize dictionary\n");
      return FALSE;
    }
    if (lm->config->ngram_filename_lr_arpa || lm->config->ngram_filename_rl_arpa || lm->config->ngram_filename) {
      if ((lm->ngram = initialize_ngram(lm->config, lm->winfo)) == NULL) {
	jlog("ERROR: m_fusion: failed to initialize N-gram\n");
	return FALSE;
      }
    }
  }
  if (lm->lmtype == LM_DFA) {
    /* DFA */
    if (lm->config->dfa_filename != NULL && lm->config->dictfilename != NULL) {
      /* here add grammar specified by "-dfa" and "-v" to grammar list */
      multigram_add_gramlist(lm->config->dfa_filename, lm->config->dictfilename, lm->config, LM_DFA_GRAMMAR);
    }
    /* load all the specified grammars */
    if (multigram_load_all_gramlist(lm) == FALSE) {
      jlog("ERROR: m_fusion: some error occured in reading grammars\n");
      return FALSE;
    }
    /* setup for later wchmm building */
    multigram_update(lm);
    /* the whole lexicon will be forced to built in the boot sequence,
       so reset the global modification flag here */
    lm->global_modified = FALSE;
  }
  
  jlog("STAT: *** LM%02d %s loaded\n", lmconf->id, lmconf->name);

  return TRUE;
}

/**********************************************************************/
/** 
 * <JA>
 * @brief  全てのモデルを読み込み，認識の準備を行なう. 
 *
 * この関数では，jconf 内にある（複数の） AM 設定パラメータ構造体やLM 
 * 設定パラメータ構造体のそれぞれに対して，AM/LM処理インスタンスを生成
 * する. そしてそれぞれのインスタンスについてその中にモデルを読み込み，
 * 認識用にセットアップする. GMMもここで読み込まれる. 
 * 
 * </JA>
 * <EN>
 * @brief  Read in all models for recognition.
 *
 * This function create AM/LM processing instance for each AM/LM
 * configurations in jconf.  Then the model for each instance will be loaded
 * into memory and set up for recognition.  GMM will also be read here.
 * 
 * </EN>
 *
 * @param recog [i/o] engine instance
 * @param jconf [in] global configuration variables
 *
 * @return TRUE on success, FALSE on failure.
 *
 * @callgraph
 * @callergraph
 * @ingroup instance
 */
boolean
j_load_all(Recog *recog, Jconf *jconf)
{
  JCONF_AM *amconf;
  JCONF_LM *lmconf;

  /* set global jconf */
  recog->jconf = jconf;

  /* load acoustic models */
  for(amconf=jconf->am_root;amconf;amconf=amconf->next) {
    if (j_load_am(recog, amconf) == FALSE) return FALSE;
  }

  /* load language models */
  for(lmconf=jconf->lm_root;lmconf;lmconf=lmconf->next) {
    if (j_load_lm(recog, lmconf) == FALSE) return FALSE;
  }

  /* GMM */
  if (jconf->reject.gmm_filename != NULL) {
    jlog("STAT: loading GMM\n");
    if ((recog->gmm = initialize_GMM(jconf)) == NULL) {
      jlog("ERROR: m_fusion: failed to initialize GMM\n");
      return FALSE;
    }
  }

  /* check sampling rate requirement on AMs and set it to global jconf */
  {
    boolean ok_p;

    /* set input sampling rate from an AM */
    jconf->input.sfreq = jconf->am_root->analysis.para.smp_freq;
    jconf->input.period = jconf->am_root->analysis.para.smp_period;
    jconf->input.frameshift = jconf->am_root->analysis.para.frameshift;
    jconf->input.framesize = jconf->am_root->analysis.para.framesize;
    /* check if the value is equal at all AMs */
    ok_p = TRUE;
    for(amconf = jconf->am_root; amconf; amconf = amconf->next) {
      if (jconf->input.sfreq != amconf->analysis.para.smp_freq) ok_p = FALSE;
    }
    if (!ok_p) {
      jlog("ERROR: required sampling rate differs in AMs!\n");
      for(amconf = jconf->am_root; amconf; amconf = amconf->next) {
	jlog("ERROR: AM%02d %s: %dHz\n", amconf->analysis.para.smp_freq);
      }
      return FALSE;
    }
    /* also check equality for GMM */
    if (recog->gmm) {
      if (jconf->input.sfreq != jconf->gmm->analysis.para.smp_freq) {
	jlog("ERROR: required sampling rate differs between AM and GMM!\n");
	jlog("ERROR: AM : %dHz\n", jconf->input.sfreq);
	jlog("ERROR: GMM: %dHz\n", jconf->gmm->analysis.para.smp_freq);
	return FALSE;
      }
    }
    for(amconf = jconf->am_root; amconf; amconf = amconf->next) {
      if (jconf->input.frameshift != amconf->analysis.para.frameshift) ok_p = FALSE;
    }
    if (!ok_p) {
      jlog("ERROR: requested frame shift differs in AMs!\n");
      for(amconf = jconf->am_root; amconf; amconf = amconf->next) {
	jlog("ERROR: AM%02d %s: %d samples\n", amconf->analysis.para.frameshift);
      }
      return FALSE;
    }
    /* also check equality for GMM */
    if (recog->gmm) {
      if (jconf->input.frameshift != jconf->gmm->analysis.para.frameshift) {
	jlog("ERROR: required frameshift differs between AM and GMM!\n");
	jlog("ERROR: AM : %d samples\n", jconf->input.frameshift);
	jlog("ERROR: GMM: %d samples\n", jconf->gmm->analysis.para.frameshift);
	return FALSE;
      }
    }
    for(amconf = jconf->am_root; amconf; amconf = amconf->next) {
      if (jconf->input.framesize != amconf->analysis.para.framesize) ok_p = FALSE;
    }
    if (!ok_p) {
      jlog("ERROR: requested frame size (window length) differs in AMs!\n");
      for(amconf = jconf->am_root; amconf; amconf = amconf->next) {
	jlog("ERROR: AM%02d %s: %d samples\n", amconf->analysis.para.framesize);
      }
      return FALSE;
    }
    /* also check equality for GMM */
    if (recog->gmm) {
      if (jconf->input.framesize != jconf->gmm->analysis.para.framesize) {
	jlog("ERROR: requested frame size differs between AM and GMM!\n");
	jlog("ERROR: AM : %d samples\n", jconf->input.framesize);
	jlog("ERROR: GMM: %d samples\n", jconf->gmm->analysis.para.framesize);
	return FALSE;
      }
    }
  }

  return TRUE;
}

/** 
 * <EN>
 * Check if parameter extraction configuration is the same between an AM
 * configuration and a MFCC instance.
 * </EN>
 * <JA>
 * AM設定パラメータと既に作られたMFCC計算インスタンス間で，パラメータ抽出の
 * 設定が同一であるかどうかをチェックする. 
 * 
 * </JA>
 * 
 * @param amconf [in] AM configuration parameters
 * @param mfcc [in] MFCC calculation instance.
 * 
 * @return TRUE if exactly the same, or FALSE if not.
 * 
 */
static boolean
mfcc_config_is_same(JCONF_AM *amconf, MFCCCalc *mfcc)
{
  char *s1, *s2;

  /* parameter extraction conditions are the same */
  /* check exact match in amconf->analysis.* */
  if (&(amconf->analysis.para) == mfcc->para || memcmp(&(amconf->analysis.para), mfcc->para, sizeof(Value)) == 0) {
    s1 = amconf->analysis.cmnload_filename;
    s2 = mfcc->cmn.load_filename;
    if (s1 == s2 || (s1 && s2 && strmatch(s1, s2))) {
      s1 = amconf->analysis.cmnsave_filename;
      s2 = mfcc->cmn.save_filename;
      if (s1 == s2 || (s1 && s2 && strmatch(s1, s2))) {
	if (amconf->analysis.cmn_update == mfcc->cmn.update
	    && amconf->analysis.cmn_map_weight == mfcc->cmn.map_weight) {
	  if (amconf->frontend.ss_alpha == mfcc->frontend.ss_alpha
	      && amconf->frontend.ss_floor == mfcc->frontend.ss_floor
	      && amconf->frontend.sscalc == mfcc->frontend.sscalc
	      && amconf->frontend.sscalc_len == mfcc->frontend.sscalc_len) {
	    s1 = amconf->frontend.ssload_filename;
	    s2 = mfcc->frontend.ssload_filename;
	    if (s1 == s2 || (s1 && s2 && strmatch(s1, s2))) {
	      return TRUE;
	    }
	  }
	}
      }
    }
  }

  return FALSE;
}

/***************************************************/
/* create MFCC calculation instance from AM config */
/* according to the fixated parameter information  */
/***************************************************/
/** 
 * <EN>
 * 
 * @brief  Create MFCC calculation instance for AM processing instances and GMM
 *
 * If more than one AM processing instance (or GMM) has the same configuration,
 * the same MFCC calculation instance will be shared among them.
 * 
 * </EN>
 * <JA>
 *
 * @brief  全てのAM処理インスタンスおよびGMM用に，MFCC計算インスタンスを生成する. 
 *
 * ２つ以上のAM処理インスタンス（およびGMM）が同一の特徴量計算条件を持
 * つ場合，それらのインスタンスはひとつの MFCC 計算インスタンスを共有する. 
 * 
 * </JA>
 * 
 * @param recog [i/o] engine instance
 *
 * @callgraph
 * @callergraph
 *
 */
void
create_mfcc_calc_instances(Recog *recog)
{
  PROCESS_AM *am;
  MFCCCalc *mfcc;
  int count;
  
  jlog("STAT: *** create MFCC calculation modules from AM\n");
  count = 0;
  for(am=recog->amlist;am;am=am->next) {
    for(mfcc=recog->mfcclist;mfcc;mfcc=mfcc->next) {
      if (mfcc_config_is_same(am->config, mfcc)) {
	/* the same */
	jlog("STAT: AM%02d %s: share MFCC%02d\n", am->config->id, am->config->name, mfcc->id);
	am->mfcc = mfcc;
	break;
      }
    }
    if (!mfcc) {		/* the same not found */
      /* initialize MFCC calculation work area */
      count++;
      /* create new mfcc instance */
      mfcc = j_mfcccalc_new(am->config);
      mfcc->id = count;
      /* assign to the am */
      am->mfcc = mfcc;
      /* add to the list of all MFCCCalc */
      mfcc->next = recog->mfcclist;
      recog->mfcclist = mfcc;
      jlog("STAT: AM%2d %s: create a new module MFCC%02d\n", am->config->id, am->config->name, mfcc->id);
    }
  }

  /* for GMM */
  if (recog->gmm) {
    /* if GMM calculation config found, make MFCC instance for that. */
    for(mfcc=recog->mfcclist;mfcc;mfcc=mfcc->next) {
      if (mfcc_config_is_same(recog->jconf->gmm, mfcc)) {
	/* the same */
	jlog("STAT: GMM: share MFCC%02d\n", mfcc->id);
	recog->gmmmfcc = mfcc;
	break;
	}
    }
    if (!mfcc) {		/* the same not found */
      /* initialize MFCC calculation work area */
      count++;
      /* create new mfcc instance */
      mfcc = j_mfcccalc_new(recog->jconf->gmm);
      mfcc->id = count;
      /* assign to gmm */
      recog->gmmmfcc = mfcc;
      /* add to the list of all MFCCCalc */
      mfcc->next = recog->mfcclist;
      recog->mfcclist = mfcc;
      jlog("STAT: GMM: create a new module MFCC%02d\n", mfcc->id);
    }
  }
  
  jlog("STAT: %d MFCC modules created\n", count);
}

/** 
 * <EN>
 * @brief  Launch a recognition process instance.
 *
 * This function will create an recognition process instance
 * using the given SEARCH configuration, and launch recognizer for
 * the search.  Then the created instance will be installed to the
 * engine instance.  The sconf should be registered to the global
 * jconf before calling this function.
 *
 * </EN>
 *
 * <JA>
 * @brief 認識処理インスタンスを立ち上げる．
 *
 * この関数は，与えられた SEARCH 設定に従って 認識処理インスタンスを生成し，
 * 対応する音声認識器を構築します．その後，その生成された認識処理インスタンスは
 * 新たにエンジンインスタンスに登録されます．SEARCH設定はこの関数を
 * 呼ぶ前にあらかじめ全体設定jconfに登録されている必要があります．
 * 
 * </JA>
 * 
 * @param recog [i/o] engine instance
 * @param sconf [in] SEARCH configuration to launch
 * 
 * @return TRUE on success, or FALSE on error.
 *
 * @callgraph
 * @callergraph
 * @ingroup instance
 * 
 */
boolean
j_launch_recognition_instance(Recog *recog, JCONF_SEARCH *sconf)
{
  RecogProcess *p;
  PROCESS_AM *am;
  PROCESS_LM *lm;

  jlog("STAT: composing recognizer instance SR%02d %s (AM%02d %s, LM%02d %s)\n", sconf->id, sconf->name, sconf->amconf->id, sconf->amconf->name, sconf->lmconf->id, sconf->lmconf->name);

  /* allocate recognition instance */
  p = j_recogprocess_new(recog, sconf);

  /* assign corresponding AM instance and LM instance to use */
  for(lm=recog->lmlist;lm;lm=lm->next) {
    if (sconf->lmconf == lm->config) {
      for(am=recog->amlist;am;am=am->next) {
	if (sconf->amconf == am->config) {
	  p->am = am;
	  p->lm = lm;
	}
      }
    }
  }

  if (p->config->sw.triphone_check_flag && p->am->hmminfo->is_triphone) {
    /* go into interactive triphone HMM check mode */
    hmm_check(p);
  }
  
  /******************************************/
  /******** set work area and flags *********/
  /******************************************/

  /* copy values of sub instances for handly access during recognition */
  /* set lm type */
  p->lmtype = p->lm->lmtype;
  p->lmvar  = p->lm->lmvar;
  p->graphout = p->config->graph.enabled;
  
  /* set flag for context dependent handling */
  if (p->config->force_ccd_handling) {
    p->ccd_flag = p->config->ccd_handling;
  } else {
    if (p->am->hmminfo->is_triphone) {
      p->ccd_flag = TRUE;
    } else {
      p->ccd_flag = FALSE;
    }
  }

  /* iwsp prepare */
  if (p->lm->config->enable_iwsp) {
    if (p->am->hmminfo->multipath) {
      /* find short-pause model */
      if (p->am->hmminfo->sp == NULL) {
	jlog("ERROR: iwsp enabled but no short pause model \"%s\" in hmmdefs\n", p->am->config->spmodel_name);
	return FALSE;
      }
      p->am->hmminfo->iwsp_penalty = p->am->config->iwsp_penalty;
    } else {
      jlog("ERROR: \"-iwsp\" needs multi-path mode\n");
      jlog("ERROR: you should use multi-path AM, or specify \"-multipath\" with \"-iwsp\"\n");
      return FALSE;
    }
  }

  /* for short-pause segmentation  */
  if (p->config->successive.enabled) {
    if (p->config->successive.pausemodelname) {
      /* pause model name string specified, divide it and store to p */
      char *s;
      int n;
      p->pass1.pausemodelnames = (char*)mymalloc(strlen(p->config->successive.pausemodelname)+1);
      strcpy(p->pass1.pausemodelnames, p->config->successive.pausemodelname);
      n = 0;
      for (s = strtok(p->pass1.pausemodelnames, " ,"); s; s = strtok(NULL, " ,")) {
	n++;
      }
      p->pass1.pausemodelnum = n;
      p->pass1.pausemodel = (char **)mymalloc(sizeof(char *) * n);
      strcpy(p->pass1.pausemodelnames, p->config->successive.pausemodelname);
      n = 0;
      for (s = strtok(p->pass1.pausemodelnames, " ,"); s; s = strtok(NULL, " ,")) {
	p->pass1.pausemodel[n++] = s;
      }
    } else {
      p->pass1.pausemodel = NULL;
    }
    /* check if pause word exists on dictionary */
    {
      WORD_ID w;
      boolean ok_p;
      ok_p = FALSE;
      for(w=0;w<p->lm->winfo->num;w++) {
	if (is_sil(w, p)) {
	  ok_p = TRUE;
	  break;
	}
      }
      if (!ok_p) {
#ifdef SPSEGMENT_NAIST
	jlog("Error: no pause word in dictionary needed for decoder-based VAD\n");
#else
	jlog("Error: no pause word in dictionary needed for short-pause segmentation\n");
#endif
	jlog("Error: you should have at least one pause word in dictionary\n");
	jlog("Error: you can specify pause model names by \"-pausemodels\"\n");
	return FALSE;
      }
    }
  }

  /**********************************************/
  /******** set model-specific defaults *********/
  /**********************************************/
  if (p->lmtype == LM_PROB) {
    /* set default lm parameter if not specified */
    if (!p->config->lmp.lmp_specified) {
      if (p->am->hmminfo->is_triphone) {
	p->config->lmp.lm_weight = DEFAULT_LM_WEIGHT_TRI_PASS1;
	p->config->lmp.lm_penalty = DEFAULT_LM_PENALTY_TRI_PASS1;
      } else {
	p->config->lmp.lm_weight = DEFAULT_LM_WEIGHT_MONO_PASS1;
	p->config->lmp.lm_penalty = DEFAULT_LM_PENALTY_MONO_PASS1;
      }
    }
    if (!p->config->lmp.lmp2_specified) {
      if (p->am->hmminfo->is_triphone) {
	p->config->lmp.lm_weight2 = DEFAULT_LM_WEIGHT_TRI_PASS2;
	p->config->lmp.lm_penalty2 = DEFAULT_LM_PENALTY_TRI_PASS2;
      } else {
	p->config->lmp.lm_weight2 = DEFAULT_LM_WEIGHT_MONO_PASS2;
	p->config->lmp.lm_penalty2 = DEFAULT_LM_PENALTY_MONO_PASS2;
      }
    }
    if (p->config->lmp.lmp_specified != p->config->lmp.lmp2_specified) {
      jlog("WARNING: m_fusion: only -lmp or -lmp2 specified, LM weights may be unbalanced\n");
    }
  }

  /****************************/
  /******* build wchmm ********/
  /****************************/
  if (p->lmtype == LM_DFA) {
    /* execute generation of global grammar and build of wchmm */
    multigram_build(p); /* some modification occured if return TRUE */
  }

  if (p->lmtype == LM_PROB) {
    /* build wchmm with N-gram */
    p->wchmm = wchmm_new();
    p->wchmm->lmtype = p->lmtype;
    p->wchmm->lmvar  = p->lmvar;
    p->wchmm->ccd_flag = p->ccd_flag;
    p->wchmm->category_tree = FALSE;
    p->wchmm->hmmwrk = &(p->am->hmmwrk);
    /* assign models */
    p->wchmm->ngram = p->lm->ngram;
    if (p->lmvar == LM_NGRAM_USER) {
      /* register LM functions for 1st pass here */
      p->wchmm->uni_prob_user = p->lm->lmfunc.uniprob;
      p->wchmm->bi_prob_user = p->lm->lmfunc.biprob;
    }
    p->wchmm->winfo = p->lm->winfo;
    p->wchmm->hmminfo = p->am->hmminfo;
    if (p->wchmm->category_tree) {
      if (p->config->pass1.old_tree_function_flag) {
	if (build_wchmm(p->wchmm, p->lm->config) == FALSE) {
	  jlog("ERROR: m_fusion: error in bulding wchmm\n");
	  return FALSE;
	}
      } else {
	if (build_wchmm2(p->wchmm, p->lm->config) == FALSE) {
	  jlog("ERROR: m_fusion: error in bulding wchmm\n");
	  return FALSE;
	}
      }
    } else {
      if (build_wchmm2(p->wchmm, p->lm->config) == FALSE) {
	jlog("ERROR: m_fusion: error in bulding wchmm\n");
	return FALSE;
      }
    }

    /* 起動時 -check でチェックモードへ */
    if (p->config->sw.wchmm_check_flag) {
      wchmm_check_interactive(p->wchmm);
    }

    /* set beam width */
    /* guess beam width from models, when not specified */
    p->trellis_beam_width = set_beam_width(p->wchmm, p->config->pass1.specified_trellis_beam_width);

    /* initialize cache for factoring */
    max_successor_cache_init(p->wchmm);
  }

  /* backtrellis initialization */
  p->backtrellis = (BACKTRELLIS *)mymalloc(sizeof(BACKTRELLIS));
  bt_init(p->backtrellis);

  /* prepare work area for 2nd pass */
  wchmm_fbs_prepare(p);

  jlog("STAT: SR%02d %s composed\n", sconf->id, sconf->name);

  if (sconf->sw.start_inactive) {
    /* start inactive */
    p->active = -1;
  } else {
    /* book activation for the recognition */
    p->active = 1;
  }
  if (p->lmtype == LM_DFA) {
    if (p->lm->winfo == NULL ||
	(p->lmvar == LM_DFA_GRAMMAR && p->lm->dfa == NULL)) {
      /* make this instance inactive */
      p->active = -1;
    }
  }

  return TRUE;
}


/** 
 * <EN>
 * @brief  Combine all loaded models and settings into one engine instance.
 *
 * This function will finalize preparation of recognition:
 * 
 *  - create required MFCC calculation instances,
 *  - create recognition process instance for specified LM/AM combination,
 *  - set model-specific recognition parameters,
 *  - build tree lexicon for each process instance for the 1st pass,
 *  - prepare work area and cache area for recognition,
 *  - initialize some values / work area for frontend processing.
 *
 * After this function, all recognition setup was done and we are ready for
 * start recognition.
 *
 * This should be called after j_jconf_finalize() and j_load_all() has been
 * completed.  You should put the jconf at recog->jconf before calling this
 * function.

 * </EN>
 * <JA>
 * @brief  全てのロードされたモデルと設定からエンジンインスタンスを
 * 最終構成する. 
 *
 * この関数は，認識準備のための最終処理を行う. 内部では，
 *
 *  - 必要な MFCC 計算インスタンスの生成
 *  - 指定された LM/AM の組からの認識処理インスタンス生成
 *  - モデルに依存する認識用パラメータの設定
 *  - 第1パス用の木構造化辞書を認識処理インスタンスごとに構築
 *  - 認識処理用ワークエリアとキャッシュエリアを確保
 *  - フロントエンド処理のためのいくつかの値とワークエリアの確保
 *
 *  を行う. この関数が終了後，エンジンインスタンス内の全てのセットアップ
 *  は終了し，認識が開始できる状態となる. 
 *
 *  この関数は，j_jconf_finalize() と j_load_all() が終わった状態で
 *  呼び出す必要がある. 呼出し前には，recog->jconf に (j_load_all でともに
 *  使用した) jconf を格納しておくこと. 
 * 
 * </JA>
 * 
 * @param recog [in] engine instance
 * 
 * @return TRUE when all initialization successfully done, or FALSE if any
 * error has been occured.
 *
 * @callgraph
 * @callergraph
 * @ingroup instance
 * 
 */
boolean
j_final_fusion(Recog *recog)
{
  MFCCCalc *mfcc;
  JCONF_SEARCH *sconf;
  PROCESS_AM *am;

  jlog("STAT: ------\n");
  jlog("STAT: All models are ready, go for final fusion\n");
  jlog("STAT: [1] create MFCC extraction instance(s)\n");
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    /***************************************************/
    /* create MFCC calculation instance from AM config */
    /* according to the fixated parameter information  */
    /***************************************************/
    create_mfcc_calc_instances(recog);
  }

  /****************************************/
  /* create recognition process instances */
  /****************************************/
  jlog("STAT: [2] create recognition processing instance(s) with AM and LM\n");
  for(sconf=recog->jconf->search_root;sconf;sconf=sconf->next) {
    if (j_launch_recognition_instance(recog, sconf) == FALSE) return FALSE;
  }

  /****************************/
  /****** initialize GMM ******/
  /****************************/
  if (recog->gmm != NULL) {
    jlog("STAT: [2.5] create GMM instance\n");
    if (gmm_init(recog) == FALSE) {
      jlog("ERROR: m_fusion: error in initializing GMM\n");
      return FALSE;
    }
  }

  /* stage 4: setup output probability function for each AM */
  jlog("STAT: [3] initialize for acoustic HMM calculation\n");
  for(am=recog->amlist;am;am=am->next) {
#ifdef ENABLE_PLUGIN
    /* set plugin function if specified */
    if (am->config->gprune_method == GPRUNE_SEL_USER) {
      am->hmmwrk.compute_gaussset = (void (*)(HMMWork *, HTK_HMM_Dens **, int, int *, int)) plugin_get_func(am->config->gprune_plugin_source, "calcmix");
      if (am->hmmwrk.compute_gaussset == NULL) {
	jlog("ERROR: calcmix plugin has no function \"calcmix\"\n");
	return FALSE;
      }
      am->hmmwrk.compute_gaussset_init = (boolean (*)(HMMWork *)) plugin_get_func(am->config->gprune_plugin_source, "calcmix_init");
      if (am->hmmwrk.compute_gaussset_init == NULL) {
	jlog("ERROR: calcmix plugin has no function \"calcmix_init\"\n");
	return FALSE;
      }
      am->hmmwrk.compute_gaussset_free = (void (*)(HMMWork *)) plugin_get_func(am->config->gprune_plugin_source, "calcmix_free");
      if (am->hmmwrk.compute_gaussset_free == NULL) {
	jlog("ERROR: calcmix plugin has no function \"calcmix_free\"\n");
	return FALSE;
      }
    }
#endif
    if (am->config->hmm_gs_filename != NULL) {/* with GMS */
      if (outprob_init(&(am->hmmwrk), am->hmminfo, am->hmm_gs, am->config->gs_statenum, am->config->gprune_method, am->config->mixnum_thres) == FALSE) {
	return FALSE;
      }
    } else {
      if (outprob_init(&(am->hmmwrk), am->hmminfo, NULL, 0, am->config->gprune_method, am->config->mixnum_thres) == FALSE) {
	return FALSE;
      }
    }
    /* when "-outprobout" is specified, ask the state computation
       module to force calculatation of ALL the states at each
       frame */
    outprob_set_batch_computation(&(am->hmmwrk), (recog->jconf->outprob_outfile != NULL) ? TRUE : FALSE);

  }

  /* stage 5: initialize work area for input and realtime decoding */

  jlog("STAT: [4] prepare MFCC storage(s)\n");
  if (recog->jconf->input.type == INPUT_VECTOR) {
    /* create an MFCC instance for MFCC input */
    /* create new mfcc instance */
    recog->mfcclist = j_mfcccalc_new(NULL);
    recog->mfcclist->id = 1;
    /* assign to the am */
    for(am=recog->amlist;am;am=am->next) {
      am->mfcc = recog->mfcclist;
    }
    if (recog->gmm) recog->gmmmfcc = recog->mfcclist;
  }
  /* allocate parameter holders */
  for(mfcc=recog->mfcclist;mfcc;mfcc=mfcc->next) {
    mfcc->param = new_param();
  }
  
  /* initialize SS calculation work area */
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    for(mfcc=recog->mfcclist;mfcc;mfcc=mfcc->next) {
      if (mfcc->frontend.sscalc) {
	mfcc->frontend.mfccwrk_ss = WMP_work_new(mfcc->para);
	if (mfcc->frontend.mfccwrk_ss == NULL) {
	  jlog("ERROR: m_fusion: failed to initialize MFCC computation for SS\n");
	  return FALSE;
	}
	if (mfcc->frontend.sscalc_len * recog->jconf->input.sfreq / 1000 < mfcc->para->framesize) {
	  jlog("ERROR: m_fusion: head sil length for SS (%d msec) is shorter than a frame (%d msec)\n", mfcc->frontend.sscalc_len, mfcc->para->framesize * 1000 / recog->jconf->input.sfreq);
	  return FALSE;
	}
      }
    }
  }

  if (recog->jconf->decodeopt.realtime_flag) {
    jlog("STAT: [5] prepare for real-time decoding\n");
    /* prepare for 1st pass pipeline processing */
    if (recog->jconf->input.type == INPUT_WAVEFORM) {
      if (RealTimeInit(recog) == FALSE) {
	jlog("ERROR: m_fusion: failed to initialize recognition process\n");
	return FALSE;
      }
    }
  }

  /* initialize CMN and CVN calculation work area for batch computation */
  if (! recog->jconf->decodeopt.realtime_flag) {
    if (recog->jconf->input.type == INPUT_WAVEFORM) {
      for(mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
	if (mfcc->cmn.load_filename) {
	  if (mfcc->para->cmn || mfcc->para->cvn) {
	    mfcc->cmn.wrk = CMN_realtime_new(mfcc->para, mfcc->cmn.map_weight);
	    if ((mfcc->cmn.loaded = CMN_load_from_file(mfcc->cmn.wrk, mfcc->cmn.load_filename))== FALSE) {
	      jlog("ERROR: m_fusion: failed to read initial cepstral mean from \"%s\"\n", mfcc->cmn.load_filename);
	      return FALSE;
	    }
	  } else {
	    jlog("WARNING: m_fusion: CMN load file specified but AM not require it, ignored\n");
	  }
	}
      }
    }
  }

  /* finished! */
  jlog("STAT: All init successfully done\n\n");

  /* set-up callback plugin if any */
#ifdef ENABLE_PLUGIN
  if (plugin_exec_engine_startup(recog) == FALSE) {
    jlog("ERROR: m_fusion: failed to execute callback setup in plugin\n");
    return FALSE;
  }
#endif

  return TRUE;
}

/** 
 * <EN>
 * @brief  Reload dictionaries.
 *
 * This function reload dictionaries and re-create all the recognition
 * process.
 *
 * @param recog [i/o] engine instance
 * @param lm [i/o] LM instance to reload
 * 
 * @return TRUE on success, or FALSE on error.
 * 
 */
boolean
j_reload_adddict(Recog *recog, PROCESS_LM *lm)
{
  RecogProcess *p, *ptmp;
  JCONF_SEARCH *sh;
  PROCESS_AM *am, *atmp;
  
  jlog("STAT: *** reloading (additional) dictionary of LM%02d %s\n", lm->config->id, lm->config->name);

  /* free current dictionary */
  if (lm->winfo) word_info_free(lm->winfo);
  if (lm->grammars) multigram_free_all(lm->grammars);
  if (lm->dfa) dfa_info_free(lm->dfa);

  /* free all current process instanfces */
  p = recog->process_list;
  while(p) {
    ptmp = p->next;
    j_recogprocess_free(p);
    p = ptmp;
  }
  recog->process_list = NULL;

  /* reload dictionary */
  if (lm->lmtype == LM_PROB) {

    if ((lm->winfo = initialize_dict(lm->config, lm->am->hmminfo)) == NULL) {
      jlog("ERROR: m_fusion: failed to reload dictionary\n");
      return FALSE;
    }
    if (lm->config->ngram_filename_lr_arpa || lm->config->ngram_filename_rl_arpa || lm->config->ngram_filename) {
      /* re-map dict item to N-gram entry */
      if (make_voca_ref(lm->ngram, lm->winfo) == FALSE) {
	jlog("ERROR: m_fusion: failed to map words in additional dictionary to N-gram\n");
	return FALSE;
      }
    }
  }
  if (lm->lmtype == LM_DFA) {
    /* DFA */
    if (lm->config->dfa_filename != NULL && lm->config->dictfilename != NULL) {
      /* here add grammar specified by "-dfa" and "-v" to grammar list */
      multigram_add_gramlist(lm->config->dfa_filename, lm->config->dictfilename, lm->config, LM_DFA_GRAMMAR);
    }
    /* load all the specified grammars */
    if (multigram_load_all_gramlist(lm) == FALSE) {
      jlog("ERROR: m_fusion: some error occured in reading grammars\n");
      return FALSE;
    }
    /* setup for later wchmm building */
    multigram_update(lm);
    /* the whole lexicon will be forced to built in the boot sequence,
       so reset the global modification flag here */
    lm->global_modified = FALSE;
  }

  /* re-create all recognition process instance */
  for(sh=recog->jconf->search_root;sh;sh=sh->next) {
    if (j_launch_recognition_instance(recog, sh) == FALSE) {
      jlog("ERROR: m_fusion: failed to re-start recognizer instance \"%s\"\n", sh->name);
      return FALSE;
    }
  }

  /* the created process will be live=FALSE, active = 1, so
     the new recognition instance is dead now but
     will be made live at next session */

  /* tell engine to update */
  recog->process_want_reload = TRUE;

  jlog("STAT: *** LM%02d %s additional dictionary reloaded\n", lm->config->id, lm->config->name);

  return TRUE;
}

/* end of file */
