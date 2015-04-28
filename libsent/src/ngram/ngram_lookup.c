/**
 * @file   ngram_lookup.c
 * 
 * <JA>
 * @brief  N-gram上の語彙エントリの検索
 * </JA>
 * 
 * <EN>
 * @brief  Look up N-gram entries from its name string
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 16:42:38 2005
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
#include <sent/ngram2.h>
#include <sent/ptree.h>

/** 
 * Make index tree for searching N-gram ID from the entry name.
 * 
 * @param ndata [in] N-gram data
 */
void
ngram_make_lookup_tree(NGRAM_INFO *ndata)
{
  int i;
  int *windex;
  char **wnameindex;
  
  windex = (int *)mymalloc(sizeof(int)*ndata->max_word_num);
  for (i=0;i<ndata->max_word_num;i++) {
    windex[i] = i;
  }
  wnameindex = (char **)mymalloc(sizeof(char *)*ndata->max_word_num);
  for (i=0;i<ndata->max_word_num;i++) {
    wnameindex[i] = ndata->wname[i];
  }

  ndata->root = make_ptree(wnameindex, windex, ndata->max_word_num, 0, &(ndata->mroot));

  free(windex);
  free(wnameindex);
}

/** 
 * Look up N-gram ID by entry name.
 * 
 * @param ndata [in] N-gram data
 * @param wordstr [in] entry name to search
 * 
 * @return the found class/word ID, or WORD_INVALID if not found.
 */
WORD_ID
ngram_lookup_word(NGRAM_INFO *ndata, char *wordstr)
{
  int data;
  data = ptree_search_data(wordstr, ndata->root);
  if (data == -1 || strcmp(wordstr, ndata->wname[data]) != 0) {
    return WORD_INVALID;
  } else {
    return(data);
  }
}

/** 
 * Return N-gram ID of entry name, or unknown class ID if not found.
 * 
 * @param ndata [in] N-gram data
 * @param wstr [in] entry name to search
 * 
 * @return the found class/word ID, or unknown ID if not found.
 */
WORD_ID
make_ngram_ref(NGRAM_INFO *ndata, char *wstr)
{
  WORD_ID nw;

  nw = ngram_lookup_word(ndata, wstr);
  if (nw == WORD_INVALID) {	/* not found */
    if (ndata->isopen) {
      jlog("Warning: ngram_lookup: \"%s\" not exist in N-gram, treat as unknown\n", wstr);
      return(ndata->unk_id);
    } else {
      jlog("Error: ngram_lookup: \"%s\" not exist in N-gram\n", wstr);
      return WORD_INVALID;
    }
  } else {
    return(nw);
  }
}
