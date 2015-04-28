/**
 * @file   cpair.c
 * 
 * <JA>
 * @brief  カテゴリ対制約へのアクセス関数およびメモリ管理
 *
 * カテゴリ対制約のメモリ確保，およびカテゴリ間の接続の可否を返す関数です．
 * </JA>
 * 
 * <EN>
 * @brief  Category-pair constraint handling
 *
 * Functions to allocate memory for category-pair constraint, and functions
 * to return whether the given category pairs can be connected or not are
 * defined here.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Tue Feb 15 13:54:44 2005
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

#include <sent/stddefs.h>
#include <sent/dfa.h>

/** 
 * Search for a terminal ID in a cp list.  Set its location to loc.
 * When not found, the location where to insert the key data iwill be set.
 * 
 * @param list [in] cp list
 * @param len [in] length of list
 * @param key [in] a terminal ID value to find
 * @param loc [out] return the to-be-inserted location of the key
 * 
 * @return its location when found, or -1 when not found.
 * 
 */
static int
cp_find(int *list, int len, int key, int *loc)
{
  int left, right, mid;
  int ret;
  
  if (len == 0) {
    *loc = 0;
    return -1;
  }

  left = 0;
  right = len - 1;
  while (left < right) {
    mid = (left + right) / 2;
    if (list[mid] < key) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  if (list[left] == key) {
    *loc = left;
    ret = left;
  } else {
    ret = -1;
    if (list[left] > key) {
      *loc = left;
    } else {
      *loc = left + 1;
    }
  }
  return ret;
}

/** 
 * Return whether the given two category can be connected or not.
 * 
 * @param dfa [in] DFA grammar holding category pair matrix
 * @param i [in] category id of left word
 * @param j [in] category id of right word
 * 
 * @return TRUE if connection is allowed by the grammar, FALSE if prohibited.
 */
boolean
dfa_cp(DFA_INFO *dfa, int i, int j)
{
  int loc;

  /*return(dfa->cp[i][j]);*/
  //return((dfa->cp[i][j>>3] & cp_table[j&7]) ? TRUE : FALSE);
  return(cp_find(dfa->cp[i], dfa->cplen[i], j, &loc) != -1 ? TRUE : FALSE);
}

/** 
 * Return whether the category can be appear at the beginning of sentence.
 * 
 * @param dfa [in] DFA grammar holding category pair matrix
 * @param i [in] category id of the word
 * 
 * @return TRUE if it can appear at the beginning of sentence, FALSE if not.
 */
boolean
dfa_cp_begin(DFA_INFO *dfa, int i)
{
  int loc;
  /*return(dfa->cp_begin[i]);*/
  //return((dfa->cp_begin[i>>3] & cp_table[i&7]) ? TRUE : FALSE);
  return(cp_find(dfa->cp_begin, dfa->cp_begin_len, i, &loc) != -1 ? TRUE : FALSE);
}

/** 
 * Return whether the category can be appear at the end of sentence.
 * 
 * @param dfa [in] DFA grammar holding category pair matrix
 * @param i [in] category id of the word
 * 
 * @return TRUE if it can appear at the end of sentence, FALSE if not.
 */
boolean
dfa_cp_end(DFA_INFO *dfa, int i)
{
  int loc;
  /*return(dfa->cp_end[i]);*/
  //return((dfa->cp_end[i>>3] & cp_table[i&7]) ? TRUE : FALSE);
  return(cp_find(dfa->cp_end, dfa->cp_end_len, i, &loc) != -1 ? TRUE : FALSE);
}

/** 
 * Add an terminal ID to a specified location in the cp list.
 *
 * @param list [i/o] cp list
 * @param len [i/o] data num in the list
 * @param alloclen [i/o] allocated length of the list
 * @param val [in] value to be added
 * @param loc [in] location where to add the val
 * 
 * @return TRUE on success, FALSE on failure.
 * 
 */
static boolean
cp_add(int **list, int *len, int *alloclen, int val, int loc)
{
  if (loc > *len) {
    jlog("InternalError: skip?\n");
    return FALSE;
  }
  if (*len + 1 > *alloclen) {
    /* expand cplist if necessary */
    *alloclen *= 2;
    *list = (int *)myrealloc(*list, sizeof(int) * (*alloclen));
  }
  if (loc < *len) {
    memmove(&((*list)[loc+1]), &((*list)[loc]), sizeof(int) * (*len - loc));
  }
  (*list)[loc] = val;
  (*len)++;
  return TRUE;
}

/** 
 * Remove an element from the cp list.
 * 
 * @param list [i/o] cp list
 * @param len [i/o] data num in the list
 * @param loc [in] location of removing value
 * 
 * @return TRUE on success, FALSE on error.
 * 
 */static boolean
cp_remove(int **list, int *len, int loc)
{
  if (*len == 0) return TRUE;
  if (loc >= *len) {
    jlog("InternalError: skip?\n");
    return FALSE;
  }
  if (loc < *len - 1) {
    memmove(&((*list)[loc]), &((*list)[loc+1]), sizeof(int) * (*len - loc - 1));
  }
  (*len) --;
  return TRUE;
}


/** 
 * Set a category-pair matrix bit.
 * 
 * @param dfa [out] DFA grammar holding category pair matrix
 * @param i [in] category id of left word
 * @param j [in] category id of right word
 * @param value TRUE if connection allowed, FALSE if connection prohibited.
 */
void
set_dfa_cp(DFA_INFO *dfa, int i, int j, boolean value)
{
  int loc;
  if (value) {
    /* add j to cp list of i */
    if (cp_find(dfa->cp[i], dfa->cplen[i], j, &loc) == -1) { /* not exist */
      cp_add(&(dfa->cp[i]), &(dfa->cplen[i]), &(dfa->cpalloclen[i]), j, loc);
    }
  } else {
    /* remove j from cp list of i */
    if (cp_find(dfa->cp[i], dfa->cplen[i], j, &loc) != -1) { /* already exist */
      cp_remove(&(dfa->cp[i]), &(dfa->cplen[i]), loc);
    }
  }
}

/** 
 * Set a category-pair matrix bit for the beginning of sentence
 * 
 * @param dfa [out] DFA grammar holding category pair matrix
 * @param i [in] category id of the word
 * @param value TRUE if the category can appear at the beginning of
 * sentence, FALSE if not.
 */
void
set_dfa_cp_begin(DFA_INFO *dfa, int i, boolean value)
{
  int loc;

  if (value) {
    /* add j to cp list of i */
    if (cp_find(dfa->cp_begin, dfa->cp_begin_len, i, &loc) == -1) { /* not exist */
      cp_add(&(dfa->cp_begin), &(dfa->cp_begin_len), &(dfa->cp_begin_alloclen), i, loc);
    }
  } else {
    /* remove j from cp list of i */
    if (cp_find(dfa->cp_begin, dfa->cp_begin_len, i, &loc) != -1) { /* already exist */
      cp_remove(&(dfa->cp_begin), &(dfa->cp_begin_len), loc);
    }
  }
}

/** 
 * Set a category-pair matrix bit for the end of sentence
 * 
 * @param dfa [out] DFA grammar holding category pair matrix
 * @param i [in] category id of the word
 * @param value TRUE if the category can appear at the end of
 * sentence, FALSE if not.
 */
void
set_dfa_cp_end(DFA_INFO *dfa, int i, boolean value)
{
  int loc;

  if (value) {
    /* add j to cp list of i */
    if (cp_find(dfa->cp_end, dfa->cp_end_len, i, &loc) == -1) { /* not exist */
      cp_add(&(dfa->cp_end), &(dfa->cp_end_len), &(dfa->cp_end_alloclen), i, loc);
    }
  } else {
    /* remove j from cp list of i */
    if (cp_find(dfa->cp_end, dfa->cp_end_len, i, &loc) != -1) { /* already exist */
      cp_remove(&(dfa->cp_end), &(dfa->cp_end_len), loc);
    }
  }
}

/** 
 * Initialize category pair matrix in the grammar data.
 * 
 * @param dfa [out] DFA grammar to hold category pair matrix
 */
void
init_dfa_cp(DFA_INFO *dfa)
{
  dfa->cp = NULL;
}

/** 
 * Allocate memory for category pair matrix and initialize it.
 * 
 * @param dfa [out] DFA grammar to hold category pair matrix
 * @param term_num [in] number of categories in the grammar
 * @param size [in] memory allocation length for each cp list as initial
 */
void
malloc_dfa_cp(DFA_INFO *dfa, int term_num, int size)
{
  int i;

  dfa->cp = (int **)mymalloc(sizeof(int *) * term_num);
  dfa->cplen = (int *)mymalloc(sizeof(int) * term_num);
  dfa->cpalloclen = (int *)mymalloc(sizeof(int) * term_num);
  for(i=0;i<term_num;i++) {
    dfa->cp[i] = (int *)mymalloc(sizeof(int) * size);
    dfa->cpalloclen[i] = size;
    dfa->cplen[i] = 0;
  }
  dfa->cp_begin = (int *)mymalloc(sizeof(int) * size);
  dfa->cp_begin_alloclen = size;
  dfa->cp_begin_len = 0;
  dfa->cp_end = (int *)mymalloc(sizeof(int) * size);
  dfa->cp_end_alloclen = size;
  dfa->cp_end_len = 0;
}

/** 
 * Append a categori-pair matrix to another.
 * This function assumes that other grammar information has been already
 * appended and dfa->term_num contains the new size.
 * 
 * @param dfa [i/o] DFA grammar to which the new category pair will be appended
 * @param src [in] source DFA 
 * @param offset [in] appending point at dfa
 *
 * @return TRUE on success, FALSE on error.
 */
boolean
dfa_cp_append(DFA_INFO *dfa, DFA_INFO *src, int offset)
{
  int i, j, size;

  if (dfa->cp == NULL) {
    /* no category pair information exist on target */
    if (dfa->term_num != src->term_num) {
      jlog("InternalError: dfa_cp_append\n");
      return FALSE;
    }
    /* just malloc and copy */
    dfa->cp = (int **)mymalloc(sizeof(int *) * dfa->term_num);
    dfa->cplen = (int *)mymalloc(sizeof(int) * dfa->term_num);
    dfa->cpalloclen = (int *)mymalloc(sizeof(int) * dfa->term_num);
    for(i=0;i<dfa->term_num;i++) {
      size = src->cplen[i];
      if (size < DFA_CP_MINSTEP) size = DFA_CP_MINSTEP;
      dfa->cp[i] = (int *)mymalloc(sizeof(int) * size);
      dfa->cpalloclen[i] = size;
      memcpy(dfa->cp[i], src->cp[i], sizeof(int) * src->cplen[i]);
      dfa->cplen[i] = src->cplen[i];
    }
    size = src->cp_begin_len;
    if (size < DFA_CP_MINSTEP) size = DFA_CP_MINSTEP;
    dfa->cp_begin = (int *)mymalloc(sizeof(int) * size);
    dfa->cp_begin_alloclen = size;
    memcpy(dfa->cp_begin, src->cp_begin, sizeof(int) * src->cp_begin_len);
    dfa->cp_begin_len = src->cp_begin_len;
    size = src->cp_end_len;
    if (size < DFA_CP_MINSTEP) size = DFA_CP_MINSTEP;
    dfa->cp_end = (int *)mymalloc(sizeof(int) * size);
    dfa->cp_end_alloclen = size;
    memcpy(dfa->cp_end, src->cp_end, sizeof(int) * src->cp_end_len);
    dfa->cp_end_len = src->cp_end_len;
    return TRUE;
  }
  /* expand index */
  dfa->cp = (int **)myrealloc(dfa->cp, sizeof(int *) * dfa->term_num);
  dfa->cplen = (int *)myrealloc(dfa->cplen, sizeof(int) * dfa->term_num);
  dfa->cpalloclen = (int *)myrealloc(dfa->cpalloclen, sizeof(int) * dfa->term_num);
  /* copy src->cp[i][j] to target->cp[i+offset][j+offset] */
  for(i=offset;i<dfa->term_num;i++) {
    size = src->cplen[i-offset];
    if (size < DFA_CP_MINSTEP) size = DFA_CP_MINSTEP;
    dfa->cp[i] = (int *)mymalloc(sizeof(int) * size);
    dfa->cpalloclen[i] = size;
    for(j=0;j<src->cplen[i-offset];j++) {
      dfa->cp[i][j] = src->cp[i-offset][j] + offset;
    }
    dfa->cplen[i] = src->cplen[i-offset];
  }
  if (dfa->cp_begin_alloclen < dfa->cp_begin_len + src->cp_begin_len) {
    dfa->cp_begin_alloclen = dfa->cp_begin_len + src->cp_begin_len;
    dfa->cp_begin = (int *)myrealloc(dfa->cp_begin, sizeof(int) * dfa->cp_begin_alloclen);
  }
  for(j=0;j<src->cp_begin_len;j++) {
    dfa->cp_begin[dfa->cp_begin_len + j] = src->cp_begin[j] + offset;
  }
  dfa->cp_begin_len += src->cp_begin_len;
  if (dfa->cp_end_alloclen < dfa->cp_end_len + src->cp_end_len) {
    dfa->cp_end_alloclen = dfa->cp_end_len + src->cp_end_len;
    dfa->cp_end = (int *)myrealloc(dfa->cp_end, sizeof(int) * dfa->cp_end_alloclen);
  }
  for(j=0;j<src->cp_end_len;j++) {
    dfa->cp_end[dfa->cp_end_len + j] = src->cp_end[j] + offset;
  }
  dfa->cp_end_len += src->cp_end_len;

  return TRUE;
}

/** 
 * Free the category pair matrix from DFA grammar.
 * 
 * @param dfa [i/o] DFA grammar holding category pair matrix
 */
void
free_dfa_cp(DFA_INFO *dfa)
{
  int i;

  if (dfa->cp != NULL) {
    free(dfa->cp_end);
    free(dfa->cp_begin);
    for(i=0;i<dfa->term_num;i++) free(dfa->cp[i]);
    free(dfa->cpalloclen);
    free(dfa->cplen);
    free(dfa->cp);
    dfa->cp = NULL;
  }
}

void
dfa_cp_output_rawdata(FILE *fp, DFA_INFO *dfa)
{
  int i, j;

  for(i=0;i<dfa->term_num;i++) {
    fprintf(fp, "%d:", i);
    for(j=0;j<dfa->cplen[i];j++) {
      fprintf(fp, " %d", dfa->cp[i][j]);
    }
    fprintf(fp, "\n");
  }
  fprintf(fp, "bgn:");
  for(j=0;j<dfa->cp_begin_len;j++) {
    fprintf(fp, " %d", dfa->cp_begin[j]);
  }
  fprintf(fp, "\n");
  fprintf(fp, "end:");
  for(j=0;j<dfa->cp_end_len;j++) {
    fprintf(fp, " %d", dfa->cp_end[j]);
  }
  fprintf(fp, "\n");
}

void
dfa_cp_count_size(DFA_INFO *dfa, unsigned long *size_ret, unsigned long *allocsize_ret)
{
  int i;
  unsigned long size, allocsize;

  size = 0;
  allocsize = 0;
  for(i=0;i<dfa->term_num;i++) {
    size += sizeof(int) * dfa->cplen[i];
    allocsize += sizeof(int) * dfa->cpalloclen[i];
  }
  size += sizeof(int) * dfa->cp_begin_len;
  allocsize += sizeof(int) * dfa->cp_begin_alloclen;
  size += sizeof(int) * dfa->cp_end_len;
  allocsize += sizeof(int) * dfa->cp_end_alloclen;

  allocsize += (sizeof(int *) + sizeof(int) + sizeof(int)) * dfa->term_num;

  *size_ret = size;
  *allocsize_ret = allocsize;
}
