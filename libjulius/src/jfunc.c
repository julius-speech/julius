/**
 * @file   jfunc.c
 * 
 * <JA>
 * @brief  アプリケーション向けの種々のAPI関数
 *
 * このファイルには，アプリケーションからJuliusLibの各機能を呼び出す
 * API関数およびライブラリ化のために実装された種々の関数が定義されています. 
 * 
 * </JA>
 * 
 * <EN>
 * @brief  API functions for applications
 *
 * This file contains for API function definitions and miscellaneous
 * functions implemented for JuliusLib.
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Wed Aug  8 15:04:28 2007
 *
 * $Revision: 1.12 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius/juliuslib.h>

/** 
 * <EN>
 * Request engine to stop recognition.  If the engine is recognizing a
 * speech input, it will stop after the current recognition ended.
 * </EN>
 * <JA>
 * エンジンに認識処理を一時停止するよう要求する. この関数を呼出し時に
 * 音声入力を実行中であった場合，その入力の認識が終了したあとで停止する. 
 * </JA>
 * 
 * @param recog [in] engine instance
 *
 * @callgraph
 * @callergraph
 * @ingroup pauseresume
 * 
 */
void
j_request_pause(Recog *recog)
{
  /* pause recognition: will stop when the current input ends */
  if (recog->process_active) {
    recog->process_want_terminate = FALSE;
    recog->process_want_reload = TRUE;
    recog->process_active = FALSE;
  }
  /* control the A/D-in module to stop recording */
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    if (recog->adin->ad_pause != NULL) {
      (*(recog->adin->ad_pause))();
    }
  } else {
    /* feature vector input */
    if (recog->jconf->input.speech_input == SP_MFCMODULE) {
      if (recog->mfcclist->func.fv_pause) recog->mfcclist->func.fv_pause();
    }
  }
}

/** 
 * <EN>
 * Request engine to terminate recognition immediately.  Even if the engine
 * is recognizing a speech input, it will stop immediately (in this case the
 * current input will be lost).
 * </EN>
 * <JA>
 * エンジンに認識処理を即時停止するよう要求する. この関数を呼出し時に
 * 音声入力を実行中の場合，その入力を破棄して即座に停止する. 
 * </JA>
 * 
 * @param recog [in] engine instance
 * 
 * @callgraph
 * @callergraph
 * @ingroup pauseresume
 */
void
j_request_terminate(Recog *recog)
{
  /* terminate recognition: input will terminate immidiately */
  /* set flags to stop adin to terminate immediately, and
     stop process */
  if (recog->process_active) {
    recog->process_want_terminate = TRUE;
    recog->process_want_reload = TRUE;
    recog->process_active = FALSE;
  }
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    if (recog->adin->ad_terminate != NULL) {
      /* control the A/D-in module to terminate recording imemdiately */
      (*(recog->adin->ad_terminate))();
    }
  } else {
    /* feature vector input */
    if (recog->jconf->input.speech_input == SP_MFCMODULE) {
      if (recog->mfcclist->func.fv_terminate) recog->mfcclist->func.fv_terminate();
    }
  }
}

/** 
 * <EN>
 * Resume the engine which has already paused or terminated.
 * </EN>
 * <JA>
 * 一時停止しているエンジンを再開させる. 
 * </JA>
 * 
 * @param recog 
 * 
 * @callgraph
 * @callergraph
 * @ingroup pauseresume
 */
void 
j_request_resume(Recog *recog)
{
  if (recog->process_active == FALSE) {
    recog->process_want_terminate = FALSE;
    recog->process_active = TRUE;
  }
  /* control the A/D-in module to restart recording now */
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    if (recog->adin->ad_resume != NULL) {
      (*(recog->adin->ad_resume))();
    }
  } else {
    /* feature vector input */
    if (recog->jconf->input.speech_input == SP_MFCMODULE) {
      if (recog->mfcclist->func.fv_resume) recog->mfcclist->func.fv_resume();
    }
  }
}

/** 
 * <EN> Request engine to check update of all grammar and re-construct
 * the glocal lexicon if needed.  The actual update will be done
 * between input segment.  This function should be called after some
 * grammars are modified.
 * 
 * </EN>
 * <JA>
 * 全文法の変更をチェックし，必要であれば認識用辞書を再構築するよう
 * エンジンに要求する. 実際の処理は次の認識の合間に行われる. 
 * この関数は文法を追加したり削除したなど，
 * 文法リストに変更を加えたあとに必ず呼ぶべきである. 
 * 
 * </JA>
 * 
 * @param recog [in] engine instance
 * 
 * @callgraph
 * @callergraph
 * @ingroup grammar
 */
void
schedule_grammar_update(Recog *recog)
{
  if (recog->process_active) {
    /* if recognition is currently running, tell engine how/when to
       re-construct global lexicon. */
    switch(recog->gram_switch_input_method) {
    case SM_TERMINATE:	/* discard input now and change (immediate) */
      recog->process_want_terminate = TRUE;
      recog->process_want_reload = TRUE;
      break;
    case SM_PAUSE:		/* segment input now, recognize it, and then change */
      recog->process_want_terminate = FALSE;
      recog->process_want_reload = TRUE;
      break;
    case SM_WAIT:		/* wait until the current input end and recognition completed */
      recog->process_want_terminate = FALSE;
      recog->process_want_reload = FALSE;
      break;
    }
    /* After the update, recognition will restart without sleeping. */
  } else {
    /* If recognition is currently not running, the received
       grammars are merely stored in memory here.  The re-construction of
       global lexicon will be delayed: it will be re-built just before
       the recognition process starts next time. */
  }
}

