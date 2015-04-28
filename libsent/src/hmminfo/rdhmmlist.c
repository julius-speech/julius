/**
 * @file   rdhmmlist.c
 * 
 * <JA>
 * @brief  HMMListファイルを読み込む
 *
 * HMMList ファイルは，辞書上の音素表記（トライフォン表記）から
 * 実際に定義されている %HMM へのマッピングを行なうファイルです．
 * 
 * HMMListファイルでは，登場しうる音素について，対応する
 * HMM 定義の名前を記述します．一行に１つづつ，第1カラムに音素名，
 * スペースで区切って第2カラムに定義されている実際の %HMM 名を指定します．
 * 第1カラムと第2カラムが全く同じ場合，すなわちその音素名のモデルが直接
 * %HMM として定義されている場合は，第2カラムは省略することができます．
 *
 * トライフォン使用時は，HMMListファイルで登場しうる全てのトライフォンに
 * ついて記述する必要がある点に注意して下さい．もし与えられた認識辞書
 * 上で登場しうるトライフォンがHMMListに記述されていない場合，
 * エラーとなります．
 * </JA>
 * 
 * <EN>
 * @brief  Read in HMMList file
 *
 * HMMList file specifies how the phones as described in word dictionary,
 * or their context-dependent form, should be mapped to actual defined %HMM.
 *
 * In HMMList file, the possible phone names and their corresponding %HMM
 * name should be specified one per line.  The phone name should be put on
 * the first column, and its corresponding %HMM name in the HTK %HMM definition
 * file should be defined on the second column.  If the two strings are
 * the same, which occurs when a %HMM of the phone name is directly defined,
 * the second column can be omitted.
 *
 * When using a triphone model, ALL the possible triphones that can appear
 * on the given word dictionary should be specified in the HMMList file.
 * If some possible triphone are not specified in the HMMList, Julius
 * produces error.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 04:04:23 2005
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
#include <sent/ptree.h>

#define MAXLINEINHMMLIST 256	///< Maximum line length in HMMList

/** 
 * Read a HMMList file and build initial logical triphone list.
 * 
 * @param fp [in] file pointer
 * @param hmminfo [i/o] %HMM definition data to store the logical phone list
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
rdhmmlist(FILE *fp, HTK_HMM_INFO *hmminfo)
{
  char *buf, *lname, *pname;
  HMM_Logical *new, *match;
  HTK_HMM_Data *mapped;
  boolean ok_flag = TRUE;
  int n;
  /* 1 column ... define HMM_Logical of the name as referring to HMM of the same name */
  /* 2 column ... define HMM_Logical of the name "$1" which has pointer to $2 */

  buf = (char *)mymalloc(MAXLINEINHMMLIST);
  n = 0;
  while (getl(buf, MAXLINEINHMMLIST, fp) != NULL) {
    n++;
    if ((lname = strtok(buf, DELM)) == NULL) {
      jlog("Error: rdhmmlist: failed to parse, corrupted or invalid data?\n");
      return FALSE;
    }
    if (strlen(lname) >= MAX_HMMNAME_LEN) {
      jlog("Error: rdhmmlist: %d: name too long: \"%s\"\n", n, lname);
      jlog("Error: rdhmmlist: try increase value of MAX_HMMNAME_LEN\n");
      return FALSE;
    }
    pname = strtok(NULL, DELM);
    if (pname == NULL) {
      /* 1 column */
      mapped = htk_hmmdata_lookup_physical(hmminfo, lname);
      if (mapped == NULL) {
	jlog("Error: rdhmmlist: line %d: physical HMM \"%s\" not found\n", n, lname);
	ok_flag = FALSE;
	continue;
      }
    } else {
      /* 2 column */
      mapped = htk_hmmdata_lookup_physical(hmminfo, pname);
      if (strlen(pname) >= MAX_HMMNAME_LEN) {
	jlog("Error: rdhmmlist: %d: name too long: \"%s\"\n", n, pname);
	jlog("Error: rdhmmlist: please increase MAX_HMMNAME_LEN (%d) and re-compile\n", MAX_HMMNAME_LEN);
	return FALSE;
      }
      if (mapped == NULL) {
	jlog("Error: rdhmmlist: line %d: physical HMM \"%s\" not found\n", n, pname);
	ok_flag = FALSE;
	continue;
      }
    }
    /* create new HMM_Logical */
    new = (HMM_Logical *)mybmalloc2(sizeof(HMM_Logical), &(hmminfo->lroot));
    new->name = mybstrdup2(lname, &(hmminfo->lroot));
    new->is_pseudo = FALSE;
    new->body.defined = mapped;
    new->next = hmminfo->lgstart;
    hmminfo->lgstart = new;
    /* add index to search index tree */
    if (hmminfo->logical_root == NULL) {
      hmminfo->logical_root = aptree_make_root_node(new, &(hmminfo->lroot));
    } else {
      match = aptree_search_data(new->name, hmminfo->logical_root);
      if (match != NULL && strmatch(match->name, new->name)) {
	jlog("Error: rdhmmlist: line %d: logical HMM \"%s\" duplicated\n", n, new->name);
	ok_flag = FALSE;
      } else {
	aptree_add_entry(new->name, new, match->name, &(hmminfo->logical_root), &(hmminfo->lroot));
      }
    }
    
  }

  hmminfo->totallogicalnum = n;
  free(buf);

  return(ok_flag);
}
