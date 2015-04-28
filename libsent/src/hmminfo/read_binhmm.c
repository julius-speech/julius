/**
 * @file   read_binhmm.c
 * 
 * <JA>
 * @brief  バイナリ形式の %HMM 定義ファイルを読み込む
 *
 * Julius は独自のバイナリ形式の %HMM 定義ファイルをサポートしています．
 * HTKのアスキー形式の %HMM 定義ファイルからバイナリ形式への変換は，
 * 附属のツール mkbinhmm で行ないます．このバイナリ形式は，HTK の
 * バイナリ形式とは非互換ですので注意して下さい．
 * </JA>
 * 
 * <EN>
 * @brief  Read a binary %HMM definition file
 *
 * Julius supports a binary format of %HMM definition file.
 * The tool "mkbinhmm" can convert the ascii format HTK %HMM definition
 * file to this format.  Please note that this binary format is 
 * not compatible with the HTK binary format.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 05:23:59 2005
 *
 * $Revision: 1.11 $
 * 
 */
/*
 * Copyright (c) 2003-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/htk_param.h>
#include <sent/htk_hmm.h>

#undef DMES			/* define to enable debug message */

static boolean gzfile;	      ///< TRUE when opened by fopen_readfile

#define rdn(A,B,C,D) if (rdnfunc(A,B,C,D) == FALSE) return FALSE
#define rdn_str(A,B,C) if ((C = rdn_strfunc(A,B)) == NULL) return FALSE

/** 
 * Binary read function with byte swaping (assume file is BIG ENDIAN)
 * 
 * @param fp [in] file pointer
 * @param buf [out] read data
 * @param unitbyte [in] size of a unit in bytes
 * @param unitnum [in] number of unit to be read
 */
static boolean
rdnfunc(FILE *fp, void *buf, size_t unitbyte, int unitnum)
{
  size_t tmp;

  if (unitnum == 0) return TRUE;

  if (gzfile) {
    tmp = myfread(buf, unitbyte, unitnum, fp);
  } else {
    tmp = fread(buf, unitbyte, unitnum, fp);
  }
  if (tmp < (size_t)unitnum) {
    jlog("Error: read_binhmm: failed to read %d bytes\n", unitbyte * unitnum);
    return FALSE;
  }
#ifndef WORDS_BIGENDIAN
  if (unitbyte != 1) {
    swap_bytes(buf, unitbyte, unitnum);
  }
#endif
  return TRUE;
}

static char buf[MAXLINELEN];	///< Local work are for text handling
static char nostr = '\0';
/** 
 * Read a string till NULL.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] pointer to %HMM definition data to store the values.
 * 
 * @return pointer to a newly allocated buffer that contains the read string.
 */
static char *
rdn_strfunc(FILE *fp, HTK_HMM_INFO *hmm)
{
  int c;
  int len;
  char *p;

  len = 0;
  while ((c = gzfile ? myfgetc(fp) : fgetc(fp)) != -1) {
    if (len >= MAXLINELEN) {
      jlog("Error: read_binhmm: string len exceeded %d bytes\n", MAXLINELEN);
      jlog("Error: read_binhmm: please check the value of MAXLINELEN\n");
      return NULL;
    }
    buf[len++] = c;
    if (c == '\0') break;
  }
  if (len == 0) return NULL;
  if (len == 1) {
    p = &nostr;
  } else {
    p = (char *)mybmalloc2(len, &(hmm->mroot));
    strcpy(p, buf);
  }
  return(p);
}


static char *binhmm_header = BINHMM_HEADER; ///< Header string
static char *binhmm_header_v2 = BINHMM_HEADER_V2; ///< Header string for V2

/** 
 * Read acoustic analysis configration parameters from header of binary HMM.
 * 
 * @param fp [in] file pointer
 * @param para [out] acoustic analysis configration parameters
 */
