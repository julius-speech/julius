/**
 * @file   adin_portaudio.c
 * 
 * <JA>
 * @brief  マイク入力 (portaudioライブラリ)
 *
 * portaudiooライブラリを使用したマイク入力のための低レベル関数です．
 * 使用するには configure 時に "--with-mictype=portaudio" を指定して下さい．
 * Linux および Win32 で使用可能です．Win32モードではこれが
 * デフォルトとなります．
 *
 * 録音デバイスは WASAPI -> ASIO -> DirectSound -> MME の順で選択されます。
 * 使用するデバイスを指定したい場合は、環境変数 PORTAUDIO_DEV でデバイス名
 * （先頭から部分マッチ）を指定するか、PORTAUDIO_DEV_NUM でデバイス番号を
 * 指定してください。使用可能なデバイス名とデバイス番号は起動時に出力されます。
 *
 * Juliusはミキサーデバイスの設定を一切行いません．録音デバイスの
 * 選択（マイク/ライン）や録音ボリュームの調節はWindowsの
 * 「ボリュームコントロール」 や Linux の xmixer など，他のツールで
 * 行なって下さい．
 *
 * portaudio はフリーでクロスプラットホームのオーディオ入出力ライブラリ
 * です．ソースは libsent/src/adin/pa/ に含まれています．このプログラムでは
 * スレッドを利用したcallback を利用して入力音声をリングバッファに取り込んで
 * います．
 *
 * @sa http://www.portaudio.com/
 * </JA>
 * <EN>
 * @brief  Microphone input using portaudio library
 *
 * Low level I/O functions for microphone input using portaudio library.
 * To use, please specify "--with-mictype=portaudio" options
 * to configure script.  This function is currently available for Linux and
 * Win32.  On Windows, this is default.
 *
 * The audio API will be chosen in the following order: WASAPI, ASIO,
 * DirectSound and MME.  You can specify which audio capture device to use
 * by setting the name (entire, or just the first part) to the
 * environment variable "PORTAUDIO_DEV", or the ID number to 
 * "PORTAUDIO_DEV_NUM".  The names and ID numbers of available devices will
 * be scanned and listed at the initialization.
 *
 * Julius does not alter any mixer device setting at all.  You should
 * configure the mixer for recording source (mic/line) and recording volume
 * correctly using other audio tool such as xmixer on Linux, or
 * 'Volume Control' on Windows.
 *
 * Portaudio is a free, cross platform, open-source audio I/O library.
 * The sources are included at libsent/src/adin/pa/.  This program uses
 * ring buffer to store captured samples in callback functions with threading.
 *
 * @sa http://www.portaudio.com/
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Mon Feb 14 12:03:48 2005
 *
 * $Revision: 1.21 $
 * 
 */
/*
 * Copyright (c) 2004-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/speech.h>
#include <sent/adin.h>

/* sound header */
#include <portaudio.h>

#ifndef paNonInterleaved
#define OLDVER
#endif

#undef DDEBUG

/**
 * Define this to choose which audio device to open by querying the
 * device API type in the following order in Windows:
 *  
 *  WASAPI -> ASIO -> DirectSound -> MME
 *  
 * This will be effective when using portaudio library with multiple
 * Host API support, in which case Pa_OpenDefaultStream() will open
 * the first found one (not the one with the optimal performance)
 *
 * (not work on OLDVER)
 * 
 */
#ifndef OLDVER
#define CHOOSE_HOST_API
#endif

/**
 * Maximum Data fragment Length in msec.  Input can be delayed to this time.
 * You can override this value by specifying environment valuable
 * "LATENCY_MSEC".
 *
 * This is not used in the new V19, it uses default value given by the library.
 * At the new V19, you can force latency by PA_MIN_LATENCY_MSEC instead of LATENCY_MSEC.
 * 
 */
#ifdef OLDVER
#define MAX_FRAGMENT_MSEC 128
#endif

/* temporal buffer */
static SP16 *speech;		///< cycle buffer for incoming speech data
static int current;		///< writing point
static int processed;		///< reading point
static boolean buffer_overflowed = FALSE; ///< TRUE if buffer overflowed
static int cycle_buffer_len;	///< length of cycle buffer based on INPUT_DELAY_SEC

/** 
 * PortAudio callback to store the incoming speech data into the cycle
 * buffer.
 * 
 * @param inbuf [in] portaudio input buffer 
 * @param outbuf [in] portaudio output buffer (not used)
 * @param len [in] length of above
 * @param outTime [in] output time (not used)
 * @param userdata [in] user defined data (not used)
 * 
 * @return 0 when no error, or 1 to terminate recording.
 */
