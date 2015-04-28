/**
 * @file   adin_file.c
 *
 * <JA>
 * @brief  ファイル入力：WAV/RAWファイルおよび標準入力からの読み込み
 *
 * 音声ファイルからの入力を行なう関数です．サポートしているファイル形式は
 * Microsoft WAVE形式の音声ファイル，およびヘッダ無し（RAW）ファイルです．
 * オーディオ形式は，無圧縮PCM，16bit，モノラルに限られています．
 * RAWファイルの場合，データのバイトオーダーが big endian であることを
 * 前提としていますので，注意してください．
 * 
 * ファイルのサンプリングレートはシステムの要求するサンプリングレート
 * （adin_standby() で指定される値）と一致する必要があります．
 * WAVE形式のファイルの場合は，
 * ファイルのサンプリングレートがこの指定値と一致しなければエラーとなります．
 * RAWファイル入力の場合は，ファイルにヘッダ情報が無く録音時の
 * サンプリングレートが不明なため，チェック無しでファイルの
 * サンプリングレートが adin_standby() で指定された値である
 * と仮定して処理されます．
 *
 * 入力ファイル名は，標準入力から読み込まれます．
 * ファイル名を列挙したファイルリストファイルが指定された場合，
 * そのファイルから入力ファイル名が順次読み込まれます．
 *
 * libsndfile を使用する場合，adin_sndfile.c 内の関数が使用されます．
 * この場合，このファイルの関数は使用されません．
 *
 * このファイル内では int を 4byte, short を 2byte と仮定しています．
 * </JA>
 * <EN>
 * @brief  Audio input from file or stdin
 *
 * Functions to get input from wave file or standard input.
 * Two file formats are supported, Microsoft WAVE format and RAW (no header)
 * format.  The audio format should be uncompressed PCM, 16bit, monoral.
 * On RAW file input, the data byte order must be in big endian.
 *
 * The sampling rate of input file must be equal to the system requirement
 * value which is specified by adin_standby() . For WAVE format file,
 * the sampling rate of the input file described in its header is checked
 * against the system value, and rejected if not matched.  But for RAW format
 * file, no check will be applied since it has no header information about
 * the recording sampling rate, so be careful of the sampling rate setting.
 *
 * When file input mode, the file name will be read from standard input.
 * If a filelist file is specified, the file names are read from the file
 * sequencially instead.
 *
 * When compiled with libsndfile support, the functions in adin_sndfile.c
 * is used for file input instead of functions below.
 *
 * In this file, assume sizeof(int)=4, sizeof(short)=2
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Sun Feb 13 13:31:20 2005
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


#include <sent/stddefs.h>
#include <sent/speech.h>
#include <sent/adin.h>

static FILE *gfp;		///< File pointer of current input file
static boolean wav_p;		///< TRUE if input is WAVE file, FALSE if RAW file
static int maxlen;		///< Number of samples, described in the header of WAVE file
static int nowlen;		///< Current number of read samples
/**
 * When file input, the first 4 bytes of the file are read at first to
 * identify whether it is WAVE file format.  This work area is used to
 * keep back the 4 bytes if the input is actually a RAW format.
 */
static SP16 pre_data[2];
static boolean has_pre;		///< TRUE if pre_data is available

static unsigned int sfreq;	///< Sampling frequency in Hz, specified by adin_standby()

static char speechfilename[MAXPATHLEN];	///< Buffer to hold input file name
static char *stdin_buf = NULL;

/* read .wav data with endian conversion */
/* (all .wav datas are in little endian) */

/** 
 * Read a header value from WAVE file with endian conversion.
 * If required number of data has not been read, it produces error.
 * 
 * @param buf [out] Pointer to store read data
 * @param unitbyte [in] Unit size
 * @param unitnum [in] Required number of units to read
 * @param fp [in] File pointer
 * 
 * @return TRUE on success, FALSE if required number of data has not been read.
 */
static boolean
myread(void *buf, size_t unitbyte, int unitnum, FILE *fp)
{
  int tmp;
  if ((tmp = fread(buf, unitbyte, unitnum, fp)) < unitnum) {
    return(FALSE);
  }
#ifdef WORDS_BIGENDIAN
  swap_bytes(buf, unitbyte, unitnum);
#endif
  return(TRUE);
}
/// Abbreviation for header reading
#define MYREAD(A,B,C,D)  if (!myread(A, B, C, D)) {jlog("Error: adin_file: file is corrupted\n"); return -1;}