static boolean
rd_para(FILE *fp, Value *para)
{
  short version;
  float dummy;

  /* read version */
  rdn(fp, &version, sizeof(short), 1);

  if (version > VALUE_VERSION) {
    jlog("Error: read_binhmm: unknown embedded parameter format version: %d\n", version);
    return FALSE;
  }
  jlog("Stat: rd_para: found embedded acoutic parameter (ver.%d)\n", version);

  /* read parameters */
  rdn(fp, &(para->smp_period), sizeof(int), 1);      
  rdn(fp, &(para->smp_freq), sizeof(int), 1);
  rdn(fp, &(para->framesize), sizeof(int), 1);        
  rdn(fp, &(para->frameshift), sizeof(int), 1);       
  /* tweak to read 64bit binhmm with older version (smp_period, smp_freq = 8byte) */
  if (para->smp_period == 0 && para->framesize == 0 &&
      para->smp_freq != 0 && para->frameshift != 0) {
    jlog("Warning: rd_para: smp_period=%d, smp_freq=%d, framesize=%d, frameshift=%d\n", para->smp_period, para->smp_freq, para->framesize, para->frameshift);
    jlog("Warning: rd_para: wrong values, may be reading binhmm created at 64bit?\n");
    jlog("Warning: rd_para: try to re-parse values from 64bit to 32bit...\n");
    para->smp_period = para->smp_freq;
    para->smp_freq = para->frameshift;
    rdn(fp, &(para->framesize), sizeof(int), 1);
    rdn(fp, &(para->frameshift), sizeof(int), 1);
    jlog("Warning: rd_para: smp_period=%d, smp_freq=%d, framesize=%d, frameshift=%d\n", para->smp_period, para->smp_freq, para->framesize, para->frameshift);
  }
  rdn(fp, &(para->preEmph), sizeof(float), 1);        
  rdn(fp, &(para->lifter), sizeof(int), 1);           
  rdn(fp, &(para->fbank_num), sizeof(int), 1);        
  rdn(fp, &(para->delWin), sizeof(int), 1);           
  rdn(fp, &(para->accWin), sizeof(int), 1);           
  rdn(fp, &(para->silFloor), sizeof(float), 1);       
  rdn(fp, &(para->escale), sizeof(float), 1);         
  rdn(fp, &(para->hipass), sizeof(int), 1);		
  rdn(fp, &(para->lopass), sizeof(int), 1);		
  rdn(fp, &(para->enormal), sizeof(int), 1);          
  rdn(fp, &(para->raw_e), sizeof(int), 1);            
  if (version == 1) {
    /* version 1 has ss related parameters, but version 2 and later not */
    /* skip ss related parameters (ss_alpha and ss_floor) */
    rdn(fp, &dummy, sizeof(float), 1);
    rdn(fp, &dummy, sizeof(float), 1);
  }
  rdn(fp, &(para->zmeanframe), sizeof(int), 1);	
  if (version >= 3) {
    rdn(fp, &(para->usepower), sizeof(int), 1);
  }

  return(TRUE);
}

/** 
 * Read header string of binary HMM file.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] pointer to %HMM definition data to store the values.
 * @param para [out] store embedded acoustic parameters if any (V2)
 * @param mpdf_macro_ret [out] will be set to TRUE if the file contains mixture pdf macro defined by "~p"
 * 
 * @return TRUE if a correct header was read, FALSE if header string does not
 * match the current version.
 */