/** 
 * <JA>
 * 再構築要求フラグをクリアする. 
 * 
 * </JA>
 * <EN>
 * Clear the grammar re-construction flag.
 * 
 * </EN>
 *
 * @param recog [in] engine instance
 * 
 * @callgraph
 * @callergraph
 * @ingroup grammar
 */
void
j_reset_reload(Recog *recog)
{
  recog->process_want_reload = FALSE;
}

/** 
 * <EN>
 * Enable debug messages in JuliusLib to log.
 * </EN>
 * <JA>
 * JuliusLib内の関数でデバッグメッセージをログに出力するようにする
 * </JA>
 * @callgraph
 * @callergraph
 * @ingroup engine
 */
void
j_enable_debug_message()
{
  debug2_flag = TRUE;
}

/** 
 * <EN>
 * Disable debug messages in JuliusLib to log.
 * </EN>
 * <JA>
 * JuliusLib内の関数でデバッグメッセージを出さないようにする. 
 * </JA>
 * 
 * @callgraph
 * @callergraph
 * @ingroup engine
 */
void
j_disable_debug_message()
{
  debug2_flag = FALSE;
}

/** 
 * <EN>
 * Enable verbose messages in JuliusLib to log.
 * </EN>
 * <JA>
 * JuliusLib内の関数で主要メッセージをログに出力するようにする. 
 * </JA>
 * 
 * @callgraph
 * @callergraph
 * @ingroup engine
 */
void
j_enable_verbose_message()
{
  verbose_flag = TRUE;
}

/** 
 * <EN>
 * Disable verbose messages in JuliusLib to log.
 * </EN>
 * <JA>
 * JuliusLib内の関数で主要メッセージのログ出力をしないようにする. 
 * </JA>
 * 
 * @callgraph
 * @callergraph
 * @ingroup engine
 */
void
j_disable_verbose_message()
{
  verbose_flag = FALSE;
}
	      

/** 
 * Output error message and exit the program.  This is just for
 * internal use.
 * 
 * @param fmt [in] format string, like printf.
 * @param ... [in] variable length argument like printf.
 * 
 */
void
j_internal_error(char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start(ap,fmt);
  ret = vfprintf(stderr, fmt, ap);
  va_end(ap);

  /* clean up socket if already opened */
  cleanup_socket();

  exit(1);
}

/** 
 * <EN>
 * If multiple instances defined from init, remove initial one (id=0)
 * </EN>
 * <JA>
 * 複数インスタンスが定義されている場合、初期インスタンス(id=0)は
 * 無効なので消す. 
 * </JA>
 * 
 * @param jconf [i/o] global configuration instance
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
static void
j_config_remove_initial(Jconf *jconf)
{
  JCONF_AM *am;
  JCONF_LM *lm;
  JCONF_SEARCH *s;

  if(jconf->am_root->next != NULL && jconf->am_root->id == 0) {
    am = jconf->am_root->next;
    free(jconf->am_root);
    jconf->am_root = am;
  }
  if(jconf->lm_root->next != NULL && jconf->lm_root->id == 0) {
    lm = jconf->lm_root->next;
    free(jconf->lm_root);
    jconf->lm_root = lm;
  }
  if(jconf->search_root->next != NULL && jconf->search_root->id == 0) {
    s = jconf->search_root->next;
    free(jconf->search_root);
    jconf->search_root = s;
  }
}

/** 
 * <EN>
 * Load parameters from command argments, and set to each configuration
 * instances in jconf.
 * </EN>
 * <JA>
 * コマンド引数からパラメータを読み込み，jconf 内の各設定インスタンスに
 * 値を格納する. 
 * </JA>
 * 
 * @param jconf [i/o] global configuration instance
 * @param argc [in] number of arguments
 * @param argv [in] list of argument strings
 * 
 * @return 0 on success, or -1 on failure.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
int
j_config_load_args(Jconf *jconf, int argc, char *argv[])
{
  /* parse options and set variables */
  if (opt_parse(argc, argv, NULL, jconf) == FALSE) {
    return -1;
  }
  /* if multiple instances defined from init, remove initial one (id=0) */
  j_config_remove_initial(jconf);

  return 0;
}

/** 
 * <EN>
 * Load parameters from command argment string, and set to each configuration
 * instances in jconf.
 * </EN>
 * <JA>
 * コマンド引数を含む文字列からパラメータを読み込み，jconf 内の各設定インスタンスに
 * 値を格納する. 
 * </JA>
 * 
 * @param jconf [i/o] global configuration instance
 * @param argstr [in] argument string
 * 
 * @return 0 on success, or -1 on failure.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
int
j_config_load_string(Jconf *jconf, char *string)
{
  int argc;
  char **argv;
  char *buf;
  
  /* parse options and set variables */
  if (config_string_parse(string, jconf) == FALSE) {
    return -1;
  }
  /* if multiple instances defined from init, remove initial one (id=0) */
  j_config_remove_initial(jconf);

  return 0;
}

/** 
 * <EN>
 * Load parameters from a jconf file and set to each configuration
 * instances in jconf.
 * </EN>
 * <JA>
 * jconf ファイルからパラメータを読み込み，jconf 内の各設定インスタンスに
 * 値を格納する. 
 * </JA>
 * 
 * @param jconf [i/o] glbal configuration instance
 * @param filename [in] jconf filename
 * 
 * @return 0 on sucess, or -1 on failure.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
int
j_config_load_file(Jconf *jconf, char *filename)
{
  /* parse options and set variables */
  if (config_file_parse(filename, jconf) == FALSE) {
    return -1;
  }
  /* if multiple instances defined from init, remove initial one (id=0) */
  j_config_remove_initial(jconf);

  return 0;
}

