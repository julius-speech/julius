/**
 * @file   adin_sndfile.c
 * 
 * <JA>
 * @brief  ファイル入力：libsndfile を用いた音声ファイル読み込み
 *
 * libsndfile を用いて音声ファイルからの入力を行なう関数です．
 * Microsoft WAVE形式の音声ファイル，およびヘッダ無し（RAW）ファイルの他に,
 * AU, AND, NIST, ADPCM など様々な形式のファイルを読み込むことができます．
 * なお，チャンネル数は１(モノラル)に限られます．またRAWファイルの場合，
 * データのバイトオーダーは big endian である必要があります．
 * 
 * ファイルのサンプリングレートはシステムの要求するサンプリングレート
 * （adin_standby() で指定される値）と一致する必要があります．
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
 * libsndfile はconfigure 時に自動検出されます．検出に失敗した場合，
 * ファイル入力には adin_file.c 内の関数が使用されます．
 *
 * Libsndfile のバージョンは 1.0.x に対応しています．
 *
 * @sa http://www.mega-nerd.com/libsndfile/
 * </JA>
 * <EN>
 * @brief  Audio input from file using libsndfile library.
 *
 * Functions to get input from wave file using libsndfile library.
 * Many file formats are supported, including Microsoft WAVE format,
 * and RAW (no header) format, AU, SND, NIST and so on.  The channel number
 * should be 1 (monaural).
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
 * libsndfile should be installed before compilation.  The library and header
 * will be automatically detected by configure script.  When failed to detect,
 * Julius uses adin_file.c instead for file input.
 *
 * This file will work on libsndfile version 1.0.x.
 *
 * @sa http://www.mega-nerd.com/libsndfile/
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Mon Feb 14 12:13:27 2005
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
#include <sent/speech.h>
#include <sent/adin.h>

#ifdef HAVE_LIBSNDFILE

/* sound header */
#include <sndfile.h>

static int sfreq;		///< Required sampling frequency in Hz
static SF_INFO sinfo;		///< Wavefile information
static SNDFILE *sp;		///< File handler
static boolean from_file;	///< TRUE if reading filename from listfile
static FILE *fp_list;		///< File pointer used for the listfile
static char speechfilename[MAXPATHLEN];	///< Buffer to hold input file name

