/**
 * @file   adintool.c
 * 
 * @brief  AD-in tool to record / split / send / receive speech data
 *
 * This tool is to handle speech input and output from/to various devices
 * using libsetn library in Julius.  It reads input from either of
 * microphone, file, adinnet network client, or standard input, and
 * perform speech detection based on level and zero cross (you can disable
 * this), and output them to either of file, adinnet network server, or
 * standard output.
 *
 * For example, you can send microphone input to Julius running on other host
 * by specifying input to microphone and output to adinnet (Julius should
 * be run with "-input adinnet"), or you can long recorded speech data by
 * long silence by specifying both input and output to file and enabling
 * segment detection.
 * 
 * @author Akinobu LEE
 * @date   Wed Mar 23 20:43:32 2005
 *
 * $Revision: 1.21 $
 * 
 */
/*
 * Copyright (c) 1991-2016 Kawahara Lab., Kyoto University
 * Copyright (c) 2001-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2016 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include "adintool.h"

// global 
AdinTool *global_a = NULL;

// allocate new audio structure
AdinTool *adintool_new()
{
  AdinTool *a;
  int i;

  a = (AdinTool *)malloc(sizeof(AdinTool));
  if (a == NULL) return NULL;

  a->conf.speech_output = SPOUT_NONE;
  a->conf.sfreq = 0;
  a->conf.continuous_segment = TRUE;
  a->conf.pause_each = FALSE;
  a->conf.loose_sync = FALSE;
  a->conf.rewind_msec = 0;
  a->conf.filename = NULL;
  a->conf.startid = 0;
  a->conf.use_raw = FALSE;
  a->conf.adinnet_port_in = ADINNET_PORT;
  for (i = 0; i < MAXCONNECTION; i++) a->conf.adinnet_serv[i] = NULL;
  for (i = 0; i < MAXCONNECTION; i++) a->conf.adinnet_port[i] = 0;
  a->conf.adinnet_servnum = 0;
  a->conf.adinnet_portnum = 0;
  a->conf.vecnet_paramtype = F_ERR_INVALID;
  a->conf.vecnet_veclen = 0;
  a->on_processing = FALSE;
  a->on_pause = FALSE;
  a->writing_file = FALSE;
  a->stop_at_next = FALSE;
  a->process_error = FALSE;
  a->total_speechlen = 0;
  a->trigger_sample = 0;
  a->unknown_command_counter = 0;
  for (i = 0; i < MAXCONNECTION; i++) a->resume_count[i] = 0;
  a->recog = NULL;
  a->jconf = NULL;
  a->speechlen = 0;
  a->fd = -1;
  a->fp = NULL;
  a->sid = 0;
  a->outpath = NULL;
  for (i = 0; i < MAXCONNECTION; i++) a->sd[i] = 0;
#ifdef USE_SDL
#ifdef HAVE_PTHREAD
  pthread_mutex_init(&(a->sdl.mutex), NULL);
#endif
  a->sdl.window = NULL;
  a->sdl.renderer = NULL;
  a->sdl.window_w = 0;
  a->sdl.items = 0;
  a->sdl.tickbuf = NULL;
  a->sdl.ticklen = 0;
  a->sdl.maxlevel = NULL;
  a->sdl.minlevel = NULL;
  a->sdl.tickbp = 0;
  a->sdl.flag = NULL;
#ifdef AUTO_ADJUST_THRESHOLD
  a->sdl.mean = NULL;
  a->sdl.var = NULL;
  a->sdl.meanofmean = NULL;
  a->sdl.validmean = NULL;
  a->sdl.varofmean = NULL;
  a->sdl.triggerrate = NULL;
  a->sdl.vvthres1 = AUTOTHRES_ADAPT_THRES_1;
  a->sdl.vvthres2 = AUTOTHRES_ADAPT_THRES_2;
#endif
  a->sdl.bp = 0;
  a->sdl.rects = NULL;
  a->sdl.rectflags = NULL;
  a->sdl.is_valid_flag = 0;
  a->sdl.totaltick = 0;
#endif

  return a;
}


// Output configuration to stderr
static void
put_status(AdinTool *a)
{
  int i;
  Recog *recog = a->recog;
  Jconf *jconf = a->jconf;

  if (recog == NULL || jconf == NULL) return;

  fprintf(stderr, "----------------------------------------\n");
  fprintf(stderr, "INPUT\n");
  fprintf(stderr, "\t   InputType: ");
  switch(jconf->input.type) {
  case INPUT_WAVEFORM:
    fprintf(stderr, "waveform\n");
    break;
  case INPUT_VECTOR:
    fprintf(stderr, "feature vector sequence\n");
    break;
  }
  fprintf(stderr, "\t InputSource: ");
  if (jconf->input.plugin_source != -1) {
    fprintf(stderr, "plugin\n");
  } else if (jconf->input.speech_input == SP_RAWFILE) {
    fprintf(stderr, "waveform file\n");
  } else if (jconf->input.speech_input == SP_MFCFILE) {
    fprintf(stderr, "feature vector file (HTK format)\n");
  } else if (jconf->input.speech_input == SP_OUTPROBFILE) {
    fprintf(stderr, "output probability file (HTK format)\n");
  } else if (jconf->input.speech_input == SP_STDIN) {
    fprintf(stderr, "standard input\n");
  } else if (jconf->input.speech_input == SP_ADINNET) {
    fprintf(stderr, "adinnet client\n");
#ifdef USE_NETAUDIO
  } else if (jconf->input.speech_input == SP_NETAUDIO) {
    char *p;
    fprintf(stderr, "NetAudio server on ");
    if (jconf->input.netaudio_devname != NULL) {
      fprintf(stderr, "%s\n", jconf->input.netaudio_devname);
    } else if ((p = getenv("AUDIO_DEVICE")) != NULL) {
      fprintf(stderr, "%s\n", p);
    } else {
      fprintf(stderr, "local port\n");
    }
#endif
  } else if (jconf->input.speech_input == SP_MIC) {
    fprintf(stderr, "microphone\n");
    fprintf(stderr, "\t   DeviceAPI: ");
    switch(jconf->input.device) {
    case SP_INPUT_DEFAULT: fprintf(stderr, "default\n"); break;
    case SP_INPUT_ALSA: fprintf(stderr, "alsa\n"); break;
    case SP_INPUT_OSS: fprintf(stderr, "oss\n"); break;
    case SP_INPUT_ESD: fprintf(stderr, "esd\n"); break;
    case SP_INPUT_PULSEAUDIO: fprintf(stderr, "pulseaudio\n"); break;
    }
  }

  fprintf(stderr, "\tSegmentation: ");
  if (jconf->detect.silence_cut) {
    if (a->conf.continuous_segment) {
      fprintf(stderr,"on, continuous\n");
    } else {
      fprintf(stderr,"on, only one snapshot\n");
    }
    if (recog->adin->down_sample) {
      fprintf(stderr, "\t  SampleRate: 48000Hz -> %d Hz\n", a->conf.sfreq);
    } else {
      fprintf(stderr, "\t  SampleRate: %d Hz\n", a->conf.sfreq);
    }
    fprintf(stderr, "\t       Level: %d / 32767\n", jconf->detect.level_thres);
    fprintf(stderr, "\t   ZeroCross: %d per sec.\n", jconf->detect.zero_cross_num);
    fprintf(stderr, "\t  HeadMargin: %d msec.\n", jconf->detect.head_margin_msec);
    fprintf(stderr, "\t  TailMargin: %d msec.\n", jconf->detect.tail_margin_msec);
  } else {
    fprintf(stderr,"OFF\n");
  }
  if (jconf->preprocess.strip_zero_sample) {
    fprintf(stderr, "\t  ZeroFrames: drop\n");
  } else {
    fprintf(stderr, "\t  ZeroFrames: keep\n");
  }
  if (jconf->preprocess.use_zmean) {
    fprintf(stderr, "\t   DCRemoval: on\n");
  } else {
    fprintf(stderr, "\t   DCRemoval: off\n");
  }
  if (a->conf.pause_each) {
    fprintf(stderr, "\t   AutoPause: on\n");
  } else {
    fprintf(stderr, "\t   Auto`ause: off\n");
  }
  if (a->conf.loose_sync) {
    fprintf(stderr, "\t   LooseSync: on\n");
  } else {
    fprintf(stderr, "\t   LooseSync: off\n");
  }
  if (a->conf.rewind_msec > 0) {
    fprintf(stderr, "\t      Rewind: %d msec\n", a->conf.rewind_msec);
  } else {
    fprintf(stderr, "\t      Rewind: no\n");
  }
  fprintf(stderr, "OUTPUT\n");
  switch(a->conf.speech_output) {
  case SPOUT_NONE:
    fprintf(stderr, "\t  OutputType: none (no output)\n");
    break;
  case SPOUT_FILE:
    fprintf(stderr, "\t  OutputType: waveform\n");
    fprintf(stderr, "\t    OutputTo: file\n");
    fprintf(stderr, "\t    FileName: ");
    if (jconf->detect.silence_cut) {
      if (a->conf.continuous_segment) {
	if (a->conf.use_raw) {
	  fprintf(stderr,"%s.%04d.raw, %s.%04d.raw, ...\n", a->conf.filename, a->conf.startid, a->conf.filename, a->conf.startid + 1);
	} else {
	  fprintf(stderr,"%s.%04d.wav, %s.%04d.wav, ...\n", a->conf.filename, a->conf.startid, a->conf.filename, a->conf.startid + 1);
	}
      } else {
	fprintf(stderr,"%s\n", a->outpath);
      }
    } else {
      fprintf(stderr,"%s (warning: inifinite recording: be care of disk space!)\n", a->outpath);
    }
    break;
  case SPOUT_STDOUT:
    fprintf(stderr, "\t  OutputType: waveform\n");
    fprintf(stderr, "\t    OutputTo: standard output\n");
    break;
  case SPOUT_ADINNET:
    fprintf(stderr, "\t  OutputType: waveform\n");
    fprintf(stderr, "\t    OutputTo: adinnet server\n");
    fprintf(stderr, "\t      SendTo:");
    for(i = 0; i < a->conf.adinnet_servnum; i++) {
      fprintf(stderr, " (%s:%d)", a->conf.adinnet_serv[i], a->conf.adinnet_port[i]);
    }
    fprintf(stderr, "\n");
    break;
  case SPOUT_VECTORNET:
    fprintf(stderr, "\t  OutputType: feature vector sequence\n");
    fprintf(stderr, "\t    OutputTo: vecnet server\n");
    fprintf(stderr, "\t      SendTo:");
    for(i = 0; i < a->conf.adinnet_servnum; i++) {
      fprintf(stderr, " (%s:%d)", a->conf.adinnet_serv[i], a->conf.adinnet_port[i]);
    }
    fprintf(stderr, "\n");
    {
      char buf[80];
      fprintf(stderr, "\t   ParamType: %s\n", param_code2str(buf, a->conf.vecnet_paramtype, FALSE));
      fprintf(stderr, "\t   VectorLen: %d\n", a->conf.vecnet_veclen);
    }
    break;
  }

  fprintf(stderr, "----------------------------------------\n");

  if (a->conf.speech_output == SPOUT_VECTORNET) {
    MFCCCalc *mfcc;

    fprintf(stderr, "Detailed parameter setting for feature extraction\n");
    for(mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
      fprintf(stderr, "[MFCC%02d]\n", mfcc->id);
      print_mfcc_info(stderr, mfcc, recog->jconf);
    }
    fprintf(stderr, "----------------------------------------\n");
  }

}    

// return newly allocated filename with the suffix
static char *
new_output_filename(char *filename, char *suffix)
{
  int len, slen;
  char *buf;

  len = strlen(filename);
  slen = strlen(suffix);
  if (len < slen || strcmp(&(filename[len-slen]), suffix) != 0) {
    buf = strcpy((char *)mymalloc(len+slen+1), filename);
    strcat(buf, suffix);
  } else {
    buf = strcpy((char *)mymalloc(len+1), filename);
  }
  return buf;
}

// Main function
int
main(int argc, char *argv[])
{
  AdinTool *a;
  int i;

#ifdef NO_SDL_MAIN
  SDL_SetMainReady();
#endif

  /* create instance */
  a = adintool_new();
  if (a == NULL) exit(1);
  global_a = a;

  /* create JuliusLib instances */
  a->recog = j_recog_new();
  a->jconf = j_jconf_new();
  a->recog->jconf = a->jconf;
  
  /*******************/
  /* parameter setup */
  /*******************/

  /* register adintool-specific options to julius library */
  register_options_to_julius();

