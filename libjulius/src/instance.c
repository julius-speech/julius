/**
 * @file   instance.c
 * 
 * <EN>
 * @brief  Allocate/free various instances
 * </EN>
 * 
 * <JA>
 * @brief  各種インスタンスの割り付けおよび開放
 * </JA>
 * 
 * @author Akinobu Lee
 * @date   Sun Oct 28 18:06:20 2007
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
 * Allocate a new MFCC calculation instance
 * </EN>
 * <JA>
 * MFCC計算インスタンスを新たに割り付ける. 
 * </JA>
 * 
 * @param amconf [in] acoustic model configuration parameters
 * 
 * @return the newly allocated MFCC calculation instance.
 *
 * @callgraph
 * @callergraph
 * 
 */
MFCCCalc *
j_mfcccalc_new(JCONF_AM *amconf)
{
  MFCCCalc *mfcc;

  mfcc = (MFCCCalc *)mymalloc(sizeof(MFCCCalc));
  memset(mfcc, 0, sizeof(MFCCCalc));
  mfcc->param = NULL;
  mfcc->rest_param = NULL;
  mfcc->frontend.ssbuf = NULL;
  mfcc->cmn.loaded = FALSE;
  mfcc->plugin_source = -1;
  if (amconf) {
    mfcc->para = &(amconf->analysis.para);
    mfcc->hmm_loaded = (amconf->analysis.para_hmm.loaded == 1) ? TRUE : FALSE;
    mfcc->htk_loaded = (amconf->analysis.para_htk.loaded == 1) ? TRUE : FALSE;
    mfcc->wrk = WMP_work_new(mfcc->para);
    if (mfcc->wrk == NULL) {
      jlog("ERROR: j_mfcccalc_new: failed to initialize feature computation\n");
      return NULL;
    }
    mfcc->cmn.load_filename = amconf->analysis.cmnload_filename;
    mfcc->cmn.update = amconf->analysis.cmn_update;
    mfcc->cmn.save_filename = amconf->analysis.cmnsave_filename;
    mfcc->cmn.map_weight = amconf->analysis.cmn_map_weight;
    mfcc->frontend.ss_alpha = amconf->frontend.ss_alpha;
    mfcc->frontend.ss_floor = amconf->frontend.ss_floor;
    mfcc->frontend.sscalc = amconf->frontend.sscalc;
    mfcc->frontend.sscalc_len = amconf->frontend.sscalc_len;
    mfcc->frontend.ssload_filename = amconf->frontend.ssload_filename;
  }
  mfcc->next = NULL;
  return mfcc;
}

/** 
 * <EN>
 * Free an MFCC calculation instance.
 * </EN>
 * <JA>
 * MFCC計算インスタンスを開放する
 * </JA>
 * 
 * @param mfcc [i/o] MFCC calculation instance
 * 
 * @callgraph
 * @callergraph
 */
void
j_mfcccalc_free(MFCCCalc *mfcc)
{
  if (mfcc->rest_param) free_param(mfcc->rest_param);
  if (mfcc->param) free_param(mfcc->param);
  if (mfcc->wrk) WMP_free(mfcc->wrk);
  if (mfcc->tmpmfcc) free(mfcc->tmpmfcc);
  if (mfcc->db) WMP_deltabuf_free(mfcc->db);
  if (mfcc->ab) WMP_deltabuf_free(mfcc->ab);
  if (mfcc->cmn.wrk) CMN_realtime_free(mfcc->cmn.wrk);
  if (mfcc->frontend.ssbuf) free(mfcc->frontend.ssbuf);
  if (mfcc->frontend.mfccwrk_ss) WMP_free(mfcc->frontend.mfccwrk_ss);

  free(mfcc);
}

/** 
 * <EN>
 * Allocate a new acoustic model processing instance.
 * </EN>
 * <JA>
 * 音響モデル計算インスタンスを新たに割り付ける. 
 * </JA>
 *
 * @param recog [i/o] engine instance
 * @param amconf [in] AM configuration to assign
 * 
 * @return newly allocated acoustic model processing instance.
 * 
 * @callgraph
 * @callergraph
 */
