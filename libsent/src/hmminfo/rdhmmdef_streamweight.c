/**
 * @file   rdhmmdef_streamweight.c
 * 
 * <JA>
 * @brief  HTK %HMM 定義ファイルの読み込み：ストリーム重み
 * </JA>
 * 
 * <EN>
 * @brief  Read HTK %HMM definition file: stream weights
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 04:01:38 2005
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
static HTK_HMM_StreamWeight *
sw_new(HTK_HMM_INFO *hmm)
{
  HTK_HMM_StreamWeight *new;

  new = (HTK_HMM_StreamWeight *)mybmalloc2(sizeof(HTK_HMM_StreamWeight), &(hmm->mroot));

  new->name = NULL;
  new->weight = NULL;
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
sw_add(HTK_HMM_INFO *hmm, HTK_HMM_StreamWeight *new)
{
  HTK_HMM_StreamWeight *match;

  /* link data structure */
  new->next = hmm->swstart;
  hmm->swstart = new;
  
  if (new->name != NULL) {
    /* add index to search index tree */
    if (hmm->sw_root == NULL) {
      hmm->sw_root = aptree_make_root_node(new, &(hmm->mroot));
    } else {
      match = aptree_search_data(new->name, hmm->sw_root);
      if (match != NULL && strmatch(match->name, new->name)) {
	jlog("Error: rdhmmdef_streamweight: ~w \"%s\" is already defined\n", new->name);
	rderr(NULL);
      } else {
	aptree_add_entry(new->name, new, match->name, &(hmm->sw_root), &(hmm->mroot));
      }
    }
  }
  
}

/** 
 * Look up a stream weight macro definition by the name.
 * 
 * @param hmm [in] %HMM definition data
 * @param keyname [in] macro name to find
 * 
 * @return pointer to the found data, or NULL if not found.
 */
static HTK_HMM_StreamWeight *
sw_lookup(HTK_HMM_INFO *hmm, char *keyname)
{
  HTK_HMM_StreamWeight *sw;

  sw = aptree_search_data(keyname, hmm->sw_root);
  if (sw != NULL && strmatch(sw->name, keyname)) {
    return sw;
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
static HTK_HMM_StreamWeight *
sw_read(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_StreamWeight *new;
  int i;

  new = sw_new(hmm);
  /* read stream weight */
  if (!currentis("SWEIGHTS")) {
    jlog("Error: rdhmmdef_streamweight: failed to read stream weight: \"%s\"\n", rdhmmdef_token);
    rderr(NULL);
  } else {
    read_token(fp);
    NoTokErr("missing SWEIGHTS vector length");
    new->len = atoi(rdhmmdef_token);
    read_token(fp);
    new->weight = (VECT *)mybmalloc2(sizeof(VECT) * new->len, &(hmm->mroot));
    /* needs conversion if integerized */
    for (i=0;i<new->len;i++) {
      NoTokErr("missing some SWEIGHTS element");
      new->weight[i] = (VECT)atof(rdhmmdef_token);
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
HTK_HMM_StreamWeight *
get_streamweight_data(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_StreamWeight *tmp;

  if (currentis("~w")) {
    /* macro reference: lookup and return the pointer */
    read_token(fp);
    NoTokErr("missing SWEIGHTS macro name");
    tmp = sw_lookup(hmm, rdhmmdef_token);
    if (tmp == NULL) {
      jlog("Error: rdhmmdef_streamweight: ~w \"%s\" not defined\n", rdhmmdef_token);
      rderr(NULL);
    }
    read_token(fp);
    return tmp;
  } else if (currentis("SWEIGHTS")){
    /* definition: define stream weight data, and return the pointer */
    tmp = sw_read(fp, hmm);
    tmp->name = NULL; /* no name */
    sw_add(hmm, tmp);
    return tmp;
  } else {
    rderr("no stream weights data");
    return NULL;
  }
}

/** 
 * Read a stream weight definition and store it as a macro.
 * 
 * @param name [in] macro name
 * @param fp [in] file pointer
 * @param hmm [i/o] %HMM definition data
 */
void
def_streamweight_macro(char *name, FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_StreamWeight *new;

  
  /* read in data and return newly malloced data */
  new = sw_read(fp, hmm);

  /* register it to the grobal HMM structure */
  new->name = name;
  sw_add(hmm, new);
}

/* end of file */