static boolean
rd_header(FILE *fp, HTK_HMM_INFO *hmm, Value *para, boolean *mpdf_macro_ret)
{
  char *p, *q;
  boolean emp, inv;
  
  rdn_str(fp, hmm, p);
  if (strmatch(p, binhmm_header)) {
    /* version 1 */
    hmm->variance_inversed = FALSE;
  } else if (strmatch(p, binhmm_header_v2)) {
    /* version 2 */
    emp = inv = FALSE;
    rdn_str(fp, hmm, q);
    if (*q != '\0') {
      while(*q == '_') {
	q++;
	switch (*q) {
	case BINHMM_HEADER_V2_EMBEDPARA:
	  /* read in embedded acoutic condition parameters */
	  emp = TRUE;
	  jlog("Stat: binhmm-header: analysis parameter embedded\n");
	  break;
	case BINHMM_HEADER_V2_VARINV:
	  inv = TRUE;
	  jlog("Stat: binhmm-header: variance inversed\n");
	  break;
	case BINHMM_HEADER_V2_MPDFMACRO:
	  *mpdf_macro_ret = TRUE;
	  jlog("Stat: binhmm-header: mixture PDF macro used\n");
	  break;
	default:
	  jlog("Error: unknown format qualifier in header: \"%c\"\n", *q);
	  return FALSE;
	}
	q++;
      }
    }
    if (emp) {
      para->loaded = 1;
      if (rd_para(fp, para) == FALSE) {
	jlog("Error: read_binhmm: failed to read embeded parameter\n");
	return FALSE;
      }
      jlog("Stat: read_binhmm: has acoutic analysis configurations in its header\n");
    }
    if (inv) {
      hmm->variance_inversed = TRUE;
      jlog("Stat: read_binhmm: has inversed variances\n");
    } else {
      hmm->variance_inversed = FALSE;
    }
  } else {
    /* failed to read header */
    return FALSE;
  }
  return TRUE;
}



/** 
 * Read %HMM option specifications.
 * 
 * @param fp [in] file pointer
 * @param opt [out] pointer to the %HMM option structure to hold the read
 * values.
 */
static boolean
rd_opt(FILE *fp, HTK_HMM_Options *opt)
{
  rdn(fp, &(opt->stream_info.num), sizeof(short), 1);
  rdn(fp, opt->stream_info.vsize, sizeof(short), MAXSTREAMNUM);
  rdn(fp, &(opt->vec_size), sizeof(short), 1);
  rdn(fp, &(opt->cov_type), sizeof(short), 1);
  rdn(fp, &(opt->dur_type), sizeof(short), 1);
  rdn(fp, &(opt->param_type), sizeof(short), 1);

  return(TRUE);
}

/** 
 * Read %HMM type of mixture tying.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] pointer to %HMM definition data to store the values.
 */
static boolean
rd_type(FILE *fp, HTK_HMM_INFO *hmm)
{
  rdn(fp, &(hmm->is_tied_mixture), sizeof(boolean), 1);
  rdn(fp, &(hmm->maxmixturenum), sizeof(int), 1);
  return TRUE;
}


/* read transition data */
static HTK_HMM_Trans **tr_index; ///< Map transition matrix id to its pointer
static unsigned int tr_num;	///< Length of above

/** 
 * @brief  Read a sequence of transition matrix data for @a tr_num.
 *
 * The transition matrixes are stored into @a hmm, and their pointers
 * are also stored in @a tr_index for later data mapping operation
 * from upper structure (state etc.).
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read transitions.
 */
static boolean
rd_trans(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Trans *t;
  unsigned int idx;
  int i;
  PROB *atmp;
  char *p;

  rdn(fp, &tr_num, sizeof(unsigned int), 1);
  tr_index = (HTK_HMM_Trans **)mymalloc(sizeof(HTK_HMM_Trans *) * tr_num);

  hmm->trstart = NULL;
  hmm->tr_root = NULL;
  for (idx = 0; idx < tr_num; idx++) {
    t = (HTK_HMM_Trans *)mybmalloc2(sizeof(HTK_HMM_Trans), &(hmm->mroot));
    rdn_str(fp, hmm, p);
    t->name = (*p == '\0') ? NULL : p;
    rdn(fp, &(t->statenum), sizeof(short), 1);
    t->a = (PROB **)mybmalloc2(sizeof(PROB *) * t->statenum, &(hmm->mroot));
    atmp = (PROB *)mybmalloc2(sizeof(PROB) * t->statenum * t->statenum, &(hmm->mroot));
    for (i=0;i<t->statenum;i++) {
      t->a[i] = &(atmp[i*t->statenum]);
      rdn(fp, t->a[i], sizeof(PROB), t->statenum);
    }
    trans_add(hmm, t);
    tr_index[idx] = t;
  }

#ifdef DMES
  jlog("Stat: read_binhmm: %d transition maxtix read\n", tr_num);
#endif
  return TRUE;
}