PROCESS_AM *
j_process_am_new(Recog *recog, JCONF_AM *amconf)
{
  PROCESS_AM *new, *atmp;

  /* allocate memory */
  new = (PROCESS_AM *)mymalloc(sizeof(PROCESS_AM));
  memset(new, 0, sizeof(PROCESS_AM));

  /* assign configuration */
  new->config = amconf;

  /* append to last */
  new->next = NULL;
  if (recog->amlist == NULL) {
    recog->amlist = new;
  } else {
    for(atmp = recog->amlist; atmp->next; atmp = atmp->next);
    atmp->next = new;
  }
  
  return new;
}

/** 
 * <EN>
 * Free an acoustic model processing instance.
 * </EN>
 * <JA>
 * 音響モデル計算インスタンスを開放する. 
 * </JA>
 * 
 * @param am [i/o] AM process instance
 * 
 * @callgraph
 * @callergraph
 */
void
j_process_am_free(PROCESS_AM *am)
{
  /* HMMWork hmmwrk */
  outprob_free(&(am->hmmwrk));
  if (am->hmminfo) hmminfo_free(am->hmminfo);
  if (am->hmm_gs) hmminfo_free(am->hmm_gs);
  /* not free am->jconf  */
  free(am);
}

/** 
 * <EN>
 * Allocate a new language model processing instance.
 * </EN>
 * <JA>
 * 言語モデル計算インスタンスを新たに割り付ける. 
 * </JA>
 *
 * @param recog [i/o] engine instance
 * @param lmconf [in] LM configuration to assign
 * 
 * @return newly allocated language model processing instance.
 * 
 * @callgraph
 * @callergraph
 */
PROCESS_LM *
j_process_lm_new(Recog *recog, JCONF_LM *lmconf)
{
  PROCESS_LM *new, *ltmp;

  /* allocate memory */
  new = (PROCESS_LM *)mymalloc(sizeof(PROCESS_LM));
  memset(new, 0, sizeof(PROCESS_LM));

  /* assign config */
  new->config = lmconf;

  /* initialize some values */
  new->lmtype = lmconf->lmtype;
  new->lmvar = lmconf->lmvar;
  new->gram_maxid = 0;
  new->global_modified = FALSE;

  /* append to last */
  new->next = NULL;
  if (recog->lmlist == NULL) {
    recog->lmlist = new;
  } else {
    for(ltmp = recog->lmlist; ltmp->next; ltmp = ltmp->next);
    ltmp->next = new;
  }

  return new;
}

/** 
 * <EN>
 * Free a language model processing instance.
 * </EN>
 * <JA>
 * 言語モデル計算インスタンスを開放する. 
 * </JA>
 * 
 * @param lm [i/o] LM process instance
 * 
 * @callgraph
 * @callergraph
 */
void
j_process_lm_free(PROCESS_LM *lm)
{
  if (lm->winfo) word_info_free(lm->winfo);
  if (lm->ngram) ngram_info_free(lm->ngram);
  if (lm->grammars) multigram_free_all(lm->grammars);
  if (lm->dfa) dfa_info_free(lm->dfa);
  /* not free lm->jconf  */
  free(lm);
}

/** 
 * <EN>
 * Allocate a new recognition process instance.
 * </EN>
 * <JA>
 * 認識処理インスタンスを新たに生成する. 
 * </JA>
 *
 * @param recog [i/o] engine instance
 * @param sconf [in] SEARCH configuration to assign
 * 
 * @return the newly allocated recognition process instance.
 * 
 * @callgraph
 * @callergraph
 */
RecogProcess *
j_recogprocess_new(Recog *recog, JCONF_SEARCH *sconf)
{
  RecogProcess *new, *ptmp;

  /* allocate memory */
  new = (RecogProcess *)mymalloc(sizeof(RecogProcess));
  memset(new, 0, sizeof(RecogProcess));
  new->live = FALSE;
  new->active = 0;
  new->next = NULL;

  /* assign configuration */
  new->config = sconf;

  /* append to last */
  new->next = NULL;
  if (recog->process_list == NULL) {
    recog->process_list = new;
  } else {
    for(ptmp = recog->process_list; ptmp->next; ptmp = ptmp->next);
    ptmp->next = new;
  }

  return new;
}

