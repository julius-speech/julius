/**
 * @file   charconv_iconv.c
 * 
 * <JA>
 * @brief  文字コード変換 (iconvライブラリ使用)
 *
 * </JA>
 * 
 * <EN>
 * @brief  Character set conversion using iconv library
 *
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 16:02:41 2005
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

#include "app.h"

#ifdef CHARACTER_CONVERSION
#ifdef HAVE_ICONV

#include <iconv.h>
static iconv_t cd = (iconv_t)-1; ///< Converstion descriptor

/** 
 * Setup charset conversion for iconv.
 * 
 * @param fromcode [in] input charset code name (NULL invalid)
 * @param tocode [in] output charset code name, or NULL when disable conversion
 * @param enable_conv [out] return whether conversion should be enabled or not
 * 
 * @return TRUE on success, FALSE on failure (unknown code name).
 */
boolean
charconv_iconv_setup(char *fromcode, char *tocode, boolean *enable_conv)
{
  /* clear already allocated descriptor */
  if (cd != (iconv_t)-1) {
    if (iconv_close(cd) < 0) {
      perror("j_prinf_set_iconv");
      return FALSE;
    }
    cd = (iconv_t)-1;
  }
  
  if (tocode == NULL) {
    /* disable conversion */
    *enable_conv = FALSE;
  } else {
    /* check for codes */
    if (fromcode == NULL) {
      jlog("Error: charconv_iconv: charset names of both input and output should be given.\n");
      jlog("Error: charconv_iconf: use \"-charconv from to\" instead of \"-kanji\".\n");
      *enable_conv = FALSE;
      return FALSE;
    }      
    /* allocate conversion descriptor */
    cd = iconv_open(tocode, fromcode);
    if (cd == (iconv_t)-1) {
      /* allocation failed */
      jlog("Error: charconv_iconv: unknown charset name in \"%s\" or \"%s\"\n", fromcode, tocode);
      jlog("Error: charconv_iconv: do \"iconv --list\" to get the list of available charset names.\n");
      *enable_conv = FALSE;
      return FALSE;
    }
    *enable_conv = TRUE;
  }
  return TRUE;
}

/** 
 * Apply charset conversion to a string using iconv.
 * 
 * @param instr [in] source string
 * @param outstr [out] destination buffer
 * @param maxoutlen [in] allocated length of outstr in byte.
 *
 * @return either of instr or outstr, that holds the result string.
 *
 */
char *
charconv_iconv(char *instr, char *outstr, int maxoutlen)
{
  char *src, *dst;
  size_t srclen, dstlen;
  size_t ret;

  if (cd == (iconv_t)-1) {
    fprintf(stderr, "InternalError: codeconv: conversion descriptor not allocated\n"); exit(-1);
  }
  srclen = strlen(instr)+1;
  dstlen = maxoutlen;
  src = instr;
  dst = outstr;
  ret = iconv(cd, (ICONV_CONST char **)&src, &srclen, &dst, &dstlen);
  if (ret == -1) {
    switch(errno) {
    case EILSEQ:
      fprintf(stderr, "InternalError: codeconv: invalid multibyte sequence in the input\n"); exit(-1);
      break;
    case EINVAL:
      fprintf(stderr, "InternalError: codeconv: incomplete multibyte sequence in the input\n"); exit(-1);
      break;
    case E2BIG:
      fprintf(stderr, "InternalError: codeconv: converted string size exceeded buffer (>%d)\n", maxoutlen); exit(-1);
      break;
    }
  }

  /* outstr always holds the result */
  return(outstr);
}

#endif /* HAVE_ICONV */
#endif /* CHARACTER_CONVERSION */
