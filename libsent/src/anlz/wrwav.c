/**
 * @file   wrwav.c
 *
 * <JA>
 * @brief  音声波形データを WAV ファイルに保存する
 * </JA>
 * <EN>
 * @brief  Write waveform data to WAV file
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Tue Feb 15 01:02:18 2005
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
#include <sent/speech.h>

/// Total number of samples written to the file.
static int totallen;

/** 
 * Write speech data in little endian.
 * 
 * @param buf [in] speech data
 * @param unitbyte [in] unit size in bytes
 * @param unitnum [in] number of units to be written
 * @param fp [in] file pointer
 * 
 * @return TRUE on success (specified number of data has been written), FALSE on failure.
 */
static boolean
mywrite(void *buf, size_t unitbyte, int unitnum, FILE *fp)
{
  int tmp;
#ifdef WORDS_BIGENDIAN
  if (unitbyte > 1) swap_bytes(buf, unitbyte, unitnum);
#endif
  if ((tmp = myfwrite(buf, unitbyte, unitnum, fp)) < unitnum) {
    return(FALSE);
  }
#ifdef WORDS_BIGENDIAN
  if (unitbyte > 1) swap_bytes(buf, unitbyte, unitnum);
#endif
  return(TRUE);
}

/// Function macro to mywrite() with error handling.
#define MYWRITE(A,B,C,D)  if (!mywrite(A, B, C, D)) {jlog("Error: wrwav: failed to write wave data\n"); fclose_writefile(fp); return NULL;}

/* open/create a WAVE file for writing, and write header */
/* return file pointer, NULL on failure */
/** 
 * @brief Open/create a WAVE file and write header.
 *
 * Open or creat a new WAV file and prepare for later data writing.
 * The frame length written here is dummy, and will be overwritten when
 * closed by wrwav_close().
 * 
 * @param filename [in] file name
 * @param sfreq [in] sampling frequency of the data you are going to write
 * 
 * @return the file pointer.
 */
FILE *
wrwav_open(char *filename, int sfreq)
{
  FILE *fp;
  unsigned int i;
  unsigned short s;

  /* open file for writing */
  if ((fp = fopen_writefile(filename)) == NULL) return NULL;

  /* write header */
  /* first 4 byte: 'R' 'I' 'F' 'F' */
  MYWRITE("RIFF", 1, 4, fp);
  /* 4 byte: byte num of rest: dummy for now */
  i = 0; MYWRITE(&i, 4, 1, fp);

  /* first part: WAVE format specifications */
  /* 8 byte: 'W' 'A' 'V' 'E' 'f' 'm' 't' ' ' */
  MYWRITE("WAVEfmt ", 1, 8, fp);
  /* 4byte: byte size of the next part (16 bytes here) */
  i = 16; MYWRITE(&i, 4, 1, fp);
  /* 2byte: data format */
  s = 1; MYWRITE(&s, 2, 1, fp);	/* PCM */
  /* 2byte: channel num */
  s = 1; MYWRITE(&s, 2, 1, fp);	/* mono */
  /* 4byte: sampling rate */
  i = sfreq; MYWRITE(&i, 4, 1, fp);
  /* 4byte: bytes per second */
  i = sfreq * sizeof(SP16); MYWRITE(&i, 4, 1, fp);
  /* 2bytes: bytes per frame ( = (bytes per sample) x channel ) */
  s = sizeof(SP16); MYWRITE(&s, 2, 1, fp);
  /* 2bytes: bits per sample */
  s = sizeof(SP16) * 8; MYWRITE(&s, 2, 1, fp);
  
  /* data part header */
  MYWRITE("data", 1, 4, fp);
  /* data length: dummy for now */
  i = 0; MYWRITE(&i, 4, 1, fp);

  totallen = 0;			/* reset total length */

  return(fp);
}

/** 
 * Write speech samples.
 * 
 * @param fp [in] file descriptor
 * @param buf [in] speech data to be written
 * @param len [in] length of above
 * 
 * @return actual number of written samples.
 */
boolean
wrwav_data(FILE *fp, SP16 *buf, int len)
{
  boolean ret;
  ret = mywrite(buf, sizeof(SP16), len, fp);
  if (ret) totallen += len;
  return(ret);
}

/* close file */
/** 
 * @brief  Close the file.
 *
 * The frame length in the header part is overwritten by the actual value
 * before file close.
 * 
 * @param fp [in] file pointer to close, previously opened by wrwav_open().
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
wrwav_close(FILE *fp)
{
  unsigned int i;
  
  /* overwrite data length after recording */
  /* at 5-8(len+36), 41-44 (len) */
  if (fseek(fp, 40, SEEK_SET) != 0) { /* error */
    jlog("Error: wrwav: failed to seek for header\n");
    return(FALSE);
  }
  i = totallen * sizeof(SP16);
  if (!mywrite(&i, 4, 1, fp)) {
    jlog("Error: wrwav: failed to re-write header\n");
    return(FALSE);
  }
  if (fseek(fp, 4, SEEK_SET) != 0) { /* error */
    jlog("Error: wrwav: failed to seek for header\n");
    return(FALSE);
  }
  i = totallen * sizeof(SP16) + 36;
  if (!mywrite(&i, 4, 1, fp)) {
    jlog("Error: wrwav: failed to re-write header\n");
    return(FALSE);
  }
  
  /* close file */
  fclose_writefile(fp);

  return(TRUE);
}
