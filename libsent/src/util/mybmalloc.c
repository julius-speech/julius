/**
 * @file   mybmalloc.c
 * 
 * <JA>
 * @brief  ブロック単位の動的メモリ確保
 *
 * このファイルには，要求されたサイズごとではなく，定められた一定の
 * 大きさ単位でメモリを確保する関数が定義されています．これを用いることで，
 * 細かい単位で大量にメモリ割り付けする際に起こるメモリ管理のオーバヘッドを
 * 改善できます．これらの関数は，主に言語モデルや音響モデルの読み込みに
 * 用いられています．
 * </JA>
 * 
 * <EN>
 * @brief  Dynamic memory allocation per large block
 *
 * This file defines functions that allocate memories per a certain big
 * block instead of allocating for each required sizes.
 * This function will improve the overhead of memory management operation,
 * especially when an application requires huge number of small segments
 * to be allocated.  These functions are mainly used for allocating memory
 * for acoustic models and language models.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 16:14:59 2005
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

#undef DEBUG			/* output debug message */

#include <sent/stddefs.h>

static boolean mybmalloc_initialized = FALSE; ///< TRUE if mybmalloc has already initialized
static unsigned int pagesize;		///< Page size for memoly allocation
static unsigned int blocksize;  ///< Block size in bytes
static int align;		///< Allocation alignment size in bytes
static unsigned int align_mask; ///< Bit mask to compute the actual aligned memory size

/** 
 * Set block size and memory alignment factor.
 * 
 */
static void
mybmalloc_set_param()
{
  unsigned int blockpagenum;

  /* block size should be rounded up by page size */
  pagesize = getpagesize();
  blockpagenum = (MYBMALLOC_BLOCK_SIZE + (pagesize - 1)) / pagesize;
  blocksize = pagesize * blockpagenum;

  /* alignment by a word (= pointer size?) */
#ifdef NO_ALIGN_DOUBLE
  align = sizeof(void *);
#else
  /* better for floating points */
  align = sizeof(double);
#endif
  align_mask = ~(align - 1);	/* assume power or 2 */
  //jlog("Stat: mybmalloc: pagesize=%d blocksize=%d align=%d (bytes)\n", (int)pagesize, blocksize, align);
  
  mybmalloc_initialized = TRUE;
}

/** 
 * Another version of memory block allocation, used for tree lexicon.
 * 
 * @param size [in] memory size to be allocated
 * @param list [i/o] total memory management information (will be updated here)
 * 
 * @return pointer to the newly allocated area.
 */
void *
mybmalloc2(unsigned int size, BMALLOC_BASE **list)
{
  void *allocated;
  BMALLOC_BASE *new;

  if (!mybmalloc_initialized) mybmalloc_set_param();  /* initialize if not yet */
  /* malloc segment should be aligned to a word boundary */
  size = (size + align - 1) & align_mask;
  if (*list == NULL || (*list)->now + size >= (*list)->end) {
    new = (BMALLOC_BASE *)mymalloc(sizeof(BMALLOC_BASE));
    if (size > blocksize) {
      /* large block, allocate a whole block */
      new->base = mymalloc(size);
      new->end = (char *)new->base + size;
    } else {
      /* allocate per blocksize */
      new->base = mymalloc(blocksize);
      new->end = (char *)new->base + blocksize;
    }
    new->now = (char *)new->base;
    new->next = (*list);
    *list = new;
  }
  /* return current pointer */
  allocated = (*list)->now;
  (*list)->now += size;
  return(allocated);
}

/** 
 * String duplication using mybmalloc2().
 * 
 * @param s [in] string to be duplicated
 * @param list [i/o] total memory management information pointer
 * 
 * @return pointer to the newly allocated string.
 */
char *
mybstrdup2(char *s, BMALLOC_BASE **list)
{
  char *allocated;
  int size = strlen(s) + 1;

  allocated = mybmalloc2(size, list);
  memcpy(allocated, s, size);
  return(allocated);
}

/** 
 * Free all memories allocated by mybmalloc2()
 * 
 * @param list [i/o] total memory management information (will be cleaned here)
 */
void
mybfree2(BMALLOC_BASE **list)
{
  BMALLOC_BASE *b, *btmp;
  b = *list;
  while (b) {
    btmp = b->next;
    free(b->base);
    free(b);
    b = btmp;
  }
  *list = NULL;
}