/** 
 * <EN>
 * Create a new configuration instance and load parameters from command
 * argments.
 * </EN>
 * <JA>
 * コマンド引数からパラメータを読み込み，その値を格納した
 * 新たな設定インスタンスを割り付けて返す. 
 * </JA>
 * 
 * @param argc [in] number of arguments
 * @param argv [in] list of argument strings
 * 
 * @return the newly allocated global configuration instance.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
Jconf *
j_config_load_args_new(int argc, char *argv[])
{
  Jconf *jconf;
  jconf = j_jconf_new();
  if (j_config_load_args(jconf, argc, argv) == -1) {
    j_jconf_free(jconf);
    return NULL;
  }
  return jconf;
}

/** 
 * <EN>
 * Create a new configuration instance and load parameters from a jconf
 * file.
 * </EN>
 * <JA>
 * 新たな設定インスタンスを割り付け，そこに
 * jconfファイルから設定パラメータを読み込んで返す. 
 * </JA>
 * 
 * @param filename [in] jconf filename
 * 
 * @return the newly allocated global configuration instance.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
Jconf *
j_config_load_file_new(char *filename)
{
  Jconf *jconf;
  jconf = j_jconf_new();
  if (j_config_load_file(jconf, filename) == -1) {
    j_jconf_free(jconf);
    return NULL;
  }
  return jconf;
}

/** 
 * <EN>
 * Create a new configuration instance and load parameters from string
 * file.
 * </EN>
 * <JA>
 * 新たな設定インスタンスを割り付け，そこに
 * 文字列から設定パラメータを読み込んで返す. 
 * </JA>
 * 
 * @param string [in] option string
 * 
 * @return the newly allocated global configuration instance.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
Jconf *
j_config_load_string_new(char *string)
{
  Jconf *jconf;
  jconf = j_jconf_new();
  if (j_config_load_string(jconf, string) == -1) {
    j_jconf_free(jconf);
    return NULL;
  }
  return jconf;
}

/** 
 * <EN>
 * Book to read an additional dictionary file to be read.
 * when called multiple times, all the file name will be stored and read.
 * The file will be read just after the normal dictionary at startup.
 * </EN>
 * <JA>
 * 追加辞書ファイルの読み込みを指定する.
 * 複数回呼ばれた場合、すべて読み込まれる。
 * 指定された辞書は起動時に通常の辞書のあとに続けて読み込まれる.
 * </JA>
 *
 * @param lm [i/o] a LM configuration
 * @param dictfile [in] dictinoary file name
 * 
 * @return the newly allocated global configuration instance.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
void
j_add_dict(JCONF_LM *lm, char *dictfile)
{
  JCONF_LM_NAMELIST *nl;
  nl = (JCONF_LM_NAMELIST *)mymalloc(sizeof(JCONF_LM_NAMELIST));
  nl->name = (char *)mymalloc(strlen(dictfile) + 1);
  strcpy(nl->name, dictfile);
  nl->next = lm->additional_dict_files;
  lm->additional_dict_files = nl;
}

/** 
 * <EN>
 * Add an additional word entry.
 * The string should contain a word entry in as the same format as dictionary.
 * If called multiple times, all the specified words will be appended.
 * </EN>
 * <JA>
 * 追加の単語エントリを指定する.
 * 内容は辞書ファイルと同じフォーマット.
 * 起動までに複数回呼ばれた場合、そのすべてが起動時に追加される.
 * </JA>
 *
 * @param lm [i/o] a LM configuration
 * @param wordentry [in] word entry string in dictionary format
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
void
j_add_word(JCONF_LM *lm, char *wordentry)
{
  JCONF_LM_NAMELIST *nl;
  nl = (JCONF_LM_NAMELIST *)mymalloc(sizeof(JCONF_LM_NAMELIST));
  nl->name = (char *)mymalloc(strlen(wordentry) + 1);
  strcpy(nl->name, wordentry);
  nl->next = lm->additional_dict_entries;
  lm->additional_dict_entries = nl;
}

/** 
 * <EN>
 * Initialize and setup A/D-in device specified by the configuration
 * for recognition.  When threading is enabled for the device,
 * A/D-in thread will start inside this function.
 * </EN>
 * <JA>
 * 設定で選択された A/D-in デバイスを初期化し認識の準備を行う. 
 * そのデバイスに対して threading が指定されている場合は，
 * A/D-in 用スレッドがここで開始される. 
 * </JA>
 * 
 * @param recog [in] engine instance
 * 
 * @return TRUE on success, FALSE on failure.
 * 
 * @callgraph
 * @callergraph
 * @ingroup engine
 */
boolean
j_adin_init(Recog *recog)
{
  boolean ret;

  if (recog->jconf->input.type == INPUT_VECTOR) {
    /* feature vector input */
    if (recog->jconf->input.speech_input == SP_MFCMODULE) {
      if (mfc_module_init(recog->mfcclist, recog) == FALSE) {
	return FALSE;
      }
      ret = mfc_module_standby(recog->mfcclist);
    } else {
      ret = TRUE;
    }
    return ret;
  }
  
  /* initialize A/D-in device */
  ret = adin_initialize(recog);

  return(ret);
}

/** 
 * <EN>
 * Return current input speech file name.  return NULL if the current
 * input device does not support this function.
 * </EN>
 * <JA>
 * 現在の入力ファイル名を返す.現在の入力デバイスがこの機能をサポート
 * していない場合は NULL を返す．
 * </JA>
 * 
 * @param recog [in] engine instance
 * 
 * @return the file name, or NULL when this function is not available on
 * the current input device.
 * 
 * @callgraph
 * @callergraph
 * @ingroup engine
 */
