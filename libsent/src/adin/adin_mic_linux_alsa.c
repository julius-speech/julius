/**
 * @file   adin_mic_linux_alsa.c
 *
 * <JA>
 * @brief  マイク入力 (Linux/ALSA)
 *
 * ALSA API を使用する，マイク入力のための低レベル関数です．
 * 使用には ALSA サウンドドライバーがインストールされていることが必要です．
 *
 * サウンドカードが 16bit モノラル で録音できることが必須です．
 *
 * JuliusはLinuxではミキサーデバイスの設定を一切行いません．録音デバイスの
 * 選択（マイク/ライン）や録音ボリュームの調節は alsamixer など他のツールで
 * 行なって下さい．
 *
 * 複数サウンドカードはサポートされていません．複数のサウンドカードが
 * インストールされている場合，最初の１つが用いられます．
 *
 * バージョン 1.x に対応しています．1.0.13 で動作を確認しました．
 *
 * デバイス名は "default" が使用されます．環境変数 ALSADEV で変更できます．
 * </JA>
 * <EN>
 * @brief  Microphone input on Linux/ALSA
 *
 * Low level I/O functions for microphone input on Linux using
 * Advanced Linux Sound Architechture (ALSA) API, developed on version 0.9.x.
 *
 * Julius does not alter any mixer device setting at all on Linux.  You should
 * configure the mixer for recording source (mic/line) and recording volume
 * correctly using other audio tool such as alsamixer.
 *
 * Note that sound card should support 16bit monaural recording, and multiple
 * cards are not supported (in that case the first one will be used).
 *
 * This file supports alsa version 1.x, and tested on 1.0.13.
 *
 * The default PCM device name is "default", and can be overwritten by
 * environment variable "ALSADEV".
 * </EN>
 *
 * @sa http://www.alsa-project.org/
 *
 * @author Akinobu LEE
 * @date   Sun Feb 13 16:18:26 2005
 *
 * $Revision: 1.14 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/adin.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAS_ALSA

#if defined(HAVE_ALSA_ASOUNDLIB_H)
#include <alsa/asoundlib.h>
#elif defined(HAVE_SYS_ASOUNDLIB_H)
#include <sys/asoundlib.h>
#endif

static int srate;		///< Required sampling rate
static snd_pcm_t *handle;	///< Audio handler
static char pcm_name[MAXPATHLEN]; ///< Name of the PCM device
static int latency = 32;	///< Lantency time in msec.  You can override this value by specifying environment valuable "LATENCY_MSEC".
static boolean need_swap;	///< Whether samples need byte swap

#if (SND_LIB_MAJOR == 0)
static struct pollfd *ufds;	///< Poll descriptor
static int count;		///< Poll descriptor count
#endif

#define MAXPOLLINTERVAL 300	///< Read timeout in msec.

#endif /* HAS_ALSA */

#ifdef HAS_ALSA
/** 
 * Output detailed device information.
 * 
 * @param pcm_name [in] device name string
 * @param handle [in] pcm audio handler
 * 
 */
static void
output_card_info(char *pcm_name, snd_pcm_t *handle)
{
  int err;
  snd_ctl_t *ctl;
  snd_ctl_card_info_t *info;
  snd_pcm_info_t *pcminfo;
  snd_ctl_card_info_alloca(&info);
  snd_pcm_info_alloca(&pcminfo);
  char ctlname[30];
  int card;
  
  /* get PCM information to set current device and subdevice name */
  if ((err = snd_pcm_info(handle, pcminfo)) < 0) {
    jlog("Warning: adin_alsa: failed to obtain pcm info\n");
    jlog("Warning: adin_alsa: skip output of detailed audio device info\n");
    return;
  }
  /* open control associated with the pcm device name */
  card = snd_pcm_info_get_card(pcminfo);
  if (card < 0) {
    strcpy(ctlname, "default");
  } else {
    snprintf(ctlname, 30, "hw:%d", card);
  }
  if ((err = snd_ctl_open(&ctl, ctlname, 0)) < 0) {
    jlog("Warning: adin_alsa: failed to open control device \"%s\", \n", ctlname);
    jlog("Warning: adin_alsa: skip output of detailed audio device info\n");
    return;
  }
  /* get its card info */
  if ((err = snd_ctl_card_info(ctl, info)) < 0) {
    jlog("Warning: adin_alsa: unable to get card info for %s\n", ctlname);
    jlog("Warning: adin_alsa: skip output of detailed audio device info\n");
    snd_ctl_close(ctl);
    return;
  }

  /* get detailed PCM information of current device from control */
  if ((err = snd_ctl_pcm_info(ctl, pcminfo)) < 0) {
    jlog("Error: adin_alsa: unable to get pcm info from card control\n");
    jlog("Warning: adin_alsa: skip output of detailed audio device info\n");
    snd_ctl_close(ctl);
    return;
  }
  /* output */
  jlog("Stat: \"%s\": %s [%s] device %s [%s] %s\n",
       pcm_name,
       snd_ctl_card_info_get_id(info),
       snd_ctl_card_info_get_name(info),
       snd_pcm_info_get_id(pcminfo),
       snd_pcm_info_get_name(pcminfo),
       snd_pcm_info_get_subdevice_name(pcminfo));

  /* close controller */
  snd_ctl_close(ctl);

}
#endif /* HAS_ALSA */