static HTK_HMM_Var **vr_index;	///< Map variance id to its pointer
static unsigned int vr_num;	///< Length of above

/** 
 * @brief  Read a sequence of variance vector for @a vr_num.
 *
 * The variance vectors are stored into @a hmm, and their pointers
 * are also stored in @a vr_index for later data mapping operation
 * from upper structure (density etc.).
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read variance.
 */
static boolean
rd_var(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Var *v;
  unsigned int idx;
  char *p;

  rdn(fp, &vr_num, sizeof(unsigned int), 1);
  vr_index = (HTK_HMM_Var **)mymalloc(sizeof(HTK_HMM_Var *) * vr_num);
  
  hmm->vrstart = NULL;
  hmm->vr_root = NULL;
  for (idx = 0; idx < vr_num; idx++) {
    v = (HTK_HMM_Var *)mybmalloc2(sizeof(HTK_HMM_Var), &(hmm->mroot));
    rdn_str(fp, hmm, p);
    v->name = (*p == '\0') ? NULL : p;
    rdn(fp, &(v->len), sizeof(short), 1);
    v->vec = (VECT *)mybmalloc2(sizeof(VECT) * v->len, &(hmm->mroot));
    rdn(fp, v->vec, sizeof(VECT), v->len);
    vr_index[idx] = v;
    var_add(hmm, v);
  }
#ifdef DMES
  jlog("Stat: read_binhmm: %d variance read\n", vr_num);
#endif
  return TRUE;
}


/* read density data */
static HTK_HMM_Dens **dens_index; ///< Map density id to its pointer
static unsigned int dens_num;	///< Length of above

/** 
 * @brief  Read a sequence of mixture densities for @a dens_num.
 *
 * The mixture densities are stored into @a hmm, and their references
 * to lower structure (variance etc.) are recovered from the id-to-pointer
 * index.  Their pointers are also stored in @a dens_index for
 * later data mapping operation from upper structure (state etc.).
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read densities.
 */
static boolean
rd_dens(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Dens *d;
  unsigned int idx;
  unsigned int vid;
  char *p;

  rdn(fp, &dens_num, sizeof(unsigned int), 1);
  hmm->totalmixnum = dens_num;
  dens_index = (HTK_HMM_Dens **)mymalloc(sizeof(HTK_HMM_Dens *) * dens_num);

  hmm->dnstart = NULL;
  hmm->dn_root = NULL;
  for (idx = 0; idx < dens_num; idx++) {
    d = (HTK_HMM_Dens *)mybmalloc2(sizeof(HTK_HMM_Dens), &(hmm->mroot));
    rdn_str(fp, hmm, p);
    d->name = (*p == '\0') ? NULL : p;
    rdn(fp, &(d->meanlen), sizeof(short), 1);
    d->mean = (VECT *)mybmalloc2(sizeof(VECT) * d->meanlen, &(hmm->mroot));
    rdn(fp, d->mean, sizeof(VECT), d->meanlen);
    rdn(fp, &vid, sizeof(unsigned int), 1);
    d->var = vr_index[vid];
    rdn(fp, &(d->gconst), sizeof(LOGPROB), 1);
    dens_index[idx] = d;
    dens_add(hmm, d);
  }
#ifdef DMES
  jlog("Stat: read_binhmm: %d gaussian densities read\n", dens_num);
#endif
  return TRUE;
}


/* read stream weight data */
static HTK_HMM_StreamWeight **streamweight_index; ///< Map stream weights id to its pointer
static unsigned int streamweight_num;	///< Length of above

/** 
 * @brief  Read a sequence of stream weights for @a streamweight_num.
 *
 * The stream weights are stored into @a hmm, and their references
 * to lower structure (variance etc.) are recovered from the id-to-pointer
 * index.  Their pointers are also stored in @a dens_index for
 * later data mapping operation from upper structure (state etc.).
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read stream weights.
 */
