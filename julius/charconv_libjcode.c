/**
 * @file   charconv_libjcode.c
 * 
 * <JA>
 * @brief  文字コード変換 (libjcode 使用)
 *
 * 日本語の文字コード(JIS,EUC,SJIS)の相互変換のみ可能である. 
 *
 * </JA>
 * 
 * <EN>
 * @brief  Character set conversion using libjcode
 *
 * Only conversion between Japanese character set (jis, euc-jp, shift-jis)
 * is supported.
 *
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 16:02:41 2005
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

#include "app.h"

#ifdef CHARACTER_CONVERSION
#ifdef USE_LIBJCODE

#include "libjcode/jlib.h"
static int convert_to = SJIS; ///< Conversion target

/** 
 * Setup charset conversion for libjcode.
 * 
 * @param fromcode [in] input charset name (ignored, will be auto-detected)
 * @param tocode [in] output charset name, or NULL when disable conversion
 * @param enable_conv [out] return whether conversion should be enabled or not
 * 
 * @return TRUE on success, FALSE on failure (unknown name).
 */
boolean
charconv_libjcode_setup(char *fromcode, char *tocode, boolean *enable_conv)
{
  if (tocode == NULL) {
    /* disable conversion */
    *enable_conv = FALSE;
  } else {
    if (strmatch(tocode, "sjis")
	|| strmatch(tocode, "sjis-win")
	|| strmatch(tocode, "shift-jis")
	|| strmatch(tocode, "shift_jis")) {
      convert_to = SJIS;
    } else if (strmatch(tocode, "euc-jp")
	       || strmatch(tocode, "euc")
	       || strmatch(tocode, "eucjp")) {
      convert_to = EUC;
    } else if (strmatch(tocode, "jis")) {
      convert_to = JIS;
    } else {
      jlog("Error: charconv_libjcode: character set \"%s\" not supported\n", tocode);
      jlog("Error: charconv_libjcode: only \"sjis\", \"euc-jp\" and \"jis\" can be used with libjcode.\n");
      *enable_conv = FALSE;
      return FALSE;
    }
    *enable_conv = TRUE;
  }
  return TRUE;
}

/** 
 * Apply charset conversion to a string using libjcode.
 * 
 * @param instr [in] source string
 * @param outstr [out] destination buffer
 * @param maxoutlen [in] allocated length of outstr in byte.
 *
 * @return either of instr or outstr, that holds the result string.
 *
 */
char *
charconv_libjcode(char *instr, char *outstr, int maxoutlen)
{
  switch(convert_to) {
  case SJIS:
    toStringSJIS(instr, outstr, maxoutlen);
    break;
  case EUC:
    toStringEUC(instr, outstr, maxoutlen);
    break;
  case JIS:
    toStringJIS(instr, outstr, maxoutlen);
    break;
  }
  return(outstr);
}

#endif /* USE_LIBJCODE */
#endif /* CHARACTER_CONVERSION */