char *
j_get_current_filename(Recog *recog)
{
  char *p;
  p = NULL;
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    /* adin function input */
    if (recog->adin->ad_input_name != NULL) {
      p = recog->adin->ad_input_name();
    }
  } else {
    switch(recog->jconf->input.speech_input) {
    case SP_MFCMODULE:
      p = mfc_module_input_name(recog->mfcclist);
      break;
    case SP_MFCFILE:
    case SP_OUTPROBFILE:
      /* already assigned */
      p = recog->adin->current_input_name;
      break;
    }
  }
  return p;
}


/** 
 * <EN>
 * Output all configurations and system informations into log.
 * </EN>
 * <JA>
 * エンジンの全設定と全システム情報をログに出力する. 
 * </JA>
 * 
 * @param recog [in] engine instance
 * 
 * @callgraph
 * @callergraph
 * @ingroup engine
 */
void
j_recog_info(Recog *recog)
{
  /* print out system information */
  print_engine_info(recog);
}

/** 
 * <EN>
 * @brief  Instanciate / generate a new engine instance according
 * to the given global configuration instance.
 *
 * It inspects all parameters in the global configuration instance, load
 * all models into memory, build tree lexicons, allocate work area and
 * caches.  It does all setup to start recognition except A/D-in
 * initialization.
 * </EN>
 *
 * <JA>
 * @brief  与えられた設定インスタンス内の情報に従って，新たな
 * エンジンインスタンスを 起動・生成する. 
 * 
 * 設定インスタンス内のパラメータのチェック後，モデルを読み込み，木構
 * 造化辞書の生成，ワークエリアおよびキャッシュの確保などを行う. 
 * A/D-in の初期化以外で認識を開始するのに必要な処理をすべて行う. 
 * </JA>
 * 
 * @param jconf [in] gloabl configuration instance
 * 
 * @return the newly created engine instance.
 * 
 * @callgraph
 * @callergraph
 * @ingroup instance
 */
Recog *
j_create_instance_from_jconf(Jconf *jconf)
{
  Recog *recog;

  /* check option values and set parameters needed for model loading */
  if (j_jconf_finalize(jconf) == FALSE) {
    return NULL;
  }

  /* create a recognition instance */
  recog = j_recog_new();

  /* assign configuration to the instance */
  recog->jconf = jconf;

  /* load all files according to the configurations */
  if (j_load_all(recog, jconf) == FALSE) {
    jlog("ERROR: j_create_instance_from_jconf: error in loading model\n");
    /* j_model_free(model); */
    return NULL;
  }

  /* checkout for recognition: build lexicon tree, allocate cache */
  if (j_final_fusion(recog) == FALSE) {
    jlog("ERROR: j_create_instance_from_jconf: error while setup for recognition\n");
    j_recog_free(recog);
    return NULL;
  }

  return recog;
}

/** 
 * <EN>
 * Assign user-defined language scoring functions into a LM processing
 * instance.  This should be called after engine instance creation and
 * before j_final_fusion() is called.  Remember that you should also
 * specify "-userlm" option at jconf to use user-define language scoring.
 * </EN>
 * <JA>
 * 言語モデル処理インスタンスにユーザ定義の言語スコア付与関数を登録する. 
 * この関数はエンジンインスタンス生成後から j_final_fusion() が呼ばれる
 * までの間に呼ぶ必要がある. 注意：ユーザ定義の言語スコア関数を使う場合は
 * 実行時オプション "-userlm" も指定する必要があることに注意せよ. 
 * </JA>
 * 
 * @param lm [i/o] LM processing instance
 * @param unifunc [in] pointer to the user-defined unigram function
 * @param bifunc [in] pointer to the user-defined bi-igram function
 * @param probfunc [in] pointer to the user-defined N-gram function
 * 
 * @return TRUE on success, FALSE on failure.
 * 
 * @callgraph
 * @callergraph
 * @ingroup userfunc
 */
boolean
j_regist_user_lm_func(PROCESS_LM *lm, 
	  LOGPROB (*unifunc)(WORD_INFO *winfo, WORD_ID w, LOGPROB ngram_prob), 
	  LOGPROB (*bifunc)(WORD_INFO *winfo, WORD_ID context, WORD_ID w, LOGPROB ngram_prob),
	  LOGPROB (*probfunc)(WORD_INFO *winfo, WORD_ID *contexts, int context_len, WORD_ID w, LOGPROB ngram_prob))
{
  lm->lmfunc.uniprob = unifunc;
  lm->lmfunc.biprob = bifunc;
  lm->lmfunc.lmprob = probfunc;
  return TRUE;
}

/** 
 * <EN>
 * Assign a user-defined parameter extraction function to engine instance.
 * </EN>
 * <JA>
 * ユーザ定義の特徴量計算関数を使うようエンジンに登録する. 
 * </JA>
 * 
 * @param recog [i/o] engine instance
 * @param user_calc_vector [in] pointer to function of parameter extraction
 * 
 * @return TRUE on success, FALSE on error.
 * 
 * @callgraph
 * @callergraph
 * @ingroup userfunc
 */
boolean
j_regist_user_param_func(Recog *recog, boolean (*user_calc_vector)(MFCCCalc *, SP16 *, int))
{
  recog->calc_vector = user_calc_vector;
  return TRUE;
}


/** 
 * <EN>
 * Get AM configuration structure in jconf by its name.
 * </EN>
 * <JA>
 * jconf内の AM モジュール設定構造体を名前で検索する. 
 * </JA>
 * 
 * @param jconf [in] global configuration
 * @param name [in] AM module name
 * 
 * @return the specified AM configuration, or NULL if not found.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
JCONF_AM *
j_get_amconf_by_name(Jconf *jconf, char *name)
{
  JCONF_AM *amconf;

  for(amconf=jconf->am_root;amconf;amconf=amconf->next) {
    if (strmatch(amconf->name, name)) {
      break;
    }
  }
  if (!amconf) {		/* error */
    jlog("ERROR: j_get_amconf_by_name: [AM \"%s\"] not found\n", name);
    return NULL;
  }
  return amconf;
}

