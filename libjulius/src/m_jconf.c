/**
 * @file   m_jconf.c
 * 
 * <JA>
 * @brief  設定ファイルの読み込み. 
 *
 * オプション指定を記述した jconf 設定ファイルを読み込みます. 
 * jconf 設定ファイル内では，ダブルクォーテーションによる文字列の
 * 指定，バックスラッシュによる文字のエスケープができます. 
 * また，各行において '#' 以降はスキップされます. 
 *
 * jconf 設定ファイル内では，全ての相対パスは，アプリケーションの
 * カレントディレクトリではなく，その jconf の存在するディレクトリからの
 * 相対パスとして解釈されます. 
 *
 * また，$HOME, ${HOME}, $(HOME), の形で指定された部分について
 * 環境変数を展開できます. 
 * 
 * </JA>
 * 
 * <EN>
 * @brief  Read a configuration file.
 *
 * These functions are for reading jconf configuration file and set the
 * parameters into jconf structure.  String bracing by double quotation,
 * and escaping character with backslash are supproted.
 * Characters after '#' at each line will be ignored.
 *
 * Note that all relative paths in jconf file are treated as relative
 * to the jconf file, not the run-time current directory.
 *
 * You can expand environment variables in a format of $HOME, ${HOME} or
 * $(HOME) in jconf file.
 *
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Thu May 12 14:16:18 2005
 *
 * $Revision: 1.11 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius/julius.h>

#if defined(_WIN32) && !defined(__CYGWIN32__)
#include <mbstring.h>
#endif

#define ISTOKEN(A) (A == ' ' || A == '\t' || A == '\n') ///< Determine token characters
#define BUFLEN 512

/** 
 * <JA>
 * @brief  jconf 用の行読み込みルーチン
 *
 * バックスラッシュによるエスケープ処理，および Mac/Win の改行コードに
 * 対応する. 空行はスキップされ，改行コードは消される. 
 * 
 * @param buf [out] 読み込んだ1行分のテキストを格納するバッファ
 * @param size [in] @a buf の大きさ（バイト数）
 * @param fp [in] ファイルポインタ
 * 
 * @return @a buf を返す. EOF でこれ以上入力がなければ NULL を返す. 
 * </JA>
 * <EN>
 * @brief  line reading function for jconf file.
 *
 * This function has capability of character escaping and newline codes
 * on Win/Mac.  Blank line will be skipped and newline characters will be
 * stripped.
 * 
 * @param buf [out] buffer to store the read text per line
 * @param size [in] size of @a buf in bytes
 * @param fp [in] file pointer
 * 
 * @return @a buf on success, or NULL when encountered EOF and no further input.
 * </EN>
 */
/* added by H.Banno for Windows & Mac */
static char *
fgets_jconf(char *buf, int size, FILE *fp)
{
  int c, prev_c;
  int pos;

  if (fp == NULL) return NULL;
    
  pos = 0;
  c = '\0';
  prev_c = '\0';
  while (1) {
    if (pos >= size) {
      pos--;
      break;
    }

    c = fgetc(fp);
    if (c == EOF) {
      buf[pos] = '\0';
      if (pos <= 0) {
	return NULL;
      } else {
	return buf;
      }
    } else if (c == '\n' || c == '\r') {
      if (c == '\r' && (c = fgetc(fp)) != '\n') { /* for Mac */
	ungetc(c, fp);
      }
      if (prev_c == '\\') {
	pos--;
      } else {
	break;
      }
    } else {
      buf[pos] = c;
      pos++;

#if defined(_WIN32) && !defined(__CYGWIN32__)
      if (c == '\\' && (_ismbblead(prev_c) && _ismbbtrail(c))) {
      c = '\0';
      }
#endif
    }
    prev_c = c;
  }
  buf[pos] = '\0';

  return buf;
}

/** 
 * <JA>
 * @brief  ファイルのパス名からディレクトリ名を抜き出す. 
 *
 * 最後の '/' は残される. 
 * 
 * @param path [i/o] ファイルのパス名（関数内で変更される）
 * </JA>
 * <EN>
 * @brief  Get directory name from a path name of a file.
 *
 * The trailing slash will be left, and the given buffer will be modified.
 * 
 * @param path [i/o] file path name, will be modified to directory name
 * </EN>
 */
