/**
 * @file   ptree.c
 * 
 * <JA>
 * @brief  パトリシア検索木を用いた名前検索：データ型が int の場合
 * </JA>
 * 
 * <EN>
 * @brief  Patricia index tree for name lookup: data type = int
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 15:34:39 2005
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
#include <sent/ptree.h>

static unsigned char mbit[] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
  

/** 
 * String bit test function.
 * 
 * @param str [in] key string
 * @param bitplace [in] bit location to test
 * 
 * @return the content of tested bit in @a tmp_str, either 0 or 1.
 */
int
testbit(char *str, int slen, int bitplace)
{
  int maskptr;
  
  if ((maskptr = bitplace >> 3) > slen) return 0;
  return(str[maskptr] & mbit[bitplace & 7]);
}

/** 
 * Local bit test function for search.
 * 
 * @param str [in] key string
 * @param bitplace [in] bit place to test.
 * @param maxbitplace [in] maximum number of bitplace
 * 
 * @return the content of tested bit in @a tmp_str, either 0 or 1.
 */
int
testbit_max(char *str, int bitplace, int maxbitplace)
{
  if (bitplace >= maxbitplace) return 0;
  return(str[bitplace >> 3] & mbit[bitplace & 7]);
}

/** 
 * Find in which bit the two strings differ, starting from the head.
 * 
 * @param str1 [in] string 1
 * @param str2 [in] string 2
 * 
 * @return the bit location in which the string differs.
 */
int
where_the_bit_differ(char *str1, char *str2)
{
  int p = 0;
  int bitloc;
  int slen1, slen2;

  /* step: char, bit */
  while(str1[p] == str2[p]) p++;
  bitloc = p * 8;
  slen1 = strlen(str1);
  slen2 = strlen(str2);
  while(testbit(str1, slen1, bitloc) == testbit(str2, slen2, bitloc)) bitloc++;

  return(bitloc);
}

/** 
 * Allocate a new node.
 * 
 * @param mroot [i/o] base pointer for block malloc
 * 
 * @return pointer to the new node.
 */
static PATNODE *
new_node(BMALLOC_BASE **mroot)
{
  PATNODE *tmp;

  tmp = (PATNODE *)mybmalloc2(sizeof(PATNODE), mroot);
  tmp->left0 = NULL;
  tmp->right1 = NULL;

  return(tmp);
}

/** 
 * Make a patricia tree for given string arrays.
 * Recursively called by descending the scan bit.
 * 
 * @param words [in] list of word strings
 * @param data [in] integer value corresponding to each string in @a words
 * @param wordsnum [in] number of above
 * @param bitplace [in] current scan bit.
 * @param mroot [i/o] base pointer for block malloc
 * 
 * @return pointer to the root node index.
 */
PATNODE *
make_ptree(char **words, int *data, int wordsnum, int bitplace, BMALLOC_BASE **mroot)
{
  int i,j, tmp;
  char *p;
  int newnum;
  PATNODE *ntmp;

#if 0
  printf("%d:", wordsnum);
  for (i=0;i<wordsnum;i++) {
    printf(" %s",words[i]);
  }
  printf("\n");
  printf("test bit = %d\n", bitplace);
#endif

  if (wordsnum == 1) {
    /* word identified: this is leaf node */
    ntmp = new_node(mroot);
    ntmp->value.data = data[0];
    return(ntmp);
  }

  newnum = 0;
  for (i=0;i<wordsnum;i++) {
    if (testbit(words[i], strlen(words[i]), bitplace) != 0) {
      newnum++;
    }
  }
  if (newnum == 0 || newnum == wordsnum) {
    /* all words has same bit, continue to descend */
    return(make_ptree(words, data, wordsnum, bitplace + 1, mroot));
  } else {
    /* sort word pointers by tested bit */
    j = wordsnum-1;
    for (i=0; i<newnum; i++) {
      if (testbit(words[i], strlen(words[i]), bitplace) == 0) {
	for (; j>=newnum; j--) {
	  if (testbit(words[j], strlen(words[j]), bitplace) != 0) {
	    p = words[i]; words[i] = words[j]; words[j] = p;
	    tmp = data[i]; data[i] = data[j]; data[j] = tmp;
	    break;
	  }
	}
      }
    }
    /* create node and descend for each node */
    ntmp = new_node(mroot);
    ntmp->value.thres_bit = bitplace;
    ntmp->right1 = make_ptree(words, data, newnum, bitplace+1, mroot);
    ntmp->left0  = make_ptree(&(words[newnum]), &(data[newnum]), wordsnum-newnum, bitplace+1, mroot);
    return(ntmp);
  }
}


/** 
 * Output a tree structure in text for debug, traversing pre-order
 *
 * @param node [in] root index node
 * @param level [in] current tree depth
 */
void
disp_ptree(PATNODE *node, int level)
{
  int i;

  for (i=0;i<level;i++) {
    printf("-");
  }
  if (node->left0 == NULL && node->right1 == NULL) {
    printf("LEAF:%d\n", node->value.data);
  } else {
    printf("%d\n", node->value.thres_bit);
    if (node->left0 != NULL) {
      disp_ptree(node->left0, level+1);
    }
    if (node->right1 != NULL) {
      disp_ptree(node->right1, level+1);
    }
  }
}