static boolean
rd_streamweight(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_StreamWeight *sw;
  unsigned int idx;
  char *p;

  rdn(fp, &streamweight_num, sizeof(unsigned int), 1);
  streamweight_index = (HTK_HMM_StreamWeight **)mymalloc(sizeof(HTK_HMM_StreamWeight *) * streamweight_num);

  hmm->swstart = NULL;
  hmm->sw_root = NULL;
  for (idx = 0; idx < streamweight_num; idx++) {
    sw = (HTK_HMM_StreamWeight *)mybmalloc2(sizeof(HTK_HMM_StreamWeight), &(hmm->mroot));
    rdn_str(fp, hmm, p);
    sw->name = (*p == '\0') ? NULL : p;
    rdn(fp, &(sw->len), sizeof(short), 1);
    sw->weight = (VECT *)mybmalloc2(sizeof(VECT) * sw->len, &(hmm->mroot));
    rdn(fp, sw->weight, sizeof(VECT), sw->len);
    streamweight_index[idx] = sw;
    sw_add(hmm, sw);
  }
#ifdef DMES
  jlog("Stat: read_binhmm: %d stream weights read\n", streamweight_num);
#endif
  return TRUE;
}


/* read tmix data */
static GCODEBOOK **tm_index;	///< Map codebook id to its pointer
static unsigned int tm_num;	///< Length of above

/** 
 * @brief  Read a sequence of mixture codebook for @a tm_num.
 *
 * The mixture codebook data are stored into @a hmm, and their references
 * to lower structure (mixtures etc.) are recovered from the id-to-pointer
 * index.  Their pointers are also stored in @a tm_index for
 * later data mapping operation from upper structure (state etc.).
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read codebooks.
 */
static boolean
rd_tmix(FILE *fp, HTK_HMM_INFO *hmm)
{
  GCODEBOOK *tm;
  unsigned int idx;
  unsigned int did;
  int i;
  char *p;

  rdn(fp, &tm_num, sizeof(unsigned int), 1);
  hmm->codebooknum = tm_num;
  tm_index = (GCODEBOOK **)mymalloc(sizeof(GCODEBOOK *) * tm_num);
  hmm->maxcodebooksize = 0;

  hmm->codebook_root = NULL;
  for (idx = 0; idx < tm_num; idx++) {
    tm = (GCODEBOOK *)mybmalloc2(sizeof(GCODEBOOK), &(hmm->mroot));
    rdn_str(fp, hmm, p);
    tm->name = (*p == '\0') ? NULL : p;
    rdn(fp, &(tm->num), sizeof(int), 1);
    if (hmm->maxcodebooksize < tm->num) hmm->maxcodebooksize = tm->num;
    tm->d = (HTK_HMM_Dens **)mybmalloc2(sizeof(HTK_HMM_Dens *) * tm->num, &(hmm->mroot));
    for(i=0;i<tm->num;i++) {
      rdn(fp, &did, sizeof(unsigned int), 1);
      if (did >= dens_num) {
	tm->d[i] = NULL;
      } else {
	tm->d[i] = dens_index[did];
      }
    }
    tm->id = idx;
    tm_index[idx] = tm;
    codebook_add(hmm, tm);
  }
#ifdef DMES
  jlog("Stat: read_binhmm: %d tied-mixture codebooks read\n", tm_num);
#endif  
  return TRUE;
}


/* read mpdf data */
static HTK_HMM_PDF **mpdf_index; ///< Map mixture pdf id to its pointer
static unsigned int mpdf_num;	///< Length of above

/**
 * Read a mixture PDF.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read codebooks.
 * @param m [out] pointer where to store the input mixture PDF.
 * 
 * @return TRUE on success, FALSE on error.
 * 
 */
