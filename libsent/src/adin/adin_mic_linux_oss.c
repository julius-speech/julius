/**
 * @file   adin_mic_linux_oss.c
 *
 * <JA>
 * @brief  マイク入力 (Linux/OSS)
 *
 * マイク入力のための低レベル関数です．インタフェースとして OSS
 * サウンドドライバを使用する場合，このファイルが使用されます．
 * カーネル標準のドライバ，OSS/Linuxのドライバ，および ALSA の
 * OSS互換モードに対応しています．
 *
 * configure でマイクタイプの自動判別を
 * 行なう場合（デフォルト），Linux ではこのOSS用インタフェースが選択されます．
 * 他のインタフェース (ALSA, esd, portaudio, spAudio等) を使用したい場合は
 * configure 時に "--with-mictype=TYPE" を明示的に指定して下さい．
 *
 * サウンドカードが 16bit モノラル で録音できることが必須です．
 * ただしLinuxでは，ステレオ録音しかできないデバイスの場合，
 * 左チャンネルのみを入力として取り出して認識することもできます．
 *
 * JuliusはLinuxではミキサーデバイスの設定を一切行いません．録音デバイスの
 * 選択（マイク/ライン）や録音ボリュームの調節は xmixer など他のツールで
 * 行なって下さい．
 *
 * デフォルトのデバイス名は "/dev/dsp" です．環境変数 AUDIODEV に
 * デバイス名を指定することで，他のデバイス名を使用できます．
 * </JA>
 * <EN>
 * @brief  Microphone input on Linux/OSS
 *
 * Low level I/O functions for microphone input on Linux using OSS API.
 * Works on kernel driver, OSS/Linux, ALSA OSS compatible device, and
 * other OSS compatible drivers.  This will work on kernel 2.0.x and later.
 *
 * This OSS API is used by default on Linux machines.  If you want
 * another API (like ALSA native, esd, portaudio, spAudio), please specify
 * "--with-mictype=TYPE" options at compilation time to configure script.
 *
 * Sound card should support 16bit monaural recording.  On Linux, however,
 * if you unfortunately have a device with only stereo recording support,
 * Julius will try to get input from the left channel of the stereo input.
 *
 * Julius does not alter any mixer device setting at all on Linux.  You should
 * configure the mixer for recording source (mic/line) and recording volume
 * correctly using other audio tool such as xmixer.
 *
 * The default device name is "/dev/dsp", which can be changed by setting
 * environment variable AUDIODEV.
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Sun Feb 13 16:18:26 2005
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

#include <sent/adin.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

/* sound header */
#ifdef HAS_OSS
#if defined(HAVE_SYS_SOUNDCARD_H)
#include <sys/soundcard.h>
#elif defined(HAVE_MACHINE_SOUNDCARD_H)
#include <machine/soundcard.h>
#endif
#endif

/// Default device name, can be overridden by AUDIODEV environment variable
#define DEFAULT_DEVICE "/dev/dsp"

#define FREQALLOWRANGE 200	///< Acceptable width of sampling frequency
#define MAXPOLLINTERVAL 300	///< Read timeout in msec.
/**
 * Maximum Data fragment Length in msec.  Input can be delayed to this time.
 * a maximum of 2^x number smaller than this value will be taken.
 * You can override this value by specifying environment valuable
 * "LATENCY_MSEC".
 * 
 */
#define MAX_FRAGMENT_MSEC 50
/**
 * Minimum fragment length in bytes
 * 
 */
#define MIN_FRAGMENT_SIZE 256

static int srate;		///< Required sampling rate
static int audio_fd;		///< Audio descriptor
static boolean need_swap;	///< Whether samples need byte swap
static int frag_size;		///< Actual data fragment size
static boolean stereo_rec;	///< TRUE if stereo recording (use left only)
static char *defaultdev = DEFAULT_DEVICE; ///< Default device name
static char devname[MAXPATHLEN];		///< Current device name

/** 
 * Device initialization: check device capability and open for recording.
 * 
 * @param sfreq [in] required sampling frequency.
 * @param dummy [in] a dummy data
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_oss_standby(int sfreq, void *dummy)
{
#ifndef HAS_OSS
  jlog("Error: OSS not compiled in\n");
  return FALSE;
#else
  /* store required sampling rate for checking after opening device */
  srate = sfreq;
  return TRUE;
}

