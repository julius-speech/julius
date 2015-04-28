/**
 * @file   gzfile.c
 * 
 * <JA>
 * @brief  zlib あるいは gzip を用いた圧縮ファイルの読み込み
 *
 * 圧縮ファイルの読み込みに対応したファイルのオープン・クローズ・
 * 読み込み関数群です．
 *
 * コンパイル時に zlib 無い場合，gzip を用いて圧縮ファイルの展開を
 * 行います．この場合，複数のファイルを同時に開くことは出来ませんので
 * 注意してください．
 * </JA>
 * 
 * <EN>
 * @brief  Read Compressed files using gzip or zlib
 *
 * These are functions to enable open/close/reading of gzipped files.
 *
 * If zlib library and header are not found, the gzip executables will
 * be used to uncompress the input file.  In the latter case, opening
 * multiple files with these functions is not allowed.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 15:46:00 2005
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
#include <errno.h>

#ifdef HAVE_ZLIB
/* use built-in zlib functions */
/* current implementatin casts gzFile to FILE... */

#include <zlib.h>

/** 
 * Open a file with zlib.
 * 
 * @param filename [in] file name to open
 * 
 * @return gzFile pointer if succeed, NULL on failure.
 */
FILE *
fopen_readfile(char *filename)
{
  gzFile gp;
  gp = gzopen(filename, "rb");
  if (gp == NULL) {
    jlog("Error: gzfile: unable to open %s\n", filename);
  }
  return(gp);
}

/** 
 * Close a file previously opened by fopen_readfile().
 * 
 * @param fp [in] gzFile pointer
 * 
 * @return 0 on success, -1 on error.
 */
int
fclose_readfile(FILE *fp)
{
  if (gzclose((gzFile)fp) < 0) {
    jlog("Error: gzfile: unable to close file\n");
    return -1;
  }
  return 0;
}

/** 
 * Read data from input stream opened by fopen_readfile().
 * 
 * @param ptr [out] data buffer 
 * @param size [in] size of unit in bytes
 * @param n [in] number of unit to be read
 * @param fp [in] gzFile pointer
 * 
 * @return number of read units or EOF, -1 on error.
 */
size_t
myfread(void *ptr, size_t size, size_t n, FILE *fp)
{
  int cnt;
  cnt = gzread((gzFile)fp, (voidp)ptr, (unsigned)size * n);
  if (cnt < 0) {
    jlog("Error: gzfile: failed to read %d bytes\n", size * n);
    return(-1);
  }
  return(cnt / size);
}

/** 
 * Read one character from input stream opened by fopen_readfile().
 * 
 * @param fp [in] gzFile pointer
 * 
 * @return the read character, or -1 on EOF or error.
 */
int
myfgetc(FILE *fp)
{
  int ret;
  ret = gzgetc((gzFile)fp);
  return(ret);
}

/** 
 * Test if reached end of file, for files opened by fopen_readfile().
 * 
 * @param fp [in] gzFile pointer.
 * 
 * @return 1 if already on EOF, 0 if otherwise.
 */
int
myfeof(FILE *fp)
{
  if (gzeof((gzFile)fp) == 0) {
    return 0;
  }
  return 1;
}

/** 
 * Seek to the first of the file.
 * 
 * @param fp [in] gzFile pointer.
 * 
 * @return 0 on success, -1 on error.
 */
int
myfrewind(FILE *fp)
{
  if (gzrewind((gzFile)fp) != 0) return -1;
  return 0;
}

#else  /* ~HAVE_ZLIB */

/* use external "gzip" via pipe */
/* can read only one file at a time */

static boolean isopen = FALSE;	///< TRUE if a file is now opened
static FILE *zcat_pipe = NULL; ///< File pointer of the gzip pipe 

/** 
 * Guess if the file is compressed or not, only by its filename suffix.
 * 
 * @param filename [in] file name
 * 
 * @return TRUE if compressed file, FALSE if not.
 */
static boolean
is_zcatfile(char *filename)
{
  int len;

  len = strlen(filename);
  if (strmatch(".Z", &filename[len - 2])) {
    return TRUE;
  } else if (strmatch(".z", &filename[len - 2]) || strmatch(".gz", &filename[len - 3])) {
    return TRUE;
  }
  return FALSE;
}

/** 
 * Open the file using external gzip command.  Only one file can open at a time.
 * 
 * @param filename [in] filename to open
 * 
 * @return the file pointer, or NULL if open failed.
 */