static boolean
rd_pdf_sub(FILE *fp, HTK_HMM_INFO *hmm, HTK_HMM_PDF *m)
{
  int i;
  unsigned int did;

  rdn(fp, &(m->mix_num), sizeof(short), 1);
  if (m->mix_num == -1) {
    /* tmix */
    rdn(fp, &did, sizeof(unsigned int), 1);
    m->b = (HTK_HMM_Dens **)tm_index[did];
    m->mix_num = (tm_index[did])->num;
    m->tmix = TRUE;
  } else {
    /* mixture */
    m->b = (HTK_HMM_Dens **)mybmalloc2(sizeof(HTK_HMM_Dens *) * m->mix_num, &(hmm->mroot));
    for (i=0;i<m->mix_num;i++) {
      rdn(fp, &did, sizeof(unsigned int), 1);
      if (did >= dens_num) {
	m->b[i] = NULL;
      } else {
	m->b[i] = dens_index[did];
      }
    }
    m->tmix = FALSE;
  }
  m->bweight = (PROB *)mybmalloc2(sizeof(PROB) * m->mix_num, &(hmm->mroot));
  rdn(fp, m->bweight, sizeof(PROB), m->mix_num);

  return TRUE;
}


/** 
 * @brief  Read a sequence of mixture pdf for @a mpdf_num.
 *
 * The mixture pdfs are stored into @a hmm, and their references
 * to lower structure (variance etc.) are recovered from the id-to-pointer
 * index.  Their pointers are also stored in @a mpdf_index for
 * later data mapping operation from upper structure (state etc.).
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read data.
 */
static boolean
rd_mpdf(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_PDF *m;
  unsigned int idx;
  char *p;

  rdn(fp, &mpdf_num, sizeof(unsigned int), 1);
  mpdf_index = (HTK_HMM_PDF **)mymalloc(sizeof(HTK_HMM_PDF *) * mpdf_num);

  hmm->pdfstart = NULL;
  hmm->pdf_root = NULL;
  for (idx = 0; idx < mpdf_num; idx++) {
    m = (HTK_HMM_PDF *)mybmalloc2(sizeof(HTK_HMM_PDF), &(hmm->mroot));
    rdn_str(fp, hmm, p);
    m->name = (*p == '\0') ? NULL : p;
    rdn(fp, &(m->stream_id), sizeof(short), 1);
    if (rd_pdf_sub(fp, hmm, m) == FALSE) return FALSE;
    mpdf_index[idx] = m;
    mpdf_add(hmm, m);
  }
#ifdef DMES
  jlog("Stat: read_binhmm: %d mixture PDFs read\n", mpdf_num);
#endif
  return TRUE;
}


/* read state data */
static HTK_HMM_State **st_index; ///< Map state id to its pointer
static unsigned int st_num;	///< Length of above

/** 
 * @brief  Read a sequence of state data for @a st_num.
 *
 * The state data are stored into @a hmm, and their references
 * to lower structure (mixture, codebook, etc.) are recovered
 * from the id-to-pointer index.  Their pointers are also stored
 * in @a st_index for later data mapping operation from
 * upper structure (models etc.).
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read states.
 * @param mpdf_macro [in] TRUE if mixture pdfs are already read separatedly, or FALSE if they are all defined in-line
 */
