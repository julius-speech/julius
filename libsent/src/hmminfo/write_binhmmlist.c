/**
 * @file   write_hmmlist.c
 * 
 * <JA>
 * @brief  HMMListファイルをバイナリ形式で出力する
 * </JA>
 * 
 * <EN>
 * @brief  Write HMMList data to binary file
 *
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 04:04:23 2005
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
#include <sent/ptree.h>

/** 
 * Callback to write hmmlist data into file.
 * 
 * @param data [in] hmmlist node data
 * @param fp [in] file pointer to write
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
save_hmmlist_callback(void *data, FILE *fp)
{
  HMM_Logical *l = data;
  char *s;
  int len;

  if (myfwrite(&(l->is_pseudo), sizeof(boolean), 1, fp) < 1) return FALSE;
  len = strlen(l->name) + 1;
  if (myfwrite(&len, sizeof(int), 1, fp) < 1) return FALSE;
  if (myfwrite(l->name, len, 1, fp) < 1) return FALSE;
  if (l->is_pseudo) {
    s = l->body.pseudo->name;
  } else {
    s = l->body.defined->name;
  }
  len = strlen(s) + 1;
  if (myfwrite(&len, sizeof(int), 1, fp) < 1) return FALSE;
  if (myfwrite(s, len, 1, fp) < 1) return FALSE;
  
  return TRUE;
}

/** 
 * Callback to write cdset data into file.
 * 
 * @param data [in] cdset node data
 * @param fp [in] file pointer to write
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
save_cdset_callback(void *data, FILE *fp)
{
  CD_Set *cd = data;
  int len;
  int i, j;

  len = strlen(cd->name) + 1;
  if (myfwrite(&len, sizeof(int), 1, fp) < 1) return FALSE;
  if (myfwrite(cd->name, len, 1, fp) < 1) return FALSE;
  if (myfwrite(&(cd->tr->id), sizeof(int), 1, fp) < 1) return FALSE;
  if (myfwrite(&(cd->state_num), sizeof(unsigned short), 1, fp) < 1) return FALSE;
  for(i=0;i<cd->state_num;i++) {
    if (myfwrite(&(cd->stateset[i].num), sizeof(unsigned short), 1, fp) < 1) return FALSE;
    for(j=0;j<cd->stateset[i].num;j++) {
      if (myfwrite(&(cd->stateset[i].s[j]->id), sizeof(int), 1, fp) < 1) return FALSE;
      
    }
  }
  
  return TRUE;
}

/** 
 * Write hmmlist (logical-to-physical mapping table) and
 * cdset (pseudo phone set) to file.
 * 
 * @param fp [in] file pointer to write
 * @param hmminfo [in] HMM definition data
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
save_hmmlist_bin(FILE *fp, HTK_HMM_INFO *hmminfo)
{
  /* write 4 byte as NULL to auto detect file format at read time */
  /* this mark will be read in init_hmminfo() */
  int x = 0;
  if (myfwrite(&x, sizeof(int), 1, fp) < 1) {
    jlog("Error: save_hmmlist_bin: failed to write hmmlist to binary file\n");
    return FALSE;
  }
  /* write hmmlist */
  if (aptree_write(fp, hmminfo->logical_root, save_hmmlist_callback) == FALSE) {
    jlog("Error: save_hmmlist_bin: failed to write hmmlist to binary file\n");
    return FALSE;
  }
  /* write cdset */
  if (aptree_write(fp, hmminfo->cdset_info.cdtree, save_cdset_callback) == FALSE) {
    jlog("Error: save_hmmlist_bin: failed to write cdset to binary file\n");
    return FALSE;
  }
  return TRUE;
}
