/**
 * @file   adinrec.c
 * 
 * <JA>
 * @brief  マイクから一発話をファイルへ記録する
 * </JA>
 * 
 * <EN>
 * @brief  Record a speech segment from microphone to a file
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Mar 23 20:33:01 2005
 *
 * $Revision: 1.13 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2001-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius/juliuslib.h>
#include <signal.h>

static int speechlen;		///< Total length of recorded sample
static int fd = -1;		///< File descriptor for output
static FILE *fp = NULL;		///< File pointer for WAV output
static int  size;		///< Output file size
static int sfreq;

static char *filename = NULL;	///< Output file name
static boolean stout = FALSE;	///< True if output to stdout
static boolean use_raw = FALSE;	///< Output in RAW format if TRUE

/** 
 * <JA>ヘルプを表示して終了する</JA>
 * <EN>Print help and exit</EN>
 */
static boolean
opt_help(Jconf *jconf, char *arg[], int argnum)
{
  fprintf(stderr, "adinrec --- record one sentence input to a file\n");
  fprintf(stderr, "Usage: adinrec [options..] filename\n");
  fprintf(stderr, "    [-input mic|pulseaudio|alsa|oss|esd|...]  input source       (mic)\n");
  fprintf(stderr, "    [-freq frequency]     sampling frequency in Hz    (%d)\n", jconf->am_root->analysis.para_default.smp_freq);
  fprintf(stderr, "    [-48]                 48000Hz recording with down sampling (16kHz only)\n");
  fprintf(stderr, "    [-lv unsignedshort]   silence cut level threshold (%d)\n", jconf->detect.level_thres);
  fprintf(stderr, "    [-zc zerocrossnum]    silence cut zerocross num   (%d)\n", jconf->detect.zero_cross_num);
  fprintf(stderr, "    [-headmargin msec]    head margin length          (%d)\n", jconf->detect.head_margin_msec);
  fprintf(stderr, "    [-tailmargin msec]    tail margin length          (%d)\n", jconf->detect.tail_margin_msec);
  fprintf(stderr, "    [-chunksize sample]   chunk size for processing   (%d)\n", jconf->detect.chunk_size);
  fprintf(stderr, "    [-nostrip]            not strip off zero samples\n");
  fprintf(stderr, "    [-zmean]              remove DC by zero mean\n");
  fprintf(stderr, "    [-nocutsilence]       disable VAD, record all stream\n");
  fprintf(stderr, "    [-raw]                output in RAW format\n");
  fprintf(stderr, "\nLibrary configuration: ");
  confout_version(stderr);
  confout_audio(stderr);
  confout_process(stderr);
  fprintf(stderr, "\n");
  exit(1);			/* exit here */
  return TRUE;
}

static boolean
opt_raw(Jconf *jconf, char *arg[], int argnum)
{
  use_raw = TRUE;
  return TRUE;
}
static boolean
opt_freq(Jconf *jconf, char *arg[], int argnum)
{
  jconf->amnow->analysis.para.smp_freq = atoi(arg[0]);
  jconf->amnow->analysis.para.smp_period = freq2period(jconf->amnow->analysis.para.smp_freq);
  return TRUE;
}

/** 
 * <JA>
 * 録音されたサンプル列を処理するコールバック関数
 * 
 * @param now [in] 録音されたサンプル列
 * @param len [in] 長さ（サンプル数）
 * 
 * @return エラー時 -1，処理成功時 0，処理成功＋区間終端検出時 1 を返す．
 * </JA>
 * <EN>
 * Callback handler of recorded sample fragments
 * 
 * @param now [in] recorded fragments of speech sample
 * @param len [in] length of above in samples
 * 
 * @return -1 on device error (require caller to exit and terminate input),
 * 0 on success (allow caller to continue),
 * 1 on succeeded but segmentation detected (require caller to exit but
 * input will continue in the next call.
 * </EN>
 */
static int
adin_callback_file(SP16 *now, int len, Recog *recog)
{
  int count;

  /* erase "<<<please speak>>>" text on tty at trigger up */
  if (speechlen == 0) {
    fprintf(stderr, "\r                    \r");
  }

  /* open output file for the first time */
  if (use_raw) {
    if (fd == -1) {
      if (stout) {
	fd = 1;
      } else {
	if ((fd = open(filename, O_CREAT | O_RDWR
#ifdef O_BINARY
		       | O_BINARY
#endif
		       , 0644)) == -1) {
	  perror("adinrec");
	  return -1;
	}
      }
    }
  } else {
    if (fp == NULL) {
      if ((fp = wrwav_open(filename, sfreq)) == NULL) {
	perror("adinrec");
	return -1;
      }
    }
  }
  /* write recorded sample to file */
  if (use_raw) {
    count = wrsamp(fd, now, len);
    if (count < 0) {
      perror("adinrec: cannot write");
      return -1;
    }
    if (count < len * sizeof(SP16)) {
      fprintf(stderr, "adinrec: cannot write more %d bytes\ncurrent length = %lu\n", count, speechlen * sizeof(SP16));
      return -1;
    }
  } else {
    if (wrwav_data(fp, now, len) == FALSE) {
      fprintf(stderr, "adinrec: cannot write\n");
      return -1;
    }
  }
  
  speechlen += len;
  
  /* progress bar in dots */
  fprintf(stderr, ".");		
  return(0);
}