/** 
 * @brief  Parse header part of a WAVE file to prepare for data reading
 *
 * The audio format will be checked here, and data length is also read from
 * the header.  Then the pointer is moved to the start point of data part.
 *
 * When called, the file pointer should be located just after 
 * the first 4 bytes, "RIFF".  It also sets @a maxlen and @a nowlen .
 * 
 * @param fp [in] File pointer
 * 
 * @return TRUE if check successfully passed, FALSE if an error occured.
 */
static boolean
setup_wav(FILE *fp)
{
  char dummy[9];
  unsigned int i, len;
  unsigned short s;

  /* 4 byte: byte num of rest ( = filesize - 8) */
  /* --- just skip them */
  MYREAD(dummy, 1, 4, fp);
  /* first part: WAVE format specifications */
  /* 4 byte: "WAVE" */
  MYREAD(dummy, 1, 4, fp);
  if (dummy[0] != 'W' ||
      dummy[1] != 'A' ||
      dummy[2] != 'V' ||
      dummy[3] != 'E') {
    jlog("Error: adin_file: WAVE header not found, file corrupted?\n");
    return FALSE;
  }
  /* format chunk: "fmt " */
  MYREAD(dummy, 1, 4, fp);
  if (dummy[0] != 'f' ||
      dummy[1] != 'm' ||
      dummy[2] != 't' ||
      dummy[3] != ' ') {
    jlog("Error: adin_file: fmt chunk not found, file corrupted?\n");
    return FALSE;
  }
  /* 4byte: byte size of this part */
  MYREAD(&len, 4, 1, fp);

  /* 2byte: data format */
  MYREAD(&s, 2, 1, fp);
  if (s != 1) {
    jlog("Error: adin_file: data format != PCM (id=%d)\n", s);
    return FALSE;
  }
  /* 2byte: channel num */
  MYREAD(&s, 2, 1, fp);
  if (s >= 2) {
    jlog("Error: adin_file: channel num != 1 (%d)\n", s);
    return FALSE;
  }
  /* 4byte: sampling rate */
  MYREAD(&i, 4, 1, fp);
  if (i != sfreq) {
    jlog("Error: adin_file: sampling rate != %d (%d)\n", sfreq, i);
    return FALSE;
  }
  /* 4byte: bytes per second */
  MYREAD(&i, 4, 1, fp);
  if (i != sfreq * sizeof(SP16)) {
    jlog("Error: adin_file: bytes per second != %d (%d)\n", sfreq * sizeof(SP16), i);
    return FALSE;
  }
  /* 2bytes: bytes per frame ( = (bytes per sample) x channel ) */
  MYREAD(&s, 2, 1, fp);
  if (s != 2) {
    jlog("Error: adin_file: (bytes per sample) x channel != 2 (%d)\n", s);
    return FALSE;
  }
  /* 2bytes: bits per sample */
  MYREAD(&s, 2, 1, fp);
  if (s != 16) {
    jlog("Error: adin_file: bits per sample != 16 (%d)\n", s);
    return FALSE;
  }
  /* skip rest */
  if (len > 16) {
    len -= 16;
    while (len > 0) {
      if (len > 8) {
	MYREAD(dummy, 1, 8, fp);
	len -= 8;
      } else {
	MYREAD(dummy, 1, len, fp);
	len = 0;
      }
    }
  }
  /* end of fmt part */

  /* seek for 'data' part */
  while (myread(dummy, 1, 4, fp)) {
    MYREAD(&len, 4, 1, fp);
    if (dummy[0] == 'd' &&
	dummy[1] == 'a' &&
	dummy[2] == 't' &&
	dummy[3] == 'a') {
      break;
    }
    for (i=0;i<len;i++) myread(dummy, 1, 1, fp);
  }
  /* ready to read in "data" part --- this is speech data */
  maxlen = len / sizeof(SP16);
  nowlen = 0;
  return TRUE;
}