void
get_dirname(char *path)
{
  char *p;
  /* /path/file -> /path/ */
  /* path/file  -> path/  */
  /* /file      -> / */
  /* file       ->  */
  /* ../file    -> ../ */
  p = path + strlen(path) - 1;
  while (*p != '/'
#if defined(_WIN32) && !defined(__CYGWIN32__)
	 && *p != '\\'
#endif
	 && p != path) p--;
  if (p == path && *p != '/') *p = '\0';
  else *(p+1) = '\0';
}

/** 
 * <JA>
 * @brief  環境変数の展開
 * 
 * 環境変数を展開する. $HOME の形の文字列を環境変数とみなし，その値で
 * 置換する. 置換が起こった際には，与えられた文字列バッファを内部で
 * 解放し，あらたに割り付けられたバッファを返す. 
 *
 * 変数の指定は $HOME, ${HOME}, $(HOME), の形で指定できる. 
 * $ を展開したくない場合はバックスラッシュ "\" でエスケープできる. 
 * またシングルクォート "'" で括られた範囲は展開を行わない. 
 * 
 * @param str [in] 対象文字列（展開発生時は内部で free されるので注意）
 * 
 * @return 展開すべき対象がなかった場合，str がそのまま返される. 展開が行われた場合，あらたに割り付けられた展開後の文字列を含むバッファが返される. 
 * </JA>
 * <EN>
 * @brief  Envronment valuable expansion for a string
 *
 * This function expands environment valuable in a string.  When an
 * expantion occurs, the given buffer will be released inside this
 * function and newly allocated buffer that holds the resulting string
 * will be returned.
 *
 * Environment valuables should be in a form of $HOME, ${HOME} or $(HOME).
 * '$' can be escaped by back slash, and strings enbraced by single quote
 * will be treated as is (no expansion).
 * 
 * @param str [in] target string
 * 
 * @return the str itself when no expansion performed, or newly
 * allocated buffer if expansion occurs.
 * </EN>
 */
static char *
expand_env(char *str)
{
  char *p, *q;
  char *bgn;
  char eb;
  char *target;
  char *envval;
  int target_malloclen;
  int len, n;
  boolean inbrace;
  char env[256];

  /* check if string contains '$' and return immediately if not */
  /* '$' = 36, '\'' = 39 */
  p = str;
  inbrace = FALSE;
  while (*p != '\0') {
    if (*p == 39) {
      if (inbrace == FALSE) {
	inbrace = TRUE;
      } else {
	inbrace = FALSE;
      }
      p++;
      continue;
    }
    if (! inbrace) {
      if (*p == '\\') {
	p++;
	if (*p == '\0') break;
      } else {
	if (*p == 36) break;
      }
    }
    p++;
  }
  if (*p == '\0') return str;

  /* prepare result buffer */
  target_malloclen = strlen(str) * 2;
  target = (char *)mymalloc(target_malloclen);

  p = str;
  q = target;

  /* parsing */
  inbrace = FALSE;
  while (*p != '\0') {

    /* look for next '$' */
    while (*p != '\0') {
      if (*p == 39) {
	if (inbrace == FALSE) {
	  inbrace = TRUE;
	} else {
	  inbrace = FALSE;
	}
	p++;
	continue;
      }
      if (! inbrace) {
	if (*p == '\\') {
	  p++;
	  if (*p == '\0') break;
	} else {
	  if (*p == 36) break;
	}
      }
      *q = *p;
      p++;
      q++;
      n = q - target;
      if (n >= target_malloclen) {
	target_malloclen *= 2;
	target = myrealloc(target, target_malloclen);
	q = target + n;
      }
    }
    if (*p == '\0') {		/* reached end of string */
      *q = '\0';
      break;
    }

    /* move to next */
    p++;

    /* check for brace */
    eb = 0;
    if (*p == '(') {
      eb = ')';
    } else if (*p == '{') {
      eb = '}';
    }

    /* proceed to find env end point and set the env string to env[] */
    if (eb != 0) {
      p++;
      bgn = p;
      while (*p != '\0' && *p != eb) p++;
      if (*p == '\0') {
	jlog("ERROR: failed to expand variable: no end brace: \"%s\"\n", str);
	free(target);
	return str;
      }
    } else {
      bgn = p;
      while (*p == '_'
	     || (*p >= '0' && *p <= '9')
	     || (*p >= 'a' && *p <= 'z')
	     || (*p >= 'A' && *p <= 'Z')) p++;
    }
    len = p - bgn;
    if (len >= 256 - 1) {
      jlog("ERROR: failed to expand variable: too long env name: \"%s\"\n", str);
      free(target);
      return str;
    }
    strncpy(env, bgn, len);
    env[len] = '\0';

    /* get value */
    if ((envval = getenv(env)) == NULL) {
      jlog("ERROR: failed to expand variable: no such variable \"%s\"\n", env);
      free(target);
      return str;
    }

    if (debug2_flag) {		/* for debug */
      jlog("DEBUG: expand $%s to %s\n", env, envval);
    }

    /* paste value to target */
    while(*envval != '\0') {
      *q = *envval;
      q++;
      envval++;
      n = q - target;
      if (n >= target_malloclen) {
	target_malloclen *= 2;
	target = myrealloc(target, target_malloclen);
	q = target + n;
      }
    }

    /* go on to next */
    if (eb != 0) p++;
  }

  free(str);
  return target;
}

