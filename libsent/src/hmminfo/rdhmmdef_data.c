/**
 * @file   rdhmmdef_data.c
 * 
 * <JA>
 * @brief  HTK %HMM 定義ファイルの読み込み：%HMM モデル
 * </JA>
 * 
 * <EN>
 * @brief  Read HTK %HMM definition file: %HMM model
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 01:12:19 2005
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
 * Allocate a new data and return it.
 * 
 * @return pointer to newly allocated data.
 */
HTK_HMM_Data *
htk_hmmdata_new(HTK_HMM_INFO *hmminfo)
{
  HTK_HMM_Data *new;

  new = (HTK_HMM_Data *)mybmalloc2(sizeof(HTK_HMM_Data), &(hmminfo->mroot));

  new->name = NULL;
  new->state_num = 0;
  new->s = NULL;
  new->tr = NULL;
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
htk_hmmdata_add(HTK_HMM_INFO *hmm, HTK_HMM_Data *new)
{
  HTK_HMM_Data *match;
  /* link data structure */
  new->next = hmm->start;
  hmm->start = new;

  if (new->name == NULL) {
    /* HMM must have a name */
    rderr("HMM has no name");
  } else {
    /* add index to search index tree */
    if (hmm->physical_root == NULL) {
      hmm->physical_root = aptree_make_root_node(new, &(hmm->mroot));
    } else {
      match = aptree_search_data(new->name, hmm->physical_root);
      if (match != NULL && strmatch(match->name, new->name)) {
	/* HMM of the same name should not be defined */
	jlog("Error: rdhmmdef_data: HMM \"%s\" is defined more than twice\n", new->name);
	rderr(NULL);
      } else {
	aptree_add_entry(new->name, new, match->name, &(hmm->physical_root), &(hmm->mroot));
      }
    }
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
static HTK_HMM_Data *
htk_hmmdata_read(FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Data *new;
  int i;
  short sid;

  new = htk_hmmdata_new(hmm);

  /* begin tag */
  if (!currentis("BEGINHMM")) rderr("<BEGINHMM> not found");
  read_token(fp);

  /* read global opt if any */
  /* read_global_opt(fp, &(new->opt)); */

  /* num of state */
  if (!currentis("NUMSTATES")) rderr("<NUMSTATES> not found");
  read_token(fp);
  NoTokErr("state num not found\n");
  new->state_num = atoi(rdhmmdef_token);
  read_token(fp);

  /* malloc state */
  new->s = (HTK_HMM_State **)mybmalloc2(sizeof(HTK_HMM_State *) * new->state_num, &(hmm->mroot));
  for(i=0;i<new->state_num;i++) {
    new->s[i] = NULL;
  }

  /* read/set each state info */
  for (;;) {
    if (!currentis("STATE")) break;
    read_token(fp); NoTokErr("STATE id not found");
    sid = atoi(rdhmmdef_token) - 1;
    read_token(fp);
    new->s[sid] = get_state_data(fp, hmm);
  }

  /* read/set transition info */
  new->tr = get_trans_data(fp, hmm);
  if ((new->tr)->statenum != new->state_num) {
    rderr("# of transition != # of state");
  }

  /* read/set duration */

  /* end tag */
  if (!currentis("ENDHMM")) rderr("<ENDHMM> not found");
  read_token(fp);

  return(new);
}  

/** 
 * Read a new data and store it as a macro.
 * 
 * @param name [in] macro name
 * @param fp [in] file pointer
 * @param hmm [i/o] %HMM definition data
 */
void
def_HMM(char *name, FILE *fp, HTK_HMM_INFO *hmm)
{
  HTK_HMM_Data *new;

  /* read in HMM model data from fp, and return newly malloced HTK_HMM_Data */
  new = htk_hmmdata_read(fp, hmm);

  /* set name and add the new data to the main structure */
  new->name = name;
  htk_hmmdata_add(hmm, new);
}
