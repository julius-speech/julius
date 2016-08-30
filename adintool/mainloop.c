/*
 * Copyright (c) 1991-2016 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2016 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include "adintool.h"

extern AdinTool *global_a;

#ifdef USE_SDL
static int sdl_check_command();
#endif

/****************************/
/***** audio processing *****/
/****************************/

/*
 * functions to process the triggered audio data.
 *
 * they will be called for each triggered audio fragment.
 *
 * all output processing callback should have arguments:
 * 
 * @param now [in] recorded fragments of speech sample
 * @param len [in] length of above in samples
 * 
 * it should return -1 on device error (require caller to exit and
 * terminate input), 0 on success (allow caller to continue), 1 on
 * succeeded but segmentation detected (require caller to exit but
 * input will continue in the next call.
 */

// callback to store to file
static int
process_callback_file(SP16 *now, int len, Recog *recog)
{
  AdinTool *a = global_a;
  int count;
  int start;
  int w;

  /* do nothing if not on processing */
  if (a->on_processing == FALSE) return 0;

  start = 0;

  if (recog->jconf->input.speech_input == SP_MIC && a->speechlen == 0) {
    /* this is first up-trigger */
    if (a->conf.rewind_msec > 0 && !recog->adin->is_valid_data) {
      /* not spoken currently but has data to process at first trigger */
      /* it means that there are old spoken segments */
      /* disgard them */
      printf("disgard already recorded %d samples\n", len);
      return 0;
    }
    /* erase "<<<please speak>>>" text on tty */
    fprintf(stderr, "\r                    \r");
    if (a->conf.rewind_msec > 0) {
      /* when -rewind value set larger than 0, the speech data spoken
	 while pause will be considered back to the specified msec.
	 */
      printf("buffered samples=%d\n", len);
      w = a->conf.rewind_msec * a->conf.sfreq / 1000;
      if (len > w) {
	start = len - w;
	len = w;
      } else {
	start = 0;
      }
      printf("will process from %d\n", start);
    }
  }

  /* open files for recording at first trigger */
  if (a->conf.speech_output == SPOUT_FILE && a->speechlen == 0) {
    if (a->conf.continuous_segment) {
      if (a->conf.use_raw) {
	sprintf(a->outpath, "%s.%04d.raw", a->conf.filename, a->sid);
      } else {
	sprintf(a->outpath, "%s.%04d.wav", a->conf.filename, a->sid);
      }
    }
    fprintf(stderr,"[%s]", a->outpath);
    if (access(a->outpath, F_OK) == 0) {
      if (access(a->outpath, W_OK) == 0) {
	fprintf(stderr, "(override)");
      } else {
	perror("adintool");
	return(-1);
      }
    }
    if (a->conf.use_raw) {
      if ((a->fd = open(a->outpath, O_CREAT | O_RDWR
#ifdef O_BINARY
		     | O_BINARY
#endif
		     , 0644)) == -1) {
	perror("adintool");
	return -1;
      }
    } else {
      if ((a->fp = wrwav_open(a->outpath, a->conf.sfreq)) == NULL) {
	perror("adintool");
	return -1;
      }
    }
    a->writing_file = TRUE;
  }

  /* write recorded sample to file */
  if (a->conf.use_raw) {
    count = wrsamp(a->fd, &(now[start]), len);
    if (count < 0) {
      perror("adinrec: cannot write");
      return -1;
    }
    if (count < len * sizeof(SP16)) {
      fprintf(stderr, "adinrec: cannot write more %d bytes\ncurrent length = %lu\n", count, a->speechlen * sizeof(SP16));
      return -1;
    }
  } else {
    if (wrwav_data(a->fp, &(now[start]), len) == FALSE) {
      fprintf(stderr, "adinrec: cannot write\n");
      return -1;
    }
  }
  
  /* accumulate sample num of this segment */
  a->speechlen += len;

  /* if input length reaches limit, rehash the ad-in buffer */
  if (recog->jconf->input.speech_input == SP_MIC) {
    if (a->speechlen > MAXSPEECHLEN - 16000) {
      recog->adin->rehash = TRUE;
    }
  }
  
  /* progress bar in dots */
  fprintf(stderr, ".");

  return(0);
}

// callback to send to adinnet server(s)
static int
process_callback_adinnet(SP16 *now, int len, Recog *recog)
{
  AdinTool *a = global_a;
  int count;
  int start, w;
  int i;

  /* do nothing if not on processing */
  if (a->on_processing == FALSE) return 0;

  start = 0;

  if (recog->jconf->input.speech_input == SP_MIC && a->speechlen == 0) {
    /* this is first up-trigger */
    if (a->conf.rewind_msec > 0 && !recog->adin->is_valid_data) {
      /* not spoken currently but has data to process at first trigger */
      /* it means that there are old spoken segments */
      /* disgard them */
      printf("disgard already recorded %d samples\n", len);
      return 0;
    }
    /* erase "<<<please speak>>>" text on tty */
    fprintf(stderr, "\r                    \r");
    if (a->conf.rewind_msec > 0) {
      /* when -rewind value set larger than 0, the speech data spoken
	 while pause will be considered back to the specified msec.
	 */
      printf("buffered samples=%d\n", len);
      w = a->conf.rewind_msec * a->conf.sfreq / 1000;
      if (len > w) {
	start = len - w;
	len = w;
      } else {
	start = 0;
      }
      printf("will process from %d\n", start);
    }
  }

#ifdef WORDS_BIGENDIAN
  swap_sample_bytes(&(now[start]), len);
#endif
  for (i = 0; i < a->conf.adinnet_servnum; i++) {
    count = wt(a->sd[i], (char *)&(now[start]), len * sizeof(SP16));
    if (count < 0) {
      perror("adintool: cannot write");
      fprintf(stderr, "failed to send data to %s:%d\n", a->conf.adinnet_serv[i], a->conf.adinnet_port[i]);
    }
  }
#ifdef WORDS_BIGENDIAN
  swap_sample_bytes(&(now[start]), len);
#endif
  /* accumulate sample num of this segment */
  a->speechlen += len;
#ifdef HAVE_PTHREAD
  if (recog->adin->enable_thread) {
    /* if input length reaches limit, rehash the ad-in buffer */
    if (recog->adin->speechlen > MAXSPEECHLEN - 16000) {
      recog->adin->rehash = TRUE;
      fprintf(stderr, "+");
    }
  }
#endif
  
  /* display progress in dots */
  fprintf(stderr, ".");
  return(0);
}

// callback and functions to send feature vector to adinnet server(s)
// initialization
boolean
vecnet_init(Recog *recog)
{
  AdinTool *a = global_a;
  Jconf *jconf = recog->jconf;
  JCONF_AM *amconf = jconf->am_root;
  MFCCCalc *mfcc;
  PROCESS_AM *am;
  
  am = j_process_am_new(recog, amconf);
  calc_para_from_header(&(jconf->am_root->analysis.para), a->conf.vecnet_paramtype, a->conf.vecnet_veclen);

  /* from j_final_fusion() */
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    create_mfcc_calc_instances(recog);
  }
  for(mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
    mfcc->param = new_param();
  }
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    for(mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
      if (mfcc->frontend.sscalc) {
        mfcc->frontend.mfccwrk_ss = WMP_work_new(mfcc->para);
        if (mfcc->frontend.mfccwrk_ss == NULL) {
          return FALSE;
        }
        if (mfcc->frontend.sscalc_len * recog->jconf->input.sfreq / 1000 < mfcc->para->framesize) {
          return FALSE;
        }
      }
    }
  }
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    if (RealTimeInit(recog) == FALSE) {
      fprintf(stderr, "Error: failed to initialize feature extraction module\n");
      return FALSE;
    }
  }

  return TRUE;
}