/** 
 * <EN>
 * Free a recognition process instance
 * </EN>
 * <JA>
 * 認識処理インスタンスを開放する. 
 * </JA>
 * 
 * @param process [i/o] recognition process instance
 * 
 * @callgraph
 * @callergraph
 */
void
j_recogprocess_free(RecogProcess *process)
{
  /* not free jconf, am, lm here */
  /* free part of StackDecode work area */
  wchmm_fbs_free(process);
  /* free cache */
  if (process->lmtype == LM_PROB) {
    max_successor_cache_free(process->wchmm);
  }
  /* free wchmm */
  if (process->wchmm) wchmm_free(process->wchmm);
  /* free backtrellis */
  if (process->backtrellis) bt_free(process->backtrellis);
  /* free pass1 work area */
  fsbeam_free(&(process->pass1));
  free(process);
}

/** 
 * <EN>
 * Allocate a new acoustic model (AM) parameter structure.
 * Default parameter values are set to it.
 * </EN>
 * <JA>
 * 音響モデル(AM)パラメータ構造体を新たに割り付ける.
 * 内部メンバにはデフォルト値が格納される.
 * </JA>
 * 
 * @return the newly allocated AM parameter structure
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
JCONF_AM *
j_jconf_am_new()
{
  JCONF_AM *new;
  new = (JCONF_AM *)mymalloc(sizeof(JCONF_AM));
  memset(new, 0, sizeof(JCONF_AM));
  jconf_set_default_values_am(new);
  new->next = NULL;
  return new;
}

/** 
 * <EN>
 * Release an acoustic model (AM) parameter structure
 * Default parameter values are set to it.
 * </EN>
 * <JA>
 * 音響モデル(AM)パラメータ構造体を解放する. 
 * 内部メンバにはデフォルト値が格納される. 
 * </JA>
 * 
 * @param amconf [in] AM configuration
 *
 * @callgraph
 * @callergraph
 * @ingroup jconf
 * 
 */
void
j_jconf_am_free(JCONF_AM *amconf)
{
  free(amconf);
}

/** 
 * <EN>
 * Register AM configuration to global jconf.
 * Returns error if the same name already exist in the jconf.
 * </EN>
 * <JA>
 * 音響モデル(AM)パラメータ構造体を jconf に登録する．
 * jconf内に同じ名前のモジュールが既に登録されている場合はエラーとなる．
 * </JA>
 * 
 * @param jconf [i/o] global jconf
 * @param amconf [in] AM configuration to register
 * @param name [in] module name
 *
 * @return TRUE on success, FALSE on failure
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
boolean
j_jconf_am_regist(Jconf *jconf, JCONF_AM *amconf, char *name)
{
  JCONF_AM *atmp;

  if (!name) {
    jlog("ERROR: j_jconf_am_regist: no name specified to register an AM conf\n");
    return FALSE;
  }

  for(atmp = jconf->am_root; atmp; atmp = atmp->next) {
    if (strmatch(atmp->name, name)) {
      jlog("ERROR: j_jconf_am_regist: failed to regist an AM conf: the same name \"%s\" already exist\n", atmp->name);
      return FALSE;
    }
  }

  /* set name */
  strncpy(amconf->name, name, JCONF_MODULENAME_MAXLEN);

  /* append to last */
  amconf->next = NULL;
  if (jconf->am_root == NULL) {
    amconf->id = 1;
    jconf->am_root = amconf;
  } else {
    for(atmp = jconf->am_root; atmp->next; atmp = atmp->next);
    amconf->id = atmp->id + 1;
    atmp->next = amconf;
  }

  return TRUE;
}


