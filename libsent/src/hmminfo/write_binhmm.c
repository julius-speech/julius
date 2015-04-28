/**
 * @file   write_binhmm.c
 * 
 * <JA>
 * @brief  %HMM 定義をバイナリ形式のファイルへ書き出す
 *
 * Julius は独自のバイナリ形式の %HMM 定義ファイルをサポートしています．
 * HTKのアスキー形式の %HMM 定義ファイルからバイナリ形式への変換は，
 * 附属のツール mkbinhmm で行ないます．このバイナリ形式は，HTK の
 * バイナリ形式とは非互換ですので注意して下さい．
 * </JA>
 * 
 * <EN>
 * @brief  Write a binary %HMM definition to a file
 *
 * Julius supports a binary format of %HMM definition file.
 * The tool "mkbinhmm" can convert the ascii format HTK %HMM definition
 * file to this format.  Please note that this binary format is 
 * not compatible with the HTK binary format.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 06:03:36 2005
 *
 * $Revision: 1.9 $
 * 
 */
/*
 * Copyright (c) 2003-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* $Id: write_binhmm.c,v 1.9 2013/12/18 03:55:21 sumomo Exp $ */

#include <sent/stddefs.h>
#include <sent/htk_param.h>
#include <sent/htk_hmm.h>
#include <sent/mfcc.h>

#define wrt(A,B,C,D) if (wrtfunc(A,B,C,D) == FALSE) return FALSE
#define wrt_str(A,B) if (wrt_strfunc(A,B) == FALSE) return FALSE


/** 
 * Binary write function with byte swap (assume file is BIG ENDIAN)
 * 
 * @param fp [in] file pointer
 * @param buf [in] data to write
 * @param unitbyte [in] size of a unit in bytes
 * @param unitnum [in] number of unit to write
 */
static boolean
wrtfunc(FILE *fp, void *buf, size_t unitbyte, size_t unitnum)
{

  if (unitnum == 0) return TRUE;

#ifndef WORDS_BIGENDIAN
  if (unitbyte != 1) {
    swap_bytes((char *)buf, unitbyte, unitnum);
  }
#endif
  if (myfwrite(buf, unitbyte, unitnum, fp) < unitnum) {
    jlog("Error: write_binhmm: failed to write %d bytes", unitbyte * unitnum);
    return FALSE;
  }
#ifndef WORDS_BIGENDIAN
  if (unitbyte != 1) {
    swap_bytes((char *)buf, unitbyte, unitnum);
  }
#endif
  return TRUE;
}

/** 
 * Write a string, teminating at NULL.
 * 
 * @param fp [in] file pointer
 * @param str [in] string to write
 */
static boolean
wrt_strfunc(FILE *fp, char *str)
{
  static char noname = '\0';
  boolean ret;
  
  if (str) {
    ret = wrtfunc(fp, str, sizeof(char), strlen(str)+1);
  } else {
    ret = wrtfunc(fp, &noname, sizeof(char), 1);
  }
  return ret;
}


static char *binhmm_header_v2 = BINHMM_HEADER_V2; ///< Header string for V2

/** 
 * Write header string as binary HMM file (ver. 2)
 * 
 * @param fp [in] file pointer
 * @param emp [in] TRUE if parameter embedded
 * @param inv [in] TRUE if variances are inversed
 * @param mpdfmacro [in] TRUE if some mixture pdfs are defined as macro
 */
static boolean
wt_header(FILE *fp, boolean emp, boolean inv, boolean mpdfmacro)
{
  char buf[50];
  char *p;

  wrt_str(fp, binhmm_header_v2);
  p = &(buf[0]);
  if (emp) {
    *p++ = '_';
    *p++ = BINHMM_HEADER_V2_EMBEDPARA;
  }
  if (inv) {
    *p++ = '_';
    *p++ = BINHMM_HEADER_V2_VARINV;
  }
  if (mpdfmacro) {
    *p++ = '_';
    *p++ = BINHMM_HEADER_V2_MPDFMACRO;
  }
  *p = '\0';
  wrt_str(fp, buf);
  jlog("Stat: write_binhmm: written header: \"%s%s\"\n", binhmm_header_v2, buf);

  return TRUE;
}