// sub function to send a data to socket
static int
vecnet_send_data(int sd, void *buf, int bytes)
{
  /* send data size header (4 byte) */
  if (send(sd, (void *)&bytes, sizeof(int), 0) != sizeof(int)) {
    fprintf(stderr, "Error: failed to send %lu bytes\n", sizeof(int));
    return -1;
  }

  /* send data body */
  if (send(sd, buf, bytes, 0) != bytes) {
    fprintf(stderr, "Error: failed to send %lu bytes\n", sizeof(int));
    return -1;
  }

  return 0;
}

// HTK parameter file header
typedef struct {
  int veclen;                 ///< (4 byte)Vector length of an input
  int fshift;                 ///< (4 byte) Frame shift in msec of the vector
  char outprob_p;             ///< (1 byte) != 0 if input is outprob vector
} ConfigurationHeader;

// send HTK parameter header to adinnet server
static void
vecnet_send_header(Recog *recog)
{
  AdinTool *a = global_a;
  ConfigurationHeader conf;
  int i;

  conf.veclen = recog->jconf->am_root->analysis.para.veclen;
  conf.fshift = 1000.0 * recog->jconf->am_root->analysis.para.frameshift / recog->jconf->am_root->analysis.para.smp_freq;
  conf.outprob_p = 0;		/* feature output */
  for (i=0; i < a->conf.adinnet_servnum; i++) {
    vecnet_send_data(a->sd[i], &conf, sizeof(ConfigurationHeader));
  }
}

// prepare for feature extraction
static boolean
vecnet_prepare(Recog *recog)
{
  AdinTool *a = global_a;
  RealBeam *r = &(recog->real);
  MFCCCalc *mfcc;

  r->windownum = 0;
  for(mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
    mfcc->param->veclen = a->conf.vecnet_veclen;
    if (mfcc->para->cmn || mfcc->para->cvn) CMN_realtime_prepare(mfcc->cmn.wrk);
    param_alloc(mfcc->param, 1, mfcc->param->veclen);
    mfcc->f = 0;
  }
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    reset_mfcc(recog);
  }
  recog->triggered = FALSE;

  return TRUE;
  
}

// advance calculation of feature vectors and send the new vectors to adinnet
static void
vecnet_sub(SP16 *Speech, int nowlen, Recog *recog)
{
  AdinTool *a = global_a;
  int i, j, now;
  MFCCCalc *mfcc;
  RealBeam *r = &(recog->real);

  now = 0;
  
  while (now < nowlen) {
    for(i = min(r->windowlen - r->windownum, nowlen - now); i > 0 ; i--)
      r->window[r->windownum++] = (float) Speech[now++];
    if (r->windownum < r->windowlen) break;
    for (mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
      mfcc->valid = FALSE;
      if (RealTimeMFCC(mfcc, r->window, r->windowlen)) {
        mfcc->valid = TRUE;
	param_alloc(mfcc->param, mfcc->f + 1, mfcc->param->veclen);
	memcpy(mfcc->param->parvec[mfcc->f], mfcc->tmpmfcc, sizeof(VECT) * mfcc->param->veclen);
#if 0
	{
	  int i;
	  for (i = 0; i < a->conf.vecnet_veclen; i++) {
	    printf(" %f", mfcc->tmpmfcc[i]);
	  }
	  printf("\n");
	}
#endif
	/* send 1 frame */
	for (j = 0; j < a->conf.adinnet_servnum; j++) {
	  vecnet_send_data(a->sd[j], mfcc->tmpmfcc, sizeof(VECT) * a->conf.vecnet_veclen);
	}
	mfcc->f++;
      }
    }
    /* shift window */
    memmove(r->window, &(r->window[recog->jconf->input.frameshift]), sizeof(SP16) * (r->windowlen - recog->jconf->input.frameshift));
    r->windownum -= recog->jconf->input.frameshift;
  }
}

// finish feature calculation at end of audio segment
static void
vecnet_param_update(Recog *recog)
{
  MFCCCalc *mfcc;

  for (mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
    mfcc->param->header.samplenum = mfcc->f;
    mfcc->param->samplenum = mfcc->f;
  }
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    for(mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
      if (mfcc->f > 0 && mfcc->para && mfcc->para->cmn) {
	if (mfcc->cmn.update) {
	  CMN_realtime_update(mfcc->cmn.wrk, mfcc->param);
	}
	if (mfcc->cmn.save_filename) {
	  CMN_save_to_file(mfcc->cmn.wrk, mfcc->cmn.save_filename);
	}
      }
    }
  }
}

// main callback to successively calculate feature vectors and send to adinnet
static int
process_callback_vecnet(SP16 *now, int len, Recog *recog)
{
  AdinTool *a = global_a;
  int start, w;

  /* do nothing if not on processing */
  if (a->on_processing == FALSE) return 0;

  start = 0;

  if (recog->jconf->input.speech_input == SP_MIC && a->speechlen == 0) {
    /* this is first up-trigger */
    if (a->conf.rewind_msec > 0 && !recog->adin->is_valid_data) {
      /* not spoken currently but has data to process at first trigger */
      /* it means that there are old spoken segments */
      /* disgard them */
      printf("disgard already recorded %d samples\n", len);
      return 0;
    }
    /* erase "<<<please speak>>>" text on tty */
    fprintf(stderr, "\r                    \r");
    if (a->conf.rewind_msec > 0) {
      /* when -rewind value set larger than 0, the speech data spoken
	 while pause will be considered back to the specified msec.
	 */
      printf("buffered samples=%d\n", len);
      w = a->conf.rewind_msec * a->conf.sfreq / 1000;
      if (len > w) {
	start = len - w;
	len = w;
      } else {
	start = 0;
      }
      printf("will process from %d\n", start);
    }
  }

  vecnet_sub(&(now[start]), len, recog);

  /* accumulate sample num of this segment */
  a->speechlen += len;
#ifdef HAVE_PTHREAD
  if (recog->adin->enable_thread) {
    /* if input length reaches limit, rehash the ad-in buffer */
    if (recog->adin->speechlen > MAXSPEECHLEN - 16000) {
      recog->adin->rehash = TRUE;
      fprintf(stderr, "+");
    }
  }
#endif
  
  /* display progress in dots */
  fprintf(stderr, ".");
  return(0);
}

// send end of segment to adinnet, cause adinnet to segment input
static void
vecnet_send_end_of_segment()
{
  AdinTool *a = global_a;
  int i, j;
  
  /* send header value of '0' as an end-of-utterance marker */
  i = 0;
  for (j = 0; j < a->conf.adinnet_servnum; j++) {
    if (send(a->sd[j], (void *)&i, sizeof(int), 0) != sizeof(int)) {
      fprintf(stderr, "Error: failed to send %lu bytes\n", sizeof(int));
      return ;
    }
  }
}