/** 
 * <EN>
 * Get AM configuration structure in jconf by its id.
 * </EN>
 * <JA>
 * jconf内の AM モジュール設定構造体を ID で検索する. 
 * </JA>
 * 
 * @param jconf [in] global configuration
 * @param id [in] AM module ID
 * 
 * @return the specified AM configuration, or NULL if not found.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
JCONF_AM *
j_get_amconf_by_id(Jconf *jconf, int id)
{
  JCONF_AM *amconf;

  for(amconf=jconf->am_root;amconf;amconf=amconf->next) {
    if (amconf->id == id) {
      break;
    }
  }
  if (!amconf) {		/* error */
    jlog("ERROR: j_get_amconf_by_id: [AM%02d] not found\n", id);
    return NULL;
  }
  return amconf;
}

/** 
 * <EN>
 * Return default AM configuration.
 *
 * If multiple AM configuration exists, return the latest one.
 * </EN>
 * <JA>
 * デフォルトの AM 設定を返す. 
 *
 * AMが複数設定されている場合，最も最近のものを返す. 
 * </JA>
 * 
 * @param jconf [in] global configuration
 * 
 * @return the specified AM configuration, or NULL if not found.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
JCONF_AM *
j_get_amconf_default(Jconf *jconf)
{
  JCONF_AM *amconf;

  if (jconf->am_root == NULL) return NULL;
  for(amconf=jconf->am_root;amconf->next;amconf=amconf->next);
  return(amconf);
}

/** 
 * <EN>
 * Get LM configuration structure in jconf by its name.
 * </EN>
 * <JA>
 * jconf内の LM モジュール設定構造体を名前で検索する. 
 * </JA>
 * 
 * @param jconf [in] global configuration
 * @param name [in] LM module name
 * 
 * @return the specified LM configuration, or NULL if not found.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
JCONF_LM *
j_get_lmconf_by_name(Jconf *jconf, char *name)
{
  JCONF_LM *lmconf;

  for(lmconf=jconf->lm_root;lmconf;lmconf=lmconf->next) {
    if (strmatch(lmconf->name, name)) {
      break;
    }
  }
  if (!lmconf) {		/* error */
    jlog("ERROR: j_get_lmconf_by_name: [LM \"%s\"] not found\n", name);
    return NULL;
  }
  return lmconf;
}

/** 
 * <EN>
 * Get LM configuration structure in jconf by its id.
 * </EN>
 * <JA>
 * jconf内の LM モジュール設定構造体を ID で検索する. 
 * </JA>
 * 
 * @param jconf [in] global configuration
 * @param id [in] LM module ID
 * 
 * @return the specified LM configuration, or NULL if not found.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
JCONF_LM *
j_get_lmconf_by_id(Jconf *jconf, int id)
{
  JCONF_LM *lmconf;

  for(lmconf=jconf->lm_root;lmconf;lmconf=lmconf->next) {
    if (lmconf->id == id) {
      break;
    }
  }
  if (!lmconf) {		/* error */
    jlog("ERROR: j_get_lmconf_by_id: [LM%02d] not found\n", id);
    return NULL;
  }
  return lmconf;
}

/** 
 * <EN>
 * Get SEARCH configuration structure in jconf by its name.
 * </EN>
 * <JA>
 * jconf内の SESARCH モジュール設定構造体を名前で検索する. 
 * </JA>
 * 
 * @param jconf [in] global configuration
 * @param name [in] SEARCH module name
 * 
 * @return the found SEARCH configuration, or NULL if not found.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
JCONF_SEARCH *
j_get_searchconf_by_name(Jconf *jconf, char *name)
{
  JCONF_SEARCH *sconf;

  for(sconf=jconf->search_root;sconf;sconf=sconf->next) {
    if (strmatch(sconf->name, name)) {
      break;
    }
  }
  if (!sconf) {		/* error */
    jlog("ERROR: j_get_searchconf_by_name: [SR \"%s\"] not found\n", name);
    return NULL;
  }
  return sconf;
}

/** 
 * <EN>
 * Get SEARCH configuration structure in jconf by its id.
 * </EN>
 * <JA>
 * jconf内の SEARCH モジュール設定構造体を ID で検索する. 
 * </JA>
 * 
 * @param jconf [in] global configuration
 * @param id [in] SEARCH module ID
 * 
 * @return the found SEARCH configuration, or NULL if not found.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
JCONF_SEARCH *
j_get_searchconf_by_id(Jconf *jconf, int id)
{
  JCONF_SEARCH *sconf;

  for(sconf=jconf->search_root;sconf;sconf=sconf->next) {
    if (sconf->id == id) {
      break;
    }
  }
  if (!sconf) {		/* error */
    jlog("ERROR: j_get_searchconf_by_id: [SR%02d] not found\n", id);
    return NULL;
  }
  return sconf;
}

/** 
 * <EN>
 * De-activate a recognition process instance designated by its name.
 * The process will actually pauses at the next recognition interval.
 * </EN>
 * <JA>
 * 指定された名前の認識処理インスタンスの動作を一時停止させる. 
 * 実際に停止するのは次の音声認識の合間である. 
 * </JA>
 * 
 * @param recog [i/o] engine instance
 * @param name [in] SR name to deactivate
 * 
 * @return TRUE on success, or FALSE on failure.
 *
 * @callgraph
 * @callergraph
 * @ingroup jfunc_process
 * 
 */
