/**
 * @file   strcasecmp.c
 * 
 * <JA>
 * @brief  strcasecmp の定義
 *
 * strcasecmp() の互換関数です．strcasecmp() が実装されていない環境で
 * 代わりに使用されます．
 * </JA>
 * 
 * <EN>
 * @brief  Definition of strcasecmp
 *
 * This function is fully compatible with strcasecmp().  This will be used
 * if system does not support strcasecmp().
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 17:02:09 2005
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

#ifndef HAVE_STRCASECMP

/** 
 * Compare two strings, ignoring case.
 * 
 * @param s1 [in] string 1
 * @param s2 [in] string 2
 * 
 * @return -1 if s1 < s2, 0 if s1 == s2, 1 if s1 > s2.
 */
int
strcasecmp(char *s1, char *s2)
{
  int c1, c2;

  do {
    c1 = (*s1 >= 'a' && *s1 <= 'z') ? *s1 - 040 : *s1;
    c2 = (*s2 >= 'a' && *s2 <= 'z') ? *s2 - 040 : *s2;
    if (c1 != c2) break;
  }  while (*(s1++) && *(s2++));
  return(c1 - c2);
}

/** 
 * Compare two strings, ignoring case, at most first @a n bytes.
 * 
 * @param s1 [in] string 1
 * @param s2 [in] string 2
 * @param n [in] maximum length to compare.
 * 
 * @return -1 if s1 < s2, 0 if s1 == s2, 1 if s1 > s2.
 */
int
strncasecmp(char *s1, char *s2, size_t n)
{
  int c1, c2;
  do {
    c1 = (*s1 >= 'a' && *s1 <= 'z') ? *s1 - 040 : *s1;
    c2 = (*s2 >= 'a' && *s2 <= 'z') ? *s2 - 040 : *s2;
    if (c1 != c2) break;
  }  while (*(s1++) && *(s2++) && (--n));
  return(c1 - c2);
}

#endif /* ~HAVE_STRCASECMP */