// send end of session to adinnet, cause adinnet to stop input
static void
vecnet_send_end_of_session()
{
  AdinTool *a = global_a;
  int i, j;

  /* send negative header value as an end-of-session marker */
  i = -1;
  for (j = 0; j < a->conf.adinnet_servnum; j++) {
    if (send(a->sd[j], (void *)&i, sizeof(int), 0) != sizeof(int)) {
      fprintf(stderr, "Error: failed to send %lu bytes\n", sizeof(int));
      return;
    }
  }
}

/**********************************************************************/
/** 
 * Send end-of-segment singal to adinnet server.
 * 
 * </EN>
 */
static void
adin_send_end_of_segment()
{
  AdinTool *a = global_a;
  char p;
  int i;

  for(i = 0; i < a->conf.adinnet_servnum; i++) {
    if (wt(a->sd[i], &p,  0) < 0) {
      perror("adintool: cannot write");
      fprintf(stderr, "failed to send EOS to %s:%d\n", a->conf.adinnet_serv[i], a->conf.adinnet_port[i]);
    }
  }
}


/**********************************************************************/

/**************************/
/***** process events *****/
/**************************/

/* reveice resume/pause command from adinnet server */
/* (for SPOUT_ADINNET only) */
/*'1' ... resume  '0' ... pause */

/** 
 * Callback function for A/D-in processing to check pause/resume
 * command from adinnet server.
 * 
 * @return 0 when no command or RESUME command to tell caller to
 * continue recording, -1 when received a PAUSE command and tell caller to
 * stop recording, or -2 to tell caller to stop recording immediately
 */
static int
adinnet_check_command()
{
  AdinTool *a = global_a;
  fd_set rfds;
  struct timeval tv;
  int status;
  int cnt, ret;
  char com;
  int i, max_sd;

#ifdef USE_SDL
  ret = sdl_check_command();
  if (ret < 0)
    return ret;
#endif

  /* do nothing if not on processing */
  if (a->on_processing == FALSE) return 0;

  /* check if some commands are waiting in queue */
  FD_ZERO(&rfds);
  max_sd = 0;
  for(i = 0; i < a->conf.adinnet_servnum; i++) {
    if (max_sd < a->sd[i]) max_sd = a->sd[i];
    FD_SET(a->sd[i], &rfds);
  }
  tv.tv_sec = 0;
  tv.tv_usec = 1;
  status = select(max_sd+1, &rfds, NULL, NULL, &tv);
  if (status < 0) {           /* error */
    fprintf(stderr, "adintool: cannot check command from adinnet server\n");
    a->process_error = TRUE;
    return -2;                        /* error return */
  }
  if (status > 0) {           /* there are some data */
    for (i = 0; i < a->conf.adinnet_servnum; i++) {
      if (FD_ISSET(a->sd[i], &rfds)) {
	ret = rd(a->sd[i], &com, &cnt, 1); /* read in command */
	if (ret == -1) {
	  /* error */
	  a->process_error = TRUE;
	  close_socket(a->sd[i]);
	  return -2;
	}
	switch (com) {
	case '0':                       /* pause */
	  fprintf(stderr, "<#%d: PAUSE>\n", i+1);
	  a->stop_at_next = TRUE;	/* mark to pause at the end of this input */
	  /* tell caller to stop recording */
	  return -1;
	case '1':                       /* resume */
	  fprintf(stderr, "<#%d: RESUME - already running, ignored>\n", i+1);
	  /* we are already running, so just continue */
	  break;
	case '2':                       /* terminate */
	  fprintf(stderr, "<#%d: TERMINATE>\n", i+1);
	  a->stop_at_next = TRUE;	/* mark to pause at the end of this input */
	  /* tell caller to stop recording immediately */
	  a->process_error = FALSE;
	  return -2;
	  break;
	default:
	  fprintf(stderr, "adintool: unknown command from #%d: %d\n", i+1,com);
	  a->unknown_command_counter++;
	  /* avoid infinite loop in that case... */
	  /* this may happen when connection is terminated from server side  */
	  if (a->unknown_command_counter > 100) {
	    fprintf(stderr, "killed by a flood of unknown commands from server\n");
	    exit(1);
	  }
	}
      }
    }
  }
  return 0;			/* continue ad-in */
}

/** 
 * Wait for resume command from server.
 * 
 */
static int
adinnet_wait_command()
{
  AdinTool *a = global_a;
  fd_set rfds;
  int status;
  int cnt, ret;
  char com;
  int i, count, max_sd;
#ifdef USE_SDL
  struct timeval tv;
#endif

#ifdef USE_SDL
  ret = sdl_check_command();
  if (ret < 0)
    return ret;
#endif

  /* do nothing if not on processing */
  if (a->on_processing == FALSE) return 0;

  fprintf(stderr, "<<< waiting RESUME >>>");

  while(1) {
    /* check for synchronized resume */
    if (a->conf.loose_sync) {
      for(i = 0; i < a->conf.adinnet_servnum; i++) {
	if (a->resume_count[i] == 0) break;
      }
      if (i >= a->conf.adinnet_servnum) { /* all count > 0 */
	for(i = 0; i < a->conf.adinnet_servnum; i++) a->resume_count[i] = 0;
	fprintf(stderr, ">>RESUME\n");
	a->process_error = FALSE;
	return -2;                       /* restart recording */
      }
    } else {
      /* force same resume count among servers */
      count = a->resume_count[0];
      for(i = 1; i < a->conf.adinnet_servnum; i++) {
	if (count != a->resume_count[i]) break;
      }
      if (i >= a->conf.adinnet_servnum && count > 0) {
	/* all resume counts are the same, actually resume */
	for(i = 0; i < a->conf.adinnet_servnum; i++) a->resume_count[i] = 0;
	fprintf(stderr, ">>RESUME\n");
	a->process_error = FALSE;
	return -2;                       /* restart recording */
      }
    }
    /* not all host send me resume command */
    FD_ZERO(&rfds);
    max_sd = 0;
    for(i = 0; i < a->conf.adinnet_servnum; i++) {
      if (max_sd < a->sd[i]) max_sd = a->sd[i];
      FD_SET(a->sd[i], &rfds);
    }
#ifdef USE_SDL
    tv.tv_sec = 0;
    tv.tv_usec = 1;
    status = select(max_sd+1, &rfds, NULL, NULL, &tv);
#else
    status = select(max_sd+1, &rfds, NULL, NULL, NULL); /* block when no command */
#endif
    if (status < 0) {         /* error */
      fprintf(stderr, "adintool: cannot check command from adinnet server\n");
      a->process_error = TRUE;
      return -2;                      /* error return */
    } else {                  /* there are some data */
      for(i = 0; i < a->conf.adinnet_servnum; i++) {
	if (FD_ISSET(a->sd[i], &rfds)) {
	  ret = rd(a->sd[i], &com, &cnt, 1);
	  if (ret == -1) {
	    /* error */
	    a->process_error = TRUE;
	    return -2;
	  }
	  switch (com) {
	  case '0':                       /* pause */
	    /* already paused, so just wait for next command */
	    if (a->conf.loose_sync) {
	      fprintf(stderr, "<#%d: PAUSE - already paused, reset sync>\n", i+1);
	      for(i = 0; i < a->conf.adinnet_servnum; i++) a->resume_count[i] = 0;
	    } else {
	      fprintf(stderr, "<#%d: PAUSE - already paused, ignored>\n", i+1);
	    }
	    break;
	  case '1':                       /* resume */
	    /* do resume */
	    a->resume_count[i]++;
	    if (a->conf.loose_sync) {
	      fprintf(stderr, "<#%d: RESUME>\n", i+1);
	    } else {
	      fprintf(stderr, "<#%d: RESUME @%d>\n", i+1, a->resume_count[i]);
	    }
	    break;
	  case '2':                       /* terminate */
	    /* already paused, so just wait for next command */
	    if (a->conf.loose_sync) {
	      fprintf(stderr, "<#%d: TERMINATE - already paused, reset sync>\n", i+1);
	      for(i = 0;i < a->conf.adinnet_servnum; i++) a->resume_count[i] = 0;
	    } else {
	      fprintf(stderr, "<#%d: TERMINATE - already paused, ignored>\n", i+1);
	    }
	    break;
	  default:
	    fprintf(stderr, "adintool: unknown command from #%d: %d\n", i+1, com);
	    a->unknown_command_counter++;
	    /* avoid infinite loop in that case... */
	    /* this may happen when connection is terminated from server side  */
	    if (a->unknown_command_counter > 100) {
	      fprintf(stderr, "killed by a flood of unknown commands from server\n");
	      a->process_error = TRUE;
	      return -2;
	    }
	  }
	}
      }
    }
#ifdef USE_SDL
    break;
#endif
    
  }
  return 0;
} 

