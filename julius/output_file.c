/**
 * @file   output_file.c
 * 
 * <EN>
 * @brief  Output recognition result to each separate file
 * </EN>
 * 
 * <JA>
 * @brief  認識結果を個別ファイルへ出力する. 
 * </JA>
 * 
 * @author Akinobu Lee
 * @date   Wed Dec 12 11:07:46 2007
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

#include "app.h"

/**
 * Assumed tty width for graph view output
 * 
 */
#define TEXTWIDTH 70
#define MAXBUFLEN 4096 ///< Maximum line length of a message sent from a client

static char fname[MAXBUFLEN];
static FILE *fp = NULL;

void
outfile_set_fname(char *input_filename)
{
  char *p;

  strncpy(fname, input_filename, MAXBUFLEN);
  if ((p = strrchr(fname, '.')) != NULL) {
    *p = '\0';
  }
  strcat(fname, OUTPUT_FILE_SUFFIX);
}

static void
outfile_open(Recog *recog, void *dummy)
{
  if ((fp = fopen(fname, "w")) == NULL) {
    fprintf(stderr, "output_rec: failed to open \"%s\", result not saved\n", fname);
    return;
  }
}

static void
outfile_close(Recog *recog, void *dummy)
{
  if (fp != NULL) {
    fclose(fp);
    fprintf(stderr, "result written to \"%s\"\n", fname);
  }
  fp = NULL;
}

static void
outfile_sentence(Recog *recog, void *dummy)
{
  RecogProcess *r;
  Sentence *s;
  WORD_INFO *winfo;
  WORD_ID *seq;
  int seqnum;
  int n, num;
  int i, j;
  boolean multi;
  static char phbuf[MAX_HMMNAME_LEN];
  SentenceAlign *align;
  HMM_Logical *p;

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    if (multi) fprintf(fp, "[#%d %s]\n", r->config->id, r->config->name);
  
    if (r->result.status < 0) {
      switch(r->result.status) {
      case J_RESULT_STATUS_REJECT_POWER:
	fprintf(fp, "<input rejected by power>\n");
	break;
      case J_RESULT_STATUS_TERMINATE:
	fprintf(fp, "<input teminated by request>\n");
	break;
      case J_RESULT_STATUS_ONLY_SILENCE:
	fprintf(fp, "<input rejected by decoder (silence input result)>\n");
	break;
      case J_RESULT_STATUS_REJECT_GMM:
	fprintf(fp, "<input rejected by GMM>\n");
	break;
      case J_RESULT_STATUS_REJECT_SHORT:
	fprintf(fp, "<input rejected by short input>\n");
	break;
      case J_RESULT_STATUS_REJECT_LONG:
	fprintf(fp, "<input rejected by long input>\n");
	break;
      case J_RESULT_STATUS_FAIL:
	fprintf(fp, "<search failed>\n");
	break;
      }
      continue;
    }

    winfo = r->lm->winfo;
    num = r->result.sentnum;

    for(n=0;n<num;n++) {
      s = &(r->result.sent[n]);
      seq = s->word;
      seqnum = s->word_num;
      
      fprintf(fp, "sentence%d:", n+1);
      for (i=0;i<seqnum;i++) {
	fprintf(fp, " %s", winfo->woutput[seq[i]]);
      }
      fprintf(fp, "\n");  

      fprintf(fp, "wseq%d:", n+1);
      for (i=0;i<seqnum;i++) {
	fprintf(fp, " %s", winfo->wname[seq[i]]);
      }
      fprintf(fp, "\n");  
      
      fprintf(fp, "phseq%d:", n+1);
      for (i=0;i<seqnum;i++) {
	if (i > 0) fprintf(fp, " |");
	for (j=0;j<winfo->wlen[seq[i]];j++) {
	  center_name(winfo->wseq[seq[i]][j]->name, phbuf);
	  fprintf(fp, " %s", phbuf);
	}
      }
      fprintf(fp, "\n");  

#ifdef CONFIDENCE_MEASURE
      fprintf(fp, "cmscore%d:", n+1);
      for (i=0;i<seqnum;i++) {
	fprintf(fp, " %5.3f", s->confidence[i]);
      }
      fprintf(fp, "\n");  
#endif
#ifdef USE_MBR
      if(r->config->mbr.use_mbr == TRUE){

	fprintf(fp, "MBRscore%d: %f\n", n+1, s->score_mbr);
      }
#endif

      fprintf(fp, "score%d: %f", n+1, s->score);
      if (r->lmtype == LM_PROB) {
	fprintf(fp, " (AM: %f  LM: %f)", s->score_am, s->score_lm);
      }
      fprintf(fp, "\n");
      if (r->lmtype == LM_DFA) {
	if (multigram_get_all_num(r->lm) > 1) {
	  fprintf(fp, "grammar%d: %d\n", n+1, s->gram_id);
	}
      }
      /* output alignment result if exist */
      for (align = s->align; align; align = align->next) {
	fprintf(fp, "=== begin forced alignment ===\n");
	switch(align->unittype) {
	case PER_WORD:
	  fprintf(fp, "-- word alignment --\n"); break;
	case PER_PHONEME:
	  fprintf(fp, "-- phoneme alignment --\n"); break;
	case PER_STATE:
	  fprintf(fp, "-- state alignment --\n"); break;
	}
	fprintf(fp, " id: from  to    n_score    unit\n");
	fprintf(fp, " ----------------------------------------\n");
	for(i=0;i<align->num;i++) {
	  fprintf(fp, "[%4d %4d]  %f  ", align->begin_frame[i], align->end_frame[i], align->avgscore[i]);
	  switch(align->unittype) {
	  case PER_WORD:
	    fprintf(fp, "%s\t[%s]\n", winfo->wname[align->w[i]], winfo->woutput[align->w[i]]);
	    break;
	  case PER_PHONEME:
	    p = align->ph[i];
	    if (p->is_pseudo) {
	      fprintf(fp, "{%s}\n", p->name);
	    } else if (strmatch(p->name, p->body.defined->name)) {
	      fprintf(fp, "%s\n", p->name);
	    } else {
	      fprintf(fp, "%s[%s]\n", p->name, p->body.defined->name);
	    }
	    break;
	  case PER_STATE:
	    p = align->ph[i];
	    if (p->is_pseudo) {
	      fprintf(fp, "{%s}", p->name);
	    } else if (strmatch(p->name, p->body.defined->name)) {
	      fprintf(fp, "%s", p->name);
	    } else {
	      fprintf(fp, "%s[%s]", p->name, p->body.defined->name);
	    }
	    if (r->am->hmminfo->multipath) {
	      if (align->is_iwsp[i]) {
		fprintf(fp, " #%d (sp)\n", align->loc[i]);
	      } else {
		fprintf(fp, " #%d\n", align->loc[i]);
	      }
	    } else {
	      fprintf(fp, " #%d\n", align->loc[i]);
	    }
	    break;
	  }
	}

	fprintf(fp, "re-computed AM score: %f\n", align->allscore);
	
	fprintf(fp, "=== end forced alignment ===\n");
      }
    }
  }

}

