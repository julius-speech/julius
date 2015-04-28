/**
 * @file   voca_util.c
 * 
 * <JA>
 * @brief  単語辞書の情報をテキストで出力
 * </JA>
 * 
 * <EN>
 * @brief  Output text informations about the grammar
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Fri Feb 18 21:41:41 2005
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
#include <sent/vocabulary.h>

/** 
 * Output overall word dictionary information to stdout.
 *
 * @param fp [in] file descriptor
 * @param winfo [in] word dictionary
 */
void
print_voca_info(FILE *fp, WORD_INFO *winfo)
{
  if (fp == NULL) return;

  fprintf(fp, " Vocabulary Info:\n");
  fprintf(fp, "        vocabulary size  = %d words, %d models\n", winfo->num, winfo->totalmodelnum);
  fprintf(fp, "        average word len = %.1f models, %.1f states\n", (float)winfo->totalmodelnum/(float)winfo->num, (float)winfo->totalstatenum/(float)winfo->num);
  fprintf(fp, "       maximum state num = %d nodes per word\n", winfo->maxwn);
  fprintf(fp, "       transparent words = ");
  if (winfo->totaltransnum > 0) {
    fprintf(fp, "%d words\n", winfo->totaltransnum);
  } else {
    fprintf(fp, "not exist\n");
  }
#ifdef CLASS_NGRAM
  fprintf(fp, "       words under class = ");
  if (winfo->cwnum > 0) {
    fprintf(fp, "%d words\n", winfo->cwnum);
  } else {
    fprintf(fp, "not exist\n");
  }    
#endif
}
	 
/** 
 * Output information of a word in dictionary to stdout.
 * 
 * @param fp [in] file descriptor
 * @param winfo [in] word dictionary
 * @param wid [in] word id to be output
 */
void
put_voca(FILE *fp, WORD_INFO *winfo, WORD_ID wid)
{
  int i;
  HMM_Logical *lg;

  if (fp == NULL) return;
  
  fprintf(fp, "%d: \"%s", wid, winfo->wname[wid]);
#ifdef CLASS_NGRAM
  fprintf(fp, " @%f", winfo->cprob[wid]);
#endif
  if (winfo->is_transparent[wid]) {
    fprintf(fp, " {%s}", winfo->woutput[wid]);
  } else {
    fprintf(fp, " [%s]", winfo->woutput[wid]);
  }
  for(i=0;i<winfo->wlen[wid];i++) {
    lg = winfo->wseq[wid][i];
    fprintf(fp, " %s", lg->name);
    if (lg->is_pseudo) {
      fprintf(fp, "(pseudo)");
    } else {
      fprintf(fp, "(%s)", lg->body.defined->name);
    }
  }
  fprintf(fp, "\"\n");
}
