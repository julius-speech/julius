/**
 * @file   readfile.c
 * 
 * <JA>
 * @brief  様々な入力からテキストを行単位で読み込む関数群
 *
 * 入力ストリームやファイルデスクプリタなど，様々なソースから
 * テキスト入力を行単位で読み込むための関数群です．
 * 読み込み時において，空行は無視されます．また行末の改行は削除されます．
 * </JA>
 * 
 * <EN>
 * @brief  Read strings per line from various input source
 *
 * This file provides functions to read text inputs from various
 * input source such as file pointer, file descpritor, socket descriptor.
 * The input will be read per line, and the newline characters are removed.
 * Also, blank lines will be ignored.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 16:41:58 2005
 *
 * $Revision: 1.7 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/tcpip.h>

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif


/** 
 * Read one line from file that has been opened by fopen_readfile().
 * Blank line will be skipped.
 * 
 * @param buf [out] data buffer
 * @param maxlen [in] maximum length of above
 * @param fp [in] file pointer or gzFile pointer
 * 
 * @return the buffer @a buf, or NULL on EOF or error.
 */
char *
getl(char *buf, int maxlen, FILE *fp)
{
  int newline;

  while(
#ifdef HAVE_ZLIB
	gzgets((gzFile)fp, buf, maxlen) != Z_NULL
#else
	fgets(buf, maxlen, fp) != NULL
#endif
	) {
    newline = strlen(buf)-1;    /* chop newline */
    if (buf[newline] == '\n') {
      buf[newline] = '\0';
      newline--;
    }
    if (newline >= 0 && buf[newline] == '\r') buf[newline] = '\0';
    if (buf[0] == '\0') continue; /* if blank line, read next */
    return buf;
  }
  return NULL;
}

/** 
 * Read one line from file pointer.
 * Blank line will be skipped.
 * 
 * @param buf [out] data buffer
 * @param maxlen [in] maximum length of above
 * @param fp [in] file pointer
 * 
 * @return the buffer @a buf, or NULL on EOF or error.
 */
char *
getl_fp(char *buf, int maxlen, FILE *fp)
{
  int newline;

  while(fgets(buf, maxlen, fp) != NULL) {
    newline = strlen(buf)-1;    /* chop newline */
    if (buf[newline] == '\n') {
      buf[newline] = '\0';
      newline--;
    }
    if (newline >= 0 && buf[newline] == '\r') buf[newline] = '\0';
    if (buf[0] == '\0') continue; /* if blank line, read next */
    return buf;
  }
  return NULL;
}

/* get 1 line input from stdin with prompt */
/* return value: newly allocated buffer */
/* repeat if no input, and */
/* returns NULL on EOF */
/** 
 * Get one file name from stdin with a prompt .  Blank line is omitted.
 * 
 * @param buf [out] buffer to hold input text line
 * @param buflen [in] length of the buffer
 * @param prompt [in] prompt string
 * 
 * @return pointer to @a buf, or NULL on EOF input.
 */
char *
get_line_from_stdin(char *buf, int buflen, char *prompt)
{
  char *p;

  do {
    fprintf(stderr, "%s",prompt);
    if (fgets(buf, buflen, stdin) == NULL) {
      return(NULL);
    }
    /* chop last newline */
    p = buf + strlen(buf) - 1;
    if (p >= buf && *p == '\n') {
      *(p --) = '\0';
    }
    if (p >= buf && *p == '\r') {
      *(p --) = '\0';
    }
    /* chop last space */
    while(p >= buf && *p == ' ') {
      *(p --) = '\0';
    }
  } while (!buf[0]);		/* loop till some input */

  return(buf);
}