static void
outfile_gmm(Recog *recog, void *dummy)
{
  HTK_HMM_Data *d;
  GMMCalc *gc;
  int i;

  gc = recog->gc;
  
  fprintf(fp, "--- GMM result begin ---\n");
  i = 0;
  for(d=recog->gmm->start;d;d=d->next) {
    fprintf(fp, "  [%8s: total=%f avg=%f]\n", d->name, gc->gmm_score[i], gc->gmm_score[i] / (float)gc->framecount);
    i++;
  }
  fprintf(fp, "  max = \"%s\"", gc->max_d->name);
#ifdef CONFIDENCE_MEASURE
  fprintf(fp, " (CM: %f)", gc->gmm_max_cm);
#endif
  fprintf(fp, "\n");
  fprintf(fp, "--- GMM result end ---\n");
}

static void
outfile_graph(Recog *recog, void *dummy)
{
  WordGraph *wg;
  int tw1, tw2, i;
  WORD_INFO *winfo;
  RecogProcess *r;
  boolean multi;

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    if (r->result.wg == NULL) continue;	/* no graphout specified */
    if (multi) fprintf(fp, "[#%d %s]\n", r->config->id, r->config->name);

    winfo = r->lm->winfo;

    /* debug: output all graph word info */
    wordgraph_dump(fp, r->result.wg, winfo);

    fprintf(fp, "-------------------------- begin wordgraph show -------------------------\n");
    for(wg=r->result.wg;wg;wg=wg->next) {
      tw1 = (TEXTWIDTH * wg->lefttime) / r->peseqlen;
      tw2 = (TEXTWIDTH * wg->righttime) / r->peseqlen;
      fprintf(fp, "%4d:", wg->id);
      for(i=0;i<tw1;i++) fprintf(fp, " ");
      fprintf(fp, " %s\n", winfo->woutput[wg->wid]);
      fprintf(fp, "%4d:", wg->lefttime);
      for(i=0;i<tw1;i++) fprintf(fp, " ");
      fprintf(fp, "|");
      for(i=tw1+1;i<tw2;i++) fprintf(fp, "-");
      fprintf(fp, "|\n");
    }
    fprintf(fp, "-------------------------- end wordgraph show ---------------------------\n");
  }
}

static void
outfile_confnet(Recog *recog, void *dummy)
{
  CN_CLUSTER *c;
  int i;
  RecogProcess *r;
  boolean multi;

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    if (r->result.confnet == NULL) continue;	/* no confnet obtained */
    if (multi) fprintf(fp, "[#%d %s]\n", r->config->id, r->config->name);

    fprintf(fp, "---- begin confusion network ---\n");
    for(c=r->result.confnet;c;c=c->next) {
      for(i=0;i<c->wordsnum;i++) {
	fprintf(fp, "(%s:%.3f)", (c->words[i] == WORD_INVALID) ? "-" : r->lm->winfo->woutput[c->words[i]], c->pp[i]);
	if (i == 0) fprintf(fp, "  ");
      }
      fprintf(fp, "\n");
    }
    fprintf(fp, "---- end confusion network ---\n");
  }
}

/******************************************************************/
void
setup_output_file(Recog *recog, void *data)
{
  callback_add(recog, CALLBACK_EVENT_RECOGNITION_BEGIN, outfile_open, data);
  callback_add(recog, CALLBACK_EVENT_RECOGNITION_END, outfile_close, data);
  callback_add(recog, CALLBACK_RESULT, outfile_sentence, data);
  callback_add(recog, CALLBACK_RESULT_GMM, outfile_gmm, data);
  callback_add(recog, CALLBACK_RESULT_GRAPH, outfile_graph, data);
  callback_add(recog, CALLBACK_RESULT_CONFNET, outfile_confnet, data);
}  