/** 
 * Recursive function to search the data in the tree
 * 
 * @param node [in] current node.
 * @param str [in] key string
 * @param maxbitplace [in] maximum number of bitplace
 * 
 * @return the found integer value.
 */
static int
ptree_search_data_r(PATNODE *node, char *str, int maxbitplace)
{
  if (node->left0 == NULL && node->right1 == NULL) {
    return(node->value.data);
  } else {
    if (testbit_max(str, node->value.thres_bit, maxbitplace) != 0) {
      return(ptree_search_data_r(node->right1, str, maxbitplace));
    } else {
      return(ptree_search_data_r(node->left0, str, maxbitplace));
    }
  }
}

/** 
 * Search for the data whose key string matches the given string.
 * 
 * @param str [in] search key string
 * @param node [in] root node of index tree
 * 
 * @return the exactly found integer value, or the nearest one.
 */
int
ptree_search_data(char *str, PATNODE *node)
{
  if (node == NULL) {
    //("Error: ptree_search_data: no node, search for \"%s\" failed\n", str);
    return -1;
  }
  return(ptree_search_data_r(node, str, strlen(str) * 8 + 8));
}

/** 
 * Recursive function to replace the data in the tree
 * 
 * @param node [in] current node.
 * @param str [in] key string
 * @param val [in] new value
 * @param maxbitplace [in] maximum number of bitplace
 * 
 * @return the found integer value.
 */
static int
ptree_replace_data_r(PATNODE *node, char *str, int val, int maxbitplace)
{
  if (node->left0 == NULL && node->right1 == NULL) {
    node->value.data = val;
    return(node->value.data);
  } else {
    if (testbit_max(str, node->value.thres_bit, maxbitplace) != 0) {
      return(ptree_replace_data_r(node->right1, str, val, maxbitplace));
    } else {
      return(ptree_replace_data_r(node->left0, str, val, maxbitplace));
    }
  }
}

/** 
 * Search for the data whose key string matches the given string, and
 * replace its value.
 * 
 * @param str [in] search key string
 * @param val [in] value
 * @param node [in] root node of index tree
 * 
 * @return the exactly found integer value, or the nearest one.
 */
int
ptree_replace_data(char *str, int val, PATNODE *node)
{
  if (node == NULL) {
    //("Error: ptree_search_data: no node, search for \"%s\" failed\n", str);
    return -1;
  }
  return(ptree_replace_data_r(node, str, val, strlen(str) * 8 + 8));
}


/*******************************************************************/
/* add 1 node to given ptree */

/** 
 * Make a root node of a index tree.
 * 
 * @param data [in] the first data
 * @param mroot [i/o] base pointer for block malloc
 * 
 * @return the newly allocated root node.
 */
PATNODE *
ptree_make_root_node(int data, BMALLOC_BASE **mroot)
{
  PATNODE *nnew;
  /* make new leaf node for newstr */
  nnew = new_node(mroot);
  nnew->value.data = data;
  return(nnew);
}

/** 
 * Insert a new node to the existing index tree.
 * 
 * @param str [in] new key string
 * @param bitloc [in] bit branch to which this node will be added
 * @param data [in] new data integer value
 * @param parentlink [i/o] the parent node to which this node will be added
 * @param mroot [i/o] base pointer for block malloc
 */
static void
ptree_add_entry_at(char *str, int slen, int bitloc, int data, PATNODE **parentlink, BMALLOC_BASE **mroot)
{
  PATNODE *node;
  node = *parentlink;
  if (node->value.thres_bit > bitloc ||
      (node->left0 == NULL && node->right1 == NULL)) {
    PATNODE *newleaf, *newbranch;
    /* insert between [parent] and [node] */
    newleaf = new_node(mroot);
    newleaf->value.data = data;
    newbranch = new_node(mroot);
    newbranch->value.thres_bit = bitloc;
    *parentlink = newbranch;
    if (testbit(str, slen, bitloc) ==0) {
      newbranch->left0  = newleaf;
      newbranch->right1 = node;
    } else {
      newbranch->left0  = node;
      newbranch->right1 = newleaf;
    }
    return;
  } else {
    if (testbit(str, slen, node->value.thres_bit) != 0) {
      ptree_add_entry_at(str, slen, bitloc, data, &(node->right1), mroot);
    } else {
      ptree_add_entry_at(str, slen, bitloc, data, &(node->left0), mroot);
    }
  }
}

/** 
 * Insert a new node to the index tree.
 * 
 * @param str [in] new key string
 * @param data [in] new data integer value
 * @param matchstr [in] the most matching data already exist in the index tree,
 * as obtained by aptree_search_data()
 * @param rootnode [i/o] pointer to root index node
 * @param mroot [i/o] base pointer for block malloc
 */
void
ptree_add_entry(char *str, int data, char *matchstr, PATNODE **rootnode, BMALLOC_BASE **mroot)
{
  int bitloc;

  bitloc = where_the_bit_differ(str, matchstr);
  if (*rootnode == NULL) {
    *rootnode = ptree_make_root_node(data, mroot);
  } else {
    ptree_add_entry_at(str, strlen(str), bitloc, data, rootnode, mroot);
  }

}