boolean
j_process_deactivate(Recog *recog, char *name)
{
  RecogProcess *r;

  for(r=recog->process_list;r;r=r->next) {
    if (strmatch(r->config->name, name)) {
      /* book to be inactive at next interval */
      r->active = -1;
      break;
    }
  }
  if (!r) {			/* not found */
    jlog("ERROR: j_process_deactivate: no SR instance named \"%s\", cannot deactivate\n", name);
    return FALSE;
  }

  /* tell engine to update */
  recog->process_want_reload = TRUE;

  return TRUE;
}

/** 
 * <EN>
 * De-activate a recognition process instance designated by its ID.
 * The process will actually pauses at the next recognition interval.
 * </EN>
 * <JA>
 * 指定された認識処理インスタンスの動作を一時停止させる. 
 * 対象インスタンスを ID 番号で指定する場合はこちらを使う. 
 * 実際に停止するのは次の音声認識の合間である. 
 * </JA>
 * 
 * @param recog [i/o] engine instance
 * @param id [in] SR ID to deactivate
 * 
 * @return TRUE on success, or FALSE on failure.
 *
 * @callgraph
 * @callergraph
 * @ingroup jfunc_process
 * 
 */
boolean
j_process_deactivate_by_id(Recog *recog, int id)
{
  RecogProcess *r;

  for(r=recog->process_list;r;r=r->next) {
    if (r->config->id == id) {
      /* book to be inactive at next interval */
      r->active = -1;
      break;
    }
  }
  if (!r) {			/* not found */
    jlog("ERROR: j_process_deactivate_by_id: no SR instance whose id is \"%02d\", cannot deactivate\n", id);
    return FALSE;
  }

  /* tell engine to update */
  recog->process_want_reload = TRUE;

  return TRUE;
}

/** 
 * <EN>
 * Activate a recognition process instance that has been made inactive, by
 * its name.
 * The process will actually starts at the next recognition interval.
 * </EN>
 * <JA>
 * 一時停止されていた認識処理インスタンスの動作を再開させる. 
 * 実際に再開するのは次の音声認識の合間である. 
 * </JA>
 * 
 * @param recog [i/o] engine instance
 * @param name [in] SR name to activate
 * 
 * @return TRUE on success, or FALSE on failure.
 *
 * @callgraph
 * @callergraph
 * @ingroup jfunc_process
 * 
 */
boolean
j_process_activate(Recog *recog, char *name)
{
  RecogProcess *r;

  for(r=recog->process_list;r;r=r->next) {
    if (strmatch(r->config->name, name)) {
      /* book to be active at next interval */
      r->active = 1;
      break;
    }
  }
  if (!r) {			/* not found */
    jlog("ERROR: j_process_activate: no SR instance named \"%s\", cannot activate\n", name);
    return FALSE;
  }

  /* tell engine to update */
  recog->process_want_reload = TRUE;

  return TRUE;
}

/** 
 * <EN>
 * Activate a recognition process instance that has been made inactive, by
 * the ID.
 * The process will actually starts at the next recognition interval.
 * </EN>
 * <JA>
 * 一時停止されていた認識処理インスタンスの動作を再開させる(ID指定).
 * 実際に再開するのは次の音声認識の合間である. 
 * </JA>
 * 
 * @param recog [i/o] engine instance
 * @param id [in] SR ID to activate
 * 
 * @return TRUE on success, or FALSE on failure.
 *
 * @callgraph
 * @callergraph
 * @ingroup jfunc_process
 * 
 */
boolean
j_process_activate_by_id(Recog *recog, int id)
{
  RecogProcess *r;

  for(r=recog->process_list;r;r=r->next) {
    if (r->config->id == id) {
      /* book to be active at next interval */
      r->active = 1;
      break;
    }
  }
  if (!r) {			/* not found */
    jlog("ERROR: j_process_activate_by_id: no SR instance whose id is \"%02d\", cannot activate\n", id);
    return FALSE;
  }

  /* tell engine to update */
  recog->process_want_reload = TRUE;

  return TRUE;
}

/** 
 * <EN>
 * @brief  Create a new recognizer with a new LM and SR configurations.
 * 
 * This function creates new LM process instance and recognition process
 * instance corresponding to the given LM and SR configurations.
 * AM process to be assigned to them is the current default AM.
 * Both the new LM and SR will be assigned the same instance name.
 * </EN>
 * <JA>
 * @brief  LM および SR 設定に基づき認識処理プロセスを追加する. 
 *
 * この関数は与えられたLM設定およびSR設定データに基づき，新たな
 * LMインスタンスおよび認識プロセスインスタンスをエンジン内部に
 * 生成する. AMについては現在のデフォルトAMが自動的に用いられる. 
 * 名前はLMインスタンス，認識プロセスインスタンスとも同じ名前が
 * あたえられる. 
 * </JA>
 * 
 * @param recog [i/o] engine instance
 * @param lmconf [in] a new LM configuration
 * @param sconf [in] a new SR configuration
 * @param name [in] name of the new instances
 * 
 * @return TRUE on success, FALSE on error.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jfunc_process
 */
