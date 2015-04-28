/**
 * @file   charconv_win32.c
 * 
 * <JA>
 * @brief  文字コード変換 (Win32 API + libjcode 使用)
 *
 * Windows の WideCharToMultiByte() および MultiByteToWideChar() を
 * 使用した文字コード変換を行う. "ansi" "oem" "mac" "utf-7" "utf-8" あるいは
 * 任意のコードページ番号間の変換を，unicode を介して行う. 
 *
 * Windows では EUC のコードページに対応していないので，変換元の文字コードが
 * euc-jp のときは，libjcode で SJIS に変換してからunicodeへ変換する. 
 *
 * </JA>
 * 
 * <EN>
 * @brief  Character set conversion using Win32 MultiByte function + libjcode
 *
 * Perform character set conversion using Windows native API
 * WideCharToMultiByte() and MultiByteToWideChar().  Conversion between
 * codepages of "ansi" "oem" "mac" "utf-7" "utf-8" or codepage number supported
 * at the running OS are supported using unicode.
 *
 * Conversion from Japanese-euc ("euc-jp") is optionally supported by the
 * libjcode library.
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

#ifdef USE_WIN32_MULTIBYTE

#include <windows.h>
#include <winnls.h>
#include "libjcode/jlib.h"

static boolean euctosjis = FALSE; ///< TRUE if use libjcode for euc->sjis conv.
static boolean only_euc_conv = FALSE; ///< Perform only euc->sjis

static unsigned int from_cp;	///< Source codepage
static unsigned int to_cp;	///< Target codepage

/** 
 * Setup charset conversion for win32.
 * 
 * @param fromcode [in] input charset code name or codepage number string, NULL invalid
 * @param tocode [in] output charset code name or codepage number string, or NULL when disable conversion
 * @param enable_conv [out] store whether conversion should be enabled or not
 * 
 * @return TRUE on success, FALSE on failure (unknown codename or unsupported codepage).
 */
boolean
charconv_win32_setup(char *fromcode, char *tocode, boolean *enable_conv)
{
  unsigned int src_p, dst_p;
  
  if (tocode == NULL) {
    /* just disable conversion */
    *enable_conv = FALSE;
  } else {
    /* determine source character set */
    if (fromcode == NULL) {
      jlog("Error: charconv_win32: charset names of both input and output should be given.\n");
      jlog("Error: charconv_win32: use \"-charconv from to\" instead of \"-kanji\".\n");
      *enable_conv = FALSE;
      return FALSE;
    }
    euctosjis = FALSE;
    if (strmatch(fromcode, "euc-jp")
	       || strmatch(fromcode, "euc")
	       || strmatch(fromcode, "eucjp")) {
      /* pre-convert Japanese euc to Shift-jis */
      euctosjis = TRUE;
      /* input = Shift_jis (codepage 932) */
      from_cp = 932;
    } else if (strmatch(fromcode, "ansi")) {
      /* ANSI codepage (MBCS) ex. shift-jis in Windows XP Japanese edition.*/
      from_cp = CP_ACP;
    } else if (strmatch(fromcode, "mac")) {
      /* Macintosh codepage */
      from_cp = CP_MACCP;
    } else if (strmatch(fromcode, "oem")) {
      /* OEM localized default codepage */
      from_cp = CP_OEMCP;
    } else if (strmatch(fromcode, "utf-7")) {
      /* UTF-7 codepage */
      from_cp = CP_UTF7;
    } else if (strmatch(fromcode, "utf-8")) {
      /* UTF-8 codepage */
      from_cp = CP_UTF8;
    } else if (strmatch(fromcode, "sjis")
	       || strmatch(fromcode, "sjis-win")
	       || strmatch(fromcode, "shift-jis")
	       || strmatch(fromcode, "shift_jis")) {
      /* sjis codepage = 932 */
      from_cp = 932;
    } else if (fromcode[0] >= '0' && fromcode[0] <= '9') {
      /* codepage number */
      from_cp = atoi(fromcode);
      if (! IsValidCodePage(from_cp)) {
	jlog("Error: charconv_win32: codepage #%d not found\n", from_cp);
	*enable_conv = FALSE;
	return FALSE;
      }
    } else {
      jlog("Error: charconv_win32: unknown source codepage \"%s\"\n", fromcode);
      jlog("Error: charconv_win32: valids are \"ansi\", \"mac\", \"oem\", \"utf-7\", \"utf-8\" and codepage number\n");
      jlog("Error: charconv_win32: the default local charcode can be speicified by \"ansi\".\n");
      *enable_conv = FALSE;
      return FALSE;
    }
    /* determine the target character set */
    if (strmatch(tocode, "ansi")) {
      /* ANSI codepage (MBCS) ex. shift-jis in Windows XP Japanese edition.*/
      to_cp = CP_ACP;
    } else if (strmatch(tocode, "mac")) {
      /* Macintosh codepage */
      to_cp = CP_MACCP;
    } else if (strmatch(tocode, "oem")) {
      /* OEM codepage */
      to_cp = CP_OEMCP;
    } else if (strmatch(tocode, "utf-7")) {
      /* UTF-7 codepage */
      to_cp = CP_UTF7;
    } else if (strmatch(tocode, "utf-8")) {
      /* UTF-8 codepage */
      to_cp = CP_UTF8;
    } else if (strmatch(tocode, "sjis")
	       || strmatch(tocode, "sjis-win")
	       || strmatch(tocode, "shift-jis")
	       || strmatch(tocode, "shift_jis")) {
      /* sjis codepage = 932 */
      to_cp = 932;
    } else if (tocode[0] >= '0' && tocode[0] <= '9') {
      /* codepage number */
      to_cp = atoi(tocode);
      if (! IsValidCodePage(to_cp)) {
	jlog("Error: charconv_win32: codepage #%d not found\n", to_cp);
	*enable_conv = FALSE;
	return FALSE;
      }
    } else {
      jlog("Error: charconv_win32: unknown target codepage \"%s\"\n", tocode);
      jlog("Error: charconv_win32: valids are \"ansi\", \"mac\", \"oem\", \"utf-7\", \"utf-8\" and codepage number\n");
      jlog("Error: charconv_win32: the default local charcode can be speicified by \"ansi\".\n");
      *enable_conv = FALSE;
      return FALSE;
    }
    
    /* check whether the actual conversion is needed */
    src_p = from_cp;
    dst_p = to_cp;
    if (src_p == CP_ACP) src_p = GetACP();
    if (dst_p == CP_ACP) dst_p = GetACP();
    if (src_p == CP_OEMCP) src_p = GetOEMCP();
    if (dst_p == CP_OEMCP) dst_p = GetOEMCP();
    
    if (src_p == dst_p) {
      if (euctosjis == FALSE) {
	only_euc_conv = FALSE;
	*enable_conv = FALSE;
      } else {
	only_euc_conv = TRUE;
	*enable_conv = TRUE;
      }
    } else {
      only_euc_conv = FALSE;
      *enable_conv = TRUE;
    }
  }
  
  return TRUE;
}