/* close file */
static boolean
close_files()
{
  AdinTool *a = global_a;

  if (a->writing_file) {
    if (a->conf.use_raw) {
      if (close(a->fd) != 0) {
	perror("adinrec");
	return FALSE;
      }
    } else {
      if (wrwav_close(a->fp) == FALSE) {
	fprintf(stderr, "adinrec: failed to close file\n");
	return FALSE;
      }
    }
    printf("%s: %d samples (%.2f sec.) [%6d (%5.2fs) - %6d (%5.2fs)]\n", 
	   a->outpath, a->speechlen, 
	   (float)a->speechlen / (float)a->conf.sfreq,
	   a->trigger_sample, (float)a->trigger_sample / (float)a->conf.sfreq, 
	   a->trigger_sample + a->speechlen, (float)(a->trigger_sample + a->speechlen) / (float)a->conf.sfreq);
    
    a->writing_file = FALSE;
  }

  return TRUE;
}  

// open connection to output device (adinnet)
static int
connect_to_output_device()
{
  AdinTool *a = global_a;
  Recog *recog = a->recog;
  int i;

  if (a->on_processing == TRUE) return 0;

  if (a->conf.speech_output == SPOUT_NONE) return 0;


  if (a->conf.speech_output == SPOUT_ADINNET || a->conf.speech_output == SPOUT_VECTORNET) {
    /* connect to adinnet server(s) */
    for(i = 0; i < a->conf.adinnet_servnum; i++) {
      fprintf(stderr, "connecting to #%d (%s:%d)...", i+1, a->conf.adinnet_serv[i], a->conf.adinnet_port[i]);
      a->sd[i] = make_connection(a->conf.adinnet_serv[i], a->conf.adinnet_port[i]);
      if (a->sd[i] < 0) return -1;	/* on error */
    }
    fprintf(stderr, "connected\n");
  } else if (a->conf.speech_output == SPOUT_STDOUT) {
    /* output to stdout */
    a->fd = 1;
  }
  if (a->conf.speech_output == SPOUT_VECTORNET) {
    vecnet_send_header(recog);
  }

  a->on_processing = TRUE;
  
  return 0;
}

// close processing
static void
close_processing(Recog *recog)
{
  AdinTool *a = global_a;
  int i;
  
  if (a->on_processing == FALSE) return;
  
  if (a->conf.speech_output == SPOUT_FILE) {
    close_files();
  }
  if (a->conf.speech_output == SPOUT_VECTORNET) {
    vecnet_send_end_of_session();
    vecnet_param_update(recog);
  }

  if (a->conf.speech_output == SPOUT_ADINNET || a->conf.speech_output == SPOUT_VECTORNET) {
    for(i = 0; i < a->conf.adinnet_servnum; i++) {
      close_socket(a->sd[i]);
    }
  }
  
  a->on_processing = FALSE;
}

#ifdef USE_SDL