/** 
 * Device initialization: check machine capability
 * 
 * @param sfreq [in] required sampling frequency.
 * @param dummy [in] a dummy data
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_alsa_standby(int sfreq, void *dummy)
{
#ifndef HAS_ALSA
  jlog("Error: ALSA not compiled in\n");
  return FALSE;
#else
  /* store required sampling rate for checking after opening device */
  srate = sfreq;
  return TRUE;
#endif
}


/** 
 * Open the specified device and check capability of the opening device.
 * 
 * @param devstr [in] device string to open
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
adin_alsa_open(char *devstr)
{
#ifndef HAS_ALSA
  jlog("Error: ALSA not compiled in\n");
  return FALSE;
#else
  int err;
  snd_pcm_hw_params_t *hwparams; ///< Pointer to device hardware parameters
#if (SND_LIB_MAJOR == 0)
  int actual_rate;		/* sample rate returned by hardware */
#else
  unsigned int actual_rate;		/* sample rate returned by hardware */
#endif
  int dir = 0;			/* comparison result of exact rate and given rate */

  /* open the device in non-block mode) */
  if ((err = snd_pcm_open(&handle, devstr, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK)) < 0) {
    jlog("Error: adin_alsa: cannot open PCM device \"%s\" (%s)\n", devstr, snd_strerror(err));
    return(FALSE);
  }
  /* set device to non-block mode */
  if ((err = snd_pcm_nonblock(handle, 1)) < 0) {
    jlog("Error: adin_alsa: cannot set PCM device to non-blocking mode\n");
    return(FALSE);
  }

  /* allocate hwparam structure */
  snd_pcm_hw_params_alloca(&hwparams);

  /* initialize hwparam structure */
  if ((err = snd_pcm_hw_params_any(handle, hwparams)) < 0) {
    jlog("Error: adin_alsa: cannot initialize PCM device parameter structure (%s)\n", snd_strerror(err));
    return(FALSE);
  }

  /* set interleaved read/write format */
  if ((err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    jlog("Error: adin_alsa: cannot set PCM device access mode (%s)\n", snd_strerror(err));
    return(FALSE);
  }

  /* set sample format */
#ifdef WORDS_BIGENDIAN
  /* try big endian, then little endian with byte swap */
  if ((err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16_BE)) >= 0) {
    need_swap = FALSE;
  } else if ((err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16_LE)) >= 0) {
    need_swap = TRUE;
  } else {
    jlog("Error: adin_alsa: cannot set PCM device format to signed 16bit (%s)\n", snd_strerror(err));
    return(FALSE);
  }
#else  /* LITTLE ENDIAN */
  /* try little endian, then big endian with byte swap */
  if ((err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16_LE)) >= 0) {
    need_swap = FALSE;
  } else if ((err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16_BE)) >= 0) {
    need_swap = TRUE;
  } else {
    jlog("Error: adin_alsa: cannot set PCM device format to signed 16bit (%s)\n", snd_strerror(err));
    return(FALSE);
  }
#endif
  /* set number of channels */
  if ((err = snd_pcm_hw_params_set_channels(handle, hwparams, 1)) < 0) {
    jlog("Error: adin_alsa: cannot set PCM channel to %d (%s)\n", 1, snd_strerror(err));
    return(FALSE);
  }
  
  /* set sample rate (if the exact rate is not supported by the hardware, use nearest possible rate */