/* read-in and parse jconf file and process those using m_options */
/** 
 * <JA>
 * @brief  オプション文字列を分解して追加格納する.
 *
 * @param buf [in] 文字列
 * @param argv [i/o] オプション列へのポインタ
 * @param argc [i/o] オプション列の数へのポインタ
 * @param maxnum [i/o] オプション列の割付最大数
 * </JA>
 * <EN>
 * @brief  Divide option string into option arguments and append to array.
 *
 * @param buf [in] option string
 * @param argv [i/o] pointer to option array
 * @param argc [i/o] pointer to the length of option array
 * @param maxnum [i/o] pointer to the allocated length of option array
 * </EN>
 */
static void
add_to_arglist(char *buf, char ***argv_ret, int *argc_ret, int *maxnum_ret)
{
  char *p = buf;
  char cpy[BUFLEN];
  char *dst, *dst_from;
  char **argv = *argv_ret;
  int argc = *argc_ret;
  int maxnum = *maxnum_ret;

  dst = cpy;
  while (1) {
    while (*p != '\0' && ISTOKEN(*p)) p++;
    if (*p == '\0') break;
      
    dst_from = dst;
      
    while (*p != '\0' && (!ISTOKEN(*p))) {
#if !defined(_WIN32)
      if (*p == '\\') {     /* escape by '\' */
	if (*(++p) == '\0') break;
	*(dst++) = *(p++);
      } else {
#endif
	if (*p == '"') { /* quote by "" */
	  p++;
	  while (*p != '\0' && *p != '"') *(dst++) = *(p++);
	  if (*p == '\0') break;
	  p++;
	} else if (*p == '\'') { /* quote by '' */
	  p++;
	  while (*p != '\0' && *p != '\'') *(dst++) = *(p++);
	  if (*p == '\0') break;
	  p++;
	} else if (*p == '#') { /* comment out by '#' */
	  *p = '\0';
	  break;
	} else {		/* other */
	  *(dst++) = *(p++);
	}
#if !defined(_WIN32)
      }
#endif
    }
    if (dst != dst_from) {
      *dst = '\0'; dst++;
      if ( argc >= maxnum) {
	maxnum += 20;
	argv = (char **)myrealloc(argv, sizeof(char *) * maxnum);
      }
      argv[argc++] = strcpy((char*)mymalloc(strlen(dst_from)+1), dst_from);
    }
  }
  *argv_ret = argv;
  *argc_ret = argc;
  *maxnum_ret = maxnum;
}

/** 
 * <JA>
 * オプション指定を含む文字列を解析して値をセットする.
 * 相対パス名はカレントからの相対として扱われる.
 * 
 * @param str [in] オプション指定を含む文字列
 * @param jconf [out] 値をセットする jconf 設定データ
 * </JA>
 * <EN>
 * Parse a string and set the specified option values.
 * Relative paths will be treated as relative to current directory.
 * 
 * @param str [in] string which contains options
 * @param jconf [out] global configuration data to be written.
 * </EN>
 *
 * @callgraph
 * @callergraph
 */
