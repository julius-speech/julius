/*
 * Copyright (c) 2002-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2002-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/*
 * mkss --- compute average spectrum of mic input for SS in Julius
 *
 * $Id: mkss.c,v 1.9 2013/12/19 16:26:22 sumomo Exp $
 *
 */

#include <julius/juliuslib.h>
#include <sys/stat.h>

static int fd = -1;		/* file descriptor for output */

static char *filename = NULL;	/* output file name */
static boolean stout = FALSE;	/* true if output to stdout ("-") */
static int sfreq;	/* sampling frequency */
static int slen  = 3000;	/* record length in msec */

/* parameter for SS */
static SP16 *speech;
static int speechnum;
static int samples;

static boolean
opt_help(Jconf *jconf, char *arg[], int argnum)
{
  fprintf(stderr, "mkss --- compute averate spectrum of mic input for SS\n");
  fprintf(stderr, "Usage: mkss [options..] filename\n");
  fprintf(stderr, "    [-freq frequency]    sampling freq in Hz   (%d)\n", jconf->am_root->analysis.para_default.smp_freq);
  fprintf(stderr, "    [-len msec]          record length in msec (%d)\n", slen);
  fprintf(stderr, "    [-fsize samplenum]   window size           (%d)\n", jconf->am_root->analysis.para_default.framesize);
  fprintf(stderr, "    [-fshift samplenum]  frame shift           (%d)\n", jconf->am_root->analysis.para_default.frameshift);
  fprintf(stderr, "    [-zmean]             enable zmean         (off)\n");
  fprintf(stderr, "    [-zmeanframe]        frame-wise zmean     (off)\n");
  fprintf(stderr, "Library configuration: ");
  confout_version(stderr);
  confout_audio(stderr);
  confout_process(stderr);
  fprintf(stderr, "\n");
  exit(1);			/* exit here */
  return TRUE;
}

static boolean
opt_freq(Jconf *jconf, char *arg[], int argnum)
{
  jconf->amnow->analysis.para.smp_freq = atoi(arg[0]);
  jconf->amnow->analysis.para.smp_period = freq2period(jconf->amnow->analysis.para.smp_freq);
  return TRUE;
}
static boolean
opt_len(Jconf *jconf, char *arg[], int argnum)
{
  slen = atoi(arg[0]);
  return TRUE;
}

static int
adin_callback(SP16 *now, int len, Recog *recog)
{
  int num;
  int ret;

  /* store recorded data up to samples */
  if (speechnum + len > samples) {
    num = samples - speechnum;
    /* stop recording */
    ret = 1;
  } else {
    num = len;
    /* continue recording */
    ret = 0;
  }
  memcpy(&(speech[speechnum]), now, num * sizeof(SP16));

  if (speechnum / sfreq != (speechnum + num) / sfreq) {
    fprintf(stderr, "|");
  } else {
    fprintf(stderr, ".");
  }

  speechnum += num;

  return(ret);
}

static int x;
static int sslen;

