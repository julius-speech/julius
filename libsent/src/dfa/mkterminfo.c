/**
 * @file   mkterminfo.c
 * 
 * <JA>
 * @brief  カテゴリごとの単語のリストを作成する
 * </JA>
 * 
 * <EN>
 * @brief  Make a word list for each category
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Tue Feb 15 14:47:27 2005
 *
 * $Revision: 1.5 $
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

/* construct table: terminal(category) ID -> word list */

/** 
 * Make a word list for each category.
 * 
 * @param tinfo [i/o] terminal data structure to hold the result
 * @param dinfo [in] DFA gammar to supply the number of category in the grammar
 * @param winfo [in] word dictionary.
 */
void
make_terminfo(TERM_INFO *tinfo, DFA_INFO *dinfo, WORD_INFO *winfo)
{
  int i,w,t;
  int tnum;

  /* set terminal number */
  tnum = tinfo->term_num = dinfo->term_num;
  /* allocate memory for the list of word num */
  tinfo->wnum = (int *)mymalloc(sizeof(int) * tnum);
  /* count number of belonging words for each category */
  for(i=0;i<tnum;i++) tinfo->wnum[i]=0;
  for(w=0;w<winfo->num;w++) {
    tinfo->wnum[winfo->wton[w]]++;
  }
  /* allocate memory for the list body */
  tinfo->tw = (WORD_ID **)mymalloc(sizeof(WORD_ID *) * tnum);
  for(i=0;i<tnum;i++) {
    tinfo->tw[i] = (WORD_ID *)mymalloc(sizeof(WORD_ID) * tinfo->wnum[i]);
  }
  /* store the word to each corresponding list */
  for(i=0;i<tnum;i++) tinfo->wnum[i]=0;
  for(w=0;w<winfo->num;w++) {
    t = winfo->wton[w];
    tinfo->tw[t][tinfo->wnum[t]] = w;
    tinfo->wnum[t]++;
  }

}

/** 
 * Free word list for each category.
 * 
 * @param tinfo [in] terminal data structure holding the content.
 * 
 */
void
free_terminfo(TERM_INFO *tinfo)
{
  int i;

  if (tinfo->tw != NULL) {
    for(i=0;i<tinfo->term_num;i++) {
      free(tinfo->tw[i]);
    }
    free(tinfo->tw);
    free(tinfo->wnum);
  }
}

/** 
 * Append the terminal(category) word list.
 * 
 * @param dst [i/o] category data
 * @param src [i/o] category data to be appended to @a dst
 * @param coffset [in] category id offset in @a dst where the new data should be stored
 * @param woffset [in] word id offset where the new data should be stored
 */
void
terminfo_append(TERM_INFO *dst, TERM_INFO *src, int coffset, int woffset)
{
  int t, new_termnum;
  int i, j;

  new_termnum = coffset + src->term_num;
  if (dst->tw == NULL) {
    dst->tw = (WORD_ID **)mymalloc(sizeof(WORD_ID *) * new_termnum);
    dst->wnum = (int *)mymalloc(sizeof(int) * new_termnum);
  } else {
    dst->tw = (WORD_ID **)myrealloc(dst->tw, sizeof(WORD_ID *) * new_termnum);
    dst->wnum = (int *)myrealloc(dst->wnum, sizeof(int) * new_termnum);
  }
  for(i=0;i<src->term_num;i++) {
    t = i + coffset;
    dst->wnum[t] = src->wnum[i];
    dst->tw[t] = (WORD_ID *)mymalloc(sizeof(WORD_ID) * src->wnum[i]);
    for(j=0;j<src->wnum[i];j++) {
      dst->tw[t][j] = src->tw[i][j] + woffset;
    }
  }
  dst->term_num = new_termnum;
}