#ifdef USE_SDL
  /* default: captguring mic input, output nothing */
  j_config_load_string(a->jconf, "-in mic -out none");
#else
  if (argc <= 1) {
    /* when no argument, output help and exit */
    show_help_and_exit(a->jconf, NULL, 0);
  }
#endif

  /* read arguments and set parameters */
  if (j_config_load_args(a->jconf, argc, argv) == -1) {
    fprintf(stderr, "Error reading arguments\n");
    return -1;
  }

  /* check arguments */
  if (a->conf.speech_output == SPOUT_FILE && a->conf.filename == NULL) {
    fprintf(stderr, "Error: output filename not specified\n");
    return(-1);
  }
  if ((a->conf.speech_output == SPOUT_ADINNET || a->conf.speech_output == SPOUT_VECTORNET) && a->conf.adinnet_servnum < 1) {
    fprintf(stderr, "Error: server name for output not specified\n");
    return(-1);
  }
  if (a->jconf->input.speech_input == SP_ADINNET &&
      a->conf.speech_output != SPOUT_ADINNET && a->conf.speech_output != SPOUT_VECTORNET &&
      a->conf.adinnet_servnum >= 1) {
    fprintf(stderr, "Warning: you specified port num by -port, but it's for output\n");
    fprintf(stderr, "Warning: you may specify input port by -inport instead.\n");
    fprintf(stderr, "Warning: now the default value (%d) will be used\n", ADINNET_PORT);
  }
