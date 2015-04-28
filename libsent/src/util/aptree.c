/**
 * @file   aptree.c
 * 
 * <JA>
 * @brief  パトリシア検索木を用いた名前検索：データ型がポインタの場合
 * </JA>
 * 
 * <EN>
 * @brief  Patricia index tree for name lookup: data type = pointer
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 15:21:53 2005
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
#include <sent/ptree.h>

/** 
 * Allocate a new node.
 * 
 * 
 * @return pointer to the new node.
 */
static APATNODE *
new_node(BMALLOC_BASE **mroot)
{
  APATNODE *tmp;

  tmp = (APATNODE *)mybmalloc2(sizeof(APATNODE), mroot);
  tmp->left0 = NULL;
  tmp->right1 = NULL;

  return(tmp);
}

/** 
 * Recursive function to search the data in the tree
 * 
 * @param node [in] current node.
 * 
 * @return pointer to the found data
 */
static void *
aptree_search_data_r(APATNODE *node, char *str, int maxbitplace)
{
#if 1
  /* non-recursive */
  APATNODE *n;
  APATNODE *branch = NULL;
  n = node;
  while(n->left0 != NULL || n->right1 != NULL) {
    branch = n;
    if (testbit_max(str, n->value.thres_bit, maxbitplace) != 0) {
      n = n->right1;
    } else {
      n = n->left0;
    }
  } 
  return(n->value.data);
#else
  if (node->left0 == NULL && node->right1 == NULL) {
    return(node->value.data);
  } else {
    if (testbit_max(str, node->value.thres_bit, maxbitplace) != 0) {
      return(aptree_search_data_r(node->right1, str, maxbitplace));
    } else {
      return(aptree_search_data_r(node->left0, str, maxbitplace));
    }
  }
#endif
}

/** 
 * Search for the data whose key string matches the given string.
 * 
 * @param str [in] search key string
 * @param node [in] root node of index tree
 * 
 * @return the exactly found data pointer, or the nearest one.
 */
void *
aptree_search_data(char *str, APATNODE *node)
{
  if (node == NULL) {
    //("Error: aptree_search_data: no node, search for \"%s\" failed\n", str);
    return NULL;
  }
  return(aptree_search_data_r(node, str, strlen(str) * 8 + 8));
}


/*******************************************************************/
/* add 1 node to given ptree */

/** 
 * Make a root node of a index tree.
 * 
 * @param data [in] the first data
 * 
 * @return the newly allocated root node.
 */
APATNODE *
aptree_make_root_node(void *data, BMALLOC_BASE **mroot)
{
  APATNODE *nnew;
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
 * @param data [in] new data pointer
 * @param parentlink [i/o] the parent node to which this node will be added
 */
static void
aptree_add_entry_at(char *str, int slen, int bitloc, void *data, APATNODE **parentlink, BMALLOC_BASE **mroot)
{
#if 1
  /* non-recursive */
  APATNODE **p;
  APATNODE *newleaf, *newbranch, *node;

  p = parentlink;
  node = *p;
  while(node->value.thres_bit <= bitloc &&
	(node->left0 != NULL || node->right1 != NULL)) {
    if (testbit(str, slen, node->value.thres_bit) != 0) {
      p = &(node->right1);
    } else {
      p = &(node->left0);
    }
    node = *p;
  }
	 
  /* insert between [parent] and [node] */
  newleaf = new_node(mroot);
  newleaf->value.data = data;
  newbranch = new_node(mroot);
  newbranch->value.thres_bit = bitloc;
  *p = newbranch;
  if (testbit(str, slen, bitloc) ==0) {
    newbranch->left0  = newleaf;
    newbranch->right1 = node;
  } else {
    newbranch->left0  = node;
    newbranch->right1 = newleaf;
  }

#else

  APATNODE *node;
  APATNODE *newleaf, *newbranch;

  node = *parentlink;
  if (node->value.thres_bit > bitloc ||
      (node->left0 == NULL && node->right1 == NULL)) {
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
      aptree_add_entry_at(str, slen, bitloc, data, &(node->right1), mroot);
    } else {
      aptree_add_entry_at(str, slen, bitloc, data, &(node->left0), mroot);
    }
  }
#endif
}

/** 
 * Insert a new node to the index tree.
 * 
 * @param str [in] new key string
 * @param data [in] new data pointer
 * @param matchstr [in] the most matching data already exist in the index tree,
 * as obtained by aptree_search_data()
 * @param rootnode [i/o] pointer to root index node
 */
void
aptree_add_entry(char *str, void *data, char *matchstr, APATNODE **rootnode, BMALLOC_BASE **mroot)
{
  int bitloc;

  bitloc = where_the_bit_differ(str, matchstr);
  if (*rootnode == NULL) {
    *rootnode = aptree_make_root_node(data, mroot);
  } else {
    aptree_add_entry_at(str, strlen(str), bitloc, data, rootnode, mroot);
  }

}

