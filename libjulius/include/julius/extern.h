/**
 * @file   extern.h
 * 
 * <JA>
 * @brief  外部関数宣言
 * </JA>
 * 
 * <EN>
 * @brief  External function declarations
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Mon Mar  7 23:19:14 2005
 *
 * $Revision: 1.24 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* should be included after all include files */

#ifdef __cplusplus
extern "C" {
#endif

/* backtrellis.c */
void bt_init(BACKTRELLIS *bt);
void bt_prepare(BACKTRELLIS *bt);
void bt_free(BACKTRELLIS *bt);
TRELLIS_ATOM *bt_new(BACKTRELLIS *bt);
void bt_store(BACKTRELLIS *bt, TRELLIS_ATOM *aotm);
void bt_relocate_rw(BACKTRELLIS *bt);
void set_terminal_words(RecogProcess *r);
void bt_discount_pescore(WCHMM_INFO *wchmm, BACKTRELLIS *bt, HTK_Param *param);
void bt_discount_lm(BACKTRELLIS *bt);
void bt_sort_rw(BACKTRELLIS *bt);
TRELLIS_ATOM *bt_binsearch_atom(BACKTRELLIS *bt, int time, WORD_ID wkey);

/* factoring_sub.c */
void make_iwcache_index(WCHMM_INFO *wchmm);
void adjust_sc_index(WCHMM_INFO *wchmm);
void make_successor_list(WCHMM_INFO *wchmm);
void make_successor_list_unigram_factoring(WCHMM_INFO *wchmm);
void max_successor_cache_init(WCHMM_INFO *wchmm);
void max_successor_cache_free(WCHMM_INFO *wchmm);
LOGPROB max_successor_prob(WCHMM_INFO *wchmm, WORD_ID lastword, int node);
LOGPROB *max_successor_prob_iw(WCHMM_INFO *wchmm, WORD_ID lastword);
void  calc_all_unigram_factoring_values(WCHMM_INFO *wchmm);
boolean can_succeed(WCHMM_INFO *wchmm, WORD_ID lastword, int node);

/* beam.c */
boolean get_back_trellis_init(HTK_Param *param, RecogProcess *r);
boolean get_back_trellis_proceed(int t, HTK_Param *param, RecogProcess *r, boolean final_for_multipath);
void get_back_trellis_end(HTK_Param *param, RecogProcess *r);
void fsbeam_free(FSBeam *d);
void finalize_1st_pass(RecogProcess *r, int len);

/* pass1.c */
#ifdef POWER_REJECT
boolean power_reject(Recog *recog);
#endif
int decode_proceed(Recog *recog);
void decode_end_segmented(Recog *recog);
void decode_end(Recog *recog);
boolean get_back_trellis(Recog *recog);

/* spsegment.c */
boolean is_sil(WORD_ID w, RecogProcess *r);
void mfcc_copy_to_rest_and_shrink(MFCCCalc *mfcc, int start, int end);
void mfcc_shrink(MFCCCalc *mfcc, int p);
boolean detect_end_of_segment(RecogProcess *r, int time);
void finalize_segment(Recog *recog);
void spsegment_init(Recog *recog);
boolean spsegment_trigger_sync(Recog *recog);
boolean spsegment_need_restart(Recog *recog, int *rf_ret, boolean *repro_ret);
void spsegment_restart_mfccs(Recog *recog, int rewind_frame, boolean reprocess);


/* outprob_style.c */
#ifdef PASS1_IWCD
void outprob_style_cache_init(WCHMM_INFO *wchmm);
CD_Set *lcdset_lookup_with_category(WCHMM_INFO *wchmm, HMM_Logical *hmm, WORD_ID category);
void lcdset_register_with_category_all(WCHMM_INFO *wchmm);
void lcdset_remove_with_category_all(WCHMM_INFO *wchmm);
#endif
LOGPROB outprob_style(WCHMM_INFO *wchmm, int node, int last_wid, int t, HTK_Param *param);
void error_missing_right_triphone(HMM_Logical *base, char *rc_name);
void error_missing_left_triphone(HMM_Logical *base, char *lc_name);

/* ngram_decode.c */
#include "search.h"
int ngram_firstwords(NEXTWORD **nw, int peseqlen, int maxnw, RecogProcess *r);
int ngram_nextwords(NODE *hypo, NEXTWORD **nw, int maxnw, RecogProcess *r);
boolean ngram_acceptable(NODE *hypo, RecogProcess *r);
int dfa_firstwords(NEXTWORD **nw, int peseqlen, int maxnw, RecogProcess *r);
int dfa_nextwords(NODE *hypo, NEXTWORD **nw, int maxnw, RecogProcess *r);
boolean dfa_acceptable(NODE *hypo, RecogProcess *r);
boolean dfa_look_around(NEXTWORD *nword, NODE *hypo, RecogProcess *r);

/* search_bestfirst_main.c */
void segment_set_last_nword(NODE *hypo, RecogProcess *r);
void pass2_finalize_on_no_result(RecogProcess *r, boolean use_1pass_as_final);
void wchmm_fbs(HTK_Param *param, RecogProcess *r, int cate_bgn, int cate_num);
void wchmm_fbs_prepare(RecogProcess *r);
void wchmm_fbs_free(RecogProcess *r);

/* search_bestfirst_v?.c */
void clear_stocker(StackDecode *s);
void free_node(NODE *node);
NODE *cpy_node(NODE *dst, NODE *src);
NODE *newnode(RecogProcess *r);
void malloc_wordtrellis(RecogProcess *r);
void free_wordtrellis(StackDecode *dwrk);
void scan_word(NODE *now, HTK_Param *param, RecogProcess *r);
void next_word(NODE *now, NODE *newParam, NEXTWORD *nword, HTK_Param *param, RecogProcess *r);
void start_word(NODE *newParam, NEXTWORD *nword, HTK_Param *param, RecogProcess *r);
void last_next_word(NODE *now, NODE *newParam, HTK_Param *param, RecogProcess *r);

/* wav2mfcc.c */
boolean wav2mfcc(SP16 speech[], int speechlen, Recog *recog);

/* version.c */
void j_put_header(FILE *stream);
void j_put_version(FILE *stream);
void j_put_compile_defs(FILE *stream);
void j_put_library_defs(FILE *stream);

/* wchmm.c */
WCHMM_INFO *wchmm_new();
void wchmm_free(WCHMM_INFO *w);
void print_wchmm_info(WCHMM_INFO *wchmm);
boolean build_wchmm(WCHMM_INFO *wchmm, JCONF_LM *lmconf);
boolean build_wchmm2(WCHMM_INFO *wchmm, JCONF_LM *lmconf);

/* wchmm_check.c */
void wchmm_check_interactive(WCHMM_INFO *wchmm);
void check_wchmm(WCHMM_INFO *wchmm);

/* realtime.c --- callback for adin_cut() */
boolean RealTimeInit(Recog *recog);
boolean RealTimePipeLinePrepare(Recog *recog);
boolean RealTimeMFCC(MFCCCalc *mfcc, SP16 *window, int windowlen);
int RealTimePipeLine(SP16 *Speech, int len, Recog *recog);
int RealTimeResume(Recog *recog);
boolean RealTimeParam(Recog *recog);
void RealTimeCMNUpdate(MFCCCalc *mfcc, Recog *recog);
void RealTimeTerminate(Recog *recog);
void realbeam_free(Recog *recog);
int mfcc_go(Recog *recog, int (*ad_check)(Recog *));

/* word_align.c */
void word_align(WORD_ID *words, short wnum, HTK_Param *param, SentenceAlign *align, RecogProcess *r);
void phoneme_align(WORD_ID *words, short wnum, HTK_Param *param, SentenceAlign *align, RecogProcess *r);
void state_align(WORD_ID *words, short wnum, HTK_Param *param, SentenceAlign *align, RecogProcess *r);
void word_rev_align(WORD_ID *revwords, short wnum, HTK_Param *param, SentenceAlign *align, RecogProcess *r);
void phoneme_rev_align(WORD_ID *revwords, short wnum, HTK_Param *param, SentenceAlign *align, RecogProcess *r);
void state_rev_align(WORD_ID *revwords, short wnum, HTK_Param *param, SentenceAlign *align, RecogProcess *r);
void do_alignment_all(RecogProcess *r, HTK_Param *param);

/* m_usage.c */
void opt_terminate();
void j_output_argument_help(FILE *fp);
/* m_options.c */
char *filepath(char *filename, char *dirname);
boolean opt_parse(int argc, char *argv[], char *cwd, Jconf *jconf);
void opt_release(Jconf *jconf);
/* m_jconf.c */
void get_dirname(char *path);
boolean config_string_parse(char *str, Jconf *jconf);
boolean config_file_parse(char *conffile, Jconf *jconf);
/* m_chkparam.c */
boolean checkpath(char *filename);
boolean j_jconf_finalize(Jconf *jconf);
int set_beam_width(WCHMM_INFO *wchmm, int specified);
/* m_info.c */
void print_jconf_overview(Jconf *jconf);
void print_mfcc_info(FILE *fp, MFCCCalc *mfcc, Jconf *jconf);
void print_engine_info(Recog *recog);
/* m_bootup.c */
void system_bootup(Recog *recog);
/* m_adin.c */
boolean adin_initialize(Recog *recog);
/* m_fusion.c */
boolean j_load_am(Recog *recog, JCONF_AM *amconf);
boolean j_load_lm(Recog *recog, JCONF_LM *lmconf);
boolean j_load_all(Recog *recog, Jconf *jconf);
boolean j_launch_recognition_instance(Recog *recog, JCONF_SEARCH *sconf);
boolean j_final_fusion(Recog *recog);
void create_mfcc_calc_instances(Recog *recog);
boolean j_reload_adddict(Recog *recog, PROCESS_LM *lm);

/* hmm_check.c */
void hmm_check(RecogProcess *r);

/* visual.c */
void visual_init(Recog *recog);
void visual_show(BACKTRELLIS *bt);
void visual2_init(int maxhypo);
void visual2_popped(NODE *n, int popctr);
void visual2_next_word(NODE *next, NODE *prev, int popctr);
void visual2_best(NODE *now, WORD_INFO *winfo);

/* gmm.c */
boolean gmm_init(Recog *recog);
void gmm_prepare(Recog *recog);
void gmm_proceed(Recog *recog);
void gmm_end(Recog *recog);
boolean gmm_valid_input(Recog *recog);
void gmm_free(Recog *recog);
#ifdef GMM_VAD
void gmm_check_trigger(Recog *recog);
#endif

/* graphout.c */
void wordgraph_init(WCHMM_INFO *wchmm);
void wordgraph_free(WordGraph *wg);
void put_wordgraph(FILE *fp, WordGraph *wg, WORD_INFO *winfo);
void wordgraph_dump(FILE *fp, WordGraph *root, WORD_INFO *winfo);
WordGraph *wordgraph_assign(WORD_ID wid, WORD_ID wid_left, WORD_ID wid_right, int leftframe, int rightframe, LOGPROB fscore_head, LOGPROB fscore_tail, LOGPROB gscore_head, LOGPROB gscore_tail, LOGPROB lscore, LOGPROB cmscore, RecogProcess *r);
boolean wordgraph_check_and_add_rightword(WordGraph *wg, WordGraph *right, LOGPROB lscore);
boolean wordgraph_check_and_add_leftword(WordGraph *wg, WordGraph *left, LOGPROB lscore);
void wordgraph_save(WordGraph *wg, WordGraph *right, WordGraph **root);
WordGraph *wordgraph_check_merge(WordGraph *now, WordGraph **root, WORD_ID next_wid, boolean *merged_p, JCONF_SEARCH *jconf);
WordGraph *wordgraph_dup(WordGraph *wg, WordGraph **root);
void wordgraph_purge_leaf_nodes(WordGraph **rootp, RecogProcess *r);
void wordgraph_depth_cut(WordGraph **rootp, RecogProcess *r);
void wordgraph_adjust_boundary(WordGraph **rootp, RecogProcess *r);
void wordgraph_clean(WordGraph **rootp);
void wordgraph_compaction_thesame(WordGraph **rootp);
void wordgraph_compaction_exacttime(WordGraph **rootp, RecogProcess *r);
void wordgraph_compaction_neighbor(WordGraph **rootp, RecogProcess *r);
int wordgraph_sort_and_annotate_id(WordGraph **rootp, RecogProcess *r);
void wordgraph_check_coherence(WordGraph *rootp, RecogProcess *r);
void graph_forward_backward(WordGraph *root, RecogProcess *r);

/* default.c */
void jconf_set_default_values(Jconf *j);
void jconf_set_default_values_am(JCONF_AM *j);
void jconf_set_default_values_lm(JCONF_LM *j);
void jconf_set_default_values_search(JCONF_SEARCH *j);


/* multi-gram.c */
int multigram_add(DFA_INFO *dfa, WORD_INFO *winfo, char *name, PROCESS_LM *lm);
boolean multigram_delete(int gid, PROCESS_LM *lm);
void multigram_delete_all(PROCESS_LM *lm);
boolean multigram_update(PROCESS_LM *lm);
boolean multigram_build(RecogProcess *r);
int multigram_activate(int gid, PROCESS_LM *lm);
int multigram_deactivate(int gid, PROCESS_LM *lm);
boolean multigram_load_all_gramlist(PROCESS_LM *lm);
int multigram_get_gram_from_category(int category, PROCESS_LM *lm);
int multigram_get_gram_from_wid(WORD_ID wid, PROCESS_LM *lm);
int multigram_get_all_num(PROCESS_LM *lm);
void multigram_free_all(MULTIGRAM *root);

int multigram_get_id_by_name(PROCESS_LM *lm, char *gramname);
MULTIGRAM *multigram_get_grammar_by_name(PROCESS_LM *lm, char *gramname);
MULTIGRAM *multigram_get_grammar_by_id(PROCESS_LM *lm, unsigned short id);
boolean multigram_add_words_to_grammar(PROCESS_LM *lm, MULTIGRAM *m, WORD_INFO *winfo);
boolean multigram_add_words_to_grammar_by_name(PROCESS_LM *lm, char *gramname, WORD_INFO *winfo);
boolean multigram_add_words_to_grammar_by_id(PROCESS_LM *lm, unsigned short id, WORD_INFO *winfo);


/* gramlist.c */
void multigram_add_gramlist(char *dfafile, char *dictfile, JCONF_LM *j, int lmvar);
void multigram_remove_gramlist(JCONF_LM *j);
boolean multigram_add_prefix_list(char *prefix_list, char *cwd, JCONF_LM *j, int lmvar);
boolean multigram_add_prefix_filelist(char *listfile, JCONF_LM *j, int lmvar);


/* adin-cut.c */
boolean adin_setup_param(ADIn *adin, Jconf *jconf);
boolean adin_thread_create(Recog *recog);
boolean adin_thread_cancel(Recog *recog);
int adin_go(int (*ad_process)(SP16 *, int, Recog *), int (*ad_check)(Recog *), Recog *recog);
boolean adin_standby(ADIn *a, int freq, void *arg);
boolean adin_begin(ADIn *a, char *file_or_dev_name);
boolean adin_end(ADIn *a);
void adin_free_param(Recog *recog);

/* confnet.c */
CN_CLUSTER *confnet_create(WordGraph *root, RecogProcess *r);
void graph_make_order(WordGraph *root, RecogProcess *r);
void graph_free_order(RecogProcess *r);
void cn_free_all(CN_CLUSTER **croot);

/* callback.c */
void callback_init(Recog *recog);
int callback_add(Recog *recog, int code, void (*func)(Recog *recog, void *data), void *data);
int callback_add_adin(Recog *recog, int code, void (*func)(Recog *recog, SP16 *buf, int len, void *data), void *data);
void callback_exec(int code, Recog *recog);
void callback_exec_adin(int code, Recog *recog, SP16 *buf, int len);
boolean callback_exist(Recog *recog, int code);
boolean callback_delete(Recog *recog, int id);

/* recogmain.c */
int adin_cut_callback_store_buffer(SP16 *now, int len, Recog *recog);
SentenceAlign *result_align_new();
void result_align_free(SentenceAlign *a);
void result_sentence_malloc(RecogProcess *r, int num);
void result_sentence_free(RecogProcess *r);
void clear_result(RecogProcess *r);

/* plugin.c */
int plugin_get_id(char *name);
void plugin_init();
boolean plugin_load_file(char *file);
boolean plugin_load_dir(char *dir);
void plugin_load_dirs(char *dirent);
int plugin_find_optname(char *optfuncname, char *str);
FUNC_VOID plugin_get_func(int sid, char *name);
boolean plugin_exec_engine_startup(Recog *recog);
void plugin_exec_adin_captured(short *buf, int len);
void plugin_exec_adin_triggered(short *buf, int len);
void plugin_exec_vector_postprocess(VECT *vecbuf, int veclen, int nframe);
void plugin_exec_vector_postprocess_all(HTK_Param *param);
void plugin_exec_process_result(Recog *recog);
boolean mfc_module_init(MFCCCalc *mfcc, Recog *recog);
boolean mfc_module_set_header(MFCCCalc *mfcc, Recog *recog);
boolean mfc_module_standby(MFCCCalc *mfcc);
boolean mfc_module_begin(MFCCCalc *mfcc);
boolean mfc_module_end(MFCCCalc *mfcc);
int mfc_module_read(MFCCCalc *mfcc, int *new_t);
char *mfc_module_input_name(MFCCCalc *mfcc);
#ifdef USE_MBR
/* mbr.c */
void candidate_mbr(NODE **r_start, NODE **r_bottom, int r_stacknum, RecogProcess *r);
#endif
#ifdef __cplusplus
}
#endif