/** 
 * @brief  Open input file
 *
 * Open the file, check the file format, and set the file pointer to @a gfp .
 * 
 * @param filename [in] file name, or NULL for standard input
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
adin_file_open(char *filename)	/* NULL for standard input */
{
  FILE *fp;
  char dummy[4];

  if (filename != NULL) {
    if ((fp = fopen(filename, "rb")) == NULL) {
      jlog("Error: adin_file: failed to open %s\n",filename);
      return(FALSE);
    }
  } else {
    fp = stdin;
#if defined(_WIN32) && !defined(__CYGWIN32__) && !defined(__MINGW32__)
    if (_setmode( _fileno( stdin ), _O_BINARY ) == -1) {
      jlog("Error: adin_file: _setmode() failed\n");
    }
#endif
    if (stdin_buf == NULL) {
      stdin_buf = (char *)mymalloc(BUFSIZ);
      setvbuf(stdin, stdin_buf, _IOFBF, BUFSIZ);
    }
  }
    
  /* check first 4 byte to detect Microsoft WAVE format */
  if (fread(dummy, 1, 4, fp) < 4) {
    jlog("Error: adin_file: size less than 4 bytes?\n",filename);
    fclose(fp);
    return(FALSE);
  }
  if (dummy[0] == 'R' &&
      dummy[1] == 'I' &&
      dummy[2] == 'F' &&
      dummy[3] == 'F') {
    /* it's a WAVE file */
    wav_p = TRUE;
    has_pre = FALSE;
    if (setup_wav(fp) == FALSE) {
      jlog("Error: adin_file: error in parsing wav header at %s\n",filename);
      fclose(fp);
      return(FALSE);
    }
  } else {
    /* read as raw format file */
    wav_p = FALSE;
    memcpy(pre_data, dummy, 4);    /* already read (4/sizeof(SP)) samples */
    has_pre = TRUE;
  }

  gfp = fp;

  return(TRUE);
}

/** 
 * Close the input file
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
adin_file_close()
{
  FILE *fp;

  fp = gfp;
  if (fclose(fp) != 0) {
    jlog("Error: adin_file: failed to close file\n");
    return FALSE;
  }
 return TRUE; 
}

static boolean from_file;	///< TRUE if list file is used to read input filename
static FILE *fp_list;		///< File pointer used for the listfile

/** 
 * Initialization: if listfile is specified, open it here.
 * 
 * @param freq [in] required sampling frequency.
 * @param arg [in] file name of listfile, or NULL if not use
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_file_standby(int freq, void *arg)
{
  char *fname = arg;
  if (fname != NULL) {
    /* read input filename from file */
    if ((fp_list = fopen(fname, "r")) == NULL) {
      jlog("Error: adin_file: failed to open %s\n", fname);
      return(FALSE);
    }
    from_file = TRUE;
  } else {
    /* read filename from stdin */
    from_file = FALSE;
  }
  /* store sampling frequency */
  sfreq = freq;
  
  return(TRUE);
}

/** 
 * @brief  Begin reading audio data from a file.
 *
 * If listfile was specified in adin_file_standby(), the next filename
 * will be read from the listfile.  Otherwise, the
 * filename will be obtained from stdin.  Then the file will be opened here.
 * 
 * @param filename [in] file name to open or NULL for prompt
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_file_begin(char *filename)
{
  boolean readp;

  if (filename != NULL) {
    /* open the file and exit with its status */
    if (adin_file_open(filename) == FALSE) {
      jlog("Error: adin_file: failed to read speech data: \"%s\"\n", filename);
      return FALSE;
    }
    jlog("Stat: adin_file: input speechfile: %s\n", filename);
    strcpy(speechfilename, filename);
    return TRUE;
  }

  /* ready to read next input */
  readp = FALSE;
  while(readp == FALSE) {
    if (from_file) {
      /* read file name from listfile */
      do {
	if (getl_fp(speechfilename, MAXPATHLEN, fp_list) == NULL) { /* end of input */
	  fclose(fp_list);
	  return(FALSE); /* end of input */
	}
      } while (speechfilename[0] == '#'); /* skip comment */
    } else {
      /* read file name from stdin */
      if (get_line_from_stdin(speechfilename, MAXPATHLEN, "enter filename->") == NULL) {
	return (FALSE);	/* end of input */
      }
    }
    /* open input file */
    if (adin_file_open(speechfilename) == FALSE) {
      jlog("Error: adin_file: failed to read speech data: \"%s\"\n", speechfilename);
    } else {
      jlog("Stat: adin_file: input speechfile: %s\n", speechfilename);
      readp = TRUE;
    }
  }
  return TRUE;
}

/** 
 * Try to read @a sampnum samples and returns actual sample num recorded.
 * 
 * @param buf [out] samples obtained in this function
 * @param sampnum [in] wanted number of samples to be read
 * 
 * @return actural number of read samples, -1 if EOF, -2 if error.
 */