boolean
config_string_parse(char *str, Jconf *jconf)
{
  int c_argc;
  char **c_argv;
  int maxnum;
  char buf[BUFLEN];
  char *cdir;
  int i;
  boolean ret;

  jlog("STAT: parsing option string: \"%s\"\n", str);
  
  /* set the content of jconf file into argument list c_argv[1..c_argc-1] */
  maxnum = 20;
  c_argv = (char **)mymalloc(sizeof(char *) * maxnum);
  c_argv[0] = strcpy((char *)mymalloc(7), "string");
  c_argc = 1;
  add_to_arglist(str, &c_argv, &c_argc, &maxnum);
  /* env expansion */
  for (i=1;i<c_argc;i++) {
    c_argv[i] = expand_env(c_argv[i]);
  }
  /* now that options are in c_argv[][], call opt_parse() to process them */
  /* relative paths in string are relative to current */
  ret = opt_parse(c_argc, c_argv, NULL, jconf);

  /* free arguments */
  while (c_argc-- > 0) {
    free(c_argv[c_argc]);
  }
  free(c_argv);

  return(ret);
}

/** 
 * <JA>
 * jconf 設定ファイルを読み込んで解析し，対応するオプションを設定する.
 * オプション内の相対パスは、その jconf 設定ファイルからの相対となる.
 * 
 * @param conffile [in] jconf ファイルのパス名
 * @param jconf [out] 値をセットする jconf 設定データ
 * </JA>
 * <EN>
 * Read and parse a jconf file, and set the specified option values.
 * Relative paths in the file will be treated as relative to the file,
 * not the application current.
 * 
 * @param conffile [in] jconf file path name
 * @param jconf [out] global configuration data to be written.
 * </EN>
 *
 * @callgraph
 * @callergraph
 */
boolean
config_file_parse(char *conffile, Jconf *jconf)
{
  int c_argc;
  char **c_argv;
  FILE *fp;
  int maxnum;
  char buf[BUFLEN];
  char *cdir;
  int i;
  boolean ret;

  jlog("STAT: include config: %s\n", conffile);
  
  /* set the content of jconf file into argument list c_argv[1..c_argc-1] */
  /* c_argv[0] will be the original conffile name */
  /* inside jconf file, quoting by ", ' and escape by '\' is supported */
  if ((fp = fopen(conffile, "r")) == NULL) {
    jlog("ERROR: m_jconf: failed to open jconf file: %s\n", conffile);
    return FALSE;
  }
  maxnum = 20;
  c_argv = (char **)mymalloc(sizeof(char *) * maxnum);
  c_argv[0] = strcpy((char *)mymalloc(strlen(conffile)+1), conffile);
  c_argc = 1;
  while (fgets_jconf(buf, BUFLEN, fp) != NULL) {
    if (buf[0] == '\0') continue;
    add_to_arglist(buf, &c_argv, &c_argc, &maxnum);
  }
  if (fclose(fp) == -1) {
    jlog("ERROR: m_jconf: cannot close jconf file\n");
    return FALSE;
  }

  /* env expansion */
  for (i=1;i<c_argc;i++) {
    c_argv[i] = expand_env(c_argv[i]);
  }

  if (debug2_flag) {		/* for debug */
    jlog("DEBUG: args:");
    for (i=1;i<c_argc;i++) jlog(" %s",c_argv[i]);
    jlog("\n");
  }

  /* now that options are in c_argv[][], call opt_parse() to process them */
  /* relative paths in jconf file are relative to the jconf file (not current) */
  cdir = strcpy((char *)mymalloc(strlen(conffile)+1), conffile);
  get_dirname(cdir);
  ret = opt_parse(c_argc, c_argv, (cdir[0] == '\0') ? NULL : cdir, jconf);
  free(cdir);

  /* free arguments */
  while (c_argc-- > 0) {
    free(c_argv[c_argc]);
  }
  free(c_argv);

  return(ret);
}

/* end of file */