/** 
 * Open the specified device and check capability of the opening device.
 * 
 * @param devstr [in] device string to open
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
adin_oss_open(char *devstr)
{
  int fmt, fmt_can, fmt1, fmt2, rfmt; /* sampling format */
  int samplerate;	/* 16kHz */
  int frag;
  int frag_msec;
  char *env, *p;

  /* open device */
  if ((audio_fd = open(devstr, O_RDONLY|O_NONBLOCK)) == -1) {
    jlog("Error: adin_oss: failed to open %s\n", devstr);
    return(FALSE);
  }

  /* check whether soundcard can record 16bit data */
  /* and set fmt */
  if (ioctl(audio_fd, SNDCTL_DSP_GETFMTS, &fmt_can) == -1) {
    jlog("Error: adin_oss: failed to get formats from audio device\n");
    return(FALSE);
  }
#ifdef WORDS_BIGENDIAN
  fmt1 = AFMT_S16_BE;
  fmt2 = AFMT_S16_LE;
#else
  fmt1 = AFMT_S16_LE;               /* 16bit signed (little endian) */
  fmt2 = AFMT_S16_BE;               /* (big endian) */
#endif /* WORDS_BIGENDIAN */
  /* fmt2 needs byte swap */
  if (fmt_can & fmt1) {
    fmt = fmt1;
    need_swap = FALSE;
  } else if (fmt_can & fmt2) {
    fmt = fmt2;
    need_swap = TRUE;
  } else {
    jlog("Error: adin_oss: 16bit recording not supported on this device\n");
    return FALSE;
  }
#ifdef DEBUG
  if (need_swap) {
    jlog("Stat: adin_oss: samples need swap\n");
  } else {
    jlog("Stat: adin_oss: samples need not swap\n");
  }