/*******************************************************************/

/** 
 * Recursive sunction to find and remove an entry.
 * 
 * @param now [in] current node
 * @param up [in] parent node
 * @param up2 [in] parent parent node
 */
static void
aptree_remove_entry_r(APATNODE *now, APATNODE *up, APATNODE *up2, char *str, int maxbitplace, APATNODE **root)
{
  APATNODE *b;

  if (now->left0 == NULL && now->right1 == NULL) {
    /* assume this is exactly the node of data that has specified key string */
    /* make sure the data of your removal request already exists before call this */
    /* execute removal */
    if (up == NULL) {
      //free(now);
      *root = NULL;
      return;
    }
    b = (up->right1 == now) ? up->left0 : up->right1;
    if (up2 == NULL) {
      //free(now);
      //free(up);
      *root = b;
      return;
    }
    if (up2->left0 == up) {
      up2->left0 = b;
    } else {
      up2->right1 = b;
    }
    //free(now);
    //free(up);
    return;
  } else {
    /* traverse */
    if (testbit_max(str, now->value.thres_bit, maxbitplace) != 0) {
      aptree_remove_entry_r(now->right1, now, up, str, maxbitplace, root);
    } else {
      aptree_remove_entry_r(now->left0, now, up, str, maxbitplace, root);
    }
  }
}
    
/** 
 * Remove a node from the index tree.
 * 
 * @param str [in] existing key string (must exist in the index tree)
 * @param rootnode [i/o] pointer to root index node
 *
 */
void
aptree_remove_entry(char *str, APATNODE **rootnode)
{
  if (*rootnode == NULL) {
    jlog("Warning: aptree: no node, deletion for \"%s\" failed\n", str);
    return;
  }
  aptree_remove_entry_r(*rootnode, NULL, NULL, str, strlen(str)*8+8, rootnode);
}

/*******************************************************************/

/** 
 * Recursive function to traverse index tree and execute
 * the callback for all the existing data.
 * 
 * @param node [in] current node
 * @param callback [in] callback function
 */
void
aptree_traverse_and_do(APATNODE *node, void (*callback)(void *))
{
  if (node->left0 == NULL && node->right1 == NULL) {
    (*callback)(node->value.data);
  } else {
    if (node->left0 != NULL) {
      aptree_traverse_and_do(node->left0, callback);
    }
    if (node->right1 != NULL) {
      aptree_traverse_and_do(node->right1, callback);
    }
  }
}

/*************************************************************/

static void
aptree_count(APATNODE *node, int *count_branch, int *count_data, int *maxbit)
{
  if (node->left0 == NULL && node->right1 == NULL) {
    (*count_data)++;
  } else {
    if (node->value.thres_bit > *maxbit) {
      *maxbit = node->value.thres_bit;
    }
    (*count_branch)++;
    if (node->left0 != NULL) {
      aptree_count(node->left0, count_branch, count_data, maxbit);
    }
    if (node->right1 != NULL) {
      aptree_count(node->right1, count_branch, count_data, maxbit);
    }
  }
}  

static int
aptree_build_index(APATNODE *node, int *num, int *data_id, int *left, int *right, int *data)
{
  int id;

  id = *num;
  (*num)++;
  if (node->left0 == NULL && node->right1 == NULL) {
    left[id] = -1;
    right[id] = -1;
    data[id] = *data_id;
    /* node->value.data を保存 */
    (*data_id)++;
  } else {
    data[id] = node->value.thres_bit;
    if (node->left0 != NULL) {
      left[id] = aptree_build_index(node->left0, num, data_id, left, right, data);
    } else {
      left[id] = -1;
    }
    if (node->right1 != NULL) {
      right[id] = aptree_build_index(node->right1, num, data_id, left, right, data);
    } else {
      right[id] = -1;
    }
  }
  return id;
}  

static void
aptree_write_leaf(FILE *fp, APATNODE *node, boolean (*callback)(void *, FILE *fp), boolean *error_p)
{
  if (node->left0 == NULL && node->right1 == NULL) {
    if ((*callback)(node->value.data, fp) == FALSE) {
      *error_p = TRUE;
    }
  } else {
    if (node->left0 != NULL) {
      aptree_write_leaf(fp, node->left0, callback, error_p);
    }
    if (node->right1 != NULL) {
      aptree_write_leaf(fp, node->right1, callback, error_p);
    }
  }
}