/** 
 * <EN>
 * Allocate a new language model (LM) parameter structure.
 * Default parameter values are set to it.
 * </EN>
 * <JA>
 * 言語モデル (LM) パラメータ構造体を新たに割り付ける
 * 内部メンバにはデフォルト値が格納される. 
 * </JA>
 * 
 * @return the newly allocated LM parameter structure.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
JCONF_LM *
j_jconf_lm_new()
{
  JCONF_LM *new;
  new = (JCONF_LM *)mymalloc(sizeof(JCONF_LM));
  memset(new, 0, sizeof(JCONF_LM));
  jconf_set_default_values_lm(new);
  new->next = NULL;
  return new;
}

/** 
 * <EN>
 * Release a language model (LM) parameter structure
 * </EN>
 * <JA>
 * 言語モデル (LM) パラメータ構造体を解放する
 * </JA>
 * 
 * @param lmconf [in] LM parameter structure
 *
 * @callgraph
 * @callergraph
 * @ingroup jconf
 * 
 */
void
j_jconf_lm_free(JCONF_LM *lmconf)
{
  JCONF_LM_NAMELIST *nl, *nltmp;
  nl = lmconf->additional_dict_files;
  while (nl) {
    nltmp = nl->next;
    free(nl->name);
    free(nl);
    nl = nltmp;
  }
  nl = lmconf->additional_dict_entries;
  while (nl) {
    nltmp = nl->next;
    free(nl->name);
    free(nl);
    nl = nltmp;
  }
  free(lmconf);
}

/** 
 * <EN>
 * Register LM configuration to global jconf.
 * Returns error if the same name already exist in the jconf.
 * </EN>
 * <JA>
 * 言語モデル(LM)パラメータ構造体を jconf に登録する．
 * jconf内に同じ名前のモジュールが既に登録されている場合はエラーとなる．
 * </JA>
 * 
 * @param jconf [i/o] global jconf
 * @param lmconf [in] LM configuration to register
 * @param name [in] module name
 *
 * @return TRUE on success, FALSE on failure
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
boolean
j_jconf_lm_regist(Jconf *jconf, JCONF_LM *lmconf, char *name)
{
  JCONF_LM *ltmp;

  if (!name) {
    jlog("ERROR: j_jconf_lm_regist: no name specified to register a LM conf\n");
    return FALSE;
  }

  for(ltmp = jconf->lm_root; ltmp; ltmp = ltmp->next) {
    if (strmatch(ltmp->name, name)) {
      jlog("ERROR: j_jconf_lm_regist: failed to regist a LM conf: the same name \"%s\" already exist\n", ltmp->name);
      return FALSE;
    }
  }

  /* set name */
  strncpy(lmconf->name, name, JCONF_MODULENAME_MAXLEN);

  /* append to last */
  lmconf->next = NULL;
  if (jconf->lm_root == NULL) {
    lmconf->id = 1;
    jconf->lm_root = lmconf;
  } else {
    for(ltmp = jconf->lm_root; ltmp->next; ltmp = ltmp->next);
    lmconf->id = ltmp->id + 1;
    ltmp->next = lmconf;
  }

  return TRUE;
}


/** 
 * <EN>
 * Allocate a new search (SEARCH) parameter structure.
 * Default parameter values are set to it.
 * </EN>
 * <JA>
 * 探索パラメータ(SEARCH)構造体を新たに割り付ける. 
 * 内部メンバにはデフォルト値が格納される. 
 * </JA>
 * 
 * @return the newly allocated SEARCH parameter structure.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
JCONF_SEARCH *
j_jconf_search_new()
{
  JCONF_SEARCH *new;
  new = (JCONF_SEARCH *)mymalloc(sizeof(JCONF_SEARCH));
  memset(new, 0, sizeof(JCONF_SEARCH));
  jconf_set_default_values_search(new);
  new->next = NULL;
  return new;
}

/** 
 * <EN>
 * Release a search (SEARCH) parameter structure
 * </EN>
 * <JA>
 * 探索パラメータ(SEARCH)構造体を解放する
 * </JA>
 * 
 * @param sconf [in] SEARCH parameter structure
 *
 * @callgraph
 * @callergraph
 * @ingroup jconf
 * 
 */
void
j_jconf_search_free(JCONF_SEARCH *sconf)
{
  free(sconf);
}