#endif
  
  if (close(audio_fd) != 0) return FALSE;

  /* re-open for recording */
  /* open device */
  if ((audio_fd = open(devstr, O_RDONLY)) == -1) {
    jlog("Error: adin_oss: failed to open %s", devstr);
    return(FALSE);
  }

  /* try to set a small fragment size to minimize delay, */
  /* although many devices use static fragment size... */
  /* (and smaller fragment causes busy buffering) */
  {
    int arg;
    int f, f2;

    /* if environment variable "LATENCY_MSEC" is defined, try to set it
       as a minimum latency in msec (will be rouneded to 2^x). */
    if ((env = getenv("LATENCY_MSEC")) == NULL) {
      frag_msec = MAX_FRAGMENT_MSEC;
    } else {
      frag_msec = atoi(env);
    }
      
    /* get fragment size from MAX_FRAGMENT_MSEC and MIN_FRAGMENT_SIZE */
    f = 0;
    f2 = 1;
    while (f2 * 1000 / (srate * sizeof(SP16)) <= frag_msec
	   || f2 < MIN_FRAGMENT_SIZE) {
      f++;
      f2 *= 2;
    }
    frag = f - 1;

    /* set to device */
    arg = 0x7fff0000 | frag;
    if (ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &arg)) {
      jlog("Stat: adin_oss: set fragment size to 2^%d=%d bytes (%d msec)\n", frag, 2 << (frag-1), (2 << (frag-1)) * 1000 / (srate * sizeof(SP16)));
    }
  }
  
  /* set format, samplerate, channels */
  rfmt = fmt;
  if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &rfmt) == -1) {
    jlog("Error: adin_oss: failed to get available formats from device\n");
    return(FALSE);
  }
  if (rfmt != fmt) {
    jlog("Error: adin_oss: 16bit recording is not supported on this device\n");
    return FALSE;
  }

  {
    /* try SNDCTL_DSP_STEREO, SNDCTL_DSP_CHANNELS, monaural, stereo */
    int channels;
    int stereo;
    boolean ok_p = FALSE;

    stereo = 0;			/* mono */
    if (ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo) == -1) {
      /* failed: SNDCTL_DSP_STEREO not supported */
      jlog("Stat: adin_oss: sndctl_dsp_stereo not supported, going to try another...\n");
    } else {
      if (stereo != 0) {
	/* failed to set monaural recording by SNDCTL_DSP_STEREO */
	jlog("Stat: adin_oss: failed to set monaural recording by sndctl_dsp_stereo\n");
	jlog("Stat: adin_oss: going to try another...\n");
      } else {
	/* succeeded to set monaural recording by SNDCTL_DSP_STEREO */
	//jlog("Stat: adin_oss: recording now set to mono\n");
	stereo_rec = FALSE;
	ok_p = TRUE;
      }
    }
    if (! ok_p) {		/* not setup yet */
      /* try using sndctl_dsp_channels */
      channels = 1;
      if (ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
	/* failed: SNDCTL_DSP_CHANNELS not supported */
	jlog("Stat: adin_oss: sndctl_dsp_channels not supported, try another...\n");
      } else {
	if (channels != 1) {
	  /* failed to set monaural recording by SNDCTL_DSP_CHANNELS */
	  jlog("Stat: adin_oss: failed to set monaural recording by sndctl_dsp_channels\n");
	  jlog("Stat: adin_oss: going to try another...\n");
	} else {
	  /* succeeded to set monaural recording by SNDCTL_DSP_CHANNELS */
	  //jlog("Stat: adin_oss: recording now set to mono\n");
	  stereo_rec = FALSE;
	  ok_p = TRUE;
	}
      }
    }
    if (! ok_p) {
      /* try using stereo input */
      jlog("Warning: adin_oss: failed to setup monaural recording, trying to use the left channel of stereo input\n");
      stereo = 1;			/* stereo */
      if (ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo) == -1) {
	/* failed: SNDCTL_DSP_STEREO not supported */
	jlog("Stat: adin_oss: failed to set stereo input using sndctl_dsp_stereo\n");
      } else {
	if (stereo != 1) {
	  /* failed to set stereo recording by SNDCTL_DSP_STEREO */
	  jlog("Stat: adin_oss: failed to set stereo input using sndctl_dsp_stereo\n");
	} else {
	  /* succeeded to set stereo recording by SNDCTL_DSP_STEREO */
	  jlog("Stat: adin_oss: recording now set to stereo, using left channel\n");
	  stereo_rec = TRUE;
	  ok_p = TRUE;
	}
      }
    }
    if (! ok_p) {		/* not setup yet */
      /* try using stereo input with sndctl_dsp_channels */
      channels = 2;
      if (ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
	/* failed: SNDCTL_DSP_CHANNELS not supported */
	jlog("Stat: adin_oss: failed to set stereo input using sndctl_dsp_channels\n");
      } else {
	if (channels != 2) {
	  /* failed to set stereo recording by SNDCTL_DSP_CHANNELS */
	  jlog("Stat: adin_oss: failed to set stereo input using sndctl_dsp_channels\n");
	} else {
	  /* succeeded to set stereo recording by SNDCTL_DSP_CHANNELS */
	  jlog("Stat: adin_oss: recording now set to stereo, using left channel\n");
	  stereo_rec = TRUE;
	  ok_p = TRUE;
	}
      }
    }
    if (! ok_p) {		/* all failed */
      jlog("Error: adin_oss: failed to setup recording channels\n");
      return FALSE;
    }
  }

  samplerate = srate;
  if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &samplerate) == -1) {
    jlog("Erorr: adin_oss: failed to set sample rate to %dHz\n", srate);
    return(FALSE);
  }
  if (samplerate < srate - FREQALLOWRANGE || samplerate > srate + FREQALLOWRANGE) {
    jlog("Error: adin_oss: failed to set sampling rate to near %dHz. (%d)\n", srate, samplerate);
    return FALSE;
  }
  if (samplerate != srate) {
    jlog("Warning: adin_oss: specified sampling rate was %dHz but set to %dHz, \n", srate, samplerate);
  }
  jlog("Stat: adin_oss: sampling rate = %dHz\n", samplerate);

  /* get actual fragment size */
  if (ioctl(audio_fd, SNDCTL_DSP_GETBLKSIZE, &frag_size) == -1) {
    jlog("Error: adin_oss: failed to get fragment size\n");
    return(FALSE);
  }
  if (env == NULL) {
    jlog("Stat: adin_oss: going to set latency to %d msec\n", frag_msec);
  } else {
    jlog("Stat: adin_oss: going to set latency to %d msec (from env LATENCY_MSEC)\n", frag_msec);
  }
  jlog("Stat: adin_oss: audio I/O Latency = %d msec (fragment size = %d samples)\n", frag_size * 1000/ (srate * sizeof(SP16)), frag_size / sizeof(SP16));

  return TRUE;

