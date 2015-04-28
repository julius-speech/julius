/**
 * @file   dfa.h
 *
 * <EN>
 * @brief  Definitions for DFA grammar and category-pair information
 *
 * This file includes definitions for a finite state grammar called DFA.
 *
 * DFA is a deterministic finite state automaton describing grammartical
 * constraint, using the category number of each dictionary word as an input.
 * It also holds lists of words belonging for each categories.
 *
 * Additionaly, the category-pair information will be generated from the given
 * DFA by extracting allowed connections between categories.  It will be used
 * as a degenerated constraint of word connection at the 1st pass.
 * </EN>
 * <JA>
 * @brief 決定性有限状態オートマトン文法(DFA)およびカテゴリ対情報の構造体定義
 *
 * このファイルには, DFAと呼ばれる有限状態文法の構造体が定義されています．
 *
 * DFAは, 単語のカテゴリ番号を入力とする決定性オートマトンで,構文制約を
 * 表現します．カテゴリごとの単語リストも保持します．
 *
 * また，第１パスの認識のために,DFAカテゴリ間の接続関係のみを抜き出した
 * 単語対情報も保持します．これは文法を読みだし後に内部でDFAから抽出されます．
 * </JA>
 *
 * @author Akinobu LEE
 * @date   Thu Feb 10 18:21:27 2005
 *
 * $Revision: 1.8 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __SENT_DFA_H__
#define __SENT_DFA_H__

#include <sent/stddefs.h>

#define DFA_STATESTEP 1000	///< Allocation step of DFA state

#define DFA_CP_MINSTEP 20	///< Minimum initial CP data size per category

#define INITIAL_S 0x10000000	///< Status flag mask specifying an initial state
#define ACCEPT_S  0x00000001	///< Status flag mask specifying an accept state

/// Transition arc of DFA
typedef struct _dfa_arc {
  short label;                  ///< Input(=category ID) corresponding to this arc
  int to_state;			///< Next state to move
  struct _dfa_arc *next;	///< Pointer to the next arc in the same state, NULL if last
} DFA_ARC;

/// State of DFA
typedef struct {
  int      	number;		///< Unique ID
  unsigned int	status;		///< Status flag
  DFA_ARC	*arc;		///< Pointer to its arc list, NULL if has no arc
} DFA_STATE;

/// Information of each terminal symbol (=category)
typedef struct {
  int term_num;			///< Total number of category
  WORD_ID **tw;			///< Word lists in each category as @c [c][0..wnum[c]-1]
  int *wnum;			///< Number of words in each category
} TERM_INFO;

/// Top structure of a DFA
typedef struct {
  DFA_STATE *st;		///< Array of all states
  int maxstatenum;		///< Number of maximum allocated states
  int state_num;		///< Total number of states actually defined
  int arc_num;			///< Total number of arcs
  int term_num;			///< Total number of categories
  int **cp;           ///< Store constraint whether @c c2 can follow @c c1
  int *cplen;			///< Lengthes of each bcp 
  int *cpalloclen;		///< Allocated lengthes of each cp
  int *cp_begin;      ///< Store constraint whether @c c can appear at beginning of sentence
  int cp_begin_len;		///< Length of cp_begin
  int cp_begin_alloclen;		///< Allocated length of cp_begin
  int *cp_end;	///< Store constraint whether @c c can appear at end of sentence
  int cp_end_len;		///< Length of cp_end
  int cp_end_alloclen;		///< Allocated length of cp_end
  TERM_INFO term;		///< Information of terminal symbols (category)
  boolean *is_sp;		///< TRUE if the category contains only \a sp word
  WORD_ID sp_id;		///< Word ID of short pause word
} DFA_INFO;

#ifdef __cplusplus
extern "C" {
#endif

DFA_INFO *dfa_info_new();
void dfa_info_free(DFA_INFO *dfa);
void dfa_state_init(DFA_INFO *dinfo);
void dfa_state_expand(DFA_INFO *dinfo, int needed);
boolean rddfa(FILE *fp, DFA_INFO *dinfo);
boolean rddfa_fp(FILE *fp, DFA_INFO *dinfo);
boolean rddfa_line(char *line, DFA_INFO *dinfo, int *state_max, int *arc_num, int *terminal_max);
void dfa_append(DFA_INFO *dst, DFA_INFO *src, int soffset, int coffset);

boolean init_dfa(DFA_INFO *dinfo, char *filename);
WORD_ID dfa_symbol_lookup(DFA_INFO *dinfo, char *terminalname);
boolean extract_cpair(DFA_INFO *dinfo);
boolean cpair_append(DFA_INFO *dst, DFA_INFO *src, int coffset);
void print_dfa_info(FILE *fp, DFA_INFO *dinfo);
void print_dfa_cp(FILE *fp, DFA_INFO *dinfo);
boolean dfa_cp(DFA_INFO *dfa, int i, int j);
boolean dfa_cp_begin(DFA_INFO *dfa, int i);
boolean dfa_cp_end(DFA_INFO *dfa, int i);
void set_dfa_cp(DFA_INFO *dfa, int i, int j, boolean value);
void set_dfa_cp_begin(DFA_INFO *dfa, int i, boolean value);
void set_dfa_cp_end(DFA_INFO *dfa, int i, boolean value);
void init_dfa_cp(DFA_INFO *dfa);
void malloc_dfa_cp(DFA_INFO *dfa, int term_num, int size);
void realloc_dfa_cp(DFA_INFO *dfa, int old_term_num, int new_term_num);
void free_dfa_cp(DFA_INFO *dfa);
void dfa_cp_output_rawdata(FILE *fp, DFA_INFO *dfa);
void dfa_cp_count_size(DFA_INFO *dfa, unsigned long *size_ret, unsigned long *allocsize_ret);
boolean dfa_cp_append(DFA_INFO *dfa, DFA_INFO *src, int offset);

#include <sent/vocabulary.h>
boolean make_dfa_voca_ref(DFA_INFO *dinfo, WORD_INFO *winfo);
void make_terminfo(TERM_INFO *tinfo, DFA_INFO *dinfo, WORD_INFO *winfo);
void free_terminfo(TERM_INFO *tinfo);
void terminfo_append(TERM_INFO *dst, TERM_INFO *src, int coffset, int woffset);
#include <sent/htk_hmm.h>
void dfa_find_pause_word(DFA_INFO *dfa, WORD_INFO *winfo, HTK_HMM_INFO *hmminfo);
boolean dfa_pause_word_append(DFA_INFO *dst, DFA_INFO *src, int coffset);

#ifdef __cplusplus
}
#endif

#endif /* __SENT_DFA_H__ */
