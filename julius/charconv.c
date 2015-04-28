/**
 * @file   charconv.c
 * 
 * <JA>
 * @brief  文字コード変換
 *
 * 実際には，環境にあわせて iconv, Win32, libjcode のどれかを呼び出す. 
 *
 * </JA>
 * 
 * <EN>
 * @brief  Character set conversion using iconv library
 *
 * The actual functions are defined for iconv, win32 and libjcode, depending
 * on the OS environment.
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

static boolean convert_enabled = FALSE; ///< TRUE if charset converstion is enabled

/** 
 * Setup charset conversion.
 * 
 * @param fromcode [in] input charset name (only libjcode accepts NULL)
 * @param tocode [in] output charset name, or NULL when disable conversion
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
charconv_setup_real(char *fromcode, char *tocode)
{
  boolean enabled;
  boolean ret;

  /* call environment-specific setup function */
#ifdef HAVE_ICONV
  ret = charconv_iconv_setup(fromcode, tocode, &enabled);
#endif
#ifdef USE_LIBJCODE
  ret = charconv_libjcode_setup(fromcode, tocode, &enabled);
#endif
#ifdef USE_WIN32_MULTIBYTE
  ret = charconv_win32_setup(fromcode, tocode, &enabled);
#endif

  /* store whether conversion should be enabled or not to outer variable */
  convert_enabled = enabled;

  /* return the status */
  return(ret);
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
  char *ret;

  /* if diabled return instr itself */
  if (convert_enabled == FALSE) return(instr); /* no conversion */

  /* call environment-specific conversion function */
#ifdef HAVE_ICONV
  ret = charconv_iconv(instr, outstr, maxoutlen);
#endif
#ifdef USE_LIBJCODE
  ret = charconv_libjcode(instr, outstr, maxoutlen);
#endif
#ifdef USE_WIN32_MULTIBYTE
  ret = charconv_win32(instr, outstr, maxoutlen);
#endif

  /* return pointer to the buffer (either instr or outstr) that have the
     resulting string */
  return(ret);
}

#endif /* CHARACTER_CONVERSION */

/************************************************************************/

static char *from_code = NULL;
static char *to_code = NULL;

static boolean
opt_charconv(Jconf *jconf, char *arg[], int argnum)
{
#ifdef CHARACTER_CONVERSION
  if (from_code) free(from_code);
  if (to_code) free(to_code);
  from_code = strcpy((char*)mymalloc(strlen(arg[0])+1), arg[0]);
  to_code = strcpy((char*)mymalloc(strlen(arg[1])+1), arg[1]);
#else
  fprintf(stderr, "Warning: character set conversion disabled, option \"-charconv\" ignored\n");
#endif
  return TRUE;
}
static boolean
opt_nocharconv(Jconf *jconf, char *arg[], int argnum)
{
#ifdef CHARACTER_CONVERSION
  if (from_code) free(from_code);
  if (to_code) free(to_code);
  from_code = NULL;
  to_code = NULL;
#else
  fprintf(stderr, "Warning: character set conversion disabled, option \"-nocharconv\" ignored\n");
#endif
  return TRUE;
}
static boolean
opt_kanji(Jconf *jconf, char *arg[], int argnum)
{
#ifdef CHARACTER_CONVERSION
  if (from_code) free(from_code);
  if (to_code) free(to_code);
  from_code = NULL;
  if (!strcmp(arg[0], "noconv")) {
    to_code = NULL;
  } else {
    to_code = strcpy((char*)mymalloc(strlen(arg[0])+1),arg[0]);
  }
#else
  fprintf(stderr, "Warning: character set conversion disabled, option \"-kanji\" ignored\n");
#endif
  return TRUE;
}

void
charconv_add_option()
{
  j_add_option("-charconv", 2, 2, "convert character set for output", opt_charconv);
  j_add_option("-nocharconv", 0, 0, "disable charconv", opt_nocharconv);
  j_add_option("-kanji", 1, 1, "convert character set for output", opt_kanji);
}

boolean
charconv_setup()
{
#ifdef CHARACTER_CONVERSION
  if (from_code != NULL && to_code != NULL) {
    if (charconv_setup_real(from_code, to_code) == FALSE) {
      fprintf(stderr, "Error: character set conversion setup failed\n");
      return FALSE;
    }
  }
#endif /* CHARACTER_CONVERSION */
  return TRUE;
}
