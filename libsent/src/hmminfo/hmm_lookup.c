/**
 * @file   hmm_lookup.c
 * 
 * <JA>
 * @brief  %HMM の名前から実体を検索する
 *
 * "g-u+i" などの %HMM 論理名から，対応する %HMM の定義を検索します．
 * 
 * 検索結果は論理%HMM HMM_Logical へのポインタで返されます．HMM_Logical は，
 * 実際に定義されている %HMM へのポインタか，あるいはバイフォン・モノフォン
 * の論理%HMM名でかつそれらが%HMM定義ファイルやHMMListで定義されていない場合，
 * 対応する pseudo %HMM set へのポインタのどちらかを保持しています．
 * 
 * また，論理名から定義名へのマッピング関数への実%HMM名と pseudo %HMM名の
 * 追加登録もここで行ないます．
 * </JA>
 * 
 * <EN>
 * @brief  Look up logical %HMM entry from phone name
 *
 * These function is for searching %HMM definition from phone name
 * like "g-u+i".
 *
 * The result is pointer to the corresponding logical %HMM (HMM_Logical).
 * The logical %HMM holds either pointer to an actual %HMM data defined in
 * HTK %HMM definition, or pointer to a pseudo %HMM set when the query name
 * is biphone or monophone and they are not defined in either HTK %HMM
 * definition or HMMList mapping file.
 *
 * Adding physical %HMM defined in HTK %HMM definitions and pseudo phones
 * to the logical %HMM mapping function is also done here.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Tue Feb 15 22:34:30 2005
 *
 * $Revision: 1.7 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* physical HMMs ... already indexed when reading in hmmdefs */
/* logical HMMs  ... already indexed when reading in HMMList */

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>
#include <sent/ptree.h>

/** 
 * Look up physical (defined in HTK %HMM definition file) %HMM by its name.
 * 
 * @param hmminfo [in] HMM definition data
 * @param keyname [in] key string of %HMM name
 * 
 * @return pointer to the found physical %HMM, NULL if not found.
 */
HTK_HMM_Data *
htk_hmmdata_lookup_physical(HTK_HMM_INFO *hmminfo, char *keyname)
{
  HTK_HMM_Data *tmp;
  tmp = aptree_search_data(keyname, hmminfo->physical_root);
  if (tmp != NULL && strmatch(tmp->name, keyname)) {
    return tmp;
  } else {
    return NULL;
  }
}

/** 
 * Look up logical %HMM by its name.
 * 
 * @param hmminfo [in] HMM definition data
 * @param keyname [in] key string of %HMM name
 * 
 * @return pointer to the found logical %HMM, NULL if not found.
 */
HMM_Logical *
htk_hmmdata_lookup_logical(HTK_HMM_INFO *hmminfo, char *keyname)
{
  HMM_Logical *tmp;
  tmp = aptree_search_data(keyname, hmminfo->logical_root);
  if (tmp != NULL && strmatch(tmp->name, keyname)) {
    return tmp;
  } else {
    return NULL;
  }
}

/** 
 * Count the number of logical %HMM and store it.
 * 
 * @param hmminfo [in] %HMM definition data.
 */
static void
hmm_count_logical_num(HTK_HMM_INFO *hmminfo)
{
  HMM_Logical *lg;
  int n;

  n = 0;
  for (lg = hmminfo->lgstart; lg; lg = lg->next) n++;
  hmminfo->totallogicalnum = n;
}

/** 
 * @brief  Add all physical %HMM to logical %HMM.
 *
 * This function should be called only if HMMList is not specified.
 * Julius assumes all the triphones should be explicitly mapped
 * using HMMList file.
 * 
 * @param hmminfo [in] %HMM definition data.
 */
void
hmm_add_physical_to_logical(HTK_HMM_INFO *hmminfo)
{
  HMM_Logical *new, *match = NULL;
  HTK_HMM_Data *ph;

  for (ph = hmminfo->start; ph; ph = ph->next) {

    /* check if same name already exist */
    if (hmminfo->logical_root != NULL) {
      match = aptree_search_data(ph->name, hmminfo->logical_root);
      if (match != NULL && strmatch(match->name, ph->name)) {
	/* the physcal name was already mapped to other HMMs in HMMList */
	jlog("Warning: hmm_lookup: \"%s\" is defined in hmmdefs, but \"%s\" will be used instead\n", ph->name, (match->body.defined)->name);
	continue;
      }
    }
    /* create new HMM_Logical */
    /* body refers to the physical HMM */
    new = (HMM_Logical *)mybmalloc2(sizeof(HMM_Logical), &(hmminfo->lroot));
    new->name = (char *)mybmalloc2(strlen(ph->name) + 1, &(hmminfo->lroot));
    strcpy(new->name, ph->name);
    new->is_pseudo = FALSE;
    new->body.defined = ph;
    new->next = hmminfo->lgstart;
    hmminfo->lgstart = new;
    if (hmminfo->logical_root == NULL) {
      hmminfo->logical_root = aptree_make_root_node(new, &(hmminfo->lroot));
    } else {
      aptree_add_entry(new->name, new, match->name, &(hmminfo->logical_root), &(hmminfo->lroot));
    }
  }

  /* re-count total number */
  hmm_count_logical_num(hmminfo);
}



