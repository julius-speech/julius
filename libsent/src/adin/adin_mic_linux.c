/**
 * @file   adin_mic_linux.c
 *
 * <JA>
 * @brief  マイク入力 (Linux) - デフォルトデバイス
 *
 * マイク入力のための低レベル関数です．
 * インタフェースを明示指定しない (-input mic) 場合に呼ばれます．
 * ALSA, PulesAudio, OSS, ESD の順で最初に見つかったものが使用されます．
 * それぞれの API を明示的に指定したい場合は "-input" にそれぞれ
 * "alsa", "oss", "pulseaudio", "esd" を指定してください。
 * </JA>
 * <EN>
 * @brief  Microphone input on Linux - default device
 *
 * Low level I/O functions for microphone input on Linux.
 * This will be called when no device was explicitly specified ("-input mic").
 * ALSA, PulseAudio, OSS, ESD will be chosen in this order at compilation time.
 * "-input alsa", "-input oss", "-input pulseaudio" or "-input esd" to
 * specify which API to use.
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

#include <sent/stddefs.h>
#include <sent/adin.h>

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
#if defined(HAS_ALSA)
  return(adin_alsa_standby(sfreq, dummy));
#elif defined(HAS_OSS)
  return(adin_oss_standby(sfreq, dummy));
#elif defined(HAS_PULSEAUDIO)
  return(adin_pulseaudio_standby(sfreq, dummy));
#elif defined(HAS_ESD)
  return(adin_esd_standby(sfreq, dummy));
#else  /* other than Linux */
  jlog("Error: neither of pulseaudio/alsa/oss/esd device is available\n");
  return FALSE;
#endif
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
#if defined(HAS_ALSA)
  return(adin_alsa_begin(pathname));
#elif defined(HAS_OSS)
  return(adin_oss_begin(pathname));
#elif defined(HAS_PULSEAUDIO)
  return(adin_pulseaudio_begin(pathname));
#elif defined(HAS_ESD)
  return(adin_esd_begin(pathname));
#else  /* other than Linux */
  jlog("Error: neither of pulseaudio/alsa/oss/esd device is available\n");
  return FALSE;
#endif
}

/** 
 * Stop recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_end()
{
#if defined(HAS_ALSA)
  return(adin_alsa_end());
#elif defined(HAS_OSS)
  return(adin_oss_end());
#elif defined(HAS_PULSEAUDIO)
  return(adin_pulseaudio_end());
#elif defined(HAS_ESD)
  return(adin_esd_end());
#else  /* other than Linux */
  jlog("Error: neither of pulseaudio/alsa/oss/esd device is available\n");
  return FALSE;
#endif
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
#if defined(HAS_ALSA)
  return(adin_alsa_read(buf, sampnum));
#elif defined(HAS_OSS)
  return(adin_oss_read(buf, sampnum));
#elif defined(HAS_PULSEAUDIO)
  return(adin_pulseaudio_read(buf, sampnum));
#elif defined(HAS_ESD)
  return(adin_esd_read(buf, sampnum));
#else  /* other than Linux */
  jlog("Error: neither of pulseaudio/alsa/oss/esd device is available\n");
  return -2;
#endif
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
#if defined(HAS_ALSA)
  return(adin_alsa_input_name());
#elif defined(HAS_OSS)
  return(adin_oss_input_name());
#elif defined(HAS_PULSEAUDIO)
  return(adin_pulseaudio_input_name());
#elif defined(HAS_ESD)
  return(adin_esd_input_name());
#else  /* other than Linux */
  return("Error: neither of pulseaudio/alsa/oss/esd device is available\n");
#endif
}
