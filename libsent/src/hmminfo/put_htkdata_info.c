/**
 * @file   put_htkdata_info.c
 * 
 * <JA>
 * @brief  %HMM 定義や特徴パラメータの情報をテキスト出力する
 * </JA>
 * 
 * <EN>
 * @brief  Output %HMM and parameter information to standard out
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Tue Feb 15 23:36:00 2005
 *
 * $Revision: 1.9 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/hmm_calc.h>


/** 
 * Output transition matrix.
 * 
 * @param fp [in] file descriptor
 * @param t [in] pointer to a transion matrix
 */
void
put_htk_trans(FILE *fp, HTK_HMM_Trans *t)
{
  int i,j;

  if (fp == NULL) return;
  if (t == NULL) {
    fprintf(fp, "no transition\n");
  } else {
    for (i=0;i<t->statenum;i++) {
      for (j=0;j<t->statenum;j++) {
	fprintf(fp, " %e", t->a[i][j]);
      }
      fprintf(fp, "\n");
    }
  }
}

/** 
 * Output variance vector (diagonal).
 * 
 * @param fp [in] file descriptor
 * @param v [in] pointer to a variance data
 */
void
put_htk_var(FILE *fp, HTK_HMM_Var *v)
{
  int i;

  if (fp == NULL) return;
  if (v == NULL) {
    fprintf(fp, "no covariance\n");
  } else {
    fprintf(fp, "variance(%d): (may be inversed)", v->len);
    for (i=0;i<v->len;i++) {
      fprintf(fp, " %e", v->vec[i]);
    }
    fprintf(fp, "\n");
  }
}

/** 
 * Output a density information, mean and variance.
 * 
 * @param fp [in] file descriptor
 * @param d [in] pointer to a density data
 */
void
put_htk_dens(FILE *fp, HTK_HMM_Dens *d)
{
  int i;
  
  if (fp == NULL) return;
  if (d == NULL) {
    fprintf(fp, "no dens\n");
  } else {
    fprintf(fp, "mean(%d):", d->meanlen);
    for (i=0;i<d->meanlen;i++) {
      fprintf(fp, " %e", d->mean[i]);
    }
    fprintf(fp, "\n");
    put_htk_var(fp, d->var);
    fprintf(fp, "gconst: %e\n", d->gconst);
  }
}

/** 
 * Output a mixture component.
 * 
 * @param fp [in] file descriptor
 * @param m [in] pointer to %HMM mixture PDF
 */
void
put_htk_mpdf(FILE *fp, HTK_HMM_PDF *m)
{
  int i;
  GCODEBOOK *book;

  if (m == NULL) {
    fprintf(fp, "no mixture pdf\n");
    return;
  }
  if (m->name != NULL) fprintf(fp, "  [~p \"%s\"] (stream %d)\n", m->name, m->stream_id + 1);
  if (m->tmix) {
    book = (GCODEBOOK *)m->b;
    fprintf(fp, "  tmix codebook = \"%s\" (size=%d)\n", book->name, book->num);
    for (i=0;i<m->mix_num;i++) {
      fprintf(fp, "    weight%d = %f\n", i, exp(m->bweight[i]));
    }
  } else {
    for (i=0;i<m->mix_num;i++) {
      fprintf(fp, "-- d%d (weight=%f)--\n",i+1,exp(m->bweight[i]));
      put_htk_dens(fp, m->b[i]);
    }
  }
}

/** 
 * Output a state.
 * 
 * @param fp [in] file descriptor
 * @param s [in] pointer to %HMM state
 */
void
put_htk_state(FILE *fp, HTK_HMM_State *s)
{
  int st;

  if (fp == NULL) return;
  if (s == NULL) {
    fprintf(fp, "no output state\n");
  } else {
    if (s->name != NULL) fprintf(fp, "[~s \"%s\"]\n", s->name);
    fprintf(fp, "id: %d\n", s->id);
    for (st=0;st<s->nstream;st++) {
      fprintf(fp, "stream %d:", st + 1);
      if (s->w != NULL) {
	fprintf(fp, " (weight=%f", s->w->weight[st]);
	if (s->w->name != NULL) {
	  fprintf(fp, " <- ~w \"%s\"", s->w->name);
	}
	fprintf(fp, ")");
      }
      fprintf(fp, "\n");
      put_htk_mpdf(fp, s->pdf[st]);
    }
  }
}