/** 
 * @brief  Add a pseudo monophone and pseudo biphone to logical %HMM.
 *
 * Logical %HMM specified in HMMlist precedes
 * pseudo %HMM: if some monophones or biphones are already
 * defined in HMMList, pseudo %HMM will not be added.
 * 
 * @param hmminfo [in] %HMM definition data.
 * @param name [in] name of the pseudo phone to add.
 */
static boolean
hmm_add_pseudo_phones_sub(HTK_HMM_INFO *hmminfo, char *name)
{
  HMM_Logical *new, *match;

  /* check if already exist */
  match = aptree_search_data(name, hmminfo->logical_root);
  if (match != NULL && strmatch(match->name, name)) {
    /* already exist in list */
    /*    if (! match->is_pseudo) {*/
      /* this pseudo-HMM is already defined as real HMM in hmmdefs or in HMMList */
    /*designated_count++;
      }*/
  } else {
    /* create new HMM_Logical with pseudo body */
    new = (HMM_Logical *)mybmalloc2(sizeof(HMM_Logical), &(hmminfo->lroot));
    new->name = (char *)mybmalloc2(strlen(name) + 1, &(hmminfo->lroot));
    strcpy(new->name, name);
    new->is_pseudo = TRUE;
    new->body.pseudo = cdset_lookup(hmminfo, name);
    if (new->body.pseudo == NULL) {	/* should never happen */
      jlog("Error: hmm_lookup: tried to add pseudo phone \"%s\" to logical HMM, but no corresponding CD_Set found.  Why??\n");
      return FALSE;
    }
    new->next = hmminfo->lgstart;
    hmminfo->lgstart = new;
    if (hmminfo->logical_root == NULL) {
      hmminfo->logical_root = aptree_make_root_node(new, &(hmminfo->lroot));
    } else {
      aptree_add_entry(new->name, new, match->name, &(hmminfo->logical_root), &(hmminfo->lroot));
    }
    hmminfo->totalpseudonum++;
  }
  return TRUE;
}
    
/** 
 * Update logical %HMM list by adding all the possible pseudo monophone
 * and biphone to the list.
 * 
 * @param hmminfo [in] %HMM definition data.
 */
void
hmm_add_pseudo_phones(HTK_HMM_INFO *hmminfo)
{
  HMM_Logical *lg;
  char buf[MAX_HMMNAME_LEN];
  boolean ok_p = TRUE;

  hmminfo->totalpseudonum = 0;
  /* add pseudo monophone */
  for (lg = hmminfo->lgstart; lg; lg = lg->next) {
    if (lg->is_pseudo) continue;
    if (hmm_add_pseudo_phones_sub(hmminfo, center_name(lg->name, buf)) == FALSE) {
      jlog("Error: hmm_lookup: failed to add \"%s\" as logical\n", center_name(lg->name, buf));
      ok_p = FALSE;
    }
  }
  /* add pseudo biphone, i.e. "a-k" etc. */
  for (lg = hmminfo->lgstart; lg; lg = lg->next) {
    if (lg->is_pseudo) continue;
    if (hmm_add_pseudo_phones_sub(hmminfo, leftcenter_name(lg->name, buf)) == FALSE) {
      jlog("Error: hmm_lookup: failed to add \"%s\" as logical\n", leftcenter_name(lg->name, buf));
      ok_p = FALSE;
    }
  }
  /* add pseudo biphone, i.e. "k+e" etc. */
  for (lg = hmminfo->lgstart; lg; lg = lg->next) {
    if (lg->is_pseudo) continue;
    if (hmm_add_pseudo_phones_sub(hmminfo, rightcenter_name(lg->name, buf)) == FALSE) {
      jlog("Error: hmm_lookup: failed to add \"%s\" as logical\n", rightcenter_name(lg->name, buf));
      ok_p = FALSE;
    }
  }
  jlog("Stat: hmm_lookup: %d pseudo phones are added to logical HMM list\n", hmminfo->totalpseudonum);
  /* re-count total number */
  hmm_count_logical_num(hmminfo);
}

/** 
 * Generic function to get the number of states in a logical %HMM.
 * 
 * @param lg [in] logical %HMM
 * 
 * @return the number of states in the logical %HMM.
 */
int
hmm_logical_state_num(HMM_Logical *lg)
{
  int len;
  if (lg->is_pseudo) len = lg->body.pseudo->state_num;
  else len = lg->body.defined->state_num;
  return(len);
}

/** 
 * Generic function to get transition matrix of a logical %HMM.
 * 
 * @param lg [in] logical %HMM
 * 
 * @return pointer to the transition matrix of the logical %HMM.
 */
HTK_HMM_Trans *
hmm_logical_trans(HMM_Logical *lg)
{
  HTK_HMM_Trans *tr;
  if (lg->is_pseudo) tr = lg->body.pseudo->tr;
  else tr = lg->body.defined->tr;
  return(tr);
}