boolean
j_process_add_lm(Recog *recog, JCONF_LM *lmconf, JCONF_SEARCH *sconf, char *name)
{
  /* add lmconf to global config */
  if (j_jconf_lm_regist(recog->jconf, lmconf, name) == FALSE) {
    jlog("ERROR: j_process_add_lm: failed to regist new LM conf as \"%s\"\n", name);
    return FALSE;
  }
  /* assign lmconf and default amconf to the sconf */
  sconf->amconf = j_get_amconf_default(recog->jconf);
  sconf->lmconf = lmconf;
  /* add the sconf to global config */
  if (j_jconf_search_regist(recog->jconf, sconf, name) == FALSE) {
    jlog("ERROR: j_process_add_lm: failed to regist new SR conf as \"%s\"\n", name);
    j_jconf_search_free(sconf);
    return FALSE;
  }
  /* finalize the whole parameters */
  if (j_jconf_finalize(recog->jconf) == FALSE) {
    jlog("ERROR: j_process_add_lm: failed to finalize the updated whole jconf\n");
    return FALSE;
  }
  /* create LM process intance for the lmconf, and load LM */
  if (j_load_lm(recog, lmconf) == FALSE) {
    jlog("ERROR: j_process_add_lm: failed to load LM \"%s\"\n", lmconf->name);
    return FALSE;
  }
  /* create recognition process instance for the sconf, and setup for recognition */
  if (j_launch_recognition_instance(recog, sconf) == FALSE) {
    jlog("ERROR: j_process_add_lm: failed to start a new recognizer instance \"%s\"\n", sconf->name);
    return FALSE;
  }
  /* the created process will be live=FALSE, active = 1, so
     the new recognition instance is dead now but
     will be made live at next session */

  /* tell engine to update */
  recog->process_want_reload = TRUE;

  return TRUE;
}

/** 
 * <EN>
 * Remove a recognition process instance.
 * The specified search conf will also be released and destroyed
 * inside this function.
 * </EN>
 * <JA>
 * 認識処理インスタンスを削除する. 
 * 指定されたSEARCH設定もこの関数内で解放・削除される. 
 * </JA>
 * 
 * @param recog [in] engine instance
 * @param sconf [in] SEARCH configuration corresponding to the target
 * recognition process to remove
 * 
 * @return TRUE on success, or FALSE on failure.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jfunc_process
 */
boolean
j_process_remove(Recog *recog, JCONF_SEARCH *sconf)
{
  RecogProcess *r, *r_prev;
  JCONF_SEARCH *sc, *sc_prev;

  if (sconf == NULL) {
    jlog("ERROR: j_process_remove: sconf == NULL\n");
    return FALSE;
  }

  /* find corresponding process in engine and remove it from list */
  r_prev = NULL;
  for(r=recog->process_list;r;r=r->next) {
    if (r->config == sconf) {
      if (r_prev == NULL) {
	recog->process_list = r->next;
      } else {
	r_prev->next = r->next;
      }
      break;
    }
    r_prev = r;
  }
  if (!r) {
    jlog("ERROR: j_process_remove: specified sconf %02d %s not found in recogprocess, removal failed\n", sconf->id, sconf->name);
    return FALSE;
  }

  /* remove config from list in engine */
  sc_prev = NULL;
  for(sc=recog->jconf->search_root;sc;sc=sc->next) {
    if (sc == sconf) {
      if (sc_prev == NULL) {
	recog->jconf->search_root = sc->next;
      } else {
	sc_prev->next = sc->next;
      }
      break;
    }
    sc_prev = sc;
  }
  if (!sc) {
    jlog("ERROR: j_process_remove: sconf %02d %s not found\n", sconf->id, sconf->name);
  }

  /* free them */
  j_recogprocess_free(r);
  if (verbose_flag) jlog("STAT: recogprocess %02d %s removed\n", sconf->id, sconf->name);
  j_jconf_search_free(sconf);

  /* tell engine to update */
  recog->process_want_reload = TRUE;

  return TRUE;
}

/** 
 * <EN>
 * Remove an LM process instance.
 * The specified lm conf will also be released and destroyed
 * inside this function.
 * </EN>
 * <JA>
 * 言語モデルインスタンスを削除する. 
 * 指定された言語モデル設定もこの関数内で解放・削除される. 
 * </JA>
 * 
 * @param recog [in] engine instance
 * @param lmconf [in] LM configuration corresponding to the target
 * LM process to remove
 * 
 * @return TRUE on success, or FALSE on failure.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jfunc_process
 */
boolean
j_process_lm_remove(Recog *recog, JCONF_LM *lmconf)
{
  RecogProcess *r;
  PROCESS_LM *lm, *lm_prev;
  JCONF_LM *l, *l_prev;

  if (lmconf == NULL) {
    jlog("ERROR: j_process_lm_remove: lmconf == NULL\n");
    return FALSE;
  }

  /* check if still used by a process */
  for(r=recog->process_list;r;r=r->next) {
    if (r->config->lmconf == lmconf) {
      jlog("ERROR: j_process_lm_remove: specified lmconf %02d %s still used in a recogprocess %02d %s\n", lmconf->id, lmconf->name, r->config->id, r->config->name);
      return FALSE;
    }
  }

  /* find corresponding LM process in engine and remove it from list */
  lm_prev = NULL;
  for(lm=recog->lmlist;lm;lm=lm->next) {
    if (lm->config == lmconf) {
      if (lm_prev == NULL) {
	recog->lmlist = lm->next;
      } else {
	lm_prev->next = lm->next;
      }
      break;
    }
    lm_prev = lm;
  }
  if (!lm) {
    jlog("ERROR: j_process_lm_remove: specified lmconf %02d %s not found in LM process, removal failed\n", lmconf->id, lmconf->name);
    return FALSE;
  }

  /* remove config from list in engine */
  l_prev = NULL;
  for(l=recog->jconf->lm_root;l;l=l->next) {
    if (l == lmconf) {
      if (l_prev == NULL) {
	recog->jconf->lm_root = l->next;
      } else {
	l_prev->next = l->next;
      }
      break;
    }
    l_prev = l;
  }
  if (!l) {
    jlog("ERROR: j_process_lm_remove: lmconf %02d %s not found\n", lmconf->id, lmconf->name);
    return FALSE;
  }

  /* free them */
  j_process_lm_free(lm);
  if (verbose_flag) jlog("STAT: LM process %02d %s removed\n", lmconf->id, lmconf->name);
  j_jconf_lm_free(lmconf);

  /* tell engine to update */
  recog->process_want_reload = TRUE;

  return TRUE;
}