#if (SND_LIB_MAJOR == 0)
  actual_rate = snd_pcm_hw_params_set_rate_near(handle, hwparams, srate, &dir);
  if (actual_rate < 0) {
    jlog("Error: adin_alsa: cannot set PCM device sample rate to %d (%s)\n", srate, snd_strerror(actual_rate));
    return(FALSE);
  }
#else
  actual_rate = srate;
  err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &actual_rate, &dir);
  if (err < 0) {
    jlog("Error: adin_alsa: cannot set PCM device sample rate to %d (%s)\n", srate, snd_strerror(err));
    return(FALSE);
  }
#endif
  if (actual_rate != srate) {
    jlog("Warning: adin_alsa: the exact rate %d Hz is not available by your PCM hardware.\n", srate);
    jlog("Warning: adin_alsa: using %d Hz instead.\n", actual_rate);
  }
  jlog("Stat: capture audio at %dHz\n", actual_rate);

  /* set period size */
  {
#if (SND_LIB_MAJOR == 0)
    int periodsize;		/* period size (bytes) */
    int actual_size;
    int maxsize, minsize;
#else
    unsigned int period_time, period_time_current;
    snd_pcm_uframes_t chunk_size;
    boolean has_current_period;
#endif
    boolean force = FALSE;
    char *p;
    
    /* set apropriate period size */
    if ((p = getenv("LATENCY_MSEC")) != NULL) {
      latency = atoi(p);
      jlog("Stat: adin_alsa: trying to set latency to %d msec from LATENCY_MSEC)\n", latency);
      force = TRUE;
    }

    /* get hardware max/min size */
#if (SND_LIB_MAJOR == 0)
    if ((maxsize = snd_pcm_hw_params_get_period_size_max(hwparams, &dir)) < 0) {
      jlog("Error: adin_alsa: cannot get maximum period size\n");
      return(FALSE);
    }
    if ((minsize = snd_pcm_hw_params_get_period_size_min(hwparams, &dir)) < 0) {
      jlog("Error: adin_alsa: cannot get minimum period size\n");
      return(FALSE);
    }
#else    
    has_current_period = TRUE;
    if ((err = snd_pcm_hw_params_get_period_time(hwparams, &period_time_current, &dir)) < 0) {
      has_current_period = FALSE;
    }
    if (has_current_period) {
      jlog("Stat: adin_alsa: current latency time: %d msec\n", period_time_current / 1000);
    }
#endif

    /* set period time (near value will be used) */
#if (SND_LIB_MAJOR == 0)
    periodsize = actual_rate * latency / 1000 * sizeof(SP16);
    if (periodsize < minsize) {
      jlog("Stat: adin_alsa: PCM latency of %d ms (%d bytes) too small, use device minimum %d bytes\n", latency, periodsize, minsize);
      periodsize = minsize;
    } else if (periodsize > maxsize) {
      jlog("Stat: adin_alsa: PCM latency of %d ms (%d bytes) too large, use device maximum %d bytes\n", latency, periodsize, maxsize);
      periodsize = maxsize;
    }
    actual_size = snd_pcm_hw_params_set_period_size_near(handle, hwparams, periodsize, &dir);
    if (actual_size < 0) {
      jlog("Error: adin_alsa: cannot set PCM record period size to %d (%s)\n", periodsize, snd_strerror(actual_size));
      return(FALSE);
    }
    if (actual_size != periodsize) {
      jlog("Stat: adin_alsa: PCM period size: %d bytes (%dms) -> %d bytes\n", periodsize, latency, actual_size);
    }
    jlog("Stat: Audio I/O Latency = %d msec (data fragment = %d frames)\n", actual_size * 1000 / (actual_rate * sizeof(SP16)), actual_size / sizeof(SP16));
#else
    period_time = latency * 1000;
    if (!force && has_current_period && period_time > period_time_current) {
	jlog("Stat: adin_alsa: current latency (%dms) is shorter than %dms, leave it\n", period_time_current / 1000, latency);
	period_time = period_time_current;
    } else {
      if ((err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, 0)) < 0) {
	jlog("Error: adin_alsa: cannot set PCM record period time to %d msec (%s)\n", period_time / 1000, snd_strerror(err));
	return(FALSE);
      }
      snd_pcm_hw_params_get_period_size(hwparams, &chunk_size, 0);
      jlog("Stat: adin_alsa: latency set to %d msec (chunk = %d bytes)\n", period_time / 1000, chunk_size);
    }
#endif