/** 
 * Output %HMM model, number of states and information for each state.
 * 
 * @param fp [in] file descriptor
 * @param h [in] pointer to %HMM model
 */
void
put_htk_hmm(FILE *fp, HTK_HMM_Data *h)
{
  int i;
  
  if (fp == NULL) return;
  fprintf(fp, "name: %s\n", h->name);
  fprintf(fp, "state num: %d\n", h->state_num);
  for (i=0;i<h->state_num;i++) {
    fprintf(fp, "**** state %d ****\n",i+1);
    put_htk_state(fp, h->s[i]);
  }
  put_htk_trans(fp, h->tr);
}

/** 
 * Output logical %HMM data and its mapping status.
 * 
 * @param fp [in] file descriptor
 * @param logical [in] pointer to a logical %HMM
 */
void
put_logical_hmm(FILE *fp, HMM_Logical *logical)
{
  if (fp == NULL) return;
  fprintf(fp, "name: %s\n", logical->name);
  if (logical->is_pseudo) {
    fprintf(fp, "mapped to: %s (pseudo)\n", logical->body.pseudo->name);
  } else {
    fprintf(fp, "mapped to: %s\n", logical->body.defined->name);
  }
}

/** 
 * Output transition arcs of an HMM instance.
 *
 * @param fp [in] file descriptor
 * @param d [in] pointer to a HMM instance.
 */
void
put_hmm_arc(FILE *fp, HMM *d)
{
  A_CELL *ac;
  int i;

  if (fp == NULL) return;
  fprintf(fp, "total len: %d\n", d->len);
  for (i=0;i<d->len;i++) {
    fprintf(fp, "node-%d\n", i);
    for (ac=d->state[i].ac;ac;ac=ac->next) {
      fprintf(fp, " arc: %d %f (%f)\n",ac->arc, ac->a, pow(10.0, ac->a));
    }
  }
  if (d->accept_ac_a != LOG_ZERO) {
    fprintf(fp, "last arc to accept state: %f\n", d->accept_ac_a);
  }
}

/** 
 * Output output probability information of an HMM instance.
 * 
 * @param fp [in] file descriptor
 * @param d [in] pointer to a HMM instance.
 */
void
put_hmm_outprob(FILE *fp, HMM *d)
{
  int i;

  if (fp == NULL) return;
  fprintf(fp, "total len: %d\n", d->len);
  for (i=0;i<d->len;i++) {
    fprintf(fp, "n%d\n", i);
    if (d->state[i].is_pseudo_state) {
      fprintf(fp, "[[[pseudo state cluster with %d states]]]\n", d->state[i].out.cdset->num);
    } else {
      put_htk_state(fp, d->state[i].out.state);
    }
  }
}

/** 
 * Output an HMM instance.
 * 
 * @param fp [in] file descriptor
 * @param d [in] pointer to a HMM instance.
 */
void
put_hmm(FILE *fp, HMM *d)
{
  if (fp == NULL) return;
  put_hmm_arc(fp, d);
  put_hmm_outprob(fp, d);
}

/** 
 * Output parameter header.
 * 
 * @param fp [in] file descriptor
 * @param h [in] pointer to a parameter header information
 */
void
put_param_head(FILE *fp, HTK_Param_Header *h)
{
  char buf[128];

  if (fp == NULL) return;
  fprintf(fp, "num of samples: %d\n", h->samplenum);
  fprintf(fp, "window shift: %d ms\n", h->wshift / 10000);
  fprintf(fp, "bytes per sample: %d\n", h->sampsize);
  fprintf(fp, "parameter type: %s\n", param_code2str(buf, h->samptype, FALSE));
}

/** 
 * Output array of vectors.
 * 
 * @param fp [in] file descriptor
 * @param p [in] pointer to vector array represented as [0..num-1][0...veclen-1]
 * @param num [in] number of vectors in @a p
 * @param veclen [in] length of each vector
 */
void
put_vec(FILE *fp, VECT **p, int num, short veclen)
{
  int t,v;

  if (fp == NULL) return;
  for (t=0;t<num;t++) {
    fprintf(fp, "%d:\t%8.3f",t,p[t][0]);
    for (v=1;v<veclen;v++) {
      if ((v % 10) ==0) fprintf(fp, "\n\t");
      fprintf(fp, "%8.3f", p[t][v]);
    }
    fprintf(fp, "\n");
  }
}

