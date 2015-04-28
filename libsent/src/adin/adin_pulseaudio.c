/**
 * @file   adin_pulseaudio.c
 *
 * <JA>
 * @brief  PulseAudio入力
 *
 * 入力ソースとして PulseAudio API を使用します。
 * PulseAudio はコンパイル時にライブラリが存在する場合、
 * デフォルトのマイク入力APIとして "-input mic" あるいは "-input pulseaudio"
 * で使用できます。
 * </JA>
 * <EN>
 * @brief  Audio input via PulseAudio API
 *
 * Low level I/O functions for audio input via PulseAudio API.
 * This API will be the default API when compiled with PulseAudio library,
 * i.e. if development files exist at compilation, "-input mic" will use
 * This API.  Or you can explicitly specify "-input pulseaudio" to use it.
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Thu Feb 24 00:18:22 2011
 *
 * $Revision: 1.3 $
 * 
 */
/*
 * Copyright (c) 2010-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/adin.h>

#ifdef HAS_PULSEAUDIO
#include <pulse/simple.h>
#include <pulse/error.h>

#define BUFSIZE 512
static pa_simple *s = NULL;
static int srate;
static char name_buf[] = "PulseAudio default device";
#endif

/** 
 * Connection initialization: check connectivity and open for recording.
 * 
 * @param sfreq [in] required sampling frequency
 * @param dummy [in] a dummy data
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_pulseaudio_standby(int sfreq, void *dummy)
{
#ifndef HAS_PULSEAUDIO
  jlog("Error: PulseAudio not compiled in\n");
  return FALSE;
#else
  srate = sfreq;
  return TRUE;
#endif
}
 
/** 
 * Start recording.
 * @a pathname is dummy.
 *
 * @param arg [in] argument
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_pulseaudio_begin(char *arg)
{
#ifndef HAS_PULSEAUDIO
  jlog("Error: PulseAudio not compiled in\n");
  return FALSE;
#else
  pa_sample_spec ss;
  int error;

  ss.format = PA_SAMPLE_S16LE;
  ss.rate = srate;
  ss.channels = 1;
  
  if (!(s = pa_simple_new(NULL, "Julius", PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
    jlog("Error: adin_pulseaudio: pa_simple_new() failed: %s\n", pa_strerror(error));
    return FALSE;
  }
  return TRUE;
#endif
}

/** 
 * Stop recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_pulseaudio_end()
{
#ifndef HAS_PULSEAUDIO
  jlog("Error: PulseAudio not compiled in\n");
  return FALSE;
#else
  if (s != NULL) {
    pa_simple_free(s);
    s = NULL;
  }
  return TRUE;
#endif
}

/**
 * @brief  Read samples from device
 * 
 * Try to read @a sampnum samples and returns actual number of recorded
 * samples currently available.  This function will block until
 * at least one sample was obtained.
 * 
 * @param buf [out] samples obtained in this function
 * @param sampnum [in] wanted number of samples to be read
 * 
 * @return actural number of read samples, -2 if an error occured.
 */
int
adin_pulseaudio_read(SP16 *buf, int sampnum)
{
#ifndef HAS_PULSEAUDIO
  return -2;
#else

  int error;
  int cnt, bufsize;

  bufsize = sampnum * sizeof(SP16);
  if (bufsize > BUFSIZE) bufsize = BUFSIZE;

  if (pa_simple_read(s, buf, bufsize, &error) < 0) {
    jlog("Error: adin_pulseaudio: pa_simple_read() failed: %s\n", pa_strerror(error));
    return (-2);
  }

  cnt = bufsize / sizeof(SP16);

  return(cnt);

#endif
}

/** 
 * 
 * Function to return current input source device name
 * 
 * @return string of current input device name.
 * 
 */
char *
adin_pulseaudio_input_name()
{
#ifndef HAS_PULSEAUDIO
  return NULL;
#else
  return(name_buf);
#endif
}