// get statistics and draw waveform and other informations on window
static void draw_wave(Recog *recog, SP16 *now, int len, void *data)
{
  AdinTool *a = global_a;
  SDLDATA *s = &(global_a->sdl);
  int freq = recog->jconf->input.sfreq;
  int i, j, k, m;
  SDL_Rect viewport;
  SDL_Rect r;
  int miny, startx;
  boolean thres_moving = FALSE;

#ifdef HAVE_PTHREAD
  pthread_mutex_lock(&(s->mutex));
#endif
  
  if (s->window == NULL) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      fprintf(stderr, "SDL could not initialize: %s\n", SDL_GetError());
      exit(1);
    }
    s->window = SDL_CreateWindow("adintool", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (s->window == NULL) {
      fprintf(stderr, "SDL window could not be created: %s\n", SDL_GetError());
      exit(1);
    }
    //screenSurface = SDL_GetWindowSurface(window);
    //  renderer = SDL_CreateSoftwareRenderer(screenSurface);
    s->renderer = SDL_CreateRenderer(s->window, -1, SDL_RENDERER_ACCELERATED);
    if (s->renderer == NULL) {
      fprintf(stderr, "SDL renderer could not be created: %s\n", SDL_GetError());
      exit(1);
    }
  }

  if (s->tickbuf == NULL) {
    s->ticklen = freq * WAVE_TICK_TIME_MSEC / 1000;
    s->tickbuf = (SP16 *)malloc(sizeof(SP16) * s->ticklen);
    s->tickbp = 0;
  }

  // get window size
  SDL_RenderGetViewport(s->renderer, &viewport);
  // update if window size changes
  if (s->maxlevel == NULL || s->window_w != viewport.w) {
    s->window_w = viewport.w;
    s->items = s->window_w / WAVE_TICK_WIDTH;
    if (s->maxlevel != NULL) {
      free(s->maxlevel);
      free(s->minlevel);
      free(s->flag);
#ifdef AUTO_ADJUST_THRESHOLD
      free(s->mean);
      free(s->var);
      free(s->meanofmean);
      free(s->validmean);
      free(s->varofmean);
      free(s->triggerrate);
#endif
      free(s->rects);
      free(s->rectflags);
    }
    s->maxlevel = (float *)malloc(sizeof(float) * s->items);
    s->minlevel = (float *)malloc(sizeof(float) * s->items);
    s->flag = (short *)malloc(sizeof(short) * s->items);
#ifdef AUTO_ADJUST_THRESHOLD
    s->mean = (float *)malloc(sizeof(float) * s->items);
    s->var = (float *)malloc(sizeof(float) * s->items);
    s->meanofmean = (float *)malloc(sizeof(float) * s->items);
    s->validmean = (float *)malloc(sizeof(float) * s->items);
    s->varofmean = (float *)malloc(sizeof(float) * s->items);
    s->triggerrate = (float *)malloc(sizeof(float) * s->items);
#endif
    s->rects = (SDL_Rect *)malloc(sizeof(SDL_Rect) * s->items);
    s->rectflags = (short *)malloc(sizeof(short) * s->items);
    for (i = 0; i < s->items; i++) {
      s->maxlevel[i] = 0.0;
      s->minlevel[i] = 0.0;
      s->flag[i] = 0;
#ifdef AUTO_ADJUST_THRESHOLD
      s->mean[i] = 0.0;
      s->var[i] = 0.0;
      s->meanofmean[i] = 0.0;
      s->validmean[i] = 0.0;
      s->varofmean[i] = 0.0;
      s->triggerrate[i] = 0.0;
#endif
    }
    s->bp = 0;
  }

  // calculate max/min per tick
  j = 0;
  while(1) {
    for (i = s->tickbp; i < s->ticklen; i++, j++) {
      if (j >= len) break;
      s->tickbuf[i] = now[j];
    }
    if (i < s->ticklen) {
      s->tickbp = i;
      break;
    } else {
      s->maxlevel[s->bp] = s->minlevel[s->bp] = 0.0;
      for (i = 0; i < s->ticklen; i++) {
	if (s->maxlevel[s->bp] < s->tickbuf[i]) {
	  s->maxlevel[s->bp] = s->tickbuf[i];
	}
	if (s->minlevel[s->bp] > s->tickbuf[i]) {
	  s->minlevel[s->bp] = s->tickbuf[i];
	}
      }
      s->maxlevel[s->bp] /= 32768.0;
      s->minlevel[s->bp] /= 32768.0;
      s->flag[s->bp] = 0;
      if (s->is_valid_flag == 0 && recog->adin->is_valid_data == TRUE) {
	// trigger up, mark previous head-margin
	for (m = (recog->adin->zc.valid_len * 1000 / freq) / WAVE_TICK_TIME_MSEC; m > 0; m--) {
	  k = s->bp - m;
	  if (k < 0) k += s->items;
	  s->flag[k] |= WAVE_TICK_FLAG_PROCESSED;
	}
      }
      if (s->is_valid_flag == 1 && recog->adin->is_valid_data == FALSE) {
	// trigger down
	s->flag[s->bp] |= WAVE_TICK_FLAG_TRIGGER;
      }
      if (recog->adin->is_valid_data == TRUE) {
	s->flag[s->bp] |= WAVE_TICK_FLAG_PROCESSED;
      }
      s->is_valid_flag = (recog->adin->is_valid_data == TRUE) ? 1 : 0;

#ifdef AUTO_ADJUST_THRESHOLD
      /* get mean and variance for maximum levels of last AUTORHES_WINDOW_SEC seconds */
      {
	int i, j;
	float mean = 0.0;
	float var = 0.0;
	int windowlen = (int)(AUTOTHRES_WINDOW_SEC * 1000.0 / WAVE_TICK_TIME_MSEC);
	if (windowlen > s->items) windowlen = s->items;
	if (windowlen > s->totaltick) windowlen = s->totaltick;
	
	j = s->bp;
	for(i = 0; i < windowlen; i++) {
	  mean += s->maxlevel[j];
	  if (--j < 0) j += s->items;
	}
	mean /= windowlen;
	j = s->bp;
	for(i = 0; i < windowlen; i++) {
	  var += (s->maxlevel[j] - mean) * (s->maxlevel[j] - mean);
	  if (--j < 0) j += s->items;
	}
	var /= windowlen;
	s->mean[s->bp] = mean;
	s->var[s->bp] = sqrt(var);
      }
      /* get long mean, long variance of variance, and trigger rate of last AUTOTHRES_STABLE_SEC seconds */
      {
	int i, j, c;
	float mean = 0.0;
	float var = 0.0;
	float varmean = 0.0;
	float triggerrate = 0.0;
	int windowlen = (int)(AUTOTHRES_STABLE_SEC * 1000.0 / WAVE_TICK_TIME_MSEC);
	if (windowlen > s->items) windowlen = s->items;
	if (windowlen > s->totaltick) windowlen = s->totaltick;
	j = s->bp;
	mean = 0.0;
	for(i = 0; i < windowlen; i++) {
	  mean += s->mean[j];
	  if (--j < 0) j += s->items;
	}
	mean /= windowlen;
	s->meanofmean[s->bp] = mean;
	j = s->bp;
	varmean = 0.0;
	for(i = 0; i < windowlen; i++) {
	  varmean += (s->mean[j] - mean) * (s->mean[j] - mean);
	  if (--j < 0) j += s->items;
	}
	varmean /= windowlen;
	s->varofmean[s->bp] = sqrt(varmean);
	j = s->bp;
	c = 0;
	mean = 0.0;
	for(i = 0; i < windowlen; i++) {
	  if (s->varofmean[j] < s->vvthres1) {
	    mean += s->mean[j];
	    c++;
	  }
	  if (--j < 0) j += s->items;
	}
	if (c > 0) mean /= c;
	s->validmean[s->bp] = mean;
	windowlen = (int)(AUTOTHRES_DOWN_SEC * 1000.0 / WAVE_TICK_TIME_MSEC);
	if (windowlen > s->items) windowlen = s->items;
	if (windowlen > s->totaltick) windowlen = s->totaltick;
	j = s->bp;
	for(i = 0; i < windowlen; i++) {
	  triggerrate += (s->flag[j] & WAVE_TICK_FLAG_PROCESSED) ? 1.0 : 0.0;
	  if (--j < 0) j += s->items;
	}
	triggerrate /= windowlen;
	s->triggerrate[s->bp] = triggerrate;
      }
#endif /* AUTO_ADJUST_THRESHOLD */
      s->bp++;
      if (s->bp >= s->items) {
	s->bp -= s->items;
      }
      s->tickbp = 0;

      s->totaltick++;
    }
  }
    