int
adin_file_read(SP16 *buf, int sampnum)
{
  FILE *fp;
  int cnt;

  fp = gfp;
  
  if (wav_p) {
    cnt = fread(buf, sizeof(SP16), sampnum, fp);
    if (cnt == 0) {
      if (feof(fp)) return -1; /* EOF */
      if (ferror(fp)) {
	jlog("Error: adin_file: an error occured while reading file\n");
	adin_file_close();
	return -2; /* error */
      }
    }
    if (nowlen + cnt > maxlen) {
      cnt = maxlen - nowlen;
    }
    nowlen += cnt;
  } else {
    if (has_pre) {
      buf[0] = pre_data[0]; buf[1] = pre_data[1];
      has_pre = FALSE;
      cnt = fread(&(buf[2]), sizeof(SP16), sampnum - 2, fp);
      if (cnt == 0) {
	if (feof(fp)) return -1; /* EOF */
	if (ferror(fp)) {
	  jlog("Error: adin_file: an error occured file reading file\n");
	  adin_file_close();
	  return -2; /* error */
	}
      }
      cnt += 2;
    } else {
      cnt = fread(buf, sizeof(SP16), sampnum, fp);
      if (cnt == 0) {
	if (feof(fp)) return -1; /* EOF */
	if (ferror(fp)) {
	  jlog("Error: adin_file: an error occured file reading file\n");
	  adin_file_close();
	  return -2; /* error */
	}
      }
    }
  }
  /* all .wav data are in little endian */
  /* assume .raw data are in big endian */
#ifdef WORDS_BIGENDIAN
  if (wav_p) swap_sample_bytes(buf, cnt);
#else
  if (!wav_p) swap_sample_bytes(buf, cnt);
#endif
  return cnt;
}

/** 
 * End recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_file_end()
{
  /* nothing needed */
  adin_file_close();
  return TRUE;
}

/** 
 * Initialization for speech input via stdin.
 * 
 * @param freq [in] required sampling frequency.
 * @param arg dummy, ignored
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_stdin_standby(int freq, void *arg)
{
  /* store sampling frequency */
  sfreq = freq;
  return(TRUE);
}

/** 
 * @brief  Begin reading audio data from stdin
 *
 * @param pathname [in] dummy
 *
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_stdin_begin(char *pathname)
{
  if (feof(stdin)) {		/* already reached the end of input stream */
    jlog("Error: adin_stdin: stdin reached EOF\n");
    return FALSE;		/* terminate search here */
  } else {
    /* open input stream */
    if (adin_file_open(NULL) == FALSE) {
      jlog("Error: adin_stdin: failed to read speech data from stdin\n");
      return FALSE;
    }
    jlog("Stat: adin_stdin: reading wavedata from stdin...\n");
  }
  return TRUE;
}

/** 
 * Try to read @a sampnum samples and returns actual sample num recorded.
 * 
 * @param buf [out] samples obtained in this function
 * @param sampnum [in] wanted number of samples to be read
 * 
 * @return actural number of read samples, -1 if EOF, -2 if error.
 */
int
adin_stdin_read(SP16 *buf, int sampnum)
{
  int cnt;

  if (wav_p) {
    cnt = fread(buf, sizeof(SP16), sampnum, stdin);
    if (cnt == 0) {
      if (feof(stdin)) return -1; /* EOF */
      if (ferror(stdin)) {
	jlog("Error: adin_stdin: an error occured while reading stdin\n");
	return -2; /* error */
      }
    }
  } else {
    if (has_pre) {
      buf[0] = pre_data[0]; buf[1] = pre_data[1];
      has_pre = FALSE;
      cnt = fread(&(buf[2]), sizeof(SP16), sampnum - 2, stdin);
      if (cnt == 0) {
	if (feof(stdin)) return -1; /* EOF */
	if (ferror(stdin)) {
	  jlog("Error: adin_stdin: an error occured while reading stdin\n");
	  return -2; /* error */
	}
      }
      cnt += 2;
    } else {
      cnt = fread(buf, sizeof(SP16), sampnum, stdin);
      if (cnt == 0) {
	if (feof(stdin)) return -1; /* EOF */
	if (ferror(stdin)) {
	  jlog("Error: adin_stdin: an error occured while reading stdin\n");
	  return -2; /* error */
	}
      }
    }
  }

  /* all .wav data are in little endian */
  /* assume .raw data are in big endian */
#ifdef WORDS_BIGENDIAN
  if (wav_p) swap_sample_bytes(buf, cnt);
#else
  if (!wav_p) swap_sample_bytes(buf, cnt);
#endif
  return cnt;
}

/** 
 * 
 * A tiny function to get current input raw speech file name.
 * 
 * @return string of current input speech file.
 * 
 */
char *
adin_file_get_current_filename()
{
  return(speechfilename);
}
/** 
 * 
 * A tiny function to get current input raw speech file name.
 * 
 * @return string of current input speech file.
 * 
 */
char *
adin_stdin_input_name()
{
  return("stdin");
}