/** 
 * <EN>
 * Remove an AM process instance (experimental).
 * The specified am conf will also be released and destroyed
 * inside this function.
 * </EN>
 * <JA>
 * 言語モデルインスタンスを削除する（実験中）.
 * 指定された言語モデル設定もこの関数内で解放・削除される. 
 * </JA>
 * 
 * @param recog [in] engine instance
 * @param amconf [in] AM configuration corresponding to the target
 * AM process to remove
 * 
 * @return TRUE on success, or FALSE on failure.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jfunc_process
 */
boolean
j_process_am_remove(Recog *recog, JCONF_AM *amconf)
{
  RecogProcess *r;
  PROCESS_LM *lm;
  PROCESS_AM *am, *am_prev;
  JCONF_AM *a, *a_prev;

  if (amconf == NULL) {
    jlog("ERROR: j_process_am_remove: amconf == NULL\n");
    return FALSE;
  }

  /* check if still used by a process */
  for(r=recog->process_list;r;r=r->next) {
    if (r->config->amconf == amconf) {
      jlog("ERROR: j_process_am_remove: specified amconf %02d %s still used in a recogprocess %02d %s\n", amconf->id, amconf->name, r->config->id, r->config->name);
      return FALSE;
    }
  }

  /* check if still used by a LM process */
  for(lm=recog->lmlist;lm;lm=lm->next) {
    if (lm->am->config == amconf) {
      jlog("ERROR: j_process_am_remove: specified amconf %02d %s still used in a LM %02d %s\n", amconf->id, amconf->name, lm->config->id, lm->config->name);
      return FALSE;
    }
  }

  /* find corresponding AM process in engine and remove it from list */
  am_prev = NULL;
  for(am=recog->amlist;am;am=am->next) {
    if (am->config == amconf) {
      if (am_prev == NULL) {
	recog->amlist = am->next;
      } else {
	am_prev->next = am->next;
      }
      break;
    }
    am_prev = am;
  }
  if (!am) {
    jlog("ERROR: j_process_am_remove: specified amconf %02d %s not found in AM process, removal failed\n", amconf->id, amconf->name);
    return FALSE;
  }

  /* remove config from list in engine */
  a_prev = NULL;
  for(a=recog->jconf->am_root;a;a=a->next) {
    if (a == amconf) {
      if (a_prev == NULL) {
	recog->jconf->am_root = a->next;
      } else {
	a_prev->next = a->next;
      }
      break;
    }
    a_prev = a;
  }
  if (!a) {
    jlog("ERROR: j_process_am_remove: amconf %02d %s not found\n", amconf->id, amconf->name);
    return FALSE;
  }

  /* free them */
  j_process_am_free(am);
  if (verbose_flag) jlog("STAT: AM process %02d %s removed\n", amconf->id, amconf->name);
  j_jconf_am_free(amconf);

  /* tell engine to update */
  recog->process_want_reload = TRUE;

  return TRUE;
}

#ifdef DEBUG_VTLN_ALPHA_TEST
void
vtln_alpha(Recog *recog, RecogProcess *r)
{
  Sentence *s;
  float alpha, alpha_bgn, alpha_end;
  float max_alpha;
  LOGPROB max_score;
  PROCESS_AM *am;
  MFCCCalc *mfcc;
  SentenceAlign *align;

  s = &(r->result.sent[0]);
  align = result_align_new();

  max_score = LOG_ZERO;

  printf("------------ begin VTLN -------------\n");

  mfcc = r->am->mfcc;

  alpha_bgn = mfcc->para->vtln_alpha - VTLN_RANGE;
  alpha_end = mfcc->para->vtln_alpha + VTLN_RANGE;

  for(alpha = alpha_bgn; alpha <= alpha_end; alpha += VTLN_STEP) {
    mfcc->para->vtln_alpha = alpha;
    if (InitFBank(mfcc->wrk, mfcc->para) == FALSE) {
      jlog("ERROR: VTLN: InitFBank() failed\n");
      return;
    }
    if (wav2mfcc(recog->speech, recog->speechlen, recog) == FALSE) {
      jlog("ERROR: VTLN: wav2mfcc() failed\n");
      return;
    }
    outprob_prepare(&(r->am->hmmwrk), mfcc->param->samplenum);
    word_align(s->word, s->word_num, mfcc->param, align, r);
    printf("%f: %f\n", alpha, align->allscore);
    if (max_score < align->allscore) {
      max_score = align->allscore;
      max_alpha = alpha;
    }
  }
  printf("MAX: %f: %f\n", max_alpha, max_score);
  mfcc->para->vtln_alpha = max_alpha;
  if (InitFBank(mfcc->wrk, mfcc->para) == FALSE) {
    jlog("ERROR: VTLN: InitFBank() failed\n");
    return;
  }

  printf("------------ end VTLN -------------\n");

  result_align_free(align);

}
#endif


/** 
 * Change the scaling factor of input audio level.  Set to 1.0 to disable.
 * 
 * @param recog [i/o] engine instance
 * @param factor [in] factor value (1.0 to disable scaling)
 * 
 * @callgraph
 * @callergraph
 * @ingroup engine
 */
void
j_adin_change_input_scaling_factor(Recog *recog, float factor)
{
  recog->adin->level_coef = factor;
  recog->jconf->preprocess.level_coef = factor;
}

/* end of file */