#ifdef AUTO_ADJUST_THRESHOLD

  j = s->bp - 1;
  if (j < 0) j += s->items;

  /* update vvthres */
  s->vvthres1 = AUTOTHRES_ADAPT_THRES_1 / (- log(s->meanofmean[j]));
  s->vvthres2 = AUTOTHRES_ADAPT_THRES_2 / (- log(s->meanofmean[j]));

  if (s->totaltick < (int)(AUTOTHRES_START_IGNORE_SEC * 1000.0 / WAVE_TICK_TIME_MSEC)) {
    /* for first AUTOTHRS_START_IGNORE_SEC, keep high threshold */
    recog->adin->thres = 32767;
    recog->jconf->detect.level_thres = recog->adin->thres;
    recog->adin->zc.trigger = recog->adin->thres;
  } else {
    float thres;
    /* get long mean, long variance of variance, and trigger rate of last AUTOTHRES_STABLE_SEC seconds */
    if (s->totaltick < (int)((AUTOTHRES_START_IGNORE_SEC + 1.0) * 1000.0 / WAVE_TICK_TIME_MSEC)) {
      /* in seconds after AUTOTHRES_START_IGNORE_SEC, force adjustent */
      thres = (s->meanofmean[j] - s->varofmean[j]) * 32768.0 * 2.3;
      if (thres > THRESHOLD_ADJUST_MAX)
	thres = THRESHOLD_ADJUST_MAX;
      if (thres < THRESHOLD_ADJUST_MIN)
	thres = THRESHOLD_ADJUST_MIN;
      recog->adin->thres = recog->adin->thres * (1.0 - AUTOTHRES_ADAPT_SPEED_COEF * 2.0) + thres * AUTOTHRES_ADAPT_SPEED_COEF * 2.0;
      recog->jconf->detect.level_thres = recog->adin->thres;
      recog->adin->zc.trigger = recog->adin->thres;
      thres_moving = TRUE;
    } else {
      if (s->varofmean[j] < s->vvthres1 && s->triggerrate[j] > 0.9) {
	/* threshold goes up to avoid triggering loud noise */
	thres = (s->validmean[j] + s->varofmean[j] * 2.0) * 32768.0 * 2.0;
	if (thres > THRESHOLD_ADJUST_MAX)
	  thres = THRESHOLD_ADJUST_MAX;
	if (thres < THRESHOLD_ADJUST_MIN)
	  thres = THRESHOLD_ADJUST_MIN;
	recog->adin->thres = recog->adin->thres * (1.0 - AUTOTHRES_ADAPT_SPEED_COEF) + thres * AUTOTHRES_ADAPT_SPEED_COEF;
	recog->jconf->detect.level_thres = recog->adin->thres;
	recog->adin->zc.trigger = recog->adin->thres;
	thres_moving = TRUE;
      } else if (s->varofmean[j] > s->vvthres2 && s->triggerrate[j] < 0.001) {
	/* threshold goes down to capture quiet speech */
	thres = (s->validmean[j] + s->varofmean[j] * 2.0) * 32768.0 * 1.7;
	if (thres > THRESHOLD_ADJUST_MAX)
	  thres = THRESHOLD_ADJUST_MAX;
	if (thres < THRESHOLD_ADJUST_MIN)
	  thres = THRESHOLD_ADJUST_MIN;
	recog->adin->thres = recog->adin->thres * (1.0 - AUTOTHRES_ADAPT_SPEED_COEF) + thres * AUTOTHRES_ADAPT_SPEED_COEF;
	recog->jconf->detect.level_thres = recog->adin->thres;
	recog->adin->zc.trigger = recog->adin->thres;
	thres_moving = TRUE;
      }	
    }
  }
#endif /* AUTO_ADJUST_THRESHOLD */

  // clear screen
  if (recog->jconf->preprocess.level_coef == 1.0f) {
    // fill black
    SDL_SetRenderDrawColor(s->renderer, 0, 0, 0, 0xFF);
  } else {
    // mute, fill in red
    SDL_SetRenderDrawColor(s->renderer, 120, 0, 0, 255);
  }
  SDL_RenderClear(s->renderer);

#ifdef AUTO_ADJUST_THRESHOLD
  /* draw varofmean */
  j = s->bp;
  for(i = 0; i < s->items; i++) {
    s->rects[i].w = WAVE_TICK_WIDTH;
    s->rects[i].h = (s->varofmean[j] * 20.0) * viewport.h * 0.1;
    s->rects[i].x = WAVE_TICK_WIDTH * i;
    s->rects[i].y = viewport.h * 0.1 - s->rects[i].h;
    j++;
    if (j >= s->items) j -= s->items;
  }
  SDL_SetRenderDrawColor(s->renderer, 128, 20, 30, 255);
  SDL_RenderFillRects(s->renderer, &(s->rects[0]), s->items);
#endif
  
  // draw connected rect
  if (a->on_processing == TRUE) {
    r.x = viewport.w - 70;
    r.y = 20;
    r.w = 50;
    r.h = 50;
    SDL_SetRenderDrawColor(s->renderer, 255, 0, 0, 255);
    if (a->on_pause == TRUE) {
      SDL_RenderDrawRect(s->renderer, &r);
    } else {
      SDL_RenderFillRect(s->renderer, &r);
    }
  }
 
  // draw threshold bar
  float y1 = (1.0 - (float)recog->jconf->detect.level_thres / 32768.0) * viewport.h * 0.5;
  float y2 = (1.0 + (float)recog->jconf->detect.level_thres / 32768.0) * viewport.h * 0.5;
  if (thres_moving) {
    r.x = 0;
    r.y = y1;
    r.w = viewport.w;
    r.h = y2 - y1;
    SDL_SetRenderDrawColor(s->renderer, 160, 160, 0, 128);
    SDL_RenderFillRect(s->renderer, &r);
  } else {
    SDL_SetRenderDrawColor(s->renderer, 160, 160, 0, 255);
    SDL_RenderDrawLine(s->renderer, 0, (int)y1, viewport.w, (int)y1);
    SDL_RenderDrawLine(s->renderer, 0, (int)y2, viewport.w, (int)y2);
  }
  
  // draw waveform
  j = s->bp;
  for(i = 0; i < s->items; i++) {
    s->rects[i].x = WAVE_TICK_WIDTH * i;
    s->rects[i].y = (int)((1.0 - s->maxlevel[j]) * viewport.h * 0.5);
    s->rects[i].w = WAVE_TICK_WIDTH;
    s->rects[i].h = (int)((1.0 - s->minlevel[j]) * viewport.h * 0.5) - s->rects[i].y;
    s->rectflags[i] = s->flag[j];
    j++;
    if (j >= s->items) j -= s->items;
  }
  m = s->rectflags[0] & WAVE_TICK_FLAG_PROCESSED;
  k = 0;
  miny = viewport.h;
  startx = 0;
  for(i = 0; i < s->items; i++) {
    if ((s->rectflags[i] & WAVE_TICK_FLAG_PROCESSED) != m) {
      SDL_SetRenderDrawColor(s->renderer, 255 * m, 128, 255 - 128 * m, 255);
      SDL_RenderFillRects(s->renderer, &(s->rects[k]), i - k);
      m = s->rectflags[i];
      if ((s->rectflags[i] & WAVE_TICK_FLAG_PROCESSED) != 0) {
	startx = i;
	miny = viewport.h;
      }
      k = i;
    }
    if ((s->rectflags[i] & WAVE_TICK_FLAG_TRIGGER) != 0) {
      SDL_SetRenderDrawColor(s->renderer, 255, 255, 255, 255);
      r.x = s->rects[startx].x;
      r.y = miny;
      r.w = s->rects[i].x - s->rects[startx].x;
      r.h = viewport.h - miny * 2;
      SDL_RenderDrawRect(s->renderer, &r);
      miny = viewport.h;
      startx = i;
    }
    if (miny > s->rects[i].y)
      miny = s->rects[i].y;
    if (miny > viewport.h - (s->rects[i].y + s->rects[i].h))
      miny = viewport.h - (s->rects[i].y + s->rects[i].h);
  }
  SDL_SetRenderDrawColor(s->renderer, 255 * m, 128, 255 - 128 * m, 255);
  SDL_RenderFillRects(s->renderer, &(s->rects[k]), s->items - k);

