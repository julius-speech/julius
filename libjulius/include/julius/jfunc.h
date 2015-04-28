/**
 * @file   jfunc.h
 * 
 * <EN>
 * @brief  API related functions (not all)
 * </EN>
 * 
 * <JA>
 * @brief  API関連関数（全てではない）
 * </JA>
 * 
 * @author Akinobu Lee
 * @date   Tue Nov  6 22:41:00 2007
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

#include <julius/julius.h>
#include <stdarg.h>

#ifndef __J_JFUNC_H__
#define __J_JFUNC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* recogmain.c */
int j_open_stream(Recog *recog, char *file_or_dev_name);
int j_close_stream(Recog *recog);
int j_recognize_stream(Recog *recog);

/* jfunc.c */
void j_request_pause(Recog *recog);
void j_request_terminate(Recog *recog);
void j_request_resume(Recog *recog);
void schedule_grammar_update(Recog *recog);
void j_reset_reload(Recog *recog);

void j_enable_debug_message();
void j_disable_debug_message();
void j_enable_verbose_message();
void j_disable_verbose_message();

void j_internal_error(char *fmt, ...);

int j_config_load_args(Jconf *jconf, int argc, char *argv[]);
int j_config_load_string(Jconf *jconf, char *string);
int j_config_load_file(Jconf *jconf, char *filename);
Jconf *j_config_load_args_new(int argc, char *argv[]);
Jconf *j_config_load_string_new(char *string);
Jconf *j_config_load_file_new(char *filename);
void j_add_dict(JCONF_LM *lm, char *dictfile);
void j_add_word(JCONF_LM *lm, char *wordentry);
boolean j_adin_init(Recog *recog);
char *j_get_current_filename(Recog *recog);
void j_recog_info(Recog *recog);
Recog *j_create_instance_from_jconf(Jconf *jconf);

boolean j_regist_user_lm_func(PROCESS_LM *lm, LOGPROB (*unifunc)(WORD_INFO *winfo, WORD_ID w, LOGPROB ngram_prob), LOGPROB (*bifunc)(WORD_INFO *winfo, WORD_ID context, WORD_ID w, LOGPROB ngram_prob), LOGPROB (*probfunc)(WORD_INFO *winfo, WORD_ID *contexts, int context_len, WORD_ID w, LOGPROB ngram_prob));
boolean j_regist_user_param_func(Recog *recog, boolean (*user_calc_vector)(MFCCCalc *, SP16 *, int));

JCONF_AM *j_get_amconf_by_name(Jconf *jconf, char *name);
JCONF_AM *j_get_amconf_by_id(Jconf *jconf, int id);
JCONF_AM *j_get_amconf_default(Jconf *jconf);
JCONF_LM *j_get_lmconf_by_name(Jconf *jconf, char *name);
JCONF_LM *j_get_lmconf_by_id(Jconf *jconf, int id);
JCONF_SEARCH *j_get_searchconf_by_name(Jconf *jconf, char *name);
JCONF_SEARCH *j_get_searchconf_by_id(Jconf *jconf, int id);

boolean j_process_deactivate(Recog *recog, char *name);
boolean j_process_deactivate_by_id(Recog *recog, int id);
boolean j_process_activate(Recog *recog, char *name);
boolean j_process_activate_by_id(Recog *recog, int id);

boolean j_process_add_lm(Recog *recog, JCONF_LM *lmconf, JCONF_SEARCH *sconf, char *name);
boolean j_remove_search(Recog *recog, JCONF_SEARCH *sconf);
boolean j_remove_lm(Recog *recog, JCONF_LM *lmconf);
boolean j_remove_am(Recog *recog, JCONF_AM *amconf);

#ifdef DEBUG_VTLN_ALPHA_TEST
void vtln_alpha(Recog *recog, RecogProcess *r);
#endif

void j_adin_change_input_scaling_factor(Recog *recog, float factor);


/* instance.c */
MFCCCalc *j_mfcccalc_new(JCONF_AM *amconf);
void j_mfcccalc_free(MFCCCalc *mfcc);
PROCESS_AM *j_process_am_new(Recog *recog, JCONF_AM *amconf);
void j_process_am_free(PROCESS_AM *am);
PROCESS_LM *j_process_lm_new(Recog *recog, JCONF_LM *lmconf);
void j_process_lm_free(PROCESS_LM *lm);
RecogProcess *j_recogprocess_new(Recog *recog, JCONF_SEARCH *sconf);
void j_recogprocess_free(RecogProcess *process);
JCONF_AM *j_jconf_am_new();
void j_jconf_am_free(JCONF_AM *amconf);
boolean j_jconf_am_regist(Jconf *jconf, JCONF_AM *amconf, char *name);
JCONF_LM *j_jconf_lm_new();
void j_jconf_lm_free(JCONF_LM *lmconf);
boolean j_jconf_lm_regist(Jconf *jconf, JCONF_LM *lmconf, char *name);
JCONF_SEARCH *j_jconf_search_new();
void j_jconf_search_free(JCONF_SEARCH *sconf);
boolean j_jconf_search_regist(Jconf *jconf, JCONF_SEARCH *sconf, char *name);
Jconf *j_jconf_new();
void j_jconf_free(Jconf *jconf);
Recog *j_recog_new();
void j_recog_free(Recog *recog);

#ifdef __cplusplus
}
#endif

#endif /* __J_JFUNC_H__ */