/** 
 * <EN>
 * Register SEARCH configuration to global jconf.
 * Returns error if the same name already exist in the jconf.
 * </EN>
 * <JA>
 * 探索(SEARCH)パラメータ構造体を jconf に登録する．
 * jconf内に同じ名前のモジュールが既に登録されている場合はエラーとなる．
 * </JA>
 * 
 * @param jconf [i/o] global jconf
 * @param sconf [in] SEARCH configuration to register
 * @param name [in] module name
 *
 * @return TRUE on success, FALSE on failure
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
boolean
j_jconf_search_regist(Jconf *jconf, JCONF_SEARCH *sconf, char *name)
{
  JCONF_SEARCH *stmp;

  if (!name) {
    jlog("ERROR: j_jconf_search_regist: no name specified to register a SR conf\n");
    return FALSE;
  }

  for(stmp = jconf->search_root; stmp; stmp = stmp->next) {
    if (strmatch(stmp->name, name)) {
      jlog("ERROR: j_jconf_search_regist: failed to regist an SR conf: the same name \"%s\" already exist\n", stmp->name);
      return FALSE;
    }
  }

  /* set name */
  strncpy(sconf->name, name, JCONF_MODULENAME_MAXLEN);

  /* append to last */
  sconf->next = NULL;
  if (jconf->search_root == NULL) {
    sconf->id = 1;
    jconf->search_root = sconf;
  } else {
    for(stmp = jconf->search_root; stmp->next; stmp = stmp->next);
    sconf->id = stmp->id + 1;
    stmp->next = sconf;
  }
  return TRUE;
}

/** 
 * <EN>
 * @brief  Allocate a new global configuration parameter structure.
 *
 * JCONF_AM, JCONF_LM, JCONF_SEARCH are defined one for each, and
 * assigned to the newly allocated structure as initial instances.
 * 
 * </EN>
 * <JA>
 * @brief  全体のパラメータ構造体を新たに割り付ける. 
 *
 * JCONF_AM, JCONF_LM, JCONF_SEARCHも１つづつ割り当てられる. 
 * これらは -AM 等の指定を含まない 3.x 以前の jconf を読み込んだときに，
 * そのまま用いられる. 
 * 
 * </JA>
 * 
 * @return the newly allocated global configuration parameter structure.
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
Jconf *
j_jconf_new()
{
  Jconf *jconf;

  /* allocate memory */
  jconf = (Jconf *)mymalloc(sizeof(Jconf));
  memset(jconf, 0, sizeof(Jconf));
  /* set default values */
  jconf_set_default_values(jconf);

  /* allocate first one am / lm /search instance with their name left NULL */
  jconf->am_root = j_jconf_am_new();
  jconf->am_root->id = 0;
  strcpy(jconf->am_root->name, JCONF_MODULENAME_DEFAULT);
  jconf->lm_root = j_jconf_lm_new();
  jconf->lm_root->id = 0;
  strcpy(jconf->lm_root->name, JCONF_MODULENAME_DEFAULT);
  jconf->search_root = j_jconf_search_new();
  jconf->search_root->id = 0;
  strcpy(jconf->search_root->name, JCONF_MODULENAME_DEFAULT);
  /* assign the am /lm instance to the instance */
  jconf->search_root->amconf = jconf->am_root;
  jconf->search_root->lmconf = jconf->lm_root;
  /* set current */
  jconf->amnow = jconf->am_root;
  jconf->lmnow = jconf->lm_root;
  jconf->searchnow = jconf->search_root;
  /* set gmm am jconf */
  jconf->gmm = NULL;
  jconf->outprob_outfile = NULL;

  return(jconf);
}

/** 
 * <EN>
 * @brief  Free a global configuration parameter structure.
 *
 * All JCONF_AM, JCONF_LM, JCONF_SEARCH are also released.
 * 
 * </EN>
 * <JA>
 * @brief  全体のパラメータ構造体を開放する. 
 *
 * JCONF_AM, JCONF_LM, JCONF_SEARCHもすべて開放される. 
 * 
 * </JA>
 * 
 * @param jconf [in] global configuration parameter structure
 * 
 * @callgraph
 * @callergraph
 * @ingroup jconf
 */