#ifdef AUTO_ADJUST_THRESHOLD

  /* draw last mean/var box */
  j = s->bp - 1;
  if (j < 0) j += s->items;
  {
    int windowlen = (int)(AUTOTHRES_WINDOW_SEC * 1000.0 / WAVE_TICK_TIME_MSEC);
    if (windowlen > s->items) windowlen = s->items;
    if (windowlen > s->totaltick) windowlen = s->totaltick;
    SDL_SetRenderDrawColor(s->renderer, 0, 255, 0, 255);
    r.x = viewport.w - WAVE_TICK_WIDTH * windowlen;
    r.y = (int)((1.0 - (s->mean[j] + s->var[j])) * viewport.h * 0.5);
    r.w = WAVE_TICK_WIDTH * windowlen;
    r.h = (s->var[j] * 2.0) * viewport.h * 0.5;
    if (r.h == 0) r.h = 1;
    SDL_RenderDrawRect(s->renderer, &r);
  }

#endif /* AUTO_ADJUST_THRESHOLD */

  SDL_RenderPresent(s->renderer);

#ifdef HAVE_PTHREAD
  pthread_mutex_unlock(&(s->mutex));
#endif

}

// check events on SDL
static int
sdl_check_command()
{
  AdinTool *a = global_a;
  SDLDATA *s = &(global_a->sdl);
  Recog *recog = a->recog;
  SDL_Event event;

  if (recog == NULL) return 0;

#ifdef HAVE_PTHREAD
  pthread_mutex_lock(&(s->mutex));
#endif

  if (s->window != NULL && s->renderer != NULL) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
	exit(0);
      }
      switch(event.key.keysym.sym) {
      case SDLK_ESCAPE:
	// ESC -> quit
	exit(0);
      case SDLK_UP:
	// UP -> thres up
	if (event.key.state != SDL_PRESSED) break;
	recog->adin->thres += THRESHOLD_ADJUST_STEP;
	if (recog->adin->thres > THRESHOLD_ADJUST_MAX) recog->adin->thres = THRESHOLD_ADJUST_MAX;
	recog->jconf->detect.level_thres = recog->adin->thres;
	recog->adin->zc.trigger = recog->adin->thres;
	break;
      case SDLK_DOWN:
	// DOWN -> thres down
	if (event.key.state != SDL_PRESSED) break;
	recog->adin->thres -= THRESHOLD_ADJUST_STEP;
	if (recog->adin->thres < THRESHOLD_ADJUST_MIN) recog->adin->thres = THRESHOLD_ADJUST_MIN;
	recog->jconf->detect.level_thres = recog->adin->thres;
	recog->adin->zc.trigger = recog->adin->thres;
	break;
      case SDLK_m:
	// 'm' -> input mute
	if (event.key.state != SDL_PRESSED || event.key.repeat != 0) break;
	if (recog->jconf->preprocess.level_coef == 1.0f) {
	  recog->jconf->preprocess.level_coef = recog->adin->level_coef = 0.00f;
	} else {
	  recog->jconf->preprocess.level_coef = recog->adin->level_coef = 1.0f;
	}
	break;
      case SDLK_c:
	// 'c' -> connect/disconnect
	if (event.key.state != SDL_PRESSED || event.key.repeat != 0) break;
	if (a->on_processing == FALSE) {
	  connect_to_output_device(recog);
	  a->on_pause = FALSE;
	} else {
	  close_processing(recog);
	}
	break;
      case SDLK_RETURN:
	if (event.key.state != SDL_PRESSED || event.key.repeat != 0) break;
	// Enter -> force segmentation
	a->process_error = FALSE;
#ifdef HAVE_PTHREAD
	pthread_mutex_unlock(&(s->mutex));
#endif
	return -2;
      }
    }
  }
#ifdef HAVE_PTHREAD
  pthread_mutex_unlock(&(s->mutex));
#endif
  return 0;
}

#endif

// Julius callback to temporally record when the last segment triggered
static void
record_trigger_time(Recog *recog, void *data)
{
  AdinTool *a = global_a;

  a->trigger_sample = recog->adin->last_trigger_sample;
}

/* Interrupt signal handling */
static void
interrupt_record(int signum)
{
  AdinTool *a = global_a;

  fprintf(stderr, "[Interrupt]");
  if (a->conf.speech_output == SPOUT_FILE) {
    /* close files */
    close_files();
  }
  if (a->conf.speech_output == SPOUT_VECTORNET) {
    vecnet_send_end_of_session();
  }
  /* terminate program */
  exit(1);
}