FILE *
fopen_readfile(char *filename)
{
  FILE *fp;
  char *cmd;

  if (isopen) {		/* already open */
    jlog("Error: gzfile: previously opened file is not closed yet.\n");
    return NULL;
  }
  if (is_zcatfile(filename)) {	/* open compressed file */
    cmd = (char *)mymalloc(strlen(ZCAT) + strlen(filename) + 2);
    strcpy(cmd, ZCAT);
    strcat(cmd," ");
    strcat(cmd, filename);
    zcat_pipe = popen(cmd, "r");
    if (zcat_pipe == NULL) {
      jlog("Error: gzfile: failed to exec \"%s\" for opening file \"%s\"\n", cmd, filename);
      return NULL;
    }
    free(cmd);
    fp = zcat_pipe;
  } else {			/* open normal file */
    fp = fopen(filename, "rb");
    if (fp == NULL) {		/* error */
      jlog("Error: gzfile: failed to open \"%s\"\n", filename);
      return NULL;
    }
    zcat_pipe = NULL;
  }
  
  isopen = TRUE;
  return (fp);
}

/** 
 * Close a file previously opened by fopen_readfile().
 * 
 * @param fp [in] gzFile pointer
 * 
 * @return 0 on success, -1 on error.
 */
int
fclose_readfile(FILE *fp)
{
  if (!isopen) {		/* not opened yet */
    return -1;
  }
  
  if (zcat_pipe != NULL) {	/* pipe opened fp */
    if (fp != zcat_pipe) {
      jlog("Error: gzfile: fp is not opened by fopen_readfile()\n");
      return -1;
    }
    if (pclose(zcat_pipe) == -1) {
      jlog("Error: gzfile: failed to close gzip pipe\n");
      return -1;
    }
    zcat_pipe = NULL;
  } else  {			/* normal opened fp */
    if (fclose(fp) != 0) {
      jlog("Error: gzfile: failed to close file\n");
      return -1;
    }
  }
  
  isopen = FALSE;
  return 0;
}

/** 
 * Read data from input stream opened by fopen_readfile().
 * 
 * @param ptr [out] data buffer 
 * @param size [in] size of unit in bytes
 * @param n [in] number of unit to be read
 * @param fp [in] gzFile pointer
 * 
 * @return number of read units or EOF, -1 on error.
 */
size_t
myfread(void *ptr, size_t size, size_t n, FILE *fp)
{
  size_t ret;
  ret = fread(ptr, size, n, fp);
  if (ret == 0) {
    if (myfeof(fp) == 1) {
      return 0;
    } else {
      return -1;
    }
  }
  return(ret);
}

/** 
 * Read one character from input stream opened by fopen_readfile().
 * 
 * @param fp [in] gzFile pointer
 * 
 * @return the read character, or -1 on EOF or error.
 */
int
myfgetc(FILE *fp)
{
  int ret;
  ret = fgetc(fp);
  if (ret == EOF) return -1;
  return(ret);
}

/** 
 * Test if reached end of file, for files opened by fopen_readfile().
 * 
 * @param fp [in] gzFile pointer.
 * 
 * @return 1 if already on EOF, 0 if otherwise.
 */
int
myfeof(FILE *fp)
{
  if (feof(fp) == 0) {
    return 0;
  }
  return 1;
}

/** 
 * Seek to the first of the file.
 * 
 * @param fp [in] File pointer.
 * 
 * @return 0 on success, -1 on error.
 */
int
myfrewind(FILE *fp)
{
  if (fseek(fp, 0L, SEEK_SET) != 0) return -1;
  return 0;
}

#endif /* ~HAVE_ZLIB */

/** 
 * Open or create a file for writing (no compression supported),
 * 
 * @param filename [in] filename
 * 
 * @return the file pointer or NULL on failure.
 */
FILE *
fopen_writefile(char *filename)
{
  FILE *fp;

  fp = fopen(filename, "wb");
  if (fp == NULL) {		/* error */
    jlog("Error: gzfile: failed to open \"%s\" for writing\n", filename);
  }
  return (fp);
}

/** 
 * Close file previously opened by open_writefile().
 * 
 * @param fp [in] file pointer
 * 
 * @return 0 on success, -1 on failure.
 */
int				/* return value: 0=success, -1=failure */
fclose_writefile(FILE *fp)
{
  if (fclose(fp) != 0) {
    return -1;
  }
  return 0;
}

/** 
 * Write data.
 * 
 * @param ptr [in] data buffer
 * @param size [in] size of unit in bytes
 * @param n [in] number of unit to write
 * @param fp [in] file pointer
 * 
 * @return the number of units successfully written, or 0 if EOF or failed.
 */
size_t
myfwrite(void *ptr, size_t size, size_t n, FILE *fp)
{
  return(fwrite(ptr, size, n, fp));
}