#if (SND_LIB_MAJOR == 0)
    /* set number of periods ( = 2) */
    if ((err = snd_pcm_hw_params_set_periods(handle, hwparams, sizeof(SP16), 0)) < 0) {
      jlog("Error: adin_alsa: cannot set PCM number of periods to %d (%s)\n", sizeof(SP16), snd_strerror(err));
      return(FALSE);
    }
#endif
  }

  /* apply the configuration to the PCM device */
  if ((err = snd_pcm_hw_params(handle, hwparams)) < 0) {
    jlog("Error: adin_alsa: cannot set PCM hardware parameters (%s)\n", snd_strerror(err));
    return(FALSE);
  }

  /* prepare for recording */
  if ((err = snd_pcm_prepare(handle)) < 0) {
    jlog("Error: adin_alsa: failed to prepare audio interface (%s)\n", snd_strerror(err));
  }

#if (SND_LIB_MAJOR == 0)
  /* prepare for polling */
  count = snd_pcm_poll_descriptors_count(handle);
  if (count <= 0) {
    jlog("Error: adin_alsa: invalid PCM poll descriptors count\n");
    return(FALSE);
  }
  ufds = mymalloc(sizeof(struct pollfd) * count);
  if ((err = snd_pcm_poll_descriptors(handle, ufds, count)) < 0) {
    jlog("Error: adin_alsa: unable to obtain poll descriptors for PCM recording (%s)\n", snd_strerror(err));
    return(FALSE);
  }
#endif

  /* output status */
  output_card_info(devstr, handle);

  return(TRUE);
#endif /* HAS_ALSA */
}

#ifdef HAS_ALSA
/** 
 * Error recovery when PCM buffer underrun or suspend.
 * 
 * @param handle [in] audio handler
 * @param err [in] error code
 * 
 * @return 0 on success, otherwise the given errno.
 */
static int
xrun_recovery(snd_pcm_t *handle, int err)
{
  if (err == -EPIPE) {    /* under-run */
    err = snd_pcm_prepare(handle);
    if (err < 0)
      jlog("Error: adin_alsa: can't recovery from PCM buffer underrun, prepare failed: %s\n", snd_strerror(err));
    return 0;
  } else if (err == -ESTRPIPE) {
    while ((err = snd_pcm_resume(handle)) == -EAGAIN)
      sleep(1);       /* wait until the suspend flag is released */
    if (err < 0) {
      err = snd_pcm_prepare(handle);
      if (err < 0)
	jlog("Error: adin_alsa: can't recovery from PCM buffer suspend, prepare failed: %s\n", snd_strerror(err));
    }
    return 0;
  }
  return err;
}
#endif /* HAS_ALSA */

/** 
 * Start recording.
 *
 * @param pathname [in] device name to open or NULL for default
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_alsa_begin(char *pathname)
{
#ifndef HAS_ALSA
  return FALSE;
#else
  int err;
  snd_pcm_state_t status;
  char *p;

  /* set device name to open to pcm_name */
  if (pathname != NULL) {
    strncpy(pcm_name, pathname, MAXPATHLEN);
    jlog("Stat: adin_alsa: device name from argument: \"%s\"\n", pcm_name);
  } else if ((p = getenv("ALSADEV")) != NULL) {
    strncpy(pcm_name, p, MAXPATHLEN);
    jlog("Stat: adin_alsa: device name from ALSADEV: \"%s\"\n", pcm_name);
  } else {
    strcpy(pcm_name, "default");
  }
  /* open the device */
  if (adin_alsa_open(pcm_name) == FALSE) {
    return FALSE;
  }

  /* check hardware status */
  while(1) {			/* wait till prepared */
    status = snd_pcm_state(handle);
    switch(status) {
    case SND_PCM_STATE_PREPARED: /* prepared for operation */
      if ((err = snd_pcm_start(handle)) < 0) {
	jlog("Error: adin_alsa: cannot start PCM (%s)\n", snd_strerror(err));
	return (FALSE);
      }
      return(TRUE);
      break;
    case SND_PCM_STATE_RUNNING:	/* capturing the samples of other application */
      if ((err = snd_pcm_drop(handle)) < 0) { /* discard the existing samples */
	jlog("Error: adin_alsa: cannot drop PCM (%s)\n", snd_strerror(err));
	return (FALSE);
      }
      break;
    case SND_PCM_STATE_XRUN:	/* buffer overrun */
      if ((err = xrun_recovery(handle, -EPIPE)) < 0) {
	jlog("Error: adin_alsa: PCM XRUN recovery failed (%s)\n", snd_strerror(err));
	return(FALSE);
      }
      break;
    case SND_PCM_STATE_SUSPENDED:	/* suspended by power management system */
      if ((err = xrun_recovery(handle, -ESTRPIPE)) < 0) {
	jlog("Error: adin_alsa: PCM XRUN recovery failed (%s)\n", snd_strerror(err));
	return(FALSE);
      }
      break;
    default:
      /* do nothing */
      break;
    }
  }

  return(TRUE);