#ifdef USE_NETAUDIO
  if (a->jconf->input.speech_input == SP_NETAUDIO && a->jconf->input.netaudio_devname == NULL) {
    fprintf(stderr, "Error: NetAudio server name not specified\n");
    return(-1);
  }
#endif
  if (a->conf.adinnet_portnum != a->conf.adinnet_servnum) {
    /* if only one server, use default */
    if (a->conf.adinnet_servnum == 1) {
      if (a->conf.speech_output == SPOUT_ADINNET) {
	a->conf.adinnet_port[0] = ADINNET_PORT;
      } else if (a->conf.speech_output == SPOUT_VECTORNET) {
	a->conf.adinnet_port[0] = VECINNET_PORT;
      }
      a->conf.adinnet_portnum = 1;
    } else {
      fprintf(stderr, "Error: you should specify both server names and different port for each!\n");
      fprintf(stderr, "\tserver:");
      for (i = 0; i < a->conf.adinnet_servnum; i++) fprintf(stderr, " %s", a->conf.adinnet_serv[i]);
      fprintf(stderr, "\n\tport  :");
      for (i = 0; i < a->conf.adinnet_portnum; i++) fprintf(stderr, " %d", a->conf.adinnet_port[i]);
      fprintf(stderr, "\n");
      return(-1);
    }
  }

  if (a->conf.speech_output == SPOUT_VECTORNET) {
    if (a->conf.vecnet_paramtype == F_ERR_INVALID || a->conf.vecnet_veclen == 0) {
      fprintf(stderr, "Error: for \"-out vecnet\", both \"-paramtype\" and \"-veclen\" is required\n");
      return -1;
    }
  }

  /* apply Julius default parameters for unspecified acoustic parameters */
  if (a->jconf->am_root->analysis.para_htk.loaded == 1) {
    apply_para(&(a->jconf->am_root->analysis.para), &(a->jconf->am_root->analysis.para_htk));
  }
  apply_para(&(a->jconf->am_root->analysis.para), &(a->jconf->am_root->analysis.para_default));

  
  /* set final parameters considering the Julius defaults above */
  a->jconf->input.sfreq = a->jconf->am_root->analysis.para.smp_freq;
  a->jconf->input.period = a->jconf->am_root->analysis.para.smp_period;
  a->jconf->input.frameshift = a->jconf->am_root->analysis.para.frameshift;
  a->jconf->input.framesize = a->jconf->am_root->analysis.para.framesize;

  /* disable successive segmentation when no segmentation available */
  if (!a->jconf->detect.silence_cut) a->conf.continuous_segment = FALSE;
  /* store sampling rate locally */
  a->conf.sfreq = a->jconf->am_root->analysis.para.smp_freq;

  if (a->conf.speech_output == SPOUT_VECTORNET) {
    /* set parameters for feature extraction */
    if (vecnet_init(a->recog) == FALSE) exit(1);
  }
  
  if (a->conf.speech_output == SPOUT_FILE) {
    /* allocate work area for output file name */
    if (a->conf.continuous_segment) {
      a->outpath = (char *)mymalloc(strlen(a->conf.filename) + 10);
    } else {
      if (a->conf.use_raw) {
	a->outpath = a->conf.filename;
      } else {
	a->outpath = new_output_filename(a->conf.filename, ".wav");
      }
    }
  }
  if (a->conf.speech_output == SPOUT_STDOUT) {
    /* alway output in raw format at stdout */
    a->conf.use_raw = TRUE;
  }    
  if (a->jconf->input.speech_input == SP_ADINNET) {
    /* set adinnet input port number to Jconf */
    a->jconf->input.adinnet_port = a->conf.adinnet_port_in;
  }

  /**************************/
  /* display configurations */
  /**************************/
  put_status(a);

  /***************************/
  /* initialize input device */
  /***************************/
  if (j_adin_init(a->recog) == FALSE) {
    fprintf(stderr, "Error in initializing adin device\n");
    return -1;
  }

  if (a->conf.rewind_msec > 0) {
    /* allow adin module to keep triggered speech while pausing */
#ifdef HAVE_PTHREAD
    if (a->recog->adin->enable_thread) {
      a->recog->adin->ignore_speech_while_recog = FALSE;
    }
#endif
  }
  /*******************/
  /* enter main loop */
  /*******************/
  mainloop();

  return 0;
}

/* end of adintool.c */