/** 
 * Write acoustic analysis configration parameters into header of binary HMM.
 * 
 * @param fp [in] file pointer
 * @param para [in] acoustic analysis configration parameters
 */
static boolean
wt_para(FILE *fp, Value *para)
{
  short version;

  version = VALUE_VERSION;
  wrt(fp, &version, sizeof(short), 1);

  wrt(fp, &(para->smp_period), sizeof(int), 1);      
  wrt(fp, &(para->smp_freq), sizeof(int), 1);	
  wrt(fp, &(para->framesize), sizeof(int), 1);        
  wrt(fp, &(para->frameshift), sizeof(int), 1);       
  wrt(fp, &(para->preEmph), sizeof(float), 1);        
  wrt(fp, &(para->lifter), sizeof(int), 1);           
  wrt(fp, &(para->fbank_num), sizeof(int), 1);        
  wrt(fp, &(para->delWin), sizeof(int), 1);           
  wrt(fp, &(para->accWin), sizeof(int), 1);           
  wrt(fp, &(para->silFloor), sizeof(float), 1);       
  wrt(fp, &(para->escale), sizeof(float), 1);         
  wrt(fp, &(para->hipass), sizeof(int), 1);		
  wrt(fp, &(para->lopass), sizeof(int), 1);		
  wrt(fp, &(para->enormal), sizeof(int), 1);          
  wrt(fp, &(para->raw_e), sizeof(int), 1);            
  wrt(fp, &(para->zmeanframe), sizeof(int), 1);	
  wrt(fp, &(para->usepower), sizeof(int), 1);

  return TRUE;
}


/** 
 * Write %HMM option specifications
 * 
 * @param fp [in] file pointer
 * @param opt [out] pointer to the %HMM option structure that holds the values.
 */
static boolean
wt_opt(FILE *fp, HTK_HMM_Options *opt)
{
  wrt(fp, &(opt->stream_info.num), sizeof(short), 1);
  wrt(fp, opt->stream_info.vsize, sizeof(short), MAXSTREAMNUM);
  wrt(fp, &(opt->vec_size), sizeof(short), 1);
  wrt(fp, &(opt->cov_type), sizeof(short), 1);
  wrt(fp, &(opt->dur_type), sizeof(short), 1);
  wrt(fp, &(opt->param_type), sizeof(short), 1);
  return TRUE;
}

/** 
 * Write %HMM type of mixture tying.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] pointer to the writing %HMM definition data
 */
static boolean
wt_type(FILE *fp, HTK_HMM_INFO *hmm)
{
  wrt(fp, &(hmm->is_tied_mixture), sizeof(boolean), 1);
  wrt(fp, &(hmm->maxmixturenum), sizeof(int), 1);
  return TRUE;
}


/* write transition data */
static HTK_HMM_Trans **tr_index; ///< Sorted data pointers for mapping from pointer to id
static unsigned int tr_num;	///< Length of above

/** 
 * qsort callback function to sort transition pointers by their
 * address for indexing.
 * 
 * @param t1 [in] data 1
 * @param t2 [in] data 2
 * 
 * @return value required for qsort.
 */
static int
qsort_tr_index(HTK_HMM_Trans **t1, HTK_HMM_Trans **t2)
{
  if (*t1 > *t2) return 1;
  else if (*t1 < *t2) return -1;
  else return 0;
}

/** 
 * @brief  Write all transition matrix data.
 *
 * The pointers of all transition matrixes are first gathered,
 * sorted by the address.  Then the transition matrix data are written
 * by the sorted order.  The index will be used later to convert any pointer
 * reference to a transition matrix into scholar id.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] writing %HMM definition data 
 */
