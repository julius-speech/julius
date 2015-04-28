/**
 * @file   ngram_util.c
 * 
 * <JA>
 * @brief  N-gramの情報をテキスト出力する
 * </JA>
 * 
 * <EN>
 * @brief  Output some N-gram information to stdout
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 17:18:55 2005
 *
 * $Revision: 1.10 $
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

/**
 * Get the work area size of an N-gram tuple.
 * 
 * @param t [in] N-gram tuple structure
 * 
 * @return the size in bytes
 * 
 */
static unsigned int
get_ngram_tuple_bytes(NGRAM_TUPLE_INFO *t)
{
  unsigned int size, unit;

  size = 0;
  if (t->num != NULL) {		/* other than 1-gram */
    /* bgn */
    if (t->is24bit) {
      unit = sizeof(NNID_UPPER) + sizeof(NNID_LOWER);
    } else {
      unit = sizeof(NNID);
    }
    /* num */
    unit += sizeof(WORD_ID);
    size += unit * t->bgnlistlen;
  }
  /* prob */
  unit = sizeof(LOGPROB);
  /* nnid2wid */
  if (t->nnid2wid) unit += sizeof(WORD_ID);
  size += unit * t->totalnum;

  if (t->bo_wt) {
    if (t->ct_compaction) {
      /* nnid2ctid */
      unit = sizeof(NNID_UPPER) + sizeof(NNID_LOWER);
      size += unit * t->totalnum;
    }
    /* bo_wt */
    size += sizeof(LOGPROB) * t->context_num;
  }

  return size;
}

/** 
 * Output misccelaneous information of N-gram to standard output.
 *
 * @param fp [in] file pointer
 * @param ndata [in] N-gram data
 */
void
print_ngram_info(FILE *fp, NGRAM_INFO *ndata)
{
  int i;
  fprintf(fp, " N-gram info:\n");
  //fprintf(fp, "\t  struct version = %d\n", ndata->version);

  fprintf(fp, "\t            spec = %d-gram", ndata->n);
  if (ndata->dir == DIR_RL) {
    fprintf(fp, ", backward (right-to-left)\n");
  } else {
    fprintf(fp, ", forward (left-to-right)\n");
  }
  if (ndata->isopen) {
    fprintf(fp, "\t        OOV word = %s(id=%d)\n", ndata->wname[ndata->unk_id],ndata->unk_id);
    if (ndata->unk_num != 0) {
      fprintf(fp, "\t        OOV size = %d words in dict\n", ndata->unk_num);
    }
  } else {
    fprintf(fp, "\t        OOV word = none (assume close vocabulary)\n");
  }
  fprintf(fp, "\t    wordset size = %d\n", ndata->max_word_num);
  for(i=0;i<ndata->n;i++) {
    fprintf(fp, "\t  %d-gram entries = %10lu  (%5.1f MB)", i+1, (long unsigned int)ndata->d[i].totalnum, get_ngram_tuple_bytes(&(ndata->d[i])) / 1048576.0);
    if (ndata->d[i].bo_wt != NULL && ndata->d[i].totalnum != ndata->d[i].context_num) {
      fprintf(fp, " (%d%% are valid contexts)", ndata->d[i].context_num * 100 / ndata->d[i].totalnum);
    }
    fprintf(fp, "\n");
  }

  if (ndata->bo_wt_1) {
    fprintf(fp, "\tLR 2-gram entries= %10lu  (%5.1f MB)\n", (long unsigned int)ndata->d[1].totalnum,
	    (sizeof(LOGPROB) * ndata->d[1].totalnum + sizeof(LOGPROB) * ndata->d[0].context_num) / 1048576.0);
  }
  fprintf(fp, "\t           pass1 = ");
  if (ndata->dir == DIR_RL) {
    if (ndata->bo_wt_1) {
      fprintf(fp, "given additional forward 2-gram\n");
    } else {
      fprintf(fp, "estimate 2-gram from the backward 2-gram\n");
    }
  } else {
    fprintf(fp, "2-gram in the forward n-gram\n");
  }
}
