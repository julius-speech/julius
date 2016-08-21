/*
 * Copyright (c) 1991-2016 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2016 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/ngram2.h>


// output of an N-gram entry, given context (recursive)
//
// fp: output file pointer
// ndata: N-gram
// n: current N for parsing
// cid: context id at (n-1)-gram
// max_n: target N to be output
// wlist: context words
// addsw: TRUE for output of additional N-gram
//
static void out(FILE *fp, NGRAM_INFO *ndata, int n, NNID cid, int max_n, WORD_ID *wlist, boolean addsw)
{
  NGRAM_TUPLE_INFO *t, *tcontext;
  NNID nnid;
  boolean has_bo_wt;
  NNID left, right;
  NNID i;
  WORD_ID w;
  int j;

  
  tcontext = &(ndata->d[n-1]);

  if (n == max_n && addsw == TRUE) {
    // for additional N-gram, reached target N-gram, output it and exit
    fprintf(fp, "%.5f", ndata->p_2[cid]);
    if (ndata->bigram_index_reversed) {
      // 2-gram index is reversed, i.e., order of additional N-gram
      fprintf(fp, " %s", ndata->wname[wlist[0]]);
      fprintf(fp, " %s", ndata->wname[wlist[1]]);
    } else {
      // 2-gram index is forwarded, i.e., order of N-gram
      fprintf(fp, " %s", ndata->wname[wlist[1]]);
      fprintf(fp, " %s", ndata->wname[wlist[0]]);
    }
    fprintf(fp, " \n"); // bogus space at EOL for debugging
    return;
  }

  // get ID to be context of next (n+1)-gram
  nnid = cid;
  has_bo_wt = TRUE;
  if (n < ndata->n) {
    if (tcontext->ct_compaction) {
      nnid = tcontext->nnid2ctid_upper[cid];
      if (nnid == NNID_INVALID_UPPER) {
	has_bo_wt = FALSE;
      } else {
	nnid = (nnid << 16) + (NNID)(tcontext->nnid2ctid_lower[cid]);
      }
    }
  }

  if (n == max_n) {
    // reached target N-gram, output it and exit
    fprintf(fp, "%.5f", tcontext->prob[cid]);
    for (j = 0; j < n; j++) {
      fprintf(fp, " %s", ndata->wname[wlist[j]]);
    }
    if (n < ndata->n) {
      // has larger N-gram, should output back-off weights
      if (has_bo_wt == FALSE) {
	// weights of 0.0 has been eliminated in bingram, so output "0.0"
	fprintf(fp, "  %.4f", 0.0);
      } else {
	fprintf(fp, "  %.4f", tcontext->bo_wt[nnid]);
      }
    } else {
      fprintf(fp, " ");// bogus space at EOL for debugging
    }
    fprintf(fp, "\n");
    return;
  }

  // if no back-off weights, no further (N+1)-gram entry with this entry
  // as context, so stop parsing here
  if (has_bo_wt == FALSE) return;

  // get location of (n+1)-gram entries whose context is this entry
  t = &(ndata->d[n]);
  if (t->is24bit) {
    left = t->bgn_upper[nnid];
    if (left == NNID_INVALID_UPPER) {
      return;
    }
    left = (left << 16) + (NNID)(t->bgn_lower[nnid]);
  } else {
    left = t->bgn[nnid];
    if (left == NNID_INVALID) {
      return;
    }
  }
  right = left + t->num[nnid] - 1;

  // recursive call for the (n+1)-gram entries
  for (i = left; i <= right; i++) {
    w = t->nnid2wid[i];
    wlist[n] = w;
    out(fp, ndata, n + 1, i, max_n, wlist, addsw);
  }
}

static void output_all(FILE *fp, NGRAM_INFO *ndata)
{
  WORD_ID *wlist;
  NNID i;
  int j;
  int max_n;

  fprintf(fp, "\\data\\\n");
  for (j = 0; j < ndata->n; j++) {
    fprintf(fp, "ngram %d=%u\n", j + 1, ndata->d[j].totalnum);
  }
  fprintf(fp, "\n");

  wlist = (WORD_ID *)mymalloc(sizeof(WORD_ID) * ndata->n);
  for (max_n = 1; max_n <= ndata->n; max_n++) {
    fprintf(fp, "\n\\%d-grams:\n", max_n);
    if (ndata->bigram_index_reversed && max_n >= 2) {
      WORD_ID w1, w2;
      NNID nnid;
      for (w1 = 0; w1 < ndata->d[0].totalnum; w1++) {
	for (w2 = 0; w2 < ndata->d[0].totalnum; w2++) {
	  wlist[0] = w1;
	  wlist[1] = w2;
	  nnid = search_ngram(ndata, 2, wlist);
	  if (nnid != NNID_INVALID) {
	    out(fp, ndata, 2, nnid, max_n, wlist, FALSE);
	  }
	}
      }
    } else {
      for (i = 0; i < ndata->d[0].totalnum; i++) {
	wlist[0] = i;
	out(fp, ndata, 1, i, max_n, wlist, FALSE);
      }
    }
  }
  free(wlist);
  fprintf(fp, "\n\\end\\\n");
}

static void output_additional_bigram(FILE *fp, NGRAM_INFO *ndata)
{
  NNID nid;
  NGRAM_TUPLE_INFO *t;
  WORD_ID wlist[2];
  NNID i;
  int j;

  fprintf(fp, "\\data\\\n");
  for (j = 0; j < 2; j++) {
    fprintf(fp, "ngram %d=%u\n", j + 1, ndata->d[j].totalnum);
  }
  fprintf(fp, "\n");

  fprintf(fp, "\n\\1-grams:\n");
  t = &(ndata->d[0]);
  for (nid = 0; nid < t->totalnum; nid++) {
    fprintf(fp, "%.5f %s  %.4f\n", t->prob[nid], ndata->wname[nid], ndata->bo_wt_1[nid]);
  }

  fprintf(fp, "\n\\2-grams:\n");
  if (!ndata->bigram_index_reversed) {
    WORD_ID w1, w2;
    NNID nnid;
    for (w1 = 0; w1 < ndata->d[0].totalnum; w1++) {
      for (w2 = 0; w2 < ndata->d[0].totalnum; w2++) {
	wlist[0] = w2;
	wlist[1] = w1;
	nnid = search_ngram(ndata, 2, wlist);
	if (nnid != NNID_INVALID) {
	  fprintf(fp, "%.5f %s %s \n", ndata->p_2[nnid], ndata->wname[w1], ndata->wname[w2]); // last space for debug
	}
      }
    }
  } else {
    for (i = 0; i < ndata->d[0].totalnum; i++) {
      wlist[0] = i;
      out(fp, ndata, 1, i, 2, wlist, TRUE);
    }
  }
  fprintf(fp, "\n\\end\\\n");
}


boolean ngram_write_arpa(NGRAM_INFO *ndata, FILE *fp, FILE *fp_rev)
{
  output_all(fp, ndata);

  if (fp_rev != NULL && ndata->bo_wt_1 != NULL) {
    output_additional_bigram(fp_rev, ndata);
  }
  
  return TRUE;
}
