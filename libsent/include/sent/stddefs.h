/**
 * @file   stddefs.h
 *
 * <EN>
 * @brief  Basic common definitions
 *
 * This is a common header that should be included in all the Julius related
 * sources.  This file contains include list of basic C headers, definition of
 * common static values and function macro, and basic typedefs to handle
 * speech input and words.
 *
 * The unix function macro definition for Win32 environment is also included.
 *
 * Only the important part is documented.  Fof all the definition, see
 * the source.
 * </EN>
 * <JA>
 * @brief  基本の共通定義
 *
 * このファイルはすべてのライブラリで include されるべき共通ヘッダです．
 * 基本的なCヘッダファイルの include, よく使われる定数値と式マクロの定義，
 * すべての関数で共通して用いられる基本的な型の定義が含まれます．
 *
 * また，Win32 モードでのコンパイルのための関数のマクロ定義も含みます．
 * </JA>
 *
 * @sa machines.h
 * 
 * @author Akinobu LEE
 * @date   Sat Feb 12 11:49:37 2005
 *
 * $Revision: 1.9 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __SENT_STANDARD_DEFS__
#define __SENT_STANDARD_DEFS__

/* load site-dependent configuration by configure script */
#if defined(_WIN32) && !defined(__CYGWIN32__) && !defined(__MINGW32__)
#include <config-msvc-libsent.h>
#else
#include <sent/config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#if !defined(_WIN32) || defined(__CYGWIN32__) || defined(__MINGW32__)
/* unixen */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <strings.h>
#include <sys/time.h>
#else
/* win32 */
#include <io.h>
#endif

/* get around sleep() */
#ifdef _WIN32
# if !defined(__CYGWIN32__) && !defined(__MINGW32__)
# define sleep(x) Sleep(x)
# endif
# ifndef HAVE_SLEEP
# define sleep(x) Sleep(x)
# endif
#endif

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>

#include <sent/machines.h>

/// Static PI value
#if !defined(PI)
#define PI 3.14159265358979
#endif
/// Static 2*PI value
#if !defined(TPI)
#define TPI 6.28318530717959 
#endif
/// Static log_e(TPI)
#if !defined(LOGTPI)
#define LOGTPI 1.83787706640935
#endif
#if !defined(LOG_TEN)
/// Static log_e(10)
#define LOG_TEN 2.30258509
/// Static 1 / LOG_TEN
#define INV_LOG_TEN .434294482
#endif

/// Boolean type
typedef unsigned char boolean;
#define TRUE 1
#define FALSE 0

#if defined(_WIN32) && !defined(__CYGWIN32__) && !defined(__MINGW32__)
/* win32 functions */
#define getpagesize() (4096)
#define access _access        
#define chmod _chmod
#define close _close
#define eof _eof
#define filelength _filelength
#define lseek _lseek
#define open _open
#define read _read
#define write _write
#define mkdir _mkdir
#define unlink _unlink
#define getcwd _getcwd
#define getpid _getpid
#define vsnprintf _vsnprintf
#define snprintf _snprintf
#define strdup _strdup
#endif

#ifndef R_OK
#define R_OK 4
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef X_OK
# if defined(_WIN32) && !defined(__CYGWIN32__) && !defined(__MINGW32__)
# define X_OK 0
# else
# define X_OK 1
# endif
#endif
#ifndef F_OK
#define F_OK 0
#endif

/* some macros */
#undef max
#undef min
#define	max(A,B)	((A)>=(B)?(A):(B))
#define	min(A,B)	((A)<(B)?(A):(B))
#define	abs(X)		((X)>0?(X):-(X))
/// String match function, 0 if the given strings did not match
#define strmatch	!strcmp
/// String match function with length limit, 0 if the given strings did not match
#define strnmatch	!strncmp
/// Common text delimiter
#define DELM " \t\n"

/// definition of log(0) used to represent 'no value' in likelihood computation
#define	LOG_ZERO	-1000000
/**
 *  -log_e(-LOG_ZERO) @sa libsent/src/phmm/output.c
 * 
 */
#define LOG_ADDMIN	-13.815510558

/// To specify log output level
enum LogOutputLevel {
  LOG_NORMAL,			///< Normal level
  LOG_VERBOSE,			///< Verbose level
  LOG_DEBUG			///< Debug level
};

/// To specify the direction of N-gram when reading ARPA file
enum {
  DIR_LR,			///< left-to-right (for 2-gram)
  DIR_RL			///< right-to-left (for reverse 3-gram)
};

/// Typedefs to handle speech inputs, parameters and words
/* you can't use double for typedefs below */
/* also look at lib/util/mybmalloc.c */
typedef float PROB;		///< Probability
typedef float LOGPROB;		///< Log probability
typedef short SP16;		///< 16bit speech data
typedef float VECT;		///< Vector element


#ifdef WORDS_INT
/* maximum number of words = 2G = 2^31 (--enable-words-int) */
typedef int WORD_ID;		///< Typedef for word ID
#define MAX_WORD_NUM 2147483647 ///< Maximum size of vocabulary
#define WORD_INVALID 2147483647 ///< Out of word ID to represent no-existence of word
#else
/* maximum number of words = 65535 */
typedef unsigned short WORD_ID; ///< Typedef for word ID
#define MAX_WORD_NUM 65535	///< Maximum size of vocabulary
#define WORD_INVALID 65535	///< Out of word ID to represent no-existence of word
#endif

/// Assumed maximum number of bytes per input line
#define MAXLINELEN 1024

/// Limit of maximum length of a file path
#ifndef MAXPATHLEN
#define MAXPATHLEN 2048
#endif

#include <sent/util.h>

#endif /* __SENT_STANDARD_DEFS__ */