boolean
aptree_write(FILE *fp, APATNODE *root, boolean (*save_data_func)(void *, FILE *fp))
{
  int count_node, count_branch, count_data, maxbit;
  int *left, *right, *value;
  int num, did;
  boolean err;

  if (root == NULL) return TRUE;

  /* count statistics */
  count_branch = count_data = 0;
  maxbit = 0;
  aptree_count(root, &count_branch, &count_data, &maxbit);
  count_node = count_branch + count_data;
  jlog("Stat: aptree_write: %d nodes (%d branch + %d data), maxbit=%d\n", count_node, count_branch, count_data, maxbit);
  /* allocate */
  left = (int *)mymalloc(sizeof(int) * count_node);
  right = (int *)mymalloc(sizeof(int) * count_node);
  value = (int *)mymalloc(sizeof(int) * count_node);
  /* make index */
  did = num = 0;
  aptree_build_index(root, &num, &did, left, right, value);
#if 0
  {
    int i;
    for(i=0;i<count_node;i++) {
      printf("%d: %d %d %d\n", i, left[i], right[i], value[i]);
    }
  }
#endif
  /* write tree to file */
  if (myfwrite(&count_node, sizeof(int), 1, fp) < 1) {
    jlog("Error: aptree_write: fail to write header\n");
    return FALSE;
  }
  if (myfwrite(&count_data, sizeof(int), 1, fp) < 1) {
    jlog("Error: aptree_write: fail to write header\n");
    return FALSE;
  }
  if (myfwrite(left, sizeof(int), count_node, fp) < count_node) {
    jlog("Error: aptree_write: fail to write %d bytes\n", sizeof(int) * count_node);
    return FALSE;
  }
  if (myfwrite(right, sizeof(int), count_node, fp) < count_node) {
    jlog("Error: aptree_write: fail to write %d bytes\n", sizeof(int) * count_node);
    return FALSE;
  }
  if (myfwrite(value, sizeof(int), count_node, fp) < count_node) {
    jlog("Error: aptree_write: fail to write %d bytes\n", sizeof(int) * count_node);
    return FALSE;
  }
  if (save_data_func != NULL) {
    /* write leaf node data */
    err = FALSE;
    aptree_write_leaf(fp, root, save_data_func, &err);
  }
  if (err) {
    jlog("Error: aptree_write: error occured when writing tree leaf data\n");
    return FALSE;
  }

  free(value);
  free(right);
  free(left);

  return TRUE;
}


boolean
aptree_read(FILE *fp, APATNODE **root, BMALLOC_BASE **mroot, void *data, boolean (*load_data_func)(void **, void *, FILE *))
{
  int count_node, count_branch, count_data, maxbit;
  int *left, *right, *value;
  int num, did;
  boolean err;
  APATNODE *nodelist;
  APATNODE *node;
  int i;

  if (*root != NULL) {
    jlog("Error: aptree_read: root node != NULL!\n");
    return FALSE;
  }

  /* read header */
  if (myfread(&count_node, sizeof(int), 1, fp) < 1) {
    jlog("Error: aptree_read: fail to read header\n");
    return FALSE;
  }
  if (myfread(&count_data, sizeof(int), 1, fp) < 1) {
    jlog("Error: aptree_read: fail to read header\n");
    return FALSE;
  }
  jlog("Stat: aptree_read: %d nodes (%d branch + %d data)\n",
       count_node, count_node - count_data, count_data);
  /* prepare buffer */
  left = (int *)mymalloc(sizeof(int) * count_node);
  right = (int *)mymalloc(sizeof(int) * count_node);
  value = (int *)mymalloc(sizeof(int) * count_node);
  /* read data */
  if (myfread(left, sizeof(int), count_node, fp) < count_node) {
    jlog("Error: aptree_read: fail to read %d bytes\n", sizeof(int) * count_node);
    return FALSE;
  }
  if (myfread(right, sizeof(int), count_node, fp) < count_node) {
    jlog("Error: aptree_read: fail to read %d bytes\n", sizeof(int) * count_node);
    return FALSE;
  }
  if (myfread(value, sizeof(int), count_node, fp) < count_node) {
    jlog("Error: aptree_read: fail to read %d bytes\n", sizeof(int) * count_node);
    return FALSE;
  }
  /* allocate nodes */
  nodelist = (APATNODE *)mybmalloc2(sizeof(APATNODE) * count_node, mroot);
  for(i=0;i<count_node;i++) {
    node = &(nodelist[i]);
    if (left[i] == -1) {
      node->left0 = NULL;
    } else {
      node->left0 = &(nodelist[left[i]]);
    }
    if (right[i] == -1) {
      node->right1 = NULL;
    } else {
      node->right1 = &(nodelist[right[i]]);
    }
    if (left[i] == -1 && right[i] == -1) {
      /* load leaf data node */
      if ((*load_data_func)(&(node->value.data), data, fp) == FALSE) {
	jlog("Error: aptree_read: failed to load leaf data entity\n");
	return FALSE;
      }
    } else {
      /* set thres bit */
	node->value.thres_bit = value[i];
    }
  }
  /* set root node */
  *root = &(nodelist[0]);
  
  free(value);
  free(right);
  free(left);

  return TRUE;
}