static boolean
rd_state(FILE *fp, HTK_HMM_INFO *hmm, boolean mpdf_macro)
{
  HTK_HMM_State *s;
  unsigned int idx;
  unsigned int mid, swid;
  int m;
  char *buf;

  rdn(fp, &st_num, sizeof(unsigned int), 1);
  hmm->totalstatenum = st_num;
  st_index = (HTK_HMM_State **)mymalloc(sizeof(HTK_HMM_State *) * st_num);

  hmm->ststart = NULL;
  hmm->st_root = NULL;
  for (idx = 0; idx < st_num; idx++) {
    s = (HTK_HMM_State *)mybmalloc2(sizeof(HTK_HMM_State), &(hmm->mroot));
    rdn_str(fp, hmm, buf);
    s->name = (*buf == '\0') ? NULL : buf;
    s->nstream = hmm->opt.stream_info.num;
    s->pdf = (HTK_HMM_PDF **)mybmalloc2(sizeof(HTK_HMM_PDF *) * s->nstream, &(hmm->mroot));
    if (mpdf_macro) {
      /* mpdf are stored separatedly, so read index */
      for(m=0;m<s->nstream;m++) {
	rdn(fp, &mid, sizeof(unsigned int), 1);
	if (mid >= mpdf_num) {
	  s->pdf[m] = NULL;
	} else {
	  s->pdf[m] = mpdf_index[mid];
	}
      }
    } else {
      /* mpdf are stored sequencially, so read the content here */
      for(m=0;m<s->nstream;m++) {
	s->pdf[m] = (HTK_HMM_PDF *)mybmalloc2(sizeof(HTK_HMM_PDF), &(hmm->mroot));
	s->pdf[m]->name = NULL;
	if (rd_pdf_sub(fp, hmm, s->pdf[m]) == FALSE) return FALSE;
	s->pdf[m]->stream_id = m;
	mpdf_add(hmm, s->pdf[m]);
      }
    }
    if (hmm->opt.stream_info.num > 1) {
      /* read steam weight info */
      rdn(fp, &swid, sizeof(unsigned int), 1);
      if (swid >= streamweight_num) {
	s->w = NULL;
      } else {
	s->w = streamweight_index[swid];
      }
    } else {
      s->w = NULL;
    }
    s->id = idx;
    st_index[idx] = s;
    state_add(hmm, s);
  }
#ifdef DMES
  jlog("Stat: read_binhmm: %d states read\n", st_num);
#endif
  return TRUE;
}

/** 
 * @brief  Read a sequence of %HMM models.
 *
 * The models are stored into @a hmm.  Their references
 * to lower structures (state, transition, etc.) are stored in schalar
 * ID, and are recovered from the previously built id-to-pointer index.
 * when reading the sub structures.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read models.
 */
static boolean
rd_data(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Data *d;
  unsigned int md_num;
  unsigned int sid, tid;
  unsigned int idx;
  int i;
  char *p;

  rdn(fp, &(md_num), sizeof(unsigned int), 1);
  hmm->totalhmmnum = md_num;

  hmm->start = NULL;
  hmm->physical_root = NULL;
  for (idx = 0; idx < md_num; idx++) {
    d = (HTK_HMM_Data *)mybmalloc2(sizeof(HTK_HMM_Data), &(hmm->mroot));
    rdn_str(fp, hmm, p);
    d->name = (*p == '\0') ? NULL : p;
    rdn(fp, &(d->state_num), sizeof(short), 1);
    d->s = (HTK_HMM_State **)mybmalloc2(sizeof(HTK_HMM_State *) * d->state_num, &(hmm->mroot));
    for (i=0;i<d->state_num;i++) {
      rdn(fp, &sid, sizeof(unsigned int), 1);
      if (sid > (unsigned int)hmm->totalstatenum) {
	d->s[i] = NULL;
      } else {
	d->s[i] = st_index[sid];
      }
    }
    rdn(fp, &tid, sizeof(unsigned int), 1);
    d->tr = tr_index[tid];
    htk_hmmdata_add(hmm, d);
  }
#ifdef DMES
  jlog("Stat: read_binhmm: %d HMM model definition read\n", md_num);
#endif
  return TRUE;
}



