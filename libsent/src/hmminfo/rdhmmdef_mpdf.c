/**
 * @file   rdhmmdef_mpdf.c
 * 
 * <JA>
 * @brief  HTK %HMM 定義ファイルの読み込み：ガウス混合分布
 * </JA>
 * 
 * <EN>
 * @brief  Read HTK %HMM definition file: Gaussian mixture PDF
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 01:43:43 2005
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
#include <sent/htk_hmm.h>

extern char *rdhmmdef_token;	///< Current token

/** 
 * Allocate a new data area and return it.
 * 
 * @return pointer to newly allocated data.
 */
static HTK_HMM_PDF *
mpdf_new(HTK_HMM_INFO *hmm)
{
  HTK_HMM_PDF *new;

  new = (HTK_HMM_PDF *)mybmalloc2(sizeof(HTK_HMM_PDF), &(hmm->mroot));

  new->name = NULL;
  new->tmix = FALSE;
  new->stream_id = -1;
  new->mix_num = 0;
  new->b = NULL;
  new->bweight = NULL;
  new->next = NULL;

  return(new);
}

/** 
 * Add a new data to the global structure.
 * 
 * @param hmm [i/o] %HMM definition data to store it
 * @param new [in] new data to be added
 */
void
mpdf_add(HTK_HMM_INFO *hmm, HTK_HMM_PDF *new)
{
  HTK_HMM_PDF *match;

  /* link data structure */
  new->next = hmm->pdfstart;
  hmm->pdfstart = new;

  if (new->name != NULL) {
    /* add index to search index tree */
    if (hmm->pdf_root == NULL) {
      hmm->pdf_root = aptree_make_root_node(new, &(hmm->mroot));
    } else {
      match = aptree_search_data(new->name, hmm->pdf_root);
      if (match != NULL && strmatch(match->name, new->name)) {
	jlog("Error: rdhmmdef_dens: ~m \"%s\" is already defined\n", new->name);
	rderr(NULL);
      } else {
	aptree_add_entry(new->name, new, match->name, &(hmm->pdf_root), &(hmm->mroot));
      }
    }
  }
}

/** 
 * Look up a data macro by the name.
 * 
 * @param hmm [in] %HMM definition data
 * @param keyname [in] macro name to find
 * 
 * @return pointer to the found data, or NULL if not found.
 */
HTK_HMM_PDF *
mpdf_lookup(HTK_HMM_INFO *hmm, char *keyname)
{
  HTK_HMM_PDF *d;

  d = aptree_search_data(keyname, hmm->pdf_root);
  if (d != NULL && strmatch(d->name, keyname)) {
    return d;
  } else {
    return NULL;
  }
}

/** 
 * @brief  Read one new data and returns the pointer
 *
 * If a sub-component of this data is directly defined at here, they
 * will be read from here and assigned to this data.  If a sub-component
 * is not defined here but a macro name referencing to the component previously
 * defined in other place, the data will be searched by the macro name and
 * the pointer to the found component will be assigned to this model.
 * 
 * @param fp [in] file pointer
 * @param hmm [in] %HMM definition data
 * @param mix_num [in] num of Gaussians to be read, or -1 if not specified
 * 
 * @return pointer to the newly read data.
 */
static HTK_HMM_PDF *
mpdf_read(FILE *fp, HTK_HMM_INFO *hmm, int mix_num)
{
  HTK_HMM_PDF *new;
  int i, mid;
  boolean no_nummixes;

  new = mpdf_new(hmm);

  /* allow <Stream> inside pdf */
  if (currentis("STREAM")) {
    read_token(fp);
    NoTokErr("missing STREAM value");
    new->stream_id = atoi(rdhmmdef_token) - 1;
    read_token(fp);
  }

  /* allow <NumMixes> in stream definition */
  if (mix_num == -1) {
    no_nummixes = TRUE;
  } else {
    no_nummixes = FALSE;
  }
  if (currentis("NUMMIXES")) {
    read_token(fp);
    new->mix_num = atoi(rdhmmdef_token);
    if (mix_num != -1 && new->mix_num != mix_num) {
      jlog("Error: rdhmmdef_mpdf: <NumMixes> exists both in mpdf definition and its referer, and the values are different (%d != %d)\n", new->mix_num, mix_num);
      rderr(NULL);
    }
    read_token(fp);
    no_nummixes = FALSE;
  } else {
    if (mix_num != -1) {
      new->mix_num = mix_num;
    } else {
      /* no NumMixes, assume single gaussian */
      new->mix_num = 1;
    }
  }
  
  if (currentis("TMIX")) {
    read_token(fp);
    /* read in TMIX */
    tmix_read(fp, new, hmm);
    /* mark this */
    new->tmix = TRUE;

  } else {
    
    new->b = (HTK_HMM_Dens **) mybmalloc2(sizeof(HTK_HMM_Dens *) * new->mix_num, &(hmm->mroot));
    new->bweight = (PROB *) mybmalloc2(sizeof(PROB) * new->mix_num, &(hmm->mroot));
    for (i=0;i<new->mix_num;i++) {
      new->b[i] = NULL;
      new->bweight[i] = LOG_ZERO;
    }
      
    if (no_nummixes) {	/* no NumMixes */
      mid = 0;
      new->bweight[mid] = 0.0;
      new->b[mid] = get_dens_data(fp, hmm);
    } else {
      for (;;) {
	if (!currentis("MIXTURE")) break;
	read_token(fp);
	NoTokErr("missing MIXTURE id");
	mid = atoi(rdhmmdef_token) - 1;
	read_token(fp);
	NoTokErr("missing MIXTURE weight");
	new->bweight[mid] = (PROB)log(atof(rdhmmdef_token));
	read_token(fp);
	new->b[mid] = get_dens_data(fp, hmm);
      }
    }

    new->tmix = FALSE;
  }

  return (new);
}

