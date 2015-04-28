/**
 * @file   rdhmmdef_state.c
 * 
 * <JA>
 * @brief  HTK %HMM 定義ファイルの読み込み：状態
 * </JA>
 * 
 * <EN>
 * @brief  Read HTK %HMM definition file: state
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 03:07:44 2005
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

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>

extern char *rdhmmdef_token;	///< Current token

/** 
 * Allocate a new data area and return it.
 * 
 * @return pointer to newly allocated data.
 */
static HTK_HMM_State *
state_new(HTK_HMM_INFO *hmm)
{
  HTK_HMM_State *new;
  int i;

  new = (HTK_HMM_State *)mybmalloc2(sizeof(HTK_HMM_State), &(hmm->mroot));
  new->name = NULL;
  new->nstream = hmm->opt.stream_info.num;
  new->w = NULL;
  new->pdf = (HTK_HMM_PDF **)mybmalloc2(sizeof(HTK_HMM_PDF *) * new->nstream, &(hmm->mroot));
  for(i=0;i<new->nstream;i++) {
    new->pdf[i] = NULL;
  }
  new->id = -1;
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
state_add(HTK_HMM_INFO *hmm, HTK_HMM_State *new)
{
  HTK_HMM_State *match;

  /* link data structure */
  new->next = hmm->ststart;
  hmm->ststart = new;

  if (new->name != NULL) {
    /* add index to search index tree */
    if (hmm->st_root == NULL) {
      hmm->st_root = aptree_make_root_node(new, &(hmm->mroot));
    } else {
      match = aptree_search_data(new->name, hmm->st_root);
      if (match != NULL && strmatch(match->name, new->name)) {
	jlog("Error: rdhmmdef_state: ~s \"%s\" is already defined\n", new->name);
	rderr(NULL);
      } else {
	aptree_add_entry(new->name, new, match->name, &(hmm->st_root), &(hmm->mroot));
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
HTK_HMM_State *
state_lookup(HTK_HMM_INFO *hmm, char *keyname)
{
  HTK_HMM_State *s;

  s = aptree_search_data(keyname, hmm->st_root);
  if (s != NULL && strmatch(s->name, keyname)) {
    return s;
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
static HTK_HMM_State *
state_read(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_State *new;
  int s, k;
  boolean no_nummixes;

  new = state_new(hmm);

  if (currentis("SID")) {
    read_token(fp);
    NoTokErr("missing SID value");
    new->id = atoi(rdhmmdef_token);
    read_token(fp);
  }

  if (currentis("NUMMIXES")) {
    if (hmm->tmp_mixnum == NULL) {
      hmm->tmp_mixnum = (int *)mybmalloc2(sizeof(int) * hmm->opt.stream_info.num, &(hmm->mroot));
    }
    for(s=0;s<new->nstream;s++) {
      read_token(fp);
      NoTokErr("missing NUMMIXES value");
      hmm->tmp_mixnum[s] = atoi(rdhmmdef_token);
    }
    read_token(fp);
    no_nummixes = FALSE;
  } else {
    no_nummixes = TRUE;
  }

  if (currentis("SWEIGHTS") || currentis("~w")) {
    new->w = get_streamweight_data(fp, hmm);
    if (new->w == NULL) {
      rderr("error reading stream weights");
    }
  }

  for(k = 0; k < new->nstream; k++) {

    if (currentis("STREAM")) {
      read_token(fp);
      NoTokErr("missing STREAM value");
      s = atoi(rdhmmdef_token) - 1;
      read_token(fp);
    } else {
      s = 0;
      if (k != 0) {		/* not a first time */
	rderr("a state does not has mixture for all streams");
      }
    }

    new->pdf[s] = get_mpdf_data(fp, hmm, no_nummixes ? -1 : hmm->tmp_mixnum[s], s);

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
HTK_HMM_State *
get_state_data(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_State *tmp;

  if (currentis("SID")||currentis("NUMMIXES")||currentis("SWEIGHTS")||currentis("~w")||currentis("STREAM")||currentis("MIXTURE")||currentis("TMIX")||currentis("MEAN")||currentis("~m")||currentis("RCLASS")) {
    /* definition: define state data, and return the pointer */
    tmp = state_read(fp, hmm);
    tmp->name = NULL; /* no name */
    state_add(hmm, tmp);
    return tmp;
  } else if (currentis("~s")) {
    /* macro reference: lookup and return the pointer */
    read_token(fp);
    NoTokErr("missing state macro name");
    tmp = state_lookup(hmm, rdhmmdef_token);
    if (tmp == NULL) {
      jlog("Error: rdhmmdef_state: ~s \"%s\" not defined\n", rdhmmdef_token);
      rderr(NULL);
    }
    read_token(fp);
    return tmp;
  } else {
    rderr("no state data");
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
def_state_macro(char *name, FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_State *new;

  /* read in data and return newly malloced data */ 
  new = state_read(fp, hmm);
 
  /* register it to the grobal HMM structure */
  new->name = name;
  state_add(hmm, new);
}
