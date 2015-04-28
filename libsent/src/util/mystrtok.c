/**
 * @file   mystrtok.c
 * 
 * <JA>
 * @brief  文字列をクォーテーションを考慮してトークンに分割する
 *
 * 文字列を空白等でトークンに分割します．このとき，指定された
 * クォーテーション記号で囲まれた部分は必ず１つのトークンとして扱われます．
 * </JA>
 * 
 * <EN>
 * @brief  Extract tokens from strings, with quotation handling.
 *
 * When extracting tokens from strings, the part enclosed by the specified
 * braces are forced to be treated as a single token.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 16:31:39 2005
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

/* strtok with quotation handling */

/// Abbreviation macro to find if A is included in string delim
#define ISTOKEN(A) strchr(delim, A)

/** 
 * Generic function to extract tokens from strings, with quotation handling.
 * The usage is as the same as strtok.
 * 
 * @param str [i/o] source string, or NULL when this is a continuous call from previous call.  Will be truncated in this function.
 * @param delim [in] string to specify the delimiters.
 * @param left_paren [in] left brace
 * @param right_paren [in] right brace
 * @param mode [in] if 1, just move to the beginning of next token
 * 
 * @return pointer to the next extracted token.
 */
char *
mystrtok_quotation(char *str, char *delim, int left_paren, int right_paren, int mode)
{
  static char *buf;		/* target string buffer */
  static char *pos;		/* current pointer position */
  char *p;
  char *from;
  int c;

  if (str != NULL) {
    pos = buf = str;
  }

  /* find start point */
  p = pos;
  while (*p != '\0' && ISTOKEN(*p)) p++;
  if (*p == '\0') return NULL;	/* no token left */

  /* if mode == 1, exit here */
  if (mode == 1) {
    pos = p;
    return p;
  }
  
  /* copy to ret_buf until end point is found */
  c = *p;
  if (c == left_paren) {
    p++;
    if (*p == '\0') return NULL;
    from = p;
    while ((c = *p) != '\0' && 
	   ((c != right_paren) || (*(p+1) != '\0' && !ISTOKEN(*(p+1))))) p++;
	
    /* if quotation not terminated, allow the rest as one token */
    /* if (*p == '\0') return NULL; */
  } else {
    from = p;
    while ((c = *p) != '\0' && (!ISTOKEN(c))) p++;
  }
  if (*p != '\0') {
    *p = '\0';
    p++;
  }
  pos = p;
  return from;
}


/** 
 * Extract tokens considering quotation by double quotation mark.
 * 
 * @param str [i/o] source string, will be truncated.
 * @param delim [in] string of all token delimiters
 * 
 * @return pointer to the next extracted token, or NULL when no token found.
 */
char  *
mystrtok_quote(char *str, char *delim)
{
  return(mystrtok_quotation(str, delim, 34, 34, 0)); /* "\"" == 34 */
}

/** 
 * Extract tokens, not considering quotation, just as the same as strtok.
 * 
 * @param str [i/o] source string, will be truncated.
 * @param delim [in] string of all token delimiters
 * 
 * @return pointer to the next extracted token, or NULL when no token found.
 */
char  *
mystrtok(char *str, char *delim)
{
  return(mystrtok_quotation(str, delim, -1, -1, 0));
}

/** 
 * Just move to the beginning of the next token, without modifying the @a str.
 * 
 * @param str [i/o] source string, will be truncated.
 * @param delim [in] string of all token delimiters
 * 
 * @return pointer to the next extracted token, or NULL when no token found.
 */
char *
mystrtok_movetonext(char *str, char *delim)
{
  return(mystrtok_quotation(str, delim, -1, -1, 1));
}