// main loop
void
mainloop()
{
  AdinTool *a = global_a;
  Recog *recog = a->recog;
  Jconf *jconf = a->jconf;
  int ret;
  boolean is_continues;

  /**********************/
  /* interrupt handling */
  /**********************/
  if (signal(SIGINT, interrupt_record) == SIG_ERR) {
    fprintf(stderr, "Warning: signal interruption may collapse output\n");
  }
  if (signal(SIGTERM, interrupt_record) == SIG_ERR) {
    fprintf(stderr, "Warning: signal interruption may collapse output\n");
  }
#ifdef SIGPIPE
  if (signal(SIGPIPE, interrupt_record) == SIG_ERR) {
    fprintf(stderr, "Warning: signal interruption may collapse output\n");
  }
#endif
#ifdef SIGQUIT
  if (signal(SIGQUIT, interrupt_record) == SIG_ERR) {
    fprintf(stderr, "Warning: signal interruption may collapse output\n");
  }
#endif

  /********************/
  /* setup for output */
  /********************/
#ifdef USE_SDL
  /* connect by key later */
#else
  if (connect_to_output_device() == -1) {
    /* error */
    return;
  }
#endif
  
  /***********************************/
  /* register callbacks to JuliusLib */
  /***********************************/
  callback_add(recog, CALLBACK_EVENT_SPEECH_START, record_trigger_time, NULL);
#ifdef USE_SDL
  callback_add_adin(recog, CALLBACK_ADIN_CAPTURED, draw_wave, NULL);
#endif

  if (a->conf.continuous_segment) {	/* reset parameter for successive output */
    a->total_speechlen = 0;
    a->sid = a->conf.startid;
  }
  fprintf(stderr,"[start recording]\n");

  /*********************/
  /* input stream loop */
  /*********************/
  while(1) {

    /* begin A/D input of a stream */
    ret = j_open_stream(recog, NULL);
    switch(ret) {
    case 0:			/* succeeded */
      break;
    case -1:      		/* error */
      /* go on to next input */
      continue;
    case -2:			/* end of recognition process */
      switch(jconf->input.speech_input) {
      case SP_RAWFILE:
	break;
      case SP_STDIN:
	fprintf(stderr, "reached end of input on stdin\n");
	break;
      default:
	fprintf(stderr, "failed to begin input stream\n");
      }
      /* exit recording */
      goto record_end;
    }

    /*********************************/
    /* do VAD and recording */
    /*********************************/
    do {
      /* process one segment with segmentation */
      /* for incoming speech input, speech detection and segmentation are
	 performed and, process_callback_* is called for speech output for each segment block.
      */
      /* adin_go() return when input segmented by long silence, or input stream reached to the end */
      a->speechlen = 0;
      a->stop_at_next = FALSE;
      if (jconf->input.speech_input == SP_MIC) {
	fprintf(stderr, "<<< please speak >>>");
      }
      if (a->conf.speech_output == SPOUT_NONE) {
	ret = adin_go(NULL, adinnet_check_command, recog);
      } else if (a->conf.speech_output == SPOUT_ADINNET) {
	ret = adin_go(process_callback_adinnet, adinnet_check_command, recog);
      } else if (a->conf.speech_output == SPOUT_VECTORNET) {
	if (vecnet_prepare(recog) == FALSE){
	  fprintf(stderr, "failed to init\n");
	  exit(1);
	}
	ret = adin_go(process_callback_vecnet, adinnet_check_command, recog);
      } else {
#ifdef USE_SDL
	ret = adin_go(process_callback_file, sdl_check_command, recog);
#else
	ret = adin_go(process_callback_file, NULL, recog);
#endif
      }
      /* return value of adin_go:
	 -2: input terminated by pause command from adinnet server
	 -1: input device read error or callback process error
	 0:  paused by input stream (end of file, etc..)
	 >0: detected end of speech segment:
             by adin-cut, or by callback process
	 (or return value of ad_check (<0) (== not used in this program))
      */
      /* if PAUSE or TERMINATE command has been received while input,
	 stop_at_next is TRUE here  */
      switch(ret) {
      case -2:	     /* terminated by terminate command from server */
	fprintf(stderr, "[terminated by server]\n");
	break;
      case -1:		     /* device read error or callback error */
	fprintf(stderr, "[error]\n");
	break;
      case 0:			/* reached to end of input */
	fprintf(stderr, "[eof]\n");
	break;
      default:	  /* input segmented by silence or callback process */
	fprintf(stderr, "[segmented]\n");
	break;
      }
      
      if (ret == -1 || (ret == -2 && a->process_error == TRUE)) {
	/* error in input device or callback function*/ 
#ifdef USE_SDL
	a->process_error = FALSE;
	a->on_processing = FALSE;
#else
	/* terminate main loop here */
	return;
#endif
      }

      /*************************/
      /* one segment processed */
      /*************************/
      if (a->conf.speech_output == SPOUT_FILE) {
	/* close output files */
	if (close_files() == FALSE) return;
      } else if (a->conf.speech_output == SPOUT_ADINNET) {
	if (a->speechlen > 0) {
	  if (ret == -2 && a->process_error == FALSE) {
	    /* in case of user-side termination, send segmentation ack */
	    adin_send_end_of_segment();
	  }
	  if (ret >= 0 || a->stop_at_next) { /* segmented by adin-cut or end of stream or server-side command */
	    /* send end-of-segment ack to client */
	    adin_send_end_of_segment();
	  }
	  /* output info */
	  printf("sent: %d samples (%.2f sec.) [%6d (%5.2fs) - %6d (%5.2fs)]\n", 
		 a->speechlen, (float)a->speechlen / (float)a->conf.sfreq,
		 a->trigger_sample, (float)a->trigger_sample / (float)a->conf.sfreq, 
		 a->trigger_sample + a->speechlen, (float)(a->trigger_sample + a->speechlen) / (float)a->conf.sfreq);
	}
      } else if (a->conf.speech_output == SPOUT_VECTORNET) {
	if (a->speechlen > 0) {
	  if (ret == -2 && a->process_error == FALSE) {
	    /* in case of user-side termination, send segmentation ack */
	    vecnet_send_end_of_segment();
	    vecnet_param_update(recog);
	  }
	  if (ret >= 0 || a->stop_at_next) { /* segmented by adin-cut or end of stream or server-side command */
	    /* send end-of-segment ack to client */
	    vecnet_send_end_of_segment();
	    vecnet_param_update(recog);
	  }
	  /* output info */
	  printf("sent: %d samples (%.2f sec.) [%6d (%5.2fs) - %6d (%5.2fs)]\n", 
		 a->speechlen, (float)a->speechlen / (float)a->conf.sfreq,
		 a->trigger_sample, (float)a->trigger_sample / (float)a->conf.sfreq, 
		 a->trigger_sample + a->speechlen, (float)(a->trigger_sample + a->speechlen) / (float)a->conf.sfreq);
	}
      }

      /*************************************/
      /* increment ID and total sample len */
      /*************************************/
      if (a->conf.continuous_segment) {
	a->total_speechlen += a->speechlen;
	if (a->speechlen > 0) a->sid++;
      }

      /***************************************************/
      /* with adinnet server, if terminated by           */
      /* server-side PAUSE command, wait for RESUME here */
      /***************************************************/
      if (a->conf.pause_each) {
	/* pause at each end */
	//if (speech_output == SPOUT_ADINNET && speechlen > 0) {
	if (a->conf.speech_output == SPOUT_ADINNET || a->conf.speech_output == SPOUT_VECTORNET) {
#ifdef USE_SDL
	  a->on_pause = TRUE;
	  if (a->conf.speech_output == SPOUT_ADINNET || a->conf.speech_output == SPOUT_VECTORNET) {
	    ret = adin_go(NULL, adinnet_wait_command, recog);
	  } else {
	    ret = adin_go(NULL, sdl_check_command, recog);
	  }
	  if (ret == -1 || (ret == -2 && a->process_error == TRUE)) {
	    /* error in input device or callback function*/ 
	    a->process_error = FALSE;
	    a->on_processing = FALSE;
	  }
	  a->on_pause = FALSE;
#else
	  if (adinnet_wait_command() == -2 && a->process_error == TRUE) {
	    /* command error: terminate program here */
	    return;
	  }
#endif
	}
      } else {
	if ((a->conf.speech_output == SPOUT_ADINNET || a->conf.speech_output == SPOUT_VECTORNET) && a->stop_at_next) {
#ifdef USE_SDL
	  a->on_pause = TRUE;
	  if (a->conf.speech_output == SPOUT_ADINNET || a->conf.speech_output == SPOUT_VECTORNET) {
	    ret = adin_go(NULL, adinnet_wait_command, recog);
	  } else {
	    ret = adin_go(NULL, sdl_check_command, recog);
	  }
	  if (ret == -1 || (ret == -2 && a->process_error == TRUE)) {
	    /* error in input device or callback function*/ 
	    a->process_error = FALSE;
	    a->on_processing = FALSE;
	  }
	  a->on_pause = FALSE;
#else
	  if (adinnet_wait_command() == -2 && a->process_error == TRUE) {
	    /* command error: terminate program here */
	    return;
	  }
#endif
	}
      }

      /* loop condition check */
      is_continues = FALSE;
      if (a->conf.continuous_segment && (ret > 0 || ret == -2)) {
	is_continues = TRUE;
      }
      
    } while (is_continues); /* to the next segment in this input stream */

    /***********************/
    /* end of input stream */
    /***********************/
    adin_end(recog->adin);

  } /* to the next input stream (i.e. next input file in SP_RAWFILE) */

 record_end:

  close_processing(recog);

  if (a->conf.speech_output == SPOUT_FILE) {
    if (a->conf.continuous_segment) {
      printf("recorded total %d samples (%.2f sec.) segmented to %s.%04d - %s.%04d files\n", a->total_speechlen, (float)a->total_speechlen / (float)a->conf.sfreq, a->conf.filename, 0, a->conf.filename, a->sid - 1);
    }
  }
  
#ifdef USE_SDL
  SDL_DestroyWindow(a->sdl.window);
  SDL_Quit();
#endif
  
}

/* end of mainloop.c */