static int
#ifdef OLDVER
Callback(void *inbuf, void *outbuf, unsigned long len, PaTimestamp outTime, void *userdata)
#else
Callback(const void *inbuf, void *outbuf, unsigned long len, const PaStreamCallbackTimeInfo *outTime, PaStreamCallbackFlags statusFlags, void *userdata)
#endif
{
#ifdef OLDVER
  SP16 *now;
  int avail;
#else
  const SP16 *now;
  unsigned long avail;
#endif
  int processed_local;
  int written;

  now = inbuf;

  processed_local = processed;

#ifdef DDEBUG
  printf("callback-1: processed=%d, current=%d: recordlen=%d\n", processed_local, current, len);
#endif

  /* check overflow */
  if (processed_local > current) {
    avail = processed_local - current;
  } else {
    avail = cycle_buffer_len + processed_local - current;
  }
  if (len > avail) {
#ifdef DDEBUG
    printf("callback-*: buffer overflow!\n");
#endif
    buffer_overflowed = TRUE;
    len = avail;
  }

  /* store to buffer */
  if (current + len <= cycle_buffer_len) {
    memcpy(&(speech[current]), now, len * sizeof(SP16));
#ifdef DDEBUG
    printf("callback-2: [%d..%d] %d samples written\n", current, current+len, len);
#endif
  } else {
    written = cycle_buffer_len - current;
    memcpy(&(speech[current]), now, written * sizeof(SP16));
#ifdef DDEBUG
    printf("callback-2-1: [%d..%d] %d samples written\n", current, current+written, written);
#endif
    memcpy(&(speech[0]), &(now[written]), (len - written) * sizeof(SP16));
#ifdef DDEBUG
    printf("callback-2-2: ->[%d..%d] %d samples written (total %d samples)\n", 0, len-written, len-written, len);
#endif
  }
  current += len;
  if (current >= cycle_buffer_len) current -= cycle_buffer_len;
#ifdef DDEBUG
  printf("callback-3: new current: %d\n", current);
#endif

  return(0);
}

#ifdef OLDVER
static PortAudioStream *stream = NULL;		///< Stream information
#else
static PaStream *stream = NULL;		///< Stream information
#endif
static int srate;		///< Required sampling rate


#ifdef CHOOSE_HOST_API

// Get device list
// If first argument is NULL, return the maximum number of devices
int
get_device_list(int *devidlist, char **namelist, int maxstrlen, int maxnum)
{
  PaDeviceIndex numDevice = Pa_GetDeviceCount(), i;
  const PaDeviceInfo *deviceInfo;
  const PaHostApiInfo *apiInfo;
  static char buf[256];
  int n;

  n = 0;
  for(i=0;i<numDevice;i++) {
    deviceInfo = Pa_GetDeviceInfo(i);
    if (!deviceInfo) continue;
    if (deviceInfo->maxInputChannels <= 0) continue;
    apiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
    if (!apiInfo) continue;
    if (devidlist != NULL) {
      if ( n >= maxnum ) break;
      snprintf(buf, 255, "%s: %s", apiInfo->name, deviceInfo->name);
      buf[255] = '\0';
      devidlist[n] = i;
      strncpy(namelist[n], buf, maxstrlen);
    }
    n++;
  }
  
  return n;
}

