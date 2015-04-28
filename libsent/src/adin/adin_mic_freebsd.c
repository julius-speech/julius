/**
 * @file   adin_mic_freebsd.c
 *
 * <JA>
 * @brief  マイク入力 (FreeBSD)
 *
 * マイク入力のための低レベル関数です．FreeBSDでこのファイルが使用されます．
 *
 * サウンドカードが 16bit モノラル で録音できることが必須です．
 * 
 * JuliusはFreeBSDでミキサーデバイスの設定を一切行いません．録音デバイスの
 * 選択（マイク/ライン）や録音ボリュームの調節は他のツールで
 * 行なって下さい．
 *
 * デフォルトのデバイス名は "/dev/dsp" です．環境変数 AUDIODEV に
 * デバイス名を指定することで，他のデバイス名を使用できます．
 *
 * 動作確認はFreeBSD 3.2-RELEASE で行なわれました．サウンドドライバは
 * snd を使用しています．
 * </JA>
 * <EN>
 * @brief  Microphone input on FreeBSD
 *
 * Low level I/O functions for microphone input on FreeBSD.
 *
 * To use microphone input in FreeBSD, sound card and sound driver must
 * support 16bit monaural recording.
 *
 * Julius does not alter any mixer device setting at all.  You should
 * configure the mixer for recording source (mic/line) and recording volume
 * correctly using other audio tool.
 *
 * The default device name is "/dev/dsp", which can be changed by setting
 * environment variable AUDIODEV.
 *
 * Tested on FreeBSD 3.2-RELEASE with snd driver.
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Sun Feb 13 16:18:26 2005
 *
 * $Revision: 1.10 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* Thanks to Kentaro Nagatomo for information */
/* All functions are the same as OSS version, except the header filename */

#include <sent/stddefs.h>
#include <sent/adin.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

/* sound header */
#if defined(HAVE_SYS_SOUNDCARD_H)
#include <sys/soundcard.h>
#elif defined(HAVE_MACHINE_SOUNDCARD_H)
#include <machine/soundcard.h>
#endif

/// Default device name, can be overridden by AUDIODEV environment variable
#define DEFAULT_DEVICE "/dev/dsp"

static int srate;		///< Required sampling rate
static int audio_fd;		///< Audio descriptor
static boolean need_swap;	///< Whether input samples need byte-swapping
struct pollfd fds[1];		///< Workarea for polling device

#define FREQALLOWRANGE 200	///< Acceptable width of sampling frequency
#define POLLINTERVAL 200	///< Polling interval in miliseconds

static char *defaultdev = DEFAULT_DEVICE; ///< Default device name
static char devname[MAXPATHLEN];		///< Current device name

/** 
 * Device initialization: check machine capability
 * 
 * @param sfreq [in] required sampling frequency.
 * @param arg [in] a dummy data
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_standby(int sfreq, void *arg)
{
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
adin_mic_open(char *devstr)
{
  int fmt, fmt_can, fmt1, fmt2, rfmt; /* sampling format */
  int samplerate;		/* actual sampling rate */
  int stereo;		/* mono */
  char *p;

  /* open device */
  if ((audio_fd = open(devstr, O_RDONLY)) == -1) {
    jlog("Error: adin_freebsd: failed to open %s\n", devstr);
    return(FALSE);
  }

  /* check whether soundcard can record 16bit data */
  /* and set fmt */
  if (ioctl(audio_fd, SNDCTL_DSP_GETFMTS, &fmt_can) == -1) {
    jlog("Error: adin_freebsd: failed to get formats from audio device\n");
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
    jlog("Error: adin_freebsd: 16bit recording not supported on this device\n");
    return FALSE;
  }
#ifdef DEBUG
  if (need_swap) {
    jlog("Stat: adin_freebsd: samples need swap\n");
  } else {
    jlog("Stat: adin_freebsd: samples need not swap\n");
  }
