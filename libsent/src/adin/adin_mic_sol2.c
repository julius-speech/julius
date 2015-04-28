/**
 * @file   adin_mic_sol2.c
 * 
 * <JA>
 * @brief  マイク入力 (Solaris2.x)
 *
 * Solaris 2.x でマイク入力を使用するための低レベル音声入力関数です．
 * Solaris 2.x のマシンではデフォルトでこのファイルが使用されます．
 *
 * Sun Solaris 2.5.1 および 2.6 で動作確認をしています．
 * ビッグエンディアンを前提としているため，Solaris x86 では動きません．
 *
 * 起動後オーディオ入力はマイクに自動的に切り替わりますが，
 * ボリュームは自動調節されません．gaintoolなどで別途調節してください． 
 *
 * デフォルトのデバイス名は "/dev/audio" です．環境変数 AUDIODEV に
 * デバイス名を指定することで，他のデバイス名を使用できます．
 * </JA>
 * <EN>
 * @brief  Microphone input on Solaris 2.x
 *
 * Low level I/O functions for microphone input on Solaris 2.x machines.
 * This file is used as default on Solaris 2.x machines.
 *
 * Tested on Sun Solaris 2.5.1 and 2.6.  Also works on later versions.
 * Please note that this will not work on Solaris x86, since machine
 * byte order is fixed to big endian.
 *
 * The microphone input device will be automatically selected by Julius
 * on startup.  Please note that the recoding volue will not be
 * altered by Julius, and appropriate value should be set by another tool
 * such as gaintool.
 * 
 * The default device name is "/dev/audio", which can be changed by setting
 * environment variable AUDIODEV.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Sun Feb 13 19:06:46 2005
 *
 * $Revision: 1.9 $
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stropts.h>

/* sound header */
#include <sys/audioio.h>

/// Default device name, can be overridden by AUDIODEV environment variable
#define DEFAULT_DEVICE "/dev/audio"

static int srate;		///< Required sampling rate
static int afd;			///< Audio file descriptor
static struct audio_info ainfo;	///< Audio format information
static char *defaultdev = DEFAULT_DEVICE;
static char devname[MAXPATHLEN];

/** 
 * Device initialization: check device capability
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
  /* open the device */
  if ((afd = open(devstr, O_RDONLY)) == -1) {
    jlog("Error: adin_sol2: failed to open audio device %s\n", devstr);
    return(FALSE);
  }

#if 0
  {
    /* output hardware info (debug) */
    struct audio_device adev;
    if (ioctl(afd, AUDIO_GETDEV, &adev)== -1) {
      jlog("Erorr: adin_sol2: failed to get hardware info\n");
      return(FALSE);
    }
    jlog("Stat: adin_sol2: Hardware name: %s\n",adev.name);
    jlog("Stat: adin_sol2: Hardware version: %s\n", adev.version);
    jlog("Stat: adin_sol2: Properties: %s\n", adev.config);
  }
#endif

  /* get current setting */
  if (ioctl(afd, AUDIO_GETINFO, &ainfo) == -1) {
    jlog("Error: adin_sol2: failed to get current setting from device\n");
    return(FALSE);
  }
  /* pause for changing setting */
  ainfo.record.pause = 1;
  if (ioctl(afd, AUDIO_SETINFO, &ainfo) == -1) {
    jlog("Erorr: adin_sol2: failed to pause for changing setting\n");
    return(FALSE);
  }
  /* flush current input buffer (in old format) */
  if((ioctl(afd , I_FLUSH , FLUSHR)) == -1) {
    jlog("Error: adin_sol2: failed to flush current input buffer\n");
    return(FALSE);
  }
  /* set record setting */
  ainfo.record.sample_rate = srate;
  ainfo.record.channels = 1;
  ainfo.record.precision = 16;
  ainfo.record.encoding = AUDIO_ENCODING_LINEAR;
  /* ainfo.record.gain = J_DEF_VOLUME * (AUDIO_MAX_GAIN - AUDIO_MIN_GAIN) / 100 + AUDIO_MIN_GAIN; */
  ainfo.record.port = AUDIO_MICROPHONE;
  /* recording should be paused when initialized */
  ainfo.record.pause = 1;

  /* set audio setting, remain pause */
  if (ioctl(afd, AUDIO_SETINFO, &ainfo) == -1) {
    jlog("Error: adin_sol2: failed to set audio setting\n");
    return(FALSE);
  }

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
    jlog("Stat: adin_sol2: device name = %s (from argument)\n", devname);
  } else if ((p = getenv("AUDIODEV")) != NULL) {
    strncpy(devname, p, MAXPATHLEN);
    jlog("Stat: adin_sol2: device name = %s (from AUDIODEV)\n", devname);
  } else {    
    strncpy(devname, defaultdev, MAXPATHLEN);
    jlog("Stat: adin_sol2: device name = %s (application default)\n", devname);
  }

  /* open the device */
  if (adin_mic_open(devname) == FALSE) return FALSE;

  if (ioctl(afd, AUDIO_GETINFO, &ainfo) == -1) {
    jlog("Error: adin_sol2: failed to get audio status\n");
    return(FALSE);
  }
  ainfo.record.pause = 0;
  if (ioctl(afd, AUDIO_SETINFO, &ainfo) == -1) {
    jlog("Error: adin_sol2: failed to set audio status\n");
    return(FALSE);
  }

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
  if (ioctl(afd, AUDIO_GETINFO, &ainfo) == -1) {
    jlog("Error: adin_sol2: failed to get audio status\n");
    return(FALSE);
  }
  ainfo.record.pause = 1;
  if (ioctl(afd, AUDIO_SETINFO, &ainfo) == -1) {
    jlog("Error: adin_sol2: failed to set audio status\n");
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
 * at least some samples are obtained.
 * 
 * @param buf [out] samples obtained in this function
 * @param sampnum [in] wanted number of samples to be read
 * 
 * @return actural number of read samples, -2 if an error occured.
 */
int
adin_mic_read(SP16 *buf, int sampnum)
{
  int cnt;
  cnt = read(afd, buf, sampnum * sizeof(SP16)) / sizeof(SP16);
  if (cnt < 0) {
    jlog("Error: adin_sol2: failed to read sample\n");
    return(-2);
  }
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

/* end of file */
