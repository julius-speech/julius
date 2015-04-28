/**
 * @file   rdhmmdef_var.c
 * 
 * <JA>
 * @brief  HTK %HMM 定義ファイルの読み込み：ガウス分布の分散ベクトル
 * </JA>
 * 
 * <EN>
 * @brief  Read HTK %HMM definition file: variance vector of Gaussian
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 04:01:38 2005
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

/* currenty cannot treat other sub macros (~u,~i,~x) */

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>

extern char *rdhmmdef_token;	///< Current token

/** 
 * Allocate a new data area and return it.
 * 
 * @return pointer to newly allocated data.
 */
static HTK_HMM_Var *
var_new(HTK_HMM_INFO *hmm)
{
  HTK_HMM_Var *new;

  new = (HTK_HMM_Var *)mybmalloc2(sizeof(HTK_HMM_Var), &(hmm->mroot));

  new->name = NULL;
  new->vec = NULL;
  new->len = 0;
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
var_add(HTK_HMM_INFO *hmm, HTK_HMM_Var *new)
{
  HTK_HMM_Var *match;

  /* link data structure */
  new->next = hmm->vrstart;
  hmm->vrstart = new;
  
  if (new->name != NULL) {
    /* add index to search index tree */
    if (hmm->vr_root == NULL) {
      hmm->vr_root = aptree_make_root_node(new, &(hmm->mroot));
    } else {
      match = aptree_search_data(new->name, hmm->vr_root);
      if (match != NULL && strmatch(match->name, new->name)) {
	jlog("Error: rdhmmdef_var: ~v \"%s\" is already defined\n", new->name);
	rderr(NULL);
      } else {
	aptree_add_entry(new->name, new, match->name, &(hmm->vr_root), &(hmm->mroot));
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
static HTK_HMM_Var *
var_lookup(HTK_HMM_INFO *hmm, char *keyname)
{
  HTK_HMM_Var *v;

  v = aptree_search_data(keyname, hmm->vr_root);
  if (v != NULL && strmatch(v->name, keyname)) {
    return v;
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
 * @param hmm [i/o] %HMM definition data to store it
 * 
 * @return pointer to the newly read data.
 */
static HTK_HMM_Var *
var_read(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Var *new;
  int i;

  new = var_new(hmm);

  /* read covariance matrix (diagonal vector) */

  if (!currentis("VARIANCE")) {
    jlog("Error: rdhmmdef_var: variance matrix type \"%s\" not supported\n", rdhmmdef_token);
    rderr(NULL);
  } else {
    read_token(fp);
    NoTokErr("missing VARIANCE vector length");
    new->len = atoi(rdhmmdef_token);
    read_token(fp);
    new->vec = (VECT *)mybmalloc2(sizeof(VECT) * new->len, &(hmm->mroot));
    /* needs comversion if integerized */
    for (i=0;i<new->len;i++) {
      NoTokErr("missing some VARIANCE element");
      new->vec[i] = (VECT)atof(rdhmmdef_token);
      read_token(fp);
    }
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
HTK_HMM_Var *
get_var_data(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Var *tmp;

  if (currentis("~v")) {
    /* macro reference: lookup and return the pointer */
    read_token(fp);
    NoTokErr("missing VARIANCE macro name");
    tmp = var_lookup(hmm, rdhmmdef_token);
    if (tmp == NULL) {
      jlog("Error: rdhmmdef_var: ~v \"%s\" not defined\n", rdhmmdef_token);
      rderr(NULL);
    }
    read_token(fp);
    return tmp;
  } else if (currentis("VARIANCE")){
    /* definition: define variance data, and return the pointer */
    tmp = var_read(fp, hmm);
    tmp->name = NULL; /* no name */
    var_add(hmm, tmp);
    return tmp;
  } else {
    rderr("no variance data");
    return NULL;
  }
}

/** 
 * Read a new data and store it as a macro.
 * 
 * @param name [in] macro name
 * @param fp [in] file pointer
 * @param hmm [i/o] %HMM definition data
 */
void
def_var_macro(char *name, FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Var *new;

  
  /* read in data and return newly malloced data */
  new = var_read(fp, hmm);

  /* register it to the grobal HMM structure */
  new->name = name;
  var_add(hmm, new);
}