/** 
 * @brief  Return a pointer to the data located at the current point.
 *
 * If the current point is a macro reference, the pointer to the
 * already defined data will be searched and returned.
 * Otherwise, the definition of the data will be read from the current
 * point and pointer to the newly allocated data will be returned.
 * 
 * @param fp [in] file pointer
 * @param hmm [i/o] %HMM definition data
 * @param mix_num [in] num of Gaussians to be read, or -1 if not specified
 * @param stream_id [in] stream ID, or -1 if not specified yet
 * 
 * @return pointer to the data located at the current point.
 */
HTK_HMM_PDF *
get_mpdf_data(FILE *fp, HTK_HMM_INFO *hmm, int mix_num, short stream_id)
{
  HTK_HMM_PDF *tmp = NULL;

  if (currentis("~p")) {
    /* macro reference: lookup and return the pointer */
    read_token(fp);
    NoTokErr("missing macro name");
    tmp = mpdf_lookup(hmm, rdhmmdef_token);
    if (tmp == NULL) {
      jlog("Error: rdhmmdef_mpdf: ~p \"%s\" not defined\n", rdhmmdef_token);
      rderr(NULL);
    }
    if (mix_num != -1 && tmp->mix_num != mix_num) {
      jlog("Error: rdhmmdef_mpdf: mixture num in ~p \"%s\" definition and referer is different (%d != %d)\n", rdhmmdef_token, tmp->mix_num, mix_num);
      rderr(NULL);
    }
    if (tmp->stream_id != stream_id) {
      jlog("Error: rdhmmdef_mpdf: stream number in ~p \"%s\" definition and referer is different (%d != %d)\n", rdhmmdef_token, tmp->stream_id + 1, stream_id + 1);
      rderr(NULL);
    }
    read_token(fp);
  } else if (currentis("NUMMIXES")||currentis("MIXTURE")||currentis("TMIX")||currentis("MEAN")||currentis("~m")||currentis("RCLASS")) {
    /* definition: define density data, and return the pointer */
    tmp = mpdf_read(fp, hmm, mix_num);
    if (tmp->stream_id == -1) {
      tmp->stream_id = stream_id;
    } else if (tmp->stream_id != stream_id) {
      jlog("Error: rdhmmdef_mpdf: stream number exist in inline mpdf definition and referer is different (%d != %d)\n", rdhmmdef_token, tmp->stream_id + 1, stream_id + 1);
      rderr(NULL);
    }
    tmp->name = NULL; /* no name */
    mpdf_add(hmm, tmp);
  } else {
    rderr("syntax error: not mixture pdf data");
  }
  return tmp;
}


/** 
 * Read a new data and store it as a macro.
 * 
 * @param name [in] macro name
 * @param fp [in] file pointer
 * @param hmm [i/o] %HMM definition data
 */
void
def_mpdf_macro(char *name, FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_PDF *new;

  /* read in data and return newly malloced data */
  new = mpdf_read(fp, hmm, -1);
  if (new->stream_id == -1) {
    jlog("Error: rdhmmdef_pdf: definition of ~p \"%s\" has no <Stream>\n", name);
    rderr(NULL);
  }

  /* register it to the grobal HMM structure */
  new->name = name;
  mpdf_add(hmm, new);
}

/* end of file */