#endif
  
  if (close(audio_fd) != 0) return FALSE;

  /* re-open for recording */
  /* open device */
  if ((audio_fd = open(devstr, O_RDONLY)) == -1) {
    jlog("Error: adin_freebsd: failed to open %s", devstr);
    return(FALSE);
  }
  /* set format, samplerate, channels */
  rfmt = fmt;
  if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &rfmt) == -1) {
    jlog("Error: adin_freebsd: failed to get available formats from device\n");
    return(FALSE);
  }
  if (rfmt != fmt) {
    jlog("Error: adin_freebsd: 16bit recording is not supported on this device\n");
    return FALSE;
  }

  stereo = 0;			/* mono */
  if (ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo) == -1) {
    jlog("Error: adin_freebsd: failed to set monoral recording\n");
    return(FALSE);
  }
  if (stereo != 0) {
    jlog("Error: adin_freebsd: monoral recording not supported on this device\n");
    return FALSE;
  }

  samplerate = srate;
  if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &samplerate) == -1) {
    jlog("Erorr: adin_freebsd: failed to set sample rate to %dHz\n", srate);
    return(FALSE);
  }
  if (samplerate < srate - FREQALLOWRANGE || samplerate > srate + FREQALLOWRANGE) {
    jlog("Error: adin_freebsd: failed to set sampling rate to near %dHz. (%d)\n", srate, samplerate);
    return FALSE;
  }
  if (samplerate != srate) {
    jlog("Warning: adin_freebsd: specified sampling rate was %dHz but set to %dHz, \n", srate, samplerate);
  }

  /* set polling status */
  fds[0].fd = audio_fd;
  fds[0].events = POLLIN;
  
  return TRUE;
}


/** 
 * Start recording.
 *
 * @param pathname [in] path name to open or NULL for default
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_begin(char *pathname)
{
  /* set device name */
  if (pathname != NULL) {
    strncpy(devname, pathname, MAXPATHLEN);
    jlog("Stat: adin_freebsd: device name = %s (from argument)\n", devname);
  } else if ((p = getenv("AUDIODEV")) != NULL) {
    strncpy(devname, p, MAXPATHLEN);
    jlog("Stat: adin_freebsd: device name = %s (from AUDIODEV)\n", devname);
  } else {
    strncpy(devname, defaultdev, MAXPATHLEN);
    jlog("Stat: adin_freebsd: device name = %s (application default)\n", devname);
  }

  /* open the device */
  return(adin_mic_open(devname));
}

/** 
 * Stop recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_end()
{
  if (close(audio_fd) != 0) return FALSE;
  return TRUE;
}

/**
 * @brief  Read samples from device
 * 
 * Try to read @a sampnum samples and returns actual number of recorded
 * samples currently available.  This function will block until at least one
 * sample can be obtained.
 * 
 * @param buf [out] samples obtained in this function
 * @param sampnum [in] wanted number of samples to be read
 * 
 * @return actural number of read samples, -2 if error.
 */
int
adin_mic_read(SP16 *buf, int sampnum)
{
  int size,cnt;
  audio_buf_info info;

  /* wait till at least one sample can be read */
  poll(fds, 1, POLLINTERVAL);
  /* get actual sample num in the device buffer */
  if (ioctl(audio_fd, SNDCTL_DSP_GETISPACE, &info) == -1) {
    jlog("Error: adin_freebsd: adin_mic_read: sndctl_dsp_getispace");
    return(-2);
  }
  
  /* get them as much as possible */
  size = sampnum * sizeof(SP16);
  if (size > info.bytes) size = info.bytes;
  cnt = read(audio_fd, buf, size);
  if ( cnt < 0 ) {
    jlog("Error: adin_freebsd: adin_mic_read: read error\n");
    return ( -2 );
  }
  cnt /= sizeof(short);
  if (need_swap) swap_sample_bytes(buf, cnt);
  return(cnt);
}

/** 
 * Function to pause audio input (wait for buffer flush)
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_pause()
{
  return TRUE;
}

/** 
 * Function to terminate audio input (disgard buffer)
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_terminate()
{
  return TRUE;
}
/** 
 * Function to resume the paused / terminated audio input
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_resume()
{
  return TRUE;
}

/** 
 * 
 * Function to return current input source device name
 * 
 * @return string of current input device name.
 * 
 */
char *
adin_mic_input_name()
{
  return(devname);
}
