/**
 * @file   rdhmmdef_trans.c
 * 
 * <JA>
 * @brief  HTK %HMM 定義ファイルの読み込み：遷移行列
 *
 * 遷移確率はファイル読み込み終了後に log10 に変換されます．
 * </JA>
 * 
 * <EN>
 * @brief  Read HTK %HMM definition file: transition matrix
 *
 * The transition probabilities will be converted to log10 scale
 * after finished reading the while %HMM definition file.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 03:50:55 2005
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

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>

extern char *rdhmmdef_token;	///< Current token

/** 
 * Allocate a new data area and return it.
 * 
 * @return pointer to newly allocated data.
 */
static HTK_HMM_Trans *
trans_new(HTK_HMM_INFO *hmm)
{
  HTK_HMM_Trans *new;

  new = (HTK_HMM_Trans *)mybmalloc2(sizeof(HTK_HMM_Trans), &(hmm->mroot));
  new->name = (char *)NULL;
  new->statenum = 0;
  new->a = (PROB **)NULL;
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
trans_add(HTK_HMM_INFO *hmm, HTK_HMM_Trans *new)
{
  HTK_HMM_Trans *match;

  /* link data structure */
  new->next = hmm->trstart;
  hmm->trstart = new;

  if (new->name != NULL) {
    /* add index to search index tree */
    if (hmm->tr_root == NULL) {
      hmm->tr_root = aptree_make_root_node(new, &(hmm->mroot));
    } else {
      match = aptree_search_data(new->name, hmm->tr_root);
      if (match != NULL && strmatch(match->name,new->name)) {
	jlog("Error: rdhmmdef_trans: ~t \"%s\" is already defined\n", new->name);
	rderr(NULL);
      } else {
	aptree_add_entry(new->name, new, match->name, &(hmm->tr_root), &(hmm->mroot));
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
static HTK_HMM_Trans *
trans_lookup(HTK_HMM_INFO *hmm, char *keyname)
{
  HTK_HMM_Trans *t;

  t = aptree_search_data(keyname, hmm->tr_root);
  if (t != NULL && strmatch(t->name, keyname)) {
    return t;
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
 * @param hmm [i/o] %HMM definition data
 * 
 * @return pointer to the newly read data.
 */
static HTK_HMM_Trans *
trans_read(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Trans *new;
  int i,j;
  PROB prob;
  PROB *atmp;

  /* read tag */
  if (!currentis("TRANSP")) rderr("<TRANSP> not found"); /* not match */
  read_token(fp);

  /* read statenum */
  new = trans_new(hmm);
  NoTokErr("missing TRANSP state num");
  new->statenum = atoi(rdhmmdef_token);
  read_token(fp);

  /* allocate array */
  new->a = (PROB **)mybmalloc2(sizeof(PROB *) * new->statenum, &(hmm->mroot));
  atmp = (PROB *)mybmalloc2(sizeof(PROB) * new->statenum * new->statenum, &(hmm->mroot));
  new->a[0] = &(atmp[0]);
  for (i=1;i<new->statenum;i++) {
    new->a[i] = &(atmp[i*new->statenum]);
  }
  
  /* begin reading transition prob */
  for (i=0;i<new->statenum; i++) {
    for (j=0;j<new->statenum; j++) {
      NoTokErr("missing some TRANSP value");
      prob = (PROB)atof(rdhmmdef_token);
      new->a[i][j] = prob;
      read_token(fp);
    }
  }

  return(new);
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
HTK_HMM_Trans *
get_trans_data(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Trans *tmp;
  
  if (currentis("TRANSP")) {
    /* definition: define transition data, and return the pointer */
    tmp = trans_read(fp, hmm);
    tmp->name = NULL; /* no name */
    trans_add(hmm, tmp);
    return(tmp);
  } else if (currentis("~t")) {
    /* macro reference: lookup and return the pointer */
    read_token(fp);
    NoTokErr("missing TRANSP macro name");
    tmp = trans_lookup(hmm, rdhmmdef_token);
    if (tmp == NULL) {
      jlog("Error: rdhmmdef_trans: ~t \"%s\" not defined\n", rdhmmdef_token);
      rderr(NULL);
    }
    read_token(fp);
    return(tmp);
  } else {
    rderr("no transition data");
    return(NULL);
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
def_trans_macro(char *name, FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Trans *new;

  /* read in data and return newly malloced data */
  new = trans_read(fp, hmm);

  /* register it to the grobal HMM structure */
  new->name = name;
  trans_add(hmm, new);
}
