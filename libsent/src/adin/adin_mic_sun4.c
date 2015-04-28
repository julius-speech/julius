/**
 * @file   adin_mic_sun4.c
 * 
 * <JA>
 * @brief  マイク入力 (Sun4)
 *
 * SunOS 4.x でマイク入力を使用するための低レベル音声入力関数です．
 * SunOS 4.x のマシンではデフォルトでこのファイルが使用されます．
 *
 * Sun SunOS 4.1.3 で動作確認をしています．Solaris2.x については
 * adin_mic_sol2.c を御覧下さい．
 *
 * 起動後オーディオ入力はマイクに自動的に切り替わり，ボリュームは
 * J_DEF_VOLUME の値に設定されます．
 *
 * デフォルトのデバイス名は "/dev/audio" です．環境変数 AUDIODEV に
 * デバイス名を指定することで，他のデバイス名を使用できます．
 * </JA>
 * <EN>
 * @brief  Microphone input on Sun4
 *
 * Low level I/O functions for microphone input on SunOS 4.x machines.
 * This file is used as default on SunOS 4.x machines.
 *
 * Tested on SunOS 4.1.3.
 *
 * The microphone input device will be automatically selected by Julius
 * on startup, and volume will be set to J_DEF_VOLUME.
 * 
 * The default device name is "/dev/audio", which can be changed by setting
 * environment variable AUDIODEV.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Sun Feb 13 18:56:13 2005
 *
 * $Revision: 1.8 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#define J_DEF_VOLUME 20		///< Recording volume (range=0-99)

#include <sent/stddefs.h>
#include <sent/adin.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stropts.h>
#include <poll.h>

/// Default device name, can be overridden by AUDIODEV environment variable
#define DEFAULT_DEVICE "/dev/audio"
/// Default volume
static int volume = J_DEF_VOLUME;

/* sound header */
#include <multimedia/libaudio.h>/* see man audio_device(3) */
#include <multimedia/audio_device.h>
static int srate;		///< Required sampling rate
static int afd;			///< Audio file descriptor
static struct pollfd pfd;	///< File descriptor for polling
static audio_info_t ainfo;	///< Audio info
static char *defaultdev = DEFAULT_DEVICE;
static char devname[MAXPATHLEN];

/** 
 * Device initialization: check device capability and open for recording.
 * 
 * @param sfreq [in] required sampling frequency.
 * @param dummy [in] a dummy data
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_standby(int sfreq, void *dummy)
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
  Audio_hdr Dev_hdr, old_hdr;
  double vol;

  /* open the device */
  if ((afd = open(devstr, O_RDONLY)) == -1) {
    if (errno == EBUSY) {
      jlog("Error: adin_sun4: audio device %s is busy\n", devstr);
      return(FALSE);
    } else {
      jlog("Error: adin_sun4: unable to open %s\n",devstr);
      return(FALSE);
    }
  }

  /* set recording port to microphone */
  AUDIO_INITINFO(&ainfo);
  ainfo.record.port = AUDIO_MICROPHONE;
  if (ioctl(afd, AUDIO_SETINFO, &ainfo) == -1) {
    jlog("Error: adin_sun4: failed to set recording port\n");
    return(FALSE);
  }

  /* set recording parameters */
  if (audio_get_record_config(afd, &Dev_hdr) != AUDIO_SUCCESS) {
    jlog("Error: adin_sun4: failed to get recording config\n"); return(FALSE);
  }
  Dev_hdr.sample_rate = srate;
  Dev_hdr.samples_per_unit = 1; /* ? I don't know this param. ? */
  Dev_hdr.bytes_per_unit = 2;
  Dev_hdr.channels = 1;
  Dev_hdr.encoding = AUDIO_ENCODING_LINEAR;
  if (audio_set_record_config(afd, &Dev_hdr) != AUDIO_SUCCESS) {
    jlog("Error: adin_sun4: failed to set recording config\n"); return(FALSE);
  }

  /* set volume */
  vol = (float)volume / (float)100;
  if (audio_set_record_gain(afd, &vol) != AUDIO_SUCCESS) {
    jlog("Error: adin_sun4: failed to set recording volume\n");
    return(FALSE);
  }

  /* flush buffer */
  if((ioctl(afd , I_FLUSH , FLUSHRW)) == -1) {
    jlog("Error: adin_sun4: cannot flush input buffer\n");
    return(FALSE);
  }
  
  /* setup polling */
  pfd.fd = afd;
  pfd.events = POLLIN;

#if 0
  /* pause transfer */
  if (audio_pause_record(afd) == AUDIO_ERR_NOEFFECT) {
    jlog("Error: adin_sun4: cannot pause audio\n");
    return(FALSE);
  }
#endif

  return(TRUE);
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
  char *p;

  /* set device name */
  if (pathname != NULL) {
    strncpy(devname, pathname, MAXPATHLEN);
    jlog("Stat: adin_sun4: device name = %s (from argument)\n", devname);
  } else if ((p = getenv("AUDIODEV")) != NULL) {
    strncpy(devname, p, MAXPATHLEN);
    jlog("Stat: adin_sun4: device name = %s (from AUDIODEV)\n", devname);
  } else {    
    strncpy(devname, defaultdev, MAXPATHLEN);
    jlog("Stat: adin_sun4: device name = %s (application default)\n", devname);
  }

  /* open the device */
  if (adin_mic_open(devname) == FALSE) return FALSE;

#if 0
  /* resume input */
  if (audio_resume_record(afd) == AUDIO_ERR_NOEFFECT) {
    jlog("Error: adin_sun4: cannot resume audio\n");
    return(FALSE);
  }
#endif

  return(TRUE);
}

/** 
 * Stop recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_end()
{
#if 1
  close(afd);
#else
  /* pause input */
  if (audio_pause_record(afd) == AUDIO_ERR_NOEFFECT) {
    jlog("Error: adin_sun4: cannot pause audio\n");
    return(FALSE);
  }
#endif
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
adin_mic_read(SP16 *buf, int sampnum)
{
  int bytes;
  int len;

  /* SunOS4.x needs special dealing when no samples are found */
  len = sampnum * sizeof(SP16);
  bytes = 0;
  while(bytes < len) {
    bytes = read(afd, buf, len);
    if (bytes < 0) {
      if (errno != EAGAIN) {	/* error */
	jlog("Erorr: adin_sun4: failed to read sample\n");
	return(-2);
      } else {			/* retry */
	poll(&pfd, 1L, -1);
      }
    }
  }
  if (bytes < 0) {
    jlog("Error: adin_sun4: failed to read sample\n");
    return(-2);
  }
  return(bytes / sizeof(SP16)); /* success */
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

/* end of file */
