/**
 * @file   init_dfa.c
 * 
 * <JA>
 * @brief  DFA文法ファイルのメモリ上への読み込みとセットアップ
 *
 * 文法をファイルから読み込んで認識処理のためにセットアップします．
 * DFA文法ファイルの読み込みや辞書との対応付け，無音カテゴリ・無音単語の
 * 検出を行います．
 * </JA>
 * 
 * <EN>
 * @brief  Load Grammar file into memory and setup
 *
 * These functions read a grammar from file and setup for recognition process.
 * They read a DFA grammar file, make mapping from word dictionary and
 * find a noise category/word for pause handling.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Tue Feb 15 14:20:43 2005
 *
 * $Revision: 1.6 $
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
#include <sent/vocabulary.h>
#include <sent/htk_hmm.h>

/* read in dfa info from file */
/** 
 * Read in a grammar file and set to DFA grammar structure
 * 
 * @param dinfo [i/o] a blank DFA data
 * @param filename [in] DFA grammar file name
 */
boolean
init_dfa(DFA_INFO *dinfo, char *filename)
{
  FILE *fd;
  
  if ((fd = fopen_readfile(filename)) == NULL) {
    jlog("Error: init_dfa: failed to open %s\n",filename);
    return FALSE;
  }
  if (!rddfa(fd, dinfo)) {
    jlog("Error; init_dfa: error in reading %s\n",filename);
    return FALSE;
  }
  if (fclose_readfile(fd) == -1) {
    jlog("Error: init_dfa: failed to close %s\n", filename);
    return FALSE;
  }

  return TRUE;
}

/** 
 * Make correspondence between all words in dictionary and categories
 * in grammar, both from a word to a category and from a category to words.
 * 
 * @param dinfo [i/o] DFA grammar, category information will be built here.
 * @param winfo [i/o] Word dictionary, word-to-category information will be build here.
 */
boolean
make_dfa_voca_ref(DFA_INFO *dinfo, WORD_INFO *winfo)
{
  WORD_ID i;
  boolean ok_flag = TRUE;

  /* word -> terminal symbol */
  for (i = 0; i < winfo->num; i++) {
    winfo->wton[i] = dfa_symbol_lookup(dinfo, winfo->wname[i]);
    if (winfo->wton[i] == WORD_INVALID) {
      /* error: not found */
      jlog("Error: init_dfa: no such terminal symbol \"%s\" in DFA grammar\n",
	     winfo->wname[i]);
      put_voca(jlog_get_fp(), winfo, i);
      ok_flag = FALSE;
    }
  }

  if (ok_flag) {
    /* terminal symbol -> word */
    make_terminfo(&(dinfo->term), dinfo, winfo);
  }

  return ok_flag;
}

/** 
 * Find pause word and pause category information, and set to the grammar data.
 * 
 * @param dfa [i/o] DFA grammar, @a sp_id and @a is_sp will be built here.
 * @param winfo [in] Word dictionary
 * @param hmminfo [in] HTK %HMM to provide which is short pause %HMM
 */
void
dfa_find_pause_word(DFA_INFO *dfa, WORD_INFO *winfo, HTK_HMM_INFO *hmminfo)
{
  int i, t,p;
  WORD_ID w;

  dfa->sp_id = WORD_INVALID;
  dfa->is_sp = (boolean *)mymalloc(sizeof(boolean) * dfa->term_num);
  for(t=0;t<dfa->term_num;t++) dfa->is_sp[t] = FALSE;
  
  for(t=0;t<dfa->term_num;t++) {
    for(i=0;i<dfa->term.wnum[t]; i++) {
      w = dfa->term.tw[t][i];
      p = 0;
      while(p < winfo->wlen[w] && winfo->wseq[w][p] == hmminfo->sp) p++;
      if (p >= winfo->wlen[w]) {	/* w consists of only hmminfo->sp model */
	dfa->is_sp[t] = TRUE;
	if (dfa->sp_id == WORD_INVALID) dfa->sp_id = w;
	break;			/* mark this category if at least 1 sp_word was found */
      }
    }
  }
}

/** 
 * Append the pause word/category information at the last.
 * 
 * @param dst [i/o] DFA grammar
 * @param src [in] DFA grammar to be appended to @a dst
 * @param coffset appending category point in @a dst
 */
boolean
dfa_pause_word_append(DFA_INFO *dst, DFA_INFO *src, int coffset)
{
  int i;
  /* dst info must be already appended */
  /* [coffset..dst->term_num-1] is the new categories */
  if (dst->term_num - coffset != src->term_num) {
    jlog("Error: init_dfa: appended term num not match!\n");
    return FALSE;
  }
  
  if (dst->is_sp == NULL) {
    dst->is_sp = (boolean *)mymalloc(sizeof(boolean) * dst->term_num);
  } else {
    dst->is_sp = (boolean *)myrealloc(dst->is_sp, sizeof(boolean) * dst->term_num);
  }
  for(i=0;i<src->term_num;i++) {
    dst->is_sp[coffset+i] = src->is_sp[i];
  }
  if (dst->sp_id == WORD_INVALID) {
    if (src->sp_id != WORD_INVALID) {/* src has pause word */
      dst->sp_id = src->sp_id;
    }
  }

  return TRUE;
}