/** 
 * Top function to read a binary %HMM file from @a fp.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] %HMM definition structure to hold the read models.
 * @param gzfile_p [in] TRUE if the file pointer points to a gzip file
 * @param para [out] store acoustic parameters if embedded in binhmm (V2)
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
read_binhmm(FILE *fp, HTK_HMM_INFO *hmm, boolean gzfile_p, Value *para)
{
  boolean mpdf_macro = FALSE;

  gzfile = gzfile_p;

  /* read header */
  if (rd_header(fp, hmm, para, &mpdf_macro) == FALSE) {
    return FALSE;
  }

  jlog("Stat: read_binhmm: binary format HMM definition\n");
  
  /* read option data */
  if (rd_opt(fp, &(hmm->opt)) == FALSE) {
    jlog("Error: read_binhmm: failed to read HMM options\n");
    return FALSE;
  }

  /* read type data */
  if (rd_type(fp, hmm) == FALSE) {
    jlog("Error: read_binhmm: failed to read HMM type of mixture tying\n");
    return FALSE;
  }

  /* read transition data */
  if (rd_trans(fp, hmm) == FALSE) {
    jlog("Error: read_binhmm: failed to read HMM transition data\n");
    return FALSE;
  }

  /* read variance data */
  if (rd_var(fp, hmm) == FALSE) {
    jlog("Error: read_binhmm: failed to read HMM variance data\n");
    return FALSE;
  }

  /* read density data */
  if (rd_dens(fp, hmm) == FALSE) {
    jlog("Error: read_binhmm: failed to read HMM density data\n");
    return FALSE;
  }

  /* read stream weight data */
  if (hmm->opt.stream_info.num > 1) {
    if (rd_streamweight(fp, hmm) == FALSE) {
      jlog("Error: read_binhmm: failed to read stream weights data\n");
      return FALSE;
    }
  }

  /* read tmix data */
  if (hmm->is_tied_mixture) {
    if (rd_tmix(fp, hmm) == FALSE) {
      jlog("Error: read_binhmm: failed to read HMM tied-mixture codebook data\n");
      return FALSE;
    }
  }

  /* read mixture pdf data */
  if (mpdf_macro) {
    if (rd_mpdf(fp, hmm) == FALSE) {
      jlog("Error: read_binhmm: failed to read mixture PDF data\n");
      return FALSE;
    }
  }

  /* read state data */
  if (rd_state(fp, hmm, mpdf_macro) == FALSE) {
    jlog("Error: read_binhmm: failed to read HMM state data\n");
    return FALSE;
  }

  /* read model data */
  if (rd_data(fp, hmm) == FALSE) {
    jlog("Error: read_binhmm: failed to read HMM data\n");
    return FALSE;
  }

  /* free pointer->index work area */
  if (mpdf_macro) free(mpdf_index);
  free(tr_index);
  free(vr_index);
  if (hmm->opt.stream_info.num > 1) free(streamweight_index);
  free(dens_index);
  if (hmm->is_tied_mixture) free(tm_index);
  free(st_index);

  /* count maximum state num (it is not stored in binhmm... */
  {
    HTK_HMM_Data *dtmp;
    int maxlen = 0;
    for (dtmp = hmm->start; dtmp; dtmp = dtmp->next) {
      if (maxlen < dtmp->state_num) maxlen = dtmp->state_num;
    }
    hmm->maxstatenum = maxlen;
  }

  /* compute total number of mixture PDFs */
  {
    HTK_HMM_PDF *p;
    int n = 0;
    for (p = hmm->pdfstart; p; p = p->next) {
      n++;
    }
    hmm->totalpdfnum = n;
  }

  /* check state id */
  {
    HTK_HMM_State *stmp;
    int n;
    boolean has_sid;

    /* check if each state is assigned a valid sid */
    if (htk_hmm_check_sid(hmm) == FALSE) {
      jlog("Error: rdhmmdef: error in SID\n");
      return FALSE;
    }
#if 0
    for (stmp = hmm->ststart; stmp; stmp = stmp->next) {
      stmp->id = n++;
    }
#endif
  }
  /* assign ID number for all HTK_HMM_Trans */
  {
    HTK_HMM_Trans *ttmp;
    int n = 0;
    for (ttmp = hmm->trstart; ttmp; ttmp = ttmp->next) {
      ttmp->id = n++;
    }
    hmm->totaltransnum = n;
  }

  /* determine whether this model needs multi-path handling */
  hmm->need_multipath = htk_hmm_has_several_arc_on_edge(hmm);
  if (hmm->need_multipath) {
    jlog("Stat: read_binhmm: this HMM requires multipath handling at decoding\n");
  } else {
    jlog("Stat: read_binhmm: this HMM does not need multipath handling\n");
  }
  
  if (! hmm->variance_inversed) {
    /* inverse all variance values for faster computation */
    htk_hmm_inverse_variances(hmm);
    hmm->variance_inversed = TRUE;
  }

#ifdef ENABLE_MSD
  /* check if MSD-HMM */
  htk_hmm_check_msd(hmm);
#endif

  return (TRUE);
}
