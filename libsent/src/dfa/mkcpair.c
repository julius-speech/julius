/**
 * @file   mkcpair.c
 * 
 * <JA>
 * @brief  文法からカテゴリ対制約を抽出する
 *
 * DFA文法から，認識処理の第１パスで用いるカテゴリ間の接続制約を抽出します．
 * 抽出したカテゴリ対情報は文法データ内に格納されます．
 *
 * ショートポーズ単語が文法上で指定されている場合，接続制約はこの
 * ショートポーズ単語のスキップを考慮して抽出されます．
 * 
 * なお，ここでは処理上の制限から，このスキップ可能なショートポーズ単語が
 * 文頭およぶ文末に出現することは許されていません．
 * 入力開始・終端の無音部分にあたる単語は，silB, silE など
 * このスキップ可能なショートポーズ以外の単語を割り当てるようにしてください
 * </JA>
 * 
 * <EN>
 * @brief  Extract category-pair constraint from DFA grammar
 *
 * These functions extract whether the each grammar category can be connected
 * or not in the given DFA grammar, and store the extracted data to the DFA
 * grammar information.  This category-pair constraint will be used at the
 * first pass of recognition as a degenerated linguistic constraint.
 *
 * If a short pause word is defined in the grammar, the connection constraint
 * will be extracted considering the skipping of this pause model, since the
 * pause word may not appear on the specified location in the actual utterance.
 *
 * Please note that a grammar rule that allows such skippable short-pause word
 * to appear at the beginning and end of sentence is prohibited.  Instead,
 * you should use another (non-skippable) silence word like "sil" as the
 * beginning and ending of sentence to match the head and tail silence.
 * 
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Tue Feb 15 14:35:33 2005
 *
 * $Revision: 1.7 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/dfa.h>

/** 
 * Extract category-pair constraint from DFA grammar and newly set the category
 * pair matrix of the give DFA.
 * 
 * @param dinfo [i/o] DFA grammar, in which the category-pair matrix will be created.
 */
boolean
extract_cpair(DFA_INFO *dinfo)
{
  int i;
  DFA_ARC *arc_l, *arc_r, *arc_r2;
  int left, right;
  int size;

  /* initialize */
  /* initial size = average fun-out num per state */
  size = dinfo->arc_num / dinfo->state_num;
  if (size < DFA_CP_MINSTEP) size = DFA_CP_MINSTEP;
  malloc_dfa_cp(dinfo, dinfo->term_num, size);

  /* extract cpair info */
  for (i=0;i<dinfo->state_num;i++) {
    if ((dinfo->st[i].status & INITIAL_S) != 0) { /* arc from initial state */
      for (arc_r = dinfo->st[i].arc; arc_r; arc_r = arc_r->next) {
	if (dinfo->is_sp[arc_r->label]) {
	  jlog("Error: mkcpair: skippable sp should not appear at end of sentence\n");
	  return FALSE;
	}
	set_dfa_cp_end(dinfo, arc_r->label, TRUE);
      }
    }
    for(arc_l = dinfo->st[i].arc; arc_l; arc_l = arc_l->next) {
      left = arc_l->label;
      if ((dinfo->st[arc_l->to_state].status & ACCEPT_S) != 0) {/* arc to accept state */
	if (dinfo->is_sp[left]) {
	  jlog("Error: mkcpair: skippable sp should not appear at beginning of sentence\n");
	  return FALSE;
	}
	set_dfa_cp_begin(dinfo, left, TRUE);
      }
      /* others */
      for (arc_r = dinfo->st[arc_l->to_state].arc; arc_r; arc_r = arc_r->next) {
	right = arc_r->label;
	set_dfa_cp(dinfo, right, left, TRUE);
	if (dinfo->is_sp[right]) {
	  for (arc_r2 = dinfo->st[arc_r->to_state].arc; arc_r2; arc_r2 = arc_r2->next) {
	    if (dinfo->is_sp[arc_r2->label]) { /* sp model continues twice */
	      jlog("Error: mkcpair: skippable sp should not repeat\n");
	      return FALSE;
	    }
	    set_dfa_cp(dinfo, arc_r2->label, left, TRUE);
	  }
	}
      }

    }
  }

  return TRUE;
}

/** 
 * Append the category pair matrix at the last.
 * 
 * @param dst [i/o] DFA grammar
 * @param src [in] DFA grammar to be appended to @a dst
 * @param coffset [in] category id in @a dst where the new data should be stored
 */
boolean
cpair_append(DFA_INFO *dst, DFA_INFO *src, int coffset)
{
  /* dst info must be already appended */
  /* [coffset..dst->term_num-1] is the new categories */
  if (dst->term_num - coffset != src->term_num) {
    jlog("Error: mkcpair: append term num not match!: %d, %d, %d\n",
	       dst->term_num, src->term_num, coffset);
    return FALSE;
  }

  /* allocate appended area */
  if (dfa_cp_append(dst, src, coffset) == FALSE) return FALSE;

  return TRUE;
}