int
main(int argc, char *argv[])
{
  Recog *recog;
  Jconf *jconf;
  float *ss;
  MFCCWork *wrk;

  /* create instance */
  recog = j_recog_new();
  jconf = j_jconf_new();
  recog->jconf = jconf;

  /* set application-specific additional options */
  j_add_option("-freq", 1, 1, "sampling freq in Hz", opt_freq);
  j_add_option("-len", 1, 1, "record length in msec", opt_len);
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
  } else {
    filename = argv[argc-1];
  }

  /* set default as same as "-input mic" */
  jconf->input.type = INPUT_WAVEFORM;
  jconf->input.speech_input = SP_MIC;
  jconf->input.device = SP_INPUT_DEFAULT;
  /* process config and load them */
  if (j_config_load_args(jconf, argc-1, argv) == -1) {
    fprintf(stderr, "Error reading arguments\n");
    return -1;
  }
  /* force some default values */
  jconf->detect.silence_cut  = 0; /* disable silence cut */
  jconf->preprocess.strip_zero_sample = TRUE; /* strip zero samples */
  jconf->detect.level_thres = 0;	/* no VAD, record all */
  /* set Julius default parameters for unspecified acoustic parameters */
  apply_para(&(jconf->am_root->analysis.para), &(jconf->am_root->analysis.para_default));
  /* set some values */
  jconf->input.sfreq = jconf->am_root->analysis.para.smp_freq;
  jconf->input.period = jconf->am_root->analysis.para.smp_period;
  jconf->input.frameshift = jconf->am_root->analysis.para.frameshift;
  jconf->input.framesize = jconf->am_root->analysis.para.framesize;

  sfreq = jconf->am_root->analysis.para.smp_freq;

  /* output file check */
  if (!stout) {
    if (access(filename, F_OK) == 0) {
      if (access(filename, W_OK) == 0) {
	fprintf(stderr, "Warning: overwriting file \"%s\"\n", filename);
      } else {
	perror("mkss");
	return(1);
      }
    }
  }

  /* allocate speech store buffer */
  samples = sfreq * slen / 1000;
  speech = (SP16 *)mymalloc(sizeof(SP16) * samples);

  /* allocate work area to compute spectrum */
  wrk = WMP_work_new(&(jconf->am_root->analysis.para));
  if (wrk == NULL) {
    jlog("ERROR: m_fusion: failed to initialize MFCC computation for SS\n");
    return -1;
  }

  /* initialize input device */
  if (j_adin_init(recog) == FALSE) {
    fprintf(stderr, "Error in initializing adin device\n");
    return -1;
  }

  /* open device */
  if (j_open_stream(recog, NULL) < 0) {
    fprintf(stderr, "Error in opening adin device\n");
  }

  /* record mic input */
  fprintf(stderr, "%dHz recording for %.2f seconds of noise\n", sfreq, (float)slen /(float)1000);
  speechnum = 0;
  adin_go(adin_callback, NULL, recog);

  /* close device */
  adin_end(recog->adin);
  fprintf(stderr, "\n%d samples (%lu bytes, %.1f sec) recorded\n", samples, samples * sizeof(SP16), (float)samples / (float)sfreq);

  /* compute SS */
  fprintf(stderr, "compute SS:\n");
  fprintf(stderr, "  fsize : %4d samples (%.1f msec)\n", jconf->input.framesize, (float)jconf->input.framesize * 1000.0/ (float)sfreq);
  fprintf(stderr, "  fshift: %4d samples (%.1f msec)\n", jconf->input.frameshift, (float)jconf->input.frameshift * 1000.0/ (float)sfreq);

  ss = new_SS_calculate(speech, samples, &sslen, wrk, &(jconf->am_root->analysis.para));

  fprintf(stderr, "  points: %4d\n", sslen);
  fprintf(stderr, "noise spectrum was measured\n");
  
  /* open file for recording */
  fprintf(stderr, "writing average noise spectrum to [%s]...", filename);
  if (stout) {
    fd = 1;
  } else {
    if ((fd = open(filename, O_CREAT | O_RDWR
#ifdef O_BINARY
		   | O_BINARY
#endif
		   , 0644)) == -1) {
      perror("mkss");
      return(1);
    }
  }
  x = sslen;
#ifndef WORDS_BIGENDIAN
  swap_bytes((char *)&x, sizeof(int), 1);
#endif
  if (write(fd, &x, sizeof(int)) < sizeof(int)) {
    perror("mkss");
    return(1);
  }
#ifndef WORDS_BIGENDIAN
  swap_bytes((char *)ss, sizeof(float), sslen);
#endif
  if (write(fd, ss, sslen * sizeof(float)) < sslen * sizeof(float)) {
    perror("mkss");
    return(1);
  }
  if (!stout) {
    if (close(fd) < 0) {
      perror("mkss");
      return(1);
    }
  }
  fprintf(stderr, "done\n");

  WMP_free(wrk);

  return 0;
}