#define UNICODE_BUFFER_SIZE 4096 ///< Buffer length to use for unicode conversion
static wchar_t unibuf[UNICODE_BUFFER_SIZE]; ///< Local work area for unicode conversion

/** 
 * Apply charset conversion to a string using win32 functions
 * 
 * @param instr [in] source string
 * @param outstr [in] destination buffer
 * @param maxoutlen [in] allocated length of outstr in byte.
 *
 * @return either of instr or outstr, that holds the result string.
 *
 */
char *
charconv_win32(char *instr, char *outstr, int maxoutlen)
{
  int unilen, newlen;
  char *srcbuf;
  
  srcbuf = instr;
  if (euctosjis == TRUE) {
    /* euc->sjis conversion */
    //toStringSJIS(instr, outstr, maxoutlen);
    EUCtoSJIS(instr, outstr, maxoutlen);
    srcbuf = outstr;
    if (only_euc_conv) {
      return(outstr);
    }
  }
  
  /* get length of unicode string */
  unilen = MultiByteToWideChar(from_cp, 0, srcbuf, -1, NULL, 0);
  if (unilen <= 0) {
    jlog("Error: charconv_win32: conversion error?\n");
    return(instr);
  }
  if (unilen > UNICODE_BUFFER_SIZE) {
    jlog("Error: charconv_win32: unicode buffer size exceeded (%d > %d)!\n", unilen, UNICODE_BUFFER_SIZE);
    return(instr);
  }
  /* convert source string to unicode */
  MultiByteToWideChar(from_cp, 0, srcbuf, -1, unibuf, unilen);
  /* get length of target string */
  newlen = WideCharToMultiByte(to_cp, 0, unibuf, -1, outstr, 0, NULL, NULL);
  if (newlen <= 0) {
    jlog("Error: charconv_win32: conversion error?\n");
    return(instr);
  }
  if (newlen > maxoutlen) {
    jlog("Error: charconv_win32: target buffer size exceeded (%d > %d)!\n", newlen, maxoutlen);
    return(instr);
  }
  /* convert unicode to target string */
  WideCharToMultiByte(to_cp, 0, unibuf, -1, outstr, newlen, NULL, NULL);

  return(outstr);
}

#endif /* USE_WIN32_MULTIBYTE */

#endif /* CHARACTER_CONVERSION */