#endif /* HAS_ALSA */
}
  
/** 
 * Stop recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_alsa_end()
{
  int err;

  if ((err = snd_pcm_close(handle)) < 0) {
    jlog("Error: adin_alsa: cannot close PCM device (%s)\n", snd_strerror(err));
    return(FALSE);
  }
  return(TRUE);
}

/**
 * @brief  Read samples from device
 * 
 * Try to read @a sampnum samples and returns actual number of recorded
 * samples currently available.  This function will block until
 * at least one sample can be obtained.
 * 
 * @param buf [out] samples obtained in this function
 * @param sampnum [in] wanted number of samples to be read
 * 
 * @return actural number of read samples, -2 if an error occured.
 */
int
adin_alsa_read(SP16 *buf, int sampnum)
{
#ifndef HAS_ALSA
  return -2;
#else
  int cnt;

#if (SND_LIB_MAJOR == 0)

  snd_pcm_sframes_t avail;

  while ((avail = snd_pcm_avail_update(handle)) <= 0) {
    usleep(latency * 1000);
  }
  if (avail < sampnum) {
    cnt = snd_pcm_readi(handle, buf, avail);
  } else {
    cnt = snd_pcm_readi(handle, buf, sampnum);
  }

#else

  int ret;
  snd_pcm_status_t *status;
  int res;
  struct timeval now, diff, tstamp;

  ret = snd_pcm_wait(handle, MAXPOLLINTERVAL);
  switch (ret) {
  case 0:			/* timeout */
    jlog("Warning: adin_alsa: no data fragment after %d msec?\n", MAXPOLLINTERVAL);
    cnt = 0;
    break;
  case 1:			/* has data */
    cnt = snd_pcm_readi(handle, buf, sampnum); /* read available (non-block) */
    break;
  case -EPIPE:			/* pipe error */
    /* try to recover the broken pipe */
    snd_pcm_status_alloca(&status);
    if ((res = snd_pcm_status(handle, status))<0) {
      jlog("Error: adin_alsa: broken pipe: status error (%s)\n", snd_strerror(res));
      return -2;
    }
    if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
      gettimeofday(&now, 0);
      snd_pcm_status_get_trigger_tstamp(status, &tstamp);
      timersub(&now, &tstamp, &diff);
      jlog("Warning: adin_alsa: overrun!!! (at least %.3f ms long)\n",
	   diff.tv_sec * 1000 + diff.tv_usec / 1000.0);
      if ((res = snd_pcm_prepare(handle))<0) {
	jlog("Error: adin_alsa: overrun: prepare error (%s)", snd_strerror(res));
	return -2;
      }
      break;         /* ok, data should be accepted again */
    } else if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {
      jlog("Warning: adin_alsa: draining: capture stream format change? attempting recover...\n");
      if ((res = snd_pcm_prepare(handle))<0) {
	jlog("Error: adin_alsa: draining: prepare error (%s)", snd_strerror(res));
	return -2;
      }
      break;
    }
    jlog("Error: adin_alsa: error in snd_pcm_wait() (%s)\n", snd_pcm_state_name(snd_pcm_status_get_state(status)));
    return -2;

  default:			/* other poll error */
    jlog("Error: adin_alsa: error in snd_pcm_wait() (%s)\n", snd_strerror(ret));
    return(-2);			/* error */
  }
#endif
  if (cnt < 0) {
    jlog("Error: adin_alsa: failed to read PCM (%s)\n", snd_strerror(cnt));
    return(-2);
  }
  if (need_swap) {
    swap_sample_bytes(buf, cnt);
  }

  return(cnt);
#endif /* HAS_ALSA */
}

/** 
 * 
 * Function to return current input source device name
 * 
 * @return string of current input device name.
 * 
 */
char *
adin_alsa_input_name()
{
#ifndef HAS_ALSA
  return NULL;
#else
  return(pcm_name);
#endif
}

/* end of file */