/// Check if the file format is 16bit, monoral.
static boolean
check_format(SF_INFO *s)
{
  if ((s->format & SF_FORMAT_TYPEMASK) != SF_FORMAT_RAW) {
    if (s->samplerate != sfreq) {
      jlog("Error: adin_sndfile: sample rate != %d, it's %d Hz data\n", sfreq, s->samplerate);
      return FALSE;
    }
  }
  if (s->channels != 1) {
    jlog("Error: adin_sndfile: channel num != 1, it has %d channels\n", s->channels);
    return FALSE;
  }
#ifdef HAVE_LIBSNDFILE_VER1
  if ((s->format & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
    jlog("Error: adin_sndfile: not 16-bit data\n");
    return FALSE;
  }
#else
  if (s->pcmbitwidth != 16) {
    jlog("Error: adin_sndfile: not 16-bit data, it's %d bit\n", s->pcmbitwidth);
    return FALSE;
  }
#endif
  return TRUE;
}

/// Output format information to stdout (compliant to libsnd-0.0.23)
static void
print_format(SF_INFO *s)
{
  switch(s->format & SF_FORMAT_TYPEMASK) {
  case SF_FORMAT_WAV:    jlog("Stat: adin_sndfile: input format = Microsoft WAV\n"); break;
  case SF_FORMAT_AIFF:   jlog("Stat: adin_sndfile: input format = Apple/SGI AIFF\n"); break;
  case SF_FORMAT_AU:     jlog("Stat: adin_sndfile: input format = Sun/NeXT AU\n"); break;
#ifndef HAVE_LIBSNDFILE_VER1
  case SF_FORMAT_AULE:   jlog("Stat: adin_sndfile: input format = DEC AU\n"); break;
#endif
  case SF_FORMAT_RAW:    jlog("Stat: adin_sndfile: input format = RAW\n"); break;
  case SF_FORMAT_PAF:    jlog("Stat: adin_sndfile: input format = Ensoniq PARIS\n"); break;
  case SF_FORMAT_SVX:    jlog("Stat: adin_sndfile: input format = Amiga IFF / SVX8 / SV16\n"); break;
  case SF_FORMAT_NIST:   jlog("Stat: adin_sndfile: input format = Sphere NIST\n"); break;
#ifdef HAVE_LIBSNDFILE_VER1
  case SF_FORMAT_VOC:	 jlog("Stat: adin_sndfile: input format = VOC file\n"); break;
  case SF_FORMAT_IRCAM:  jlog("Stat: adin_sndfile: input format = Berkeley/IRCAM/CARL\n"); break;
  case SF_FORMAT_W64:	 jlog("Stat: adin_sndfile: input format = Sonic Foundry's 64bit RIFF/WAV\n"); break;
  case SF_FORMAT_MAT4:   jlog("Stat: adin_sndfile: input format = Matlab (tm) V4.2 / GNU Octave 2.0\n"); break;
  case SF_FORMAT_MAT5:   jlog("Stat: adin_sndfile: input format = Matlab (tm) V5.0 / GNU Octave 2.1\n"); break;
#endif
  default: jlog("Stat: adin_sndfile: input format = UNKNOWN TYPE\n"); break;
  }
  switch(s->format & SF_FORMAT_SUBMASK) {
#ifdef HAVE_LIBSNDFILE_VER1
  case SF_FORMAT_PCM_U8:    jlog("Stat: adin_sndfile: input type = Unsigned 8 bit PCM\n"); break;
  case SF_FORMAT_PCM_S8:    jlog("Stat: adin_sndfile: input type = Signed 8 bit PCM\n"); break;
  case SF_FORMAT_PCM_16:    jlog("Stat: adin_sndfile: input type = Signed 16 bit PCM\n"); break;
  case SF_FORMAT_PCM_24:    jlog("Stat: adin_sndfile: input type = Signed 24 bit PCM\n"); break;
  case SF_FORMAT_PCM_32:    jlog("Stat: adin_sndfile: input type = Signed 32 bit PCM\n"); break;
  case SF_FORMAT_FLOAT:     jlog("Stat: adin_sndfile: input type = 32bit float\n"); break;
  case SF_FORMAT_DOUBLE:    jlog("Stat: adin_sndfile: input type = 64bit float\n"); break;
  case SF_FORMAT_ULAW:      jlog("Stat: adin_sndfile: input type = U-Law\n"); break;
  case SF_FORMAT_ALAW:      jlog("Stat: adin_sndfile: input type = A-Law\n"); break;
  case SF_FORMAT_IMA_ADPCM: jlog("Stat: adin_sndfile: input type = IMA ADPCM\n"); break;
  case SF_FORMAT_MS_ADPCM:  jlog("Stat: adin_sndfile: input type = Microsoft ADPCM\n"); break;
  case SF_FORMAT_GSM610:    jlog("Stat: adin_sndfile: input type = GSM 6.10, \n"); break;
  case SF_FORMAT_G721_32:   jlog("Stat: adin_sndfile: input type = 32kbs G721 ADPCM\n"); break;
  case SF_FORMAT_G723_24:   jlog("Stat: adin_sndfile: input type = 24kbs G723 ADPCM\n"); break;
  case SF_FORMAT_G723_40:   jlog("Stat: adin_sndfile: input type = 40kbs G723 ADPCM\n"); break;
#else
  case SF_FORMAT_PCM:       jlog("Stat: adin_sndfile: input type = PCM\n"); break;
  case SF_FORMAT_FLOAT:     jlog("Stat: adin_sndfile: input type = floats\n"); break;
  case SF_FORMAT_ULAW:      jlog("Stat: adin_sndfile: input type = U-Law\n"); break;
  case SF_FORMAT_ALAW:      jlog("Stat: adin_sndfile: input type = A-Law\n"); break;
  case SF_FORMAT_IMA_ADPCM: jlog("Stat: adin_sndfile: input type = IMA ADPCM\n"); break;
  case SF_FORMAT_MS_ADPCM:  jlog("Stat: adin_sndfile: input type = Microsoft ADPCM\n"); break;
  case SF_FORMAT_PCM_BE:    jlog("Stat: adin_sndfile: input type = Big endian PCM\n"); break;
  case SF_FORMAT_PCM_LE:    jlog("Stat: adin_sndfile: input type = Little endian PCM\n"); break;
  case SF_FORMAT_PCM_S8:    jlog("Stat: adin_sndfile: input type = Signed 8 bit PCM\n"); break;
  case SF_FORMAT_PCM_U8:    jlog("Stat: adin_sndfile: input type = Unsigned 8 bit PCM\n"); break;
  case SF_FORMAT_SVX_FIB:   jlog("Stat: adin_sndfile: input type = SVX Fibonacci Delta\n"); break;
  case SF_FORMAT_SVX_EXP:   jlog("Stat: adin_sndfile: input type = SVX Exponential Delta\n"); break;
  case SF_FORMAT_GSM610:    jlog("Stat: adin_sndfile: input type = GSM 6.10, \n"); break;
  case SF_FORMAT_G721_32:   jlog("Stat: adin_sndfile: input type = 32kbs G721 ADPCM\n"); break;
  case SF_FORMAT_G723_24:   jlog("Stat: adin_sndfile: input type = 24kbs G723 ADPCM\n"); break;
#endif
  default: jlog("Stat: adin_sndfile: input type = UNKNOWN SUBTYPE\n"); break;
  }

#ifdef HAVE_LIBSNDFILE_VER1
  switch(s->format & SF_FORMAT_ENDMASK) {
  case SF_ENDIAN_FILE:      jlog("Stat: adin_sndfile: endian = file native endian\n"); break;
  case SF_ENDIAN_LITTLE:    jlog("Stat: adin_sndfile: endian = forced little endian\n"); break;
  case SF_ENDIAN_BIG:       jlog("Stat: adin_sndfile: endian = forced big endian\n"); break;
  case SF_ENDIAN_CPU:       jlog("Stat: adin_sndfile: endian = forced CPU native endian\n"); break;
  }
  jlog("Stat: adin_sndfile: %d Hz, %d channels\n", s->samplerate, s->channels);
#else
  jlog("Stat: adin_sndfile: %d bit, %d Hz, %d channels\n", s->pcmbitwidth, s->samplerate, s->channels);
#endif
}

/** 
 * Initialization: if listfile is specified, open it here. Else, just store
 * the required frequency.
 * 
 * @param freq [in] required sampling frequency
 * @param arg [in] file name of listfile, or NULL if not use
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_sndfile_standby(int freq, void *arg)
{
  char *fname = arg;
  if (fname != NULL) {
    /* read input filename from file */
    if ((fp_list = fopen(fname, "r")) == NULL) {
      jlog("Error: adin_sndfile: failed to open %s\n", fname);
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
 * @brief  Open a file and check the format
 *
 * @param filename [in] file name to open
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
adin_sndfile_open(char *filename)
{
#ifndef HAVE_LIBSNDFILE_VER1
  sinfo.samplerate = sfreq;
  sinfo.pcmbitwidth = 16;
  sinfo.channels = 1;
#endif
  sinfo.format = 0x0;
  if ((sp = 
#ifdef HAVE_LIBSNDFILE_VER1
       sf_open(filename, SFM_READ, &sinfo)
#else
       sf_open_read(filename, &sinfo)
#endif
       ) == NULL) {
    /* retry assuming raw format */
    sinfo.samplerate = sfreq;
    sinfo.channels = 1;
#ifdef HAVE_LIBSNDFILE_VER1
    sinfo.format = SF_FORMAT_RAW | SF_FORMAT_PCM_16 | SF_ENDIAN_BIG;
#else
    sinfo.pcmbitwidth = 16;
    sinfo.format = SF_FORMAT_RAW | SF_FORMAT_PCM_BE;
#endif
    if ((sp =
#ifdef HAVE_LIBSNDFILE_VER1
	 sf_open(filename, SFM_READ, &sinfo)
#else
	 sf_open_read(filename, &sinfo)
#endif
	 ) == NULL) {
      sf_perror(sp);
      jlog("Error: adin_sndfile: failed to open speech data: \"%s\"\n",filename);
    }
  }
  if (sp == NULL) {		/* open failure */
    return FALSE;
  }
  /* check its format */
  if (! check_format(&sinfo)) {
    return FALSE;
  }
  return TRUE;
}

/** 
 * @brief  Begin reading audio data from a file.
 *
 * If listfile was specified in adin_sndfile_standby(), the next filename
 * will be read from the listfile.  Otherwise, the
 * filename will be obtained from stdin.  Then the file will be opened here.
 *
 * @param filename [in] file name to open or NULL for prompt
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_sndfile_begin(char *filename)
{
  boolean readp;

  if (filename != NULL) {
    if (adin_sndfile_open(filename) == FALSE) {
      jlog("Error: adin_sndfile: invalid format: \"%s\"\n", filename);
      print_format(&sinfo);
      return FALSE;
    }
    jlog("Stat: adin_sndfile: input speechfile: %s\n", filename);
    print_format(&sinfo);
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
    if (adin_sndfile_open(speechfilename) == FALSE) {
      jlog("Error: adin_sndfile: invalid format: \"%s\"\n",speechfilename);
      print_format(&sinfo);
    } else {
      jlog("Stat: adin_sndfile: input speechfile: %s\n",speechfilename);
      print_format(&sinfo);
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
adin_sndfile_read(SP16 *buf, int sampnum)
{
  int cnt;

  cnt = sf_read_short(sp, buf, sampnum);
  if (cnt == 0) {		/* EOF */
    return -1;
  } else if (cnt < 0) {		/* error */
    sf_perror(sp);
    sf_close(sp);
    return -2;		/* error */
  }
  return cnt;
}

/** 
 * End recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_sndfile_end()
{
  /* close files */
  if (sf_close(sp) != 0) {
    sf_perror(sp);
    jlog("Error: adin_sndfile: failed to close\n");
    return FALSE;
  }
  return TRUE;
}

/** 
 * 
 * A tiny function to get current input raw speech file name.
 * 
 * @return string of current input speech file.
 * 
 */
char *
adin_sndfile_get_current_filename()
{
  return(speechfilename);
}

#endif /* ~HAVE_LIBSNDFILE */