#endif /* HAS_OSS */
}
 
/** 
 * Start recording.
 * 
 * @param pathname [in] path name to open or NULL for default
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_oss_begin(char *pathname)
{
  char buf[4];
  char *p;
  size_t ret;

  /* set device name */
  if (pathname != NULL) {
    strncpy(devname, pathname, MAXPATHLEN);
    jlog("Stat: adin_oss: device name = %s (from argument)\n", devname);
  } else if ((p = getenv("AUDIODEV")) != NULL) {
    strncpy(devname, p, MAXPATHLEN);
    jlog("Stat: adin_oss: device name = %s (from AUDIODEV)\n", devname);
  } else {
    strncpy(devname, defaultdev, MAXPATHLEN);
    jlog("Stat: adin_oss: device name = %s (application default)\n", devname);
  }

  /* open the device */
  if (adin_oss_open(devname) == FALSE) return FALSE;

  /* Read 1 sample (and ignore it) to tell the audio device start recording.
     (If you knows better way, teach me...) */
  if (stereo_rec) {
    ret = read(audio_fd, buf, 4);
  } else {
    ret = read(audio_fd, buf, 2);
  }
  return(TRUE);
}

/** 
 * Stop recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_oss_end()
{
  if (close(audio_fd) != 0) return FALSE;
  return TRUE;
}

/**
 * @brief  Read samples from device
 * 
 * Try to read @a sampnum samples and returns actual number of recorded
 * samples currently available.  This function will block at most
 * MAXPOLLINTERVAL msec, until at least one sample can be obtained.
 * If no data has been obtained after waiting for MAXPOLLINTERVAL msec,
 * returns 0.
 * 
 * When stereo input, only left channel will be used.
 * 
 * @param buf [out] samples obtained in this function
 * @param sampnum [in] wanted number of samples to be read
 * 
 * @return actural number of read samples, 0 of no sample has been captured in
 * MAXPOLLINTERVAL msec, -2 if error.
 */
int
adin_oss_read(SP16 *buf, int sampnum)
{
#ifndef HAS_OSS
  return -2;
#else
  int size,cnt,i;
  audio_buf_info info;
  fd_set rfds;
  struct timeval tv;
  int status;

  /* check for incoming samples in device buffer */
  /* if there is at least one sample fragment, go next */
  /* if not exist, wait for the data to come for at most MAXPOLLINTERVAL msec */
  /* if no sample fragment has come in the MAXPOLLINTERVAL period, go next */
  FD_ZERO(&rfds);
  FD_SET(audio_fd, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = MAXPOLLINTERVAL * 1000;
  status = select(audio_fd+1, &rfds, NULL, NULL, &tv);
  if (status < 0) {
    /* select() failed */
    jlog("Error: adin_oss: failed to poll device\n");
    return(-2);			/* error */
  }
  if (FD_ISSET(audio_fd, &rfds)) { /* has some data */
    /* get sample num that can be read without blocking */
    if (ioctl(audio_fd, SNDCTL_DSP_GETISPACE, &info) == -1) {
      jlog("Error: adin_oss: failed to get number of samples in the buffer\n");
      return(-2);
    }
    /* get them as much as possible */
    size = sampnum * sizeof(SP16);
    if (size > info.bytes) size = info.bytes;
    if (size < frag_size) size = frag_size;
    size &= ~ 1;		/* Force 16bit alignment */
    cnt = read(audio_fd, buf, size);
    if ( cnt < 0 ) {
      jlog("Error: adin_oss: failed to read samples\n");
      return ( -2 );
    }
    cnt /= sizeof(short);

    if (stereo_rec) {
      /* remove R channel */
      for(i=1;i<cnt;i+=2) buf[(i-1)/2]=buf[i];
      cnt/=2;
    }
    
    if (need_swap) swap_sample_bytes(buf, cnt);
  } else {			/* no data after waiting */
    jlog("Warning: adin_oss: no data fragment after %d msec?\n", MAXPOLLINTERVAL);
    cnt = 0;
  }

  return(cnt);
#endif /* HAS_OSS */
}

/** 
 * 
 * Function to return current input source device name
 * 
 * @return string of current input device name.
 * 
 */
char *
adin_oss_input_name()
{
#ifndef HAS_OSS
  return NULL;
#else
  return(devname);
#endif
}
