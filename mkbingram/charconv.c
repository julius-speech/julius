/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <stdio.h>
#include <string.h>

#define strmatch !strcmp

#if defined(_WIN32)

/* winnls */
#include <windows.h>
#include <winnls.h>
#define UNICODE_BUFFER_SIZE 4096 ///< Buffer length for unicode conversion
static unsigned int from_cp;	///< Source codepage
static unsigned int to_cp;	///< Target codepage
static wchar_t unibuf[UNICODE_BUFFER_SIZE]; ///< buffer for unicode conversion

#else

/* iconv */
#include <iconv.h>
#include <errno.h>
#include <stdlib.h>
static iconv_t cd = (iconv_t)-1; ///< Converstion descriptor

#endif

static int convert_enabled = 0; ///< 1 if charset converstion is enabled


#if defined(_WIN32)

static int
str2code(char *codestr, unsigned int *code)
{
  if (strmatch(codestr, "euc-jp")
      || strmatch(codestr, "euc")
      || strmatch(codestr, "eucjp")) {
    /* input = Shift_jis (codepage 932) */
    *code = 20932;
  } else if (strmatch(codestr, "ansi")) {
    /* ANSI codepage (MBCS) ex. shift-jis in Windows XP Japanese edition.*/
    *code = CP_ACP;
  } else if (strmatch(codestr, "mac")) {
    /* Macintosh codepage */
    *code = CP_MACCP;
  } else if (strmatch(codestr, "oem")) {
    /* OEM localized default codepage */
    *code = CP_OEMCP;
  } else if (strmatch(codestr, "utf-7")) {
    /* UTF-7 codepage */
    *code = CP_UTF7;
  } else if (strmatch(codestr, "utf-8")) {
    /* UTF-8 codepage */
    *code = CP_UTF8;
  } else if (strmatch(codestr, "sjis")
	     || strmatch(codestr, "sjis-win")
	     || strmatch(codestr, "shift-jis")
	     || strmatch(codestr, "shift_jis")) {
    /* sjis codepage = 932 */
    *code = 932;
  } else if (codestr[0] >= '0' && codestr[0] <= '9') {
    /* codepage number */
    *code = atoi(codestr);
    if (! IsValidCodePage(*code)) {
      jlog("Error: charconv_win32: codepage \"%d\" not found\n", codestr);
      return -1;
    }
  } else {
    fprintf(stderr, "Error: str2code: unknown source codepage \"%s\"\n", codestr);
    fprintf(stderr, "Error: str2code: valids are \"euc-jp\", \"ansi\", \"mac\", \"oem\", \"utf-7\", \"utf-8\", \"sjis\" and codepage number\n");
    return -1;
  }
  
  return 0;
}
#endif


/** 
 * Setup charset conversion.
 * 
 * @param fromcode [in] input charset name (only libjcode accepts NULL)
 * @param tocode [in] output charset name, or NULL when disable conversion
 * 
 * @return 0 on success, -1 on failure.
 */
int
charconv_setup(char *fromcode, char *tocode)
{
  convert_enabled = 0;

  if (fromcode == NULL || tocode == NULL) {
    fprintf(stderr, "Error: charconv_setup: input code or output code not specified\n");
    return -1;
  }

#if defined(_WIN32)
  if (str2code(fromcode, &from_cp) == -1) {
    fprintf(stderr, "Error: charconv_setup: unknown codepage specified\n");
    return -1;
  }
  if (str2code(tocode, &to_cp) == -1) {
    fprintf(stderr, "Error: charconv_setup: unknown codepage specified\n");
    return -1;
  }
#else
  /* clear already allocated descriptor */
  if (cd != (iconv_t)-1) {
    if (iconv_close(cd) < 0) {
      fprintf(stderr, "Error: charconv_setup: failed to close iconv\n");
      return -1;
    }
    cd = (iconv_t)-1;
  }
  /* allocate conversion descriptor */
  cd = iconv_open(tocode, fromcode);
  if (cd == (iconv_t)-1) {
    /* allocation failed */
    fprintf(stderr, "Error: charconv_setup: unknown charset name in \"%s\" or \"%s\"\n", fromcode, tocode);
    fprintf(stderr, "Error: charconv_setup: do \"iconv --list\" to get the list of available charset names.\n");
    return -1;
  }

#endif

  convert_enabled = 1;

  return(0);
}

/** 
 * Apply charset conversion to a string.
 * 
 * @param instr [in] source string
 * @param outstr [in] destination buffer
 * @param maxoutlen [in] allocated length of outstr in byte.
 *
 * @return either of instr or outstr, that holds the result string.
 *
 */
char *
charconv(char *instr, char *outstr, int maxoutlen)
{

#if defined(_WIN32)

  int unilen, newlen;
  char *srcbuf;

  /* if diabled return instr itself */
  if (convert_enabled == 0) return(instr); /* no conversion */
  
  srcbuf = instr;

  /* get length of unicode string */
  unilen = MultiByteToWideChar(from_cp, 0, srcbuf, -1, NULL, 0);
  if (unilen <= 0) {
    jlog("Error: charconv: conversion error?\n");
    return(instr);
  }
  if (unilen > UNICODE_BUFFER_SIZE) {
    jlog("Error: charconv: unicode buffer size exceeded (%d > %d)!\n", unilen, UNICODE_BUFFER_SIZE);
    return(instr);
  }
  /* convert source string to unicode */
  MultiByteToWideChar(from_cp, 0, srcbuf, -1, unibuf, unilen);
  /* get length of target string */
  newlen = WideCharToMultiByte(to_cp, 0, unibuf, -1, outstr, 0, NULL, NULL);
  if (newlen <= 0) {
    jlog("Error: charconv: conversion error?\n");
    return(instr);
  }
  if (newlen > maxoutlen) {
    jlog("Error: charconv: target buffer size exceeded (%d > %d)!\n", newlen, maxoutlen);
    return(instr);
  }
  /* convert unicode to target string */
  WideCharToMultiByte(to_cp, 0, unibuf, -1, outstr, newlen, NULL, NULL);
  return(outstr);

#else

  char *src, *dst;
  size_t srclen, dstlen;
  size_t ret;

  /* if diabled return instr itself */
  if (convert_enabled == 0) return(instr); /* no conversion */

  if (cd == (iconv_t)-1) {
    fprintf(stderr, "Error: charconv: conversion descriptor not allocated\n");
    return(instr);
  }

  srclen = strlen(instr)+1;
  dstlen = maxoutlen;
  src = instr;
  dst = outstr;
  ret = iconv(cd, &src, &srclen, &dst, &dstlen);
  if (ret == -1) {
    switch(errno) {
    case EILSEQ:
      fprintf(stderr, "Error: charconv: invalid multibyte sequence in the input\n"); exit(-1);
      break;
    case EINVAL:
      fprintf(stderr, "Error: charconv: incomplete multibyte sequence in the input\n"); exit(-1);
      break;
    case E2BIG:
      fprintf(stderr, "Error: charconv: converted string size exceeded buffer (>%d)\n", maxoutlen); exit(-1);
      break;
    }
  }

  return(outstr);

#endif

}