// Automatically choose a device to open
// 
// 1. if the env value of "PORTAUDIO_DEV" is defined and matches any of 
//    "apiInfo->name: deviceInfo->name" string, use it.
// 2. if the env value "PORTAUDIO_DEV_NUM" is defined, use the (value-1)
//    as device id.
// 3. if not yet, default device will be chosen:
// 3.1. in WIN32 environment, search for supported and available API in
//      the following order: WASAPI, ASIO, DirectSound, MME.
//      Note that only the APIs supported by the linked PortAudio
//      library are available.
// 3.2  in other environment, use the default input device.
//
// store the device id to devId_ret and returns 0.
// if devId_ret is -1, tell caller to use default.
// returns -1 on error.
static int
auto_determine_device(int *devId_ret)
{
  int devId;
  PaDeviceIndex numDevice = Pa_GetDeviceCount(), i;
  const PaDeviceInfo *deviceInfo;
  const PaHostApiInfo *apiInfo;
  char *devname;
  static char buf[256];
#if defined(_WIN32) || defined(__CYGWIN__)
  // at win32, force preference order: iWASAPI > ASIO > DirectSound > MME > Other
  int iMME = -1, iDS = -1, iASIO = -1, iWASAPI = -1;
#endif

  // if PORTAUDIO_DEV is specified, match it against available APIs
  devname = getenv("PORTAUDIO_DEV");
  devId = -1;

  // get list of available capture devices
  jlog("Stat: adin_portaudio: sound capture devices:\n");
  for(i=0;i<numDevice;i++) {
    deviceInfo = Pa_GetDeviceInfo(i);
    if (!deviceInfo) continue;
    if (deviceInfo->maxInputChannels <= 0) continue;
    apiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
    if (!apiInfo) continue;
    snprintf(buf, 255, "%s: %s", apiInfo->name, deviceInfo->name);
    buf[255] = '\0';
    jlog("  %d [%s]\n", i+1, buf);
    if (devname && !strncmp(devname, buf, strlen(devname))) {
      // device name (partially) matches PORTAUDIO_DEV
      devId = i;
    }
#if defined(_WIN32) || defined(__CYGWIN__)
    // store the device ID for each API
    switch(apiInfo->type) {
    case paWASAPI: if (iWASAPI < 0) iWASAPI = i; break;
    case paMME:if (iMME   < 0) iMME   = i; break;
    case paDirectSound:if (iDS    < 0) iDS    = i; break;
    case paASIO:if (iASIO  < 0) iASIO  = i; break;
    }
#endif
  }
  if (devname) {
    if (devId == -1) {
      jlog("Error: adin_portaudio: PORTAUDIO_DEV=\"%s\", but no device matches it\n", devname);
      return -1;
    }
    jlog("  --> #%d matches PORTAUDIO_DEV, use it\n", devId + 1);
    *devId_ret = devId;
    return 0;
  }
  if (getenv("PORTAUDIO_DEV_NUM")) {
    devId = atoi(getenv("PORTAUDIO_DEV_NUM")) - 1;
    if (devId < 0 || devId >= numDevice) {
      jlog("Error: PORTAUDIO_DEV_NUM=%d, but device %d not exist\n", devId+1, devId+1);
      return -1;
    }
    jlog("  --> use device %d, specified by PORTAUDIO_DEV_NUM\n", devId + 1);
    *devId_ret = devId;
    return 0;
  }
#if defined(_WIN32) || defined(__CYGWIN__)
  jlog("Stat: adin_portaudio: APIs:");
  if (iWASAPI >= 0) jlog(" WASAPI");
  if (iASIO >= 0) jlog(" ASIO");
  if (iDS >= 0) jlog(" DirectSound");
  if (iMME >= 0) jlog(" MME");
  jlog("\n");
  if (iWASAPI >= 0) {
    jlog("Stat: adin_portaudio: -- WASAPI selected\n");
    devId = iWASAPI;
  } else if (iASIO >= 0) {
    jlog("Stat: adin_portaudio: -- ASIO selected\n");
    devId = iASIO;
  } else if (iDS >= 0) {
    jlog("Stat: adin_portaudio: -- DirectSound selected\n");
    devId = iDS;
  } else if (iMME >= 0) {
    jlog("Stat: adin_portaudio: -- MME selected\n");
    devId = iMME;
  } else {
    jlog("Error: adin_portaudio: no device available, try default\n");
    devId = -1;
  }
  *devId_ret = devId;
#else
  jlog("Stat: adin_portaudio: use default device\n");
  *devId_ret = -1;
#endif
  return 0;
}