static boolean
wt_trans(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Trans *t;
  unsigned int idx;
  int i;

  tr_num = 0;
  for(t = hmm->trstart; t; t = t->next) tr_num++;
  tr_index = (HTK_HMM_Trans **)mymalloc(sizeof(HTK_HMM_Trans *) * tr_num);
  idx = 0;
  for(t = hmm->trstart; t; t = t->next) tr_index[idx++] = t;
  qsort(tr_index, tr_num, sizeof(HTK_HMM_Trans *), (int (*)(const void *, const void *))qsort_tr_index);
  
  wrt(fp, &tr_num, sizeof(unsigned int), 1);
  for (idx = 0; idx < tr_num; idx++) {
    t = tr_index[idx];
    wrt_str(fp, t->name);
    wrt(fp, &(t->statenum), sizeof(short), 1);
    for(i=0;i<t->statenum;i++) {
      wrt(fp, t->a[i], sizeof(PROB), t->statenum);
    }
  }

  jlog("Stat: write_binhmm: %d transition maxtix written\n", tr_num);

  return TRUE;
}

/** 
 * Binary search function to convert transition matrix pointer to a scholar ID.
 * 
 * @param t [in] pointer to a transition matrix
 * 
 * @return the corresponding scholar ID.
 */
static unsigned int
search_trid(HTK_HMM_Trans *t)
{
  unsigned int left = 0;
  unsigned int right = tr_num - 1;
  unsigned int mid;

  while (left < right) {
    mid = (left + right) / 2;
    if (tr_index[mid] < t) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return(left);
}


/* write variance data */
static HTK_HMM_Var **vr_index;	///< Sorted data pointers for mapping from pointer to id
static unsigned int vr_num;	///< Length of above

/** 
 * qsort callback function to sort variance pointers by their
 * address for indexing.
 * 
 * @param v1 [in] data 1
 * @param v2 [in] data 2
 * 
 * @return value required for qsort.
 */
static int
qsort_vr_index(HTK_HMM_Var **v1, HTK_HMM_Var **v2)
{
  if (*v1 > *v2) return 1;
  else if (*v1 < *v2) return -1;
  else return 0;
}

/** 
 * @brief  Write all variance data.
 *
 * The pointers of all variance vectors are first gathered,
 * sorted by the address.  Then the variance vectors are written
 * by the sorted order.  The index will be used later to convert any pointer
 * reference to a variance vector into scholar id.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] writing %HMM definition data 
 */
static boolean
wt_var(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Var *v;
  unsigned int idx;

  vr_num = 0;
  for(v = hmm->vrstart; v; v = v->next) vr_num++;
  vr_index = (HTK_HMM_Var **)mymalloc(sizeof(HTK_HMM_Var *) * vr_num);
  idx = 0;
  for(v = hmm->vrstart; v; v = v->next) vr_index[idx++] = v;
  qsort(vr_index, vr_num, sizeof(HTK_HMM_Var *), (int (*)(const void *, const void *))qsort_vr_index);  

  wrt(fp, &vr_num, sizeof(unsigned int), 1);
  for (idx = 0; idx < vr_num; idx++) {
    v = vr_index[idx];
    wrt_str(fp, v->name);
    wrt(fp, &(v->len), sizeof(short), 1);
    wrt(fp, v->vec, sizeof(VECT), v->len);
  }
  jlog("Stat: write_binhmm: %d variance written\n", vr_num);

  return TRUE;
}

/** 
 * Binary search function to convert variance pointer to a scholar ID.
 * 
 * @param v [in] pointer to a variance data
 * 
 * @return the corresponding scholar ID.
 */
static unsigned int
search_vid(HTK_HMM_Var *v)
{
  unsigned int left = 0;
  unsigned int right = vr_num - 1;
  unsigned int mid;

  while (left < right) {
    mid = (left + right) / 2;
    if (vr_index[mid] < v) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return(left);
}


/* write density data */
static HTK_HMM_Dens **dens_index; ///< Sorted data pointers for mapping from pointer to id
static unsigned int dens_num;	///< Length of above

/** 
 * qsort callback function to sort density pointers by their
 * address for indexing.
 * 
 * @param d1 [in] data 1
 * @param d2 [in] data 2
 * 
 * @return value required for qsort.
 */
static int
qsort_dens_index(HTK_HMM_Dens **d1, HTK_HMM_Dens **d2)
{
  if (*d1 > *d2) return 1;
  else if (*d1 < *d2) return -1;
  else return 0;
}

/** 
 * @brief  Write all mixture density data.
 *
 * The pointers of all mixture densities are first gathered,
 * sorted by the address.  Then the densities are written
 * by the sorted order.  The pointers to the lower structure (variance etc.)
 * in the data are written in a corresponding scholar id.
 * The pointer index of this data will be used later to convert any pointer
 * reference to a density data into scholar id.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] writing %HMM definition data 
 */
static boolean
wt_dens(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Dens *d;
  unsigned int idx;
  unsigned int vid;

  dens_num = hmm->totalmixnum;
  dens_index = (HTK_HMM_Dens **)mymalloc(sizeof(HTK_HMM_Dens *) * dens_num);
  idx = 0;
  for(d = hmm->dnstart; d; d = d->next) dens_index[idx++] = d;
  qsort(dens_index, dens_num, sizeof(HTK_HMM_Dens *), (int (*)(const void *, const void *))qsort_dens_index);
  
  wrt(fp, &dens_num, sizeof(unsigned int), 1);
  for (idx = 0; idx < dens_num; idx++) {
    d = dens_index[idx];
    wrt_str(fp, d->name);
    wrt(fp, &(d->meanlen), sizeof(short), 1);
    wrt(fp, d->mean, sizeof(VECT), d->meanlen);
    vid = search_vid(d->var);
    /* for debug */
    if (d->var != vr_index[vid]) {
      jlog("Error: write_binhmm: index not match!!!\n");
      return FALSE;
    }
    wrt(fp, &vid, sizeof(unsigned int), 1);
    wrt(fp, &(d->gconst), sizeof(LOGPROB), 1);
  }
  jlog("Stat: write_binhmm: %d gaussian densities written\n", dens_num);

  return TRUE;
}

/** 
 * Binary search function to convert density pointer to a scholar ID.
 * 
 * @param d [in] pointer to a mixture density
 * 
 * @return the corresponding scholar ID.
 */
static unsigned int
search_did(HTK_HMM_Dens *d)
{
  unsigned int left = 0;
  unsigned int right = dens_num - 1;
  unsigned int mid;

  while (left < right) {
    mid = (left + right) / 2;
    if (dens_index[mid] < d) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return(left);
}

/* write stream weight data */
static HTK_HMM_StreamWeight **streamweight_index; ///< Sorted data pointers for mapping from pointer to id
static unsigned int streamweight_num;	///< Length of above

/** 
 * qsort callback function to sort stream weight pointers by their
 * address for indexing.
 * 
 * @param d1 [in] data 1
 * @param d2 [in] data 2
 * 
 * @return value required for qsort.
 */
static int
qsort_streamweight_index(HTK_HMM_StreamWeight **d1, HTK_HMM_StreamWeight **d2)
{
  if (*d1 > *d2) return 1;
  else if (*d1 < *d2) return -1;
  else return 0;
}

/** 
 * @brief  Write all stream weight data.
 *
 * The pointers of all stream weights are first gathered,
 * sorted by the address.  Then the stream weights are written
 * by the sorted order.  The pointers to the lower structure (variance etc.)
 * in the data are written in a corresponding scholar id.
 * The pointer index of this data will be used later to convert any pointer
 * reference to a data into scholar id.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] writing %HMM definition data 
 */
static boolean
wt_streamweight(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_StreamWeight *sw;
  unsigned int idx;

  streamweight_num = 0;
  for(sw=hmm->swstart;sw;sw=sw->next) streamweight_num++;
  streamweight_index = (HTK_HMM_StreamWeight **)mymalloc(sizeof(HTK_HMM_StreamWeight *) * streamweight_num);
  idx = 0;
  for(sw = hmm->swstart; sw; sw = sw->next) streamweight_index[idx++] = sw;
  qsort(streamweight_index, streamweight_num, sizeof(HTK_HMM_StreamWeight *), (int (*)(const void *, const void *))qsort_streamweight_index);
  
  wrt(fp, &streamweight_num, sizeof(unsigned int), 1);
  for (idx = 0; idx < streamweight_num; idx++) {
    sw = streamweight_index[idx];
    wrt_str(fp, sw->name);
    wrt(fp, &(sw->len), sizeof(short), 1);
    wrt(fp, sw->weight, sizeof(VECT), sw->len);
  }
  jlog("Stat: write_binhmm: %d stream weights written\n", streamweight_num);

  return TRUE;
}

/** 
 * Binary search function to convert stream weight pointer to a scholar ID.
 * 
 * @param d [in] pointer to a mixture density
 * 
 * @return the corresponding scholar ID.
 */
static unsigned int
search_swid(HTK_HMM_StreamWeight *sw)
{
  unsigned int left = 0;
  unsigned int right = streamweight_num - 1;
  unsigned int mid;

  while (left < right) {
    mid = (left + right) / 2;
    if (streamweight_index[mid] < sw) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return(left);
}


/* write tmix data */
static GCODEBOOK **tm_index; ///< Sorted data pointers for mapping from pointer to id
static unsigned int tm_num;	///< Length of above
static unsigned int tm_idx;	///< Current index

/** 
 * Traverse callback function to store pointers in @a tm_index.
 * 
 * @param p [in] pointer to the codebook data
 */
static void
tmix_list_callback(void *p)
{
  GCODEBOOK *tm;
  tm = p;
  tm_index[tm_idx++] = tm;
}

/** 
 * qsort callback function to sort density pointers by their
 * address for indexing.
 * 
 * @param tm1 [in] data 1
 * @param tm2 [in] data 2
 * 
 * @return value required for qsort.
 */
static int
qsort_tm_index(GCODEBOOK **tm1, GCODEBOOK **tm2)
{
  if (*tm1 > *tm2) return 1;
  else if (*tm1 < *tm2) return -1;
  else return 0;
}

/** 
 * @brief  Write all codebook data.
 *
 * The pointers of all codebook densities are first gathered,
 * sorted by the address.  Then the densities are written
 * by the sorted order.  The pointers to the lower structure (mixture etc.)
 * in the data are written by the corresponding scholar id.
 * The pointer index of this data will be used later to convert any pointer
 * reference to a codebook into scholar id.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] writing %HMM definition data 
 */
static boolean
wt_tmix(FILE *fp, HTK_HMM_INFO *hmm)
{
  GCODEBOOK *tm;
  unsigned int idx;
  unsigned int did;
  int i;

  tm_num = hmm->codebooknum;
  tm_index = (GCODEBOOK **)mymalloc(sizeof(GCODEBOOK *) * tm_num);
  tm_idx = 0;
  aptree_traverse_and_do(hmm->codebook_root, tmix_list_callback);
  qsort(tm_index, tm_num, sizeof(GCODEBOOK *), (int (*)(const void *, const void *))qsort_tm_index);  

  wrt(fp, &tm_num, sizeof(unsigned int), 1);
  for (idx = 0; idx < tm_num; idx++) {
    tm = tm_index[idx];
    wrt_str(fp, tm->name);
    wrt(fp, &(tm->num), sizeof(int), 1);
    for(i=0;i<tm->num;i++) {
      if (tm->d[i] == NULL) {
	did = dens_num;
      } else {
	did = search_did(tm->d[i]);
	/* for debug */
	if (tm->d[i] != dens_index[did]) {
	  jlog("Error: write_binhmm: index not match!!!\n");
	  return FALSE;
	}
      }
      wrt(fp, &did, sizeof(unsigned int), 1);
    }
  }
  jlog("Stat: write_binhmm: %d tied-mixture codebooks written\n", tm_num);

  return TRUE;
}

/** 
 * Binary search function to convert codebook pointer to a scholar ID.
 * 
 * @param tm [in] pointer to a codebook
 * 
 * @return the corresponding scholar ID.
 */
static unsigned int
search_tmid(GCODEBOOK *tm)
{
  unsigned int left = 0;
  unsigned int right = tm_num - 1;
  unsigned int mid;

  while (left < right) {
    mid = (left + right) / 2;
    if (tm_index[mid] < tm) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return(left);
}


/* write mixture pdf data */
static HTK_HMM_PDF **mpdf_index; ///< Sorted data pointers for mapping from pointer to id
static unsigned int mpdf_num;	///< Length of above

/** 
 * qsort callback function to sort mixture PDF pointers by their
 * address for indexing.
 * 
 * @param d1 [in] data 1
 * @param d2 [in] data 2
 * 
 * @return value required for qsort.
 */
static int
qsort_mpdf_index(HTK_HMM_PDF **d1, HTK_HMM_PDF **d2)
{
  if (*d1 > *d2) return 1;
  else if (*d1 < *d2) return -1;
  else return 0;
}

/**
 * Write a mixture PDF.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] writing %HMM definition data 
 * @param m [out] mixture PDF to be written
 * 
 * @return TRUE on success, FALSE on error.
 * 
 */
static boolean
wt_pdf_sub(FILE *fp, HTK_HMM_INFO *hmm, HTK_HMM_PDF *m)
{
  unsigned int did;
  int i;
  short dummy;
  
  if (hmm->is_tied_mixture) {
    /* try tmix */
    did = search_tmid((GCODEBOOK *)(m->b));
    if ((GCODEBOOK *)m->b == tm_index[did]) {
      /* tmix */
      dummy = -1;
      wrt(fp, &dummy, sizeof(short), 1);
      wrt(fp, &did, sizeof(unsigned int), 1);
    } else {
      /* tmix failed -> normal mixture */
      wrt(fp, &(m->mix_num), sizeof(short), 1);
      for (i=0;i<m->mix_num;i++) {
	if (m->b[i] == NULL) {
	  did = dens_num;
	} else {
	  did = search_did(m->b[i]);
	  if (m->b[i] != dens_index[did]) {
	    jlog("Error: write_binhmm: index not match!!!\n");
	    return FALSE;
	  }
	}
	wrt(fp, &did, sizeof(unsigned int), 1);
      }
    }
  } else {			/* not tied mixture */
    wrt(fp, &(m->mix_num), sizeof(short), 1);
    for (i=0;i<m->mix_num;i++) {
      if (m->b[i] == NULL) {
	did = dens_num;
      } else {
	did = search_did(m->b[i]);
	if (m->b[i] != dens_index[did]) {
	  jlog("Error: write_binhmm: index not match!!!\n");
	  return FALSE;
	}
      }
      wrt(fp, &did, sizeof(unsigned int), 1);
    }
  }
  wrt(fp, m->bweight, sizeof(PROB), m->mix_num);

  return TRUE;
}

/** 
 * @brief  Write all mixture pdf data.
 *
 * The pointers of all mixture pdfs are first gathered,
 * sorted by the address.  Then the mixture pdfs are written
 * by the sorted order.  The pointers to the lower structure (variance etc.)
 * in the data are written in a corresponding scholar id.
 * The pointer index of this data will be used later to convert any pointer
 * reference to a data into scholar id.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] writing %HMM definition data 
 */
static boolean
wt_mpdf(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_PDF *m;
  unsigned int idx;

  mpdf_num = 0;
  for(m=hmm->pdfstart;m;m=m->next) mpdf_num++;
  mpdf_index = (HTK_HMM_PDF **)mymalloc(sizeof(HTK_HMM_PDF *) * mpdf_num);
  idx = 0;
  for(m=hmm->pdfstart;m;m=m->next) mpdf_index[idx++] = m;
  qsort(mpdf_index, mpdf_num, sizeof(HTK_HMM_PDF *), (int (*)(const void *, const void *))qsort_mpdf_index);
  
  wrt(fp, &mpdf_num, sizeof(unsigned int), 1);
  for (idx = 0; idx < mpdf_num; idx++) {
    m = mpdf_index[idx];
    wrt_str(fp, m->name);
    wrt(fp, &(m->stream_id), sizeof(short), 1);
    if (wt_pdf_sub(fp, hmm, m) == FALSE) return FALSE;
  }

  jlog("Stat: write_binhmm: %d mixture PDF written\n", mpdf_num);

  return TRUE;
}

/** 
 * Binary search function to convert mixture pdf pointer to a scholar ID.
 * 
 * @param m [in] pointer to a mixture pdf
 * 
 * @return the corresponding scholar ID.
 */
static unsigned int
search_mpdfid(HTK_HMM_PDF *m)
{
  unsigned int left = 0;
  unsigned int right = mpdf_num - 1;
  unsigned int mid;

  while (left < right) {
    mid = (left + right) / 2;
    if (mpdf_index[mid] < m) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return(left);
}


/* write state data */
static HTK_HMM_State **st_index; ///< Sorted data pointers for mapping from pointer to id
static unsigned int st_num;	///< Length of above

/** 
 * qsort callback function to sort state pointers by their
 * address for indexing.
 * 
 * @param s1 [in] data 1
 * @param s2 [in] data 2
 * 
 * @return value required for qsort.
 */
static int
qsort_st_index(HTK_HMM_State **s1, HTK_HMM_State **s2)
{
  /* keep ID order */
  if ((*s1)->id > (*s2)->id) return 1;
  else if ((*s1)->id < (*s2)->id) return -1;
  else return 0;
}

/** 
 * @brief  Write all state data.
 *
 * The pointers of all states are first gathered,
 * sorted by the address.  Then the state informations are written
 * by the sorted order.  The pointers to the lower structure (mixture etc.)
 * in the data are written in a corresponding scholar id.
 * The pointer index of this data will be used later to convert any pointer
 * reference to a state data into scholar id.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] writing %HMM definition data
 * @param mpdf_macro [in] TRUE if mixture PDFs are already read as separated definitions
 */
static boolean
wt_state(FILE *fp, HTK_HMM_INFO *hmm, boolean mpdf_macro)
{
  HTK_HMM_State *s;
  unsigned int idx;
  unsigned int mid;
  unsigned int swid;
  int m;

  st_num = hmm->totalstatenum;
  st_index = (HTK_HMM_State **)mymalloc(sizeof(HTK_HMM_State *) * st_num);
  idx = 0;
  for(s = hmm->ststart; s; s = s->next) st_index[idx++] = s;
  qsort(st_index, st_num, sizeof(HTK_HMM_State *), (int (*)(const void *, const void *))qsort_st_index);
  
  wrt(fp, &st_num, sizeof(unsigned int), 1);
  for (idx = 0; idx < st_num; idx++) {
    s = st_index[idx];
    wrt_str(fp, s->name);
    if (mpdf_macro) {
      /* mpdf are already written, so write index */
      for(m=0;m<s->nstream;m++) {
	if (s->pdf[m] == NULL) {
	  mid = mpdf_num;
	} else {
	  mid = search_mpdfid(s->pdf[m]);
	  if (s->pdf[m] != mpdf_index[mid]) {
	    jlog("Error: write_binhmm: index not match!!!\n");
	    return FALSE;
	  }
	}
	wrt(fp, &mid, sizeof(unsigned int), 1);
      }
    } else {
      /* mpdf should be written here */
      for(m=0;m<s->nstream;m++) {
	/* stream_id will not be written */
	if (wt_pdf_sub(fp, hmm, s->pdf[m]) == FALSE) return FALSE;
      }
    }
    if (hmm->opt.stream_info.num > 1) {
      /* write steam weight */
      if (s->w == NULL) {
	swid = streamweight_num;
      } else {
	swid = search_swid(s->w);
	if (s->w != streamweight_index[swid]) {
	  jlog("Error: write_binhmm: index not match!!!\n");
	  return FALSE;
	}
      }
      wrt(fp, &swid, sizeof(unsigned int), 1);
    }
  }

  jlog("Stat: write_binhmm: %d states written\n", st_num);

  return TRUE;
}

/** 
 * Binary search function to convert state pointer to a scholar ID.
 * 
 * @param s [in] pointer to a state
 * 
 * @return the corresponding scholar ID.
 */
static unsigned int
search_stid(HTK_HMM_State *s)
{
  unsigned int left = 0;
  unsigned int right = st_num - 1;
  unsigned int mid;

  while (left < right) {
    mid = (left + right) / 2;
    /* search by id */
    if (st_index[mid]->id < s->id) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return(left);
}


/** 
 * @brief  Write all model data.
 *
 * The data of all models are written.  The order is not important
 * at this top level, since there are no reference to this data.
 * The pointers to the lower structure (states, transitions, etc.)
 * in the data are written by the corresponding scholar id.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] writing %HMM definition data 
 */
static boolean
wt_data(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Data *d;
  unsigned int md_num;
  unsigned int sid, tid;
  int i;

  md_num = hmm->totalhmmnum;

  wrt(fp, &(md_num), sizeof(unsigned int), 1);
  for(d = hmm->start; d; d = d->next) {
    wrt_str(fp, d->name);
    wrt(fp, &(d->state_num), sizeof(short), 1);
    for (i=0;i<d->state_num;i++) {
      if (d->s[i] != NULL) {
	sid = search_stid(d->s[i]);
	/* for debug */
	if (d->s[i] != st_index[sid]) {
	  jlog("Error: write_binhmm: index not match!!!\n");
	  return FALSE;
	}
      } else {
	sid = hmm->totalstatenum + 1; /* error value */
      }
      wrt(fp, &sid, sizeof(unsigned int), 1);
    }
    tid = search_trid(d->tr);
    /* for debug */
    if (d->tr != tr_index[tid]) {
      jlog("Error: write_binhmm: index not match!!!\n");
      return FALSE;
    }
    wrt(fp, &tid, sizeof(unsigned int), 1);
  }
  jlog("Stat: write_binhmm: %d HMM model definition written\n", md_num);
  return TRUE;
}


/** 
 * Top function to write %HMM definition data to a binary file.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] %HMM definition structure to be written
 * @param para [in] acoustic analysis parameter, or NULL if not available
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
write_binhmm(FILE *fp, HTK_HMM_INFO *hmm, Value *para)
{
  boolean mpdf_macro;

  if (hmm->pdf_root != NULL) {
    /* "~p" macro definition exist */
    /* save mixture pdf separatedly from state definition */
    mpdf_macro = TRUE;
    jlog("Stat: write_binhmm: mixture PDF macro \"~p\" used, use qualifier \'M\'\n");
  } else {
    mpdf_macro = FALSE;
  }

  /* write header */
  if (wt_header(fp, (para ? TRUE : FALSE), hmm->variance_inversed, mpdf_macro) == FALSE) {
    jlog("Error: write_binhmm: failed to write header\n");
    return FALSE;
  }

  if (para) {
    /* write acoustic analysis parameter info */
    if (wt_para(fp, para) == FALSE) {
      jlog("Error: write_binhmm: failed to write acoustic analysis parameters\n");
      return FALSE;
    }
  }
  
  /* write option data */
  if (wt_opt(fp, &(hmm->opt)) == FALSE) {
    jlog("Error: write_binhmm: failed to write option data\n");
    return FALSE;
  }

  /* write type data */
  if (wt_type(fp, hmm) == FALSE) {
    jlog("Error: write_binhmm: failed to write HMM type data\n");
    return FALSE;
  }

  /* write transition data */
  if (wt_trans(fp, hmm) == FALSE) {
    jlog("Error: write_binhmm: failed to write HMM transition data\n");
    return FALSE;
  }

  /* write variance data */
  if (wt_var(fp, hmm) == FALSE) {
    jlog("Error: write_binhmm: failed to write HMM variance data\n");
    return FALSE;
  }

  /* write density data */
  if (wt_dens(fp, hmm) == FALSE) {
    jlog("Error: write_binhmm: failed to write density data\n");
    return FALSE;
  }

  /* write stream weight data */
  if (hmm->opt.stream_info.num > 1) {
    if (wt_streamweight(fp, hmm) == FALSE) {
      jlog("Error: write_binhmm: failed to write stream weights data\n");
      return FALSE;
    }
  }

  /* write tmix data */
  if (hmm->is_tied_mixture) {
    if (wt_tmix(fp, hmm) == FALSE) {
      jlog("Error: write_binhmm: failed to write tied-mixture codebook data\n");
      return FALSE;
    }
  }

  /* write mixture pdf data */
  if (mpdf_macro) {
    if (wt_mpdf(fp, hmm) == FALSE) {
      jlog("Error: write_binhmm: failed to write mixture pdf data\n");
      return FALSE;
    }
  }
    
  /* write state data */
  if (wt_state(fp, hmm, mpdf_macro) == FALSE) {
    jlog("Error: write_binhmm: failed to write HMM state data\n");
    return FALSE;
  }

  /* write model data */
  if (wt_data(fp, hmm) == FALSE) {
    jlog("Error: write_binhmm: failed to write HMM data\n");
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

  return (TRUE);
}
