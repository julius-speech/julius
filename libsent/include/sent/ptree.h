/**
 * @file   ptree.h
 *
 * <EN>
 * @brief Patricia binary tree for data search
 *
 * This is a structure to build a patricia binary tree for searching
 * various data or IDs from its name string.
 * </EN>
 * <JA>
 * @brief データ検索用汎用パトリシア検索木の定義
 *
 * 文字列からその名前を持つ構造体や対応するIDを検索するための
 * パトリシア木の構造体です．
 * </JA>
 *
 * @author Akinobu LEE
 * @date   Fri Feb 11 17:27:24 2005
 *
 * $Revision: 1.9 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __PATRICIA_TREE_H__
#define __PATRICIA_TREE_H__

/// Patricia binary tree node, to search related pointer from string
typedef struct _apatnode {
  /**
   * @brief Node value
   *
   * Pointer adreess if this is leaf node (in case both @a left0 and @a right1
   * are NULL), or threshold bit if this is branch node (otherwise)
   * 
   */
  union {
    void        *data;		///< Pointer address at leaf
    int		thres_bit;	///< Threshold bit at branch
  } value;
  struct _apatnode *left0;	///< Link to left node (bit=0)
  struct _apatnode *right1;	///< Link to right node (bit=1)
} APATNODE;

/// Another patricia binary tree node, to search integer value from string
typedef struct _patnode {
  /**
   * @brief Node value
   *
   * Integer value if this is leaf node (in case both @a left0 and @a right1
   * are NULL), or threshold bit if this is branch node (otherwise)
   * 
   */
  union {
    int         data;		///< Integer value at leaf
    int		thres_bit;	///< Threshold bit at branch
  } value;
  struct _patnode *left0;	///< Link to left node (bit=0)
  struct _patnode *right1;	///< Link to right node (bit=1)
} PATNODE;


#ifdef __cplusplus
extern "C" {
#endif

int testbit(char *str, int slen, int bitplace);
int testbit_max(char *str, int bitplace, int maxbitplace);
int where_the_bit_differ(char *str1, char *str2);
PATNODE *make_ptree(char **words, int *data, int wordsnum, int bitplace, BMALLOC_BASE **mroot);
void disp_ptree(PATNODE *node, int level);
int ptree_search_data(char *str, PATNODE *rootnode);
int ptree_replace_data(char *str, int val, PATNODE *node);
PATNODE *ptree_make_root_node(int data, BMALLOC_BASE **mroot);
void ptree_add_entry(char *str, int data, char *matchstr, PATNODE **rootnode, BMALLOC_BASE **mroot);

void *aptree_search_data(char *str, APATNODE *rootnode);
APATNODE *aptree_make_root_node(void *data, BMALLOC_BASE **mroot);
void aptree_add_entry(char *str, void *data, char *matchstr, APATNODE **rootnode, BMALLOC_BASE **mroot);
void aptree_remove_entry(char *str, APATNODE **rootnode);
void aptree_traverse_and_do(APATNODE *node, void (*callback)(void *));
boolean aptree_write(FILE *fp, APATNODE *root, boolean (*save_data_func)(void *, FILE *fp));
boolean aptree_read(FILE *fp, APATNODE **root, BMALLOC_BASE **mroot, void *data, boolean (*load_data_func)(void **, void *, FILE *fp));

#ifdef __cplusplus
}
#endif

#endif /* __PATRICIA_TREE_H__ */
