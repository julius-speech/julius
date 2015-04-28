/**
 * @file   rdhmmdef_dens.c
 * 
 * <JA>
 * @brief  HTK %HMM 定義ファイルの読み込み：ガウス分布
 * </JA>
 * 
 * <EN>
 * @brief  Read HTK %HMM definition file: Gaussian density
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 01:43:43 2005
 *
 * $Revision: 1.6 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* $Id: rdhmmdef_dens.c,v 1.6 2013/06/20 17:14:21 sumomo Exp $ */

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>

extern char *rdhmmdef_token;	///< Current token

/** 
 * Calculate and set the GCONST value.
 * 
 * @param d [i/o] density data
 */
/* GCONST = log((2*PI)^n|\sigma|) */
static void
update_gconst(HTK_HMM_Dens *d)
{
  LOGPROB gconst;
  int i;

  gconst = (LOGPROB)(d->var->len * LOGTPI);
  for (i=0;i<d->var->len;i++) {
    gconst += (LOGPROB)log(d->var->vec[i]);
  }
  d->gconst = gconst;
}

/** 
 * Allocate a new data area and return it.
 * 
 * @return pointer to newly allocated data.
 */
static HTK_HMM_Dens *
dens_new(HTK_HMM_INFO *hmm)
{
  HTK_HMM_Dens *new;

  new = (HTK_HMM_Dens *)mybmalloc2(sizeof(HTK_HMM_Dens), &(hmm->mroot));

  new->name = NULL;
  new->meanlen = 0;
  new->mean = NULL;
  new->var = NULL;
  new->gconst = 0.0;
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
dens_add(HTK_HMM_INFO *hmm, HTK_HMM_Dens *new)
{
  HTK_HMM_Dens *match;

  /* link data structure */
  new->next = hmm->dnstart;
  hmm->dnstart = new;

  if (new->name != NULL) {
    /* add index to search index tree */
    if (hmm->dn_root == NULL) {
      hmm->dn_root = aptree_make_root_node(new, &(hmm->mroot));
    } else {
      match = aptree_search_data(new->name, hmm->dn_root);
      if (match != NULL && strmatch(match->name, new->name)) {
	jlog("Error: rdhmmdef_dens: ~m \"%s\" is already defined\n", new->name);
	rderr(NULL);
      } else {
	aptree_add_entry(new->name, new, match->name, &(hmm->dn_root), &(hmm->mroot));
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
HTK_HMM_Dens *
dens_lookup(HTK_HMM_INFO *hmm, char *keyname)
{
  HTK_HMM_Dens *d;

  d = aptree_search_data(keyname, hmm->dn_root);
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
 * 
 * @return pointer to the newly read data.
 */
static HTK_HMM_Dens *
dens_read( FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Dens *new;
  int i;

  new = dens_new(hmm);

  /* read regression class ID (just skip) */
  if (currentis("RCLASS")) {
    read_token(fp);
    NoTokErr("no RCLASS arg");
    read_token(fp);
  }
  /* read mean vector */
  if (!currentis("MEAN")) rderr("<MEAN> not found");
  read_token(fp); NoTokErr("MEAN vector length not found");
  new->meanlen = atoi(rdhmmdef_token);
  read_token(fp);
  new->mean = (VECT *)mybmalloc2(sizeof(VECT) * new->meanlen, &(hmm->mroot));
  /* needs comversion if integerized */
  for (i=0;i<new->meanlen;i++) {
    NoTokErr("missing MEAN element");
    new->mean[i] = (VECT)atof(rdhmmdef_token);
    read_token(fp);
  }

  /* read covariance matrix data */
  new->var = get_var_data(fp, hmm);
  if ((new->var)->len != new->meanlen) {
    rderr("mean vector length != variance vector len");
  }

  /* read GCONST if any */
  if (currentis("GCONST")) {
    read_token(fp);
    NoTokErr("GCONST found but no value");
    new->gconst = (LOGPROB)atof(rdhmmdef_token);
    read_token(fp);
  } else {
    /* calc */
    update_gconst(new);
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
 * 
 * @return pointer to the data located at the current point.
 */
HTK_HMM_Dens *
get_dens_data(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Dens *tmp = NULL;

  if (currentis("~m")) {
    /* macro reference: lookup and return the pointer */
    read_token(fp);
    NoTokErr("missing macro name");
    tmp = dens_lookup(hmm, rdhmmdef_token);
    if (tmp == NULL) {
      jlog("Error: rdhmmdef_dens: ~m \"%s\" not defined\n", rdhmmdef_token);
      rderr(NULL);
    }
    read_token(fp);
  } else if (currentis("MEAN") || currentis("RCLASS")) {
    /* definition: define density data, and return the pointer */
    tmp = dens_read(fp, hmm);
    tmp->name = NULL; /* no name */
    dens_add(hmm, tmp);
  } else {
    rderr("no density data");
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
def_dens_macro(char *name, FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Dens *new;

  /* read in data and return newly malloced data */
  new = dens_read(fp, hmm);

  /* register it to the grobal HMM structure */
  new->name = name;
  dens_add(hmm, new);
}