/** 
 * Output the whole parameter information, including header and all vectors.
 * 
 * @param fp [in] file descriptor
 * @param pinfo [in] pointer to parameter structure.
 */
void
put_param(FILE *fp, HTK_Param *pinfo)
{
  if (fp == NULL) return;
  put_param_head(fp, &(pinfo->header));
  put_vec(fp, pinfo->parvec, pinfo->samplenum, pinfo->veclen);
}

/** 
 * Output the length of an input parameter by number of frames and seconds.
 * 
 * @param fp [in] file descriptor
 * @param pinfo [in] pointer to parameter structure.
 */
void
put_param_info(FILE *fp, HTK_Param *pinfo)
{
  HTK_Param_Header *h;
  float sec;

  if (fp == NULL) return;
  h = &(pinfo->header);
  sec = (float)h->samplenum * (float)h->wshift / 10000000;
  fprintf(fp, "length: %d frames (%.2f sec.)\n", h->samplenum, sec);
}

/** 
 * Output total statistic informations of the %HMM definition data.
 * 
 * @param fp [in] file descriptor
 * @param hmminfo [in] %HMM definition data.
 */
void
print_hmmdef_info(FILE *fp, HTK_HMM_INFO *hmminfo)
{
  char buf[128];
  int i, d;

  if (fp == NULL) return;
  fprintf(fp, " HMM Info:\n");
  fprintf(fp, "    %d models, %d states, %d mpdfs, %d Gaussians are defined\n", hmminfo->totalhmmnum, hmminfo->totalstatenum, hmminfo->totalpdfnum, hmminfo->totalmixnum);
  fprintf(fp, "\t      model type = ");
  if (hmminfo->is_tied_mixture) fprintf(fp, "has tied-mixture, ");
  if (hmminfo->opt.stream_info.num > 1) fprintf(fp, "multi-stream, ");
#ifdef ENABLE_MSD
  if (hmminfo->has_msd) fprintf(fp, "MSD-HMM, ");
#endif
  fprintf(fp, "context dependency handling %s\n",
	     (hmminfo->is_triphone) ? "ON" : "OFF");

  fprintf(fp, "      training parameter = %s\n",param_code2str(buf, hmminfo->opt.param_type, FALSE));
  fprintf(fp, "\t   vector length = %d\n", hmminfo->opt.vec_size);
  fprintf(fp, "\tnumber of stream = %d\n", hmminfo->opt.stream_info.num);
  fprintf(fp, "\t     stream info =");
  for(d=0,i=0;i<hmminfo->opt.stream_info.num;i++) {
    if (hmminfo->opt.stream_info.vsize[i] == 1) {
      fprintf(fp, " [%d]", d);
    } else {
      fprintf(fp, " [%d-%d]", d, d + hmminfo->opt.stream_info.vsize[i] - 1);
    }
    d += hmminfo->opt.stream_info.vsize[i];
  }
  fprintf(fp, "\n");
  fprintf(fp, "\tcov. matrix type = %s\n", get_cov_str(hmminfo->opt.cov_type));
  fprintf(fp, "\t   duration type = %s\n", get_dur_str(hmminfo->opt.dur_type));

  if (hmminfo->is_tied_mixture) {
    fprintf(fp, "\t    codebook num = %d\n", hmminfo->codebooknum);
    fprintf(fp, "       max codebook size = %d\n", hmminfo->maxcodebooksize);
  }
  fprintf(fp, "\tmax mixture size = %d Gaussians\n", hmminfo->maxmixturenum);
  fprintf(fp, "     max length of model = %d states\n", hmminfo->maxstatenum);

  fprintf(fp, "     logical base phones = %d\n", hmminfo->basephone.num);

  fprintf(fp, "       model skip trans. = ");
  if (hmminfo->need_multipath) {
    fprintf(fp, "exist, require multi-path handling\n");
  } else {
    fprintf(fp, "not exist, no multi-path handling\n");
  }

  if (hmminfo->need_multipath) {
    fprintf(fp, "      skippable models =");
    {
      HTK_HMM_Data *dtmp;
      int n = 0;
      for (dtmp = hmminfo->start; dtmp; dtmp = dtmp->next) {
	if (is_skippable_model(dtmp)) {
	  fprintf(fp, " %s", dtmp->name);
	  n++;
	}
      }
      if (n == 0) {
	fprintf(fp, " none\n");
      } else {
	fprintf(fp, " (%d model(s))\n", n);
      }
    }
  }
}