#endif

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
 * Open the portaudio device and check capability of the opening device.
 *
 * @param arg [in] argument: if number, use it as device ID
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
adin_mic_open(char *arg)
{
  int sfreq = srate;
  PaError err;
#ifdef OLDVER
  int frames_per_buffer;
  int num_buffer;
#endif
  int latency;
  char *p;
  int devId;

  /* set cycle buffer length */
  cycle_buffer_len = INPUT_DELAY_SEC * sfreq;
  //jlog("Stat: adin_portaudio: INPUT_DELAY_SEC = %d\n", INPUT_DELAY_SEC);
  jlog("Stat: adin_portaudio: audio cycle buffer length = %d bytes\n", cycle_buffer_len * sizeof(SP16));

#ifdef OLDVER
  /* for safety... */
  if (sizeof(SP16) != paInt16) {
    jlog("Error: adin_portaudio: SP16 != paInt16 !!\n");
    return FALSE;
  }
  /* set buffer parameter*/
  frames_per_buffer = 256;
#endif

  /* allocate and init */
  current = processed = 0;
  speech = (SP16 *)mymalloc(sizeof(SP16) * cycle_buffer_len);
  buffer_overflowed = FALSE;


  /* get user-specified latency parameter */
  latency = 0;
  if ((p = getenv("LATENCY_MSEC")) != NULL) {
    latency = atoi(p);
    jlog("Stat: adin_portaudio: setting latency to %d msec (obtained from LATENCY_MSEC)\n", latency);
  }
#ifdef OLDVER
  if (latency == 0) {
    latency = MAX_FRAGMENT_MSEC;
    jlog("Stat: adin_portaudio: setting latency to %d msec\n", latency);
  }
  num_buffer = sfreq * latency / (frames_per_buffer * 1000);
  jlog("Stat: adin_portaudio: framesPerBuffer=%d, NumBuffers(guess)=%d\n",
       frames_per_buffer, num_buffer);
  jlog("Stat: adin_portaudio: audio I/O Latency = %d msec (data fragment = %d frames)\n",
	   (frames_per_buffer * num_buffer) * 1000 / sfreq, 
	   (frames_per_buffer * num_buffer));
#endif

  /* initialize device */
  err = Pa_Initialize();
  if (err != paNoError) {
    jlog("Error: adin_portaudio: failed to initialize: %s\n", Pa_GetErrorText(err));
    return(FALSE);
  }

#ifdef CHOOSE_HOST_API

  if (arg == NULL) {
    // first try to determine the best device
    if (auto_determine_device(&devId) == -1) {
      jlog("Error: adin_portaudio: failed to choose the specified device\n");
      return(FALSE);
    }
    if (devId == -1) {
      // No device has been determined, use the default input device
      devId = Pa_GetDefaultInputDevice();
      if (devId == paNoDevice) {
	jlog("Error: adin_portaudio: no default input device is available or an error was encountered\n");
	return FALSE;
      }
    }
  } else {
    // use the given number as device id
    devId = atoi(arg);
  }
  // output device information to use
  {
    const PaDeviceInfo *deviceInfo;
    const PaHostApiInfo *apiInfo;
    static char buf[256];
    deviceInfo = Pa_GetDeviceInfo(devId);
    if (deviceInfo == NULL) {
      jlog("Error: adin_portaudio: failed to get info for device id %d\n", devId);
      return FALSE;
    }
    apiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
    if (apiInfo == NULL) {
      jlog("Error: adin_portaudio: failed to get API info for device id %d\n", devId);
      return FALSE;
    }
    snprintf(buf, 255, "%s: %s", apiInfo->name, deviceInfo->name);
    buf[255] = '\0';
    jlog("Stat: adin_portaudio: [%s]\n", buf);
    jlog("Stat: adin_portaudio: (you can specify device by \"PORTAUDIO_DEV_NUM=number\"\n");
  }
  
  // open the device
  {
    PaStreamParameters param;
    memset( &param, 0, sizeof(param));
    param.channelCount = 1;
    param.device = devId;
    param.sampleFormat = paInt16;
    if (latency == 0) {
      param.suggestedLatency = Pa_GetDeviceInfo(devId)->defaultLowInputLatency;
      jlog("Stat: adin_portaudio: try to set default low latency from portaudio: %d msec\n", param.suggestedLatency * 1000.0);
    } else {
      param.suggestedLatency = latency / 1000.0;
      jlog("Stat: adin_portaudio: try to set latency to %d msec\n", param.suggestedLatency * 1000.0);
    }
    err = Pa_OpenStream(&stream, &param, NULL, sfreq, 
			0, paNoFlag,
			Callback, NULL);
    if (err != paNoError) {
      jlog("Error: adin_portaudio: error in opening stream: %s\n", Pa_GetErrorText(err));
      return(FALSE);
    }
  }
  {
    const PaStreamInfo *stinfo;
    stinfo = Pa_GetStreamInfo(stream);
    jlog("Stat: adin_portaudio: latency was set to %f msec\n", stinfo->inputLatency * 1000.0);
  }

#else

  // Just open the default stream
  err = Pa_OpenDefaultStream(&stream, 1, 0, paInt16, sfreq, 
#ifdef OLDVER
			     frames_per_buffer,
			     num_buffer, 
#else
				 0,
#endif
			     Callback, NULL);
  if (err != paNoError) {
    jlog("Error: adin_portaudio: error in opening stream: %s\n", Pa_GetErrorText(err));
    return(FALSE);
  }

#endif /* CHOOSE_HOST_API */

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
adin_mic_begin(char *arg)
{
  PaError err;

  /* initialize device and open stream */
  if (adin_mic_open(arg) == FALSE) {
    stream = NULL;
    return(FALSE);
  }
  /* start stream */
  err = Pa_StartStream(stream);
  if (err != paNoError) {
    jlog("Error: adin_portaudio: failed to begin stream: %s\n", Pa_GetErrorText(err));
    stream = NULL;
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
  PaError err;

  if (stream == NULL) return(TRUE);

  /* stop stream (do not wait callback and buffer flush, stop immediately) */
  err = Pa_AbortStream(stream);
  if (err != paNoError) {
    jlog("Error: adin_portaudio: failed to stop stream: %s\n", Pa_GetErrorText(err));
    return(FALSE);
  }
  /* close stream */
  err = Pa_CloseStream(stream);
  if (err != paNoError) {
    jlog("Error: adin_portaudio: failed to close stream: %s\n", Pa_GetErrorText(err));
    return(FALSE);
  }
  /* terminate library */
  err = Pa_Terminate();
  if (err != paNoError) {
    jlog("Error: adin_portaudio: failed to terminate library: %s\n", Pa_GetErrorText(err));
    return(FALSE);
  }
  
  stream = NULL;

  return TRUE;
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
  int current_local;
  int avail;
  int len;

  if (buffer_overflowed) {
    jlog("Error: adin_portaudio: input buffer OVERFLOW, increase INPUT_DELAY_SEC in sent/speech.h\n");
    buffer_overflowed = FALSE;
  }

  while (current == processed) {
#ifdef DDEBUG
    printf("process  : current == processed: %d: wait\n", current);
#endif
    Pa_Sleep(20); /* wait till some input comes */
    if (stream == NULL) return(-1);
  }

  current_local = current;

#ifdef DDEBUG
  printf("process-1: processed=%d, current=%d\n", processed, current_local);
#endif

  if (processed < current_local) {
    avail = current_local - processed;
    if (avail > sampnum) avail = sampnum;
    memcpy(buf, &(speech[processed]), avail * sizeof(SP16));
#ifdef DDEBUG
    printf("process-2: [%d..%d] %d samples read\n", processed, processed+avail, avail);
#endif
    len = avail;
    processed += avail;
  } else {
    avail = cycle_buffer_len - processed;
    if (avail > sampnum) avail = sampnum;
    memcpy(buf, &(speech[processed]), avail * sizeof(SP16));
#ifdef DDEBUG
    printf("process-2-1: [%d..%d] %d samples read\n", processed, processed+avail, avail);
#endif
    len = avail;
    processed += avail;
    if (processed >= cycle_buffer_len) processed -= cycle_buffer_len;
    if (sampnum - avail > 0) {
      if (sampnum - avail < current_local) {
	avail = sampnum - avail;
      } else {
	avail = current_local;
      }
      if (avail > 0) {
	memcpy(&(buf[len]), &(speech[0]), avail * sizeof(SP16));
#ifdef DDEBUG
	printf("process-2-2: [%d..%d] %d samples read (total %d)\n", 0, avail, avail, len + avail);
#endif
	len += avail;
	processed += avail;
	if (processed >= cycle_buffer_len) processed -= cycle_buffer_len;
      }
    }
  }
#ifdef DDEBUG
  printf("process-3: new processed: %d\n", processed);
#endif
  return len;
}

/** 
 * Function to pause audio input (wait for buffer flush)
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_pause()
{
  PaError err;

  err = Pa_StopStream(stream);
  if (err != paNoError) {
    jlog("Error: adin_portaudio: failed to pause stream: %s\n", Pa_GetErrorText(err));
    return FALSE;
  }
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
  PaError err;

  err = Pa_AbortStream(stream);
  if (err != paNoError) {
    jlog("Error: adin_portaudio: failed to terminate stream: %s\n", Pa_GetErrorText(err));
    return FALSE;
  }
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
  PaError err;

  err = Pa_StartStream(stream);
  if (err != paNoError) {
    jlog("Error: adin_portaudio: failed to resume stream: %s\n", Pa_GetErrorText(err));
    return FALSE;
  }
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
  return("Portaudio default device");
}
