/**
 * @file   voca_malloc.c
 * 
 * <JA>
 * @brief  単語辞書構造体のメモリ割り付けと解放
 * </JA>
 * 
 * <EN>
 * @brief  Memory allocation of word dictionary information
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Fri Feb 18 21:33:29 2005
 *
 * $Revision: 1.11 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/vocabulary.h>


/** 
 * Allocate a new word dictionary structure.
 * 
 * @return pointer to the newly allocated WORD_INFO.
 */
WORD_INFO *
word_info_new()
{
  WORD_INFO *new;

  new = (WORD_INFO *)mymalloc(sizeof(WORD_INFO));
  new->mroot = NULL;
  new->work = NULL;

  new->wname = NULL;
  new->woutput = NULL;
  new->wlen = NULL;
  new->wton = NULL;
#ifdef CLASS_NGRAM
  new->cprob = NULL;
#endif
  new->is_transparent = NULL;
#ifdef USE_MBR
  new->weight = NULL;
#endif

  return(new);
}

/** 
 * Free all informations in the WORD_INFO.
 * 
 * @param winfo [i/o] word dictionary data to be freed.
 */
void
word_info_free(WORD_INFO *winfo)
{
  /* free each word info */
  if (winfo->mroot != NULL) mybfree2(&(winfo->mroot));
  /* free word info */
  if (winfo->wname != NULL) free(winfo->wname);
  if (winfo->woutput != NULL) free(winfo->woutput);
  if (winfo->wlen != NULL) free(winfo->wlen);
  if (winfo->wton != NULL) free(winfo->wton);
#ifdef CLASS_NGRAM
  if (winfo->cprob != NULL) free(winfo->cprob);
#endif
  if (winfo->is_transparent != NULL) free(winfo->is_transparent);
  /* free whole */
#ifdef USE_MBR
  if (winfo->weight != NULL) free(winfo->weight);
#endif
  free(winfo);
}

/** 
 * Initialize a new word dictionary structure.
 * 
 * @param winfo [i/o] word dictionary to be initialized.
 */
void
winfo_init(WORD_INFO *winfo)
{
  int n;
  
  n = MAXWSTEP;
  winfo->wlen = (unsigned char *)mymalloc(sizeof(unsigned char)*n);
  winfo->wname = (char **)mymalloc(sizeof(char *)*n);
  winfo->woutput = (char **)mymalloc(sizeof(char *)*n);
  winfo->wseq = (HMM_Logical ***)mymalloc(sizeof(HMM_Logical **)*n);
  winfo->wton = (WORD_ID *)mymalloc(sizeof(WORD_ID)*n);
#ifdef CLASS_NGRAM
  winfo->cprob = (LOGPROB *)mymalloc(sizeof(LOGPROB)*n);
  winfo->cwnum = 0;
#endif
  winfo->is_transparent = (boolean *)mymalloc(sizeof(boolean)*n);
  winfo->maxnum = n;
  winfo->num = 0;
  winfo->head_silwid = winfo->tail_silwid = WORD_INVALID;
  winfo->maxwn = 0;
  winfo->maxwlen = 0;
  winfo->errnum = 0;
  winfo->errph_root = NULL;
}

/** 
 * Expand the word dictionary.
 * 
 * @param winfo [i/o] word dictionary to be expanded.
 */
boolean
winfo_expand(WORD_INFO *winfo)
{
  int n;

  n = winfo->maxnum;
  if (n >= MAX_WORD_NUM) {
    jlog("Error: voca_malloc: maximum dict size exceeded limit (%d)\n", MAX_WORD_NUM);
    return FALSE;
  }
  n *= 2;
  if (n > MAX_WORD_NUM) n = MAX_WORD_NUM;

  winfo->wlen = (unsigned char *)myrealloc(winfo->wlen, sizeof(unsigned char)*n);
  winfo->wname = (char **)myrealloc(winfo->wname, sizeof(char *)*n);
  winfo->woutput = (char **)myrealloc(winfo->woutput, sizeof(char *)*n);
  winfo->wseq = (HMM_Logical ***)myrealloc(winfo->wseq, sizeof(HMM_Logical **)*n);
  winfo->wton = (WORD_ID *)myrealloc(winfo->wton, sizeof(WORD_ID)*n);
#ifdef CLASS_NGRAM
  winfo->cprob = (LOGPROB *)myrealloc(winfo->cprob, sizeof(LOGPROB)*n);
#endif
  winfo->is_transparent = (boolean *)myrealloc(winfo->is_transparent, sizeof(boolean)*n);

#ifdef USE_MBR
  if (winfo->weight)
    winfo->weight = (LOGPROB *)myrealloc(winfo->weight, sizeof(LOGPROB)*n);
#endif

  winfo->maxnum = n;

  return TRUE;
}