/* close file */
static void
close_file()
{
  size = sizeof(SP16) * speechlen;
  if (use_raw) {
    if (fd >= 0) {
      if (close(fd) != 0) {
	perror("adinrec");
      }
    }
  } else {
    if (fp != NULL) {
      if (wrwav_close(fp) == FALSE) {
	fprintf(stderr, "adinrec: failed to close file\n");
      }
    }
  }
  fprintf(stderr, "\n%d samples (%d bytes, %.2f sec.) recorded\n", speechlen, size, (float)speechlen / (float)sfreq);
}  

/* Interrupt signal handling */
static void
interrupt_record(int signum)
{
  fprintf(stderr, "[Interrupt]");
  /* close files */
  close_file();
  /* terminate program */
  exit(1);
}


/** 
 * <JA>
 * メイン関数
 * 
 * @param argc [in] 引数列の長さ
 * @param argv [in] 引数列
 * 
 * @return 
 * </JA>エラー時 1，通常終了時 0 を返す．
 * <EN>
 * Main function.
 * 
 * @param argc [in] number of argument.
 * @param argv [in] array of arguments.
 * 
 * @return 1 on error, 0 on success.
 * </EN>
 */
int
main(int argc, char *argv[])
{
  Recog *recog;
  Jconf *jconf;

  /* create instance */
  jconf = j_jconf_new();

  /* register application options */
  j_add_option("-freq", 1, 1, "sampling frequency in Hz", opt_freq);
  j_add_option("-raw", 0, 0, "save in raw (BE) format", opt_raw);
  j_add_option("-h", 0, 0, "display this help", opt_help);
  j_add_option("-help", 0, 0, "display this help", opt_help);
  j_add_option("--help", 0, 0, "display this help", opt_help);

  /* when no argument, output help and exit */
  if (argc <= 1) {
    opt_help(jconf, NULL, 0);
    return 0;
  }

  /* regard last arg as filename */
  if (strmatch(argv[argc-1], "-")) {
    stout = TRUE;
    use_raw = TRUE;
  } else {
    filename = argv[argc-1];
  }

  /* set default as same as "-input mic" */
  jconf->input.type = INPUT_WAVEFORM;
  jconf->input.speech_input = SP_MIC;
  jconf->input.device = SP_INPUT_DEFAULT;

  /* read arguments and set parameters */
  if (j_config_load_args(jconf, argc-1, argv) == -1) {
    fprintf(stderr, "Error reading arguments\n");
    return -1;
  }

  /* exit if no file name specified */
  if (filename == NULL && stout == FALSE) {
    opt_help(jconf, NULL, 0);
    return -1;
  }

  /* finalize config */
  //if (j_jconf_finalize(jconf) == FALSE) return -1;

  /* set Julius default parameters for unspecified acoustic parameters */
  apply_para(&(jconf->am_root->analysis.para), &(jconf->am_root->analysis.para_default));
  
  /* set some values */
  jconf->input.sfreq = jconf->am_root->analysis.para.smp_freq;
  jconf->input.period = jconf->am_root->analysis.para.smp_period;
  jconf->input.frameshift = jconf->am_root->analysis.para.frameshift;
  jconf->input.framesize = jconf->am_root->analysis.para.framesize;

  /* preliminary check of output file */
  /* (output file will be opened later when input is triggered) */
  if (!stout) {
    if (access(filename, F_OK) == 0) {
      if (access(filename, W_OK) == 0) {
	fprintf(stderr, "Warning: overwriting file \"%s\"\n", filename);
      } else {
	perror("adinrec");
	return(1);
      }
    }
  }
  /* set signal handlers to properly close output file */
  if (signal(SIGINT, interrupt_record) == SIG_ERR) {
    fprintf(stderr, "Warning: signal intterupt may collapse output\n");
  }
  if (signal(SIGTERM, interrupt_record) == SIG_ERR) {
    fprintf(stderr, "Warning: signal intterupt may collapse output\n");
  }
#ifdef SIGPIPE
  if (signal(SIGPIPE, interrupt_record) == SIG_ERR) {
    fprintf(stderr, "Warning: signal intterupt may collapse output\n");
  }
#endif
#ifdef SIGQUIT
  if (signal(SIGQUIT, interrupt_record) == SIG_ERR) {
    fprintf(stderr, "Warning: signal intterupt may collapse output\n");
  }
#endif

  recog = j_recog_new();
  recog->jconf = jconf;

  /* initialize input device */
  if (j_adin_init(recog) == FALSE) {
    fprintf(stderr, "Error in initializing adin device\n");
    return -1;
  }
  /* open device */
  if (j_open_stream(recog, NULL) < 0) {
    fprintf(stderr, "Error in opening adin device\n");
  }
  /* do recoding */
  speechlen = 0;
  sfreq = recog->jconf->input.sfreq;
  fprintf(stderr, "<<< please speak >>>"); /* moved from adin-cut.c */
  adin_go(adin_callback_file, NULL, recog);
  /* close device */
  adin_end(recog->adin);
  /* close output file */
  close_file();

  return 0;
}