void
j_jconf_free(Jconf *jconf)
{
  JCONF_AM *am, *amtmp;
  JCONF_LM *lm, *lmtmp;
  JCONF_SEARCH *sc, *sctmp;

  opt_release(jconf);

  am = jconf->am_root;
  while(am) {
    amtmp = am->next;
    j_jconf_am_free(am);
    am = amtmp;
  }
  lm = jconf->lm_root;
  while(lm) {
    lmtmp = lm->next;
    j_jconf_lm_free(lm);
    lm = lmtmp;
  }
  sc = jconf->search_root;
  while(sc) {
    sctmp = sc->next;
    j_jconf_search_free(sc);
    sc = sctmp;
  }
  if (jconf->outprob_outfile) {
    free(jconf->outprob_outfile);
    jconf->outprob_outfile = NULL;
  }

  free(jconf);
}

/** 
 * <EN>
 * Allocate memory for a new engine instance.
 * </EN>
 * <JA>
 * エンジンインスタンスを新たにメモリ割り付けする. 
 * </JA>
 * 
 * @return the newly allocated engine instance.
 * 
 * @callgraph
 * @callergraph
 * @ingroup instance
 */
Recog *
j_recog_new()
{
  Recog *recog;

  /* allocate memory */
  recog = (Recog *)mymalloc(sizeof(Recog));

  /* clear all values to 0 (NULL)  */
  memset(recog, 0, sizeof(Recog));

  /* initialize some values */
  recog->jconf = NULL;
  recog->amlist = NULL;
  recog->lmlist = NULL;
  recog->process_list = NULL;

  recog->process_online = FALSE;
  recog->process_active = TRUE;
  recog->process_want_terminate = FALSE;
  recog->process_want_reload = FALSE;
  recog->gram_switch_input_method = SM_PAUSE;
  recog->process_segment = FALSE;

  /* set default function for vector calculation to RealTimeMFCC() */
  recog->calc_vector = RealTimeMFCC;

  /* clear callback func. */
  callback_init(recog);

  recog->adin = (ADIn *)mymalloc(sizeof(ADIn));
  memset(recog->adin, 0, sizeof(ADIn));

  return(recog);
}

/** 
 * <EN>
 * @brief  Free an engine instance.
 *
 * All allocated memories in the instance will be also released.
 * </EN>
 * <JA>
 * @brief  エンジンインスタンスを開放する
 *
 * インスタンス内でこれまでにアロケートされた全てのメモリも開放される. 
 * </JA>
 * 
 * @param recog [in] engine instance.
 * 
 * @callgraph
 * @callergraph
 * @ingroup instance
 */
void
j_recog_free(Recog *recog)
{
  if (recog->gmm) hmminfo_free(recog->gmm);

  if (recog->speech) free(recog->speech);

  /* free adin work area */
  adin_free_param(recog);
  /* free GMM calculation work area if any */
  gmm_free(recog);

  /* Output result -> free just after malloced and used */
  /* StackDecode pass2 -> allocate and free within search */

  /* RealBeam real */
  realbeam_free(recog);

  /* adin */
  if (recog->adin) free(recog->adin);

  /* instances */
  {
    RecogProcess *p, *ptmp;
    p = recog->process_list;
    while(p) {
      ptmp = p->next;
      j_recogprocess_free(p);
      p = ptmp;
    }
  }
  {
    PROCESS_LM *lm, *lmtmp;
    lm = recog->lmlist;
    while(lm) {
      lmtmp = lm->next;
      j_process_lm_free(lm);
      lm = lmtmp;
    }
  }
  {
    PROCESS_AM *am, *amtmp;
    am = recog->amlist;
    while(am) {
      amtmp = am->next;
      j_process_am_free(am);
      am = amtmp;
    }
  }
  {
    MFCCCalc *mfcc, *tmp;
    mfcc = recog->mfcclist;
    while(mfcc) {
      tmp = mfcc->next;
      j_mfcccalc_free(mfcc);
      mfcc = tmp;
    }
  }

  /* jconf */
  if (recog->jconf) {
    j_jconf_free(recog->jconf);
  }

  free(recog);
}

/* end of file */
