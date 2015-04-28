/**
 * @file   adin_mic_sp.c
 * 
 * <JA>
 * @brief  マイク入力 (spAudioライブラリ)
 *
 * spAudioライブラリを使用したマイク入力のための低レベル関数です．
 * 使用するには configure 時に "--with-mictype=sp" を指定して下さい．
 *
 * JuliusはLinuxではミキサーデバイスの設定を一切行いません．録音デバイスの
 * 選択（マイク/ライン）や録音ボリュームの調節は xmixer など他のツールで
 * 行なって下さい．
 *
 * このコードは坂野秀樹さんの作です．spAudio については以下もご覧下さい．
 *
 * @sa http://www.sp.m.is.nagoya-u.ac.jp/people/banno/spLibs/index-j.html
 * 
 * </JA>
 * <EN>
 * @brief  Microphone input using spAudio library
 *
 * Low level I/O functions for microphone input using spAudio library.
 * To use, please specify "--with-mictype=sp" options to configure script.
 *
 * Julius does not alter any mixer device setting at all on Linux.  You should
 * configure the mixer for recording source (mic/line) and recording volume
 * correctly using other audio tool such as xmixer.
 *
 * This code has been contributed by Hideaki Banno.
 *
 * @sa http://www.sp.m.is.nagoya-u.ac.jp/people/banno/spLibs/index.html
 * 
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Sun Feb 13 19:16:43 2005
 *
 * $Revision: 1.5 $
 * 
 */
/* adin_mic_sp.c --- adin microphone library for spAudio
 * by Hideki Banno */

#include <sp/spAudioLib.h>

#include <sent/stddefs.h>
#include <sent/adin.h>

static spAudio audio = NULL;	///< Audio descriptor
static long buffer_length = 256; ///< Buffer length

static float rate;		///< Sampling rate specified in adin_mic_standby()

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
  rate = sfreq;
  if (adin_mic_start() == FALSE) return FALSE;
  if (adin_mic_stop() == FALSE) return FALSE;
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
  if (audio == NULL) {
    audio = spInitAudio();
  }
  spSetAudioSampleRate(audio, rate);
  spSetAudioChannel(audio, 1);
  spSetAudioSampleBit(audio, 16);
#ifdef SP_AUDIO_NONBLOCKING
  spSetAudioBlockingMode(audio, SP_AUDIO_NONBLOCKING);
#endif
  
  if (!spOpenAudioDevice(audio, "ro")) {
    jlog("Error: adin_sp: failed to open device\n");
    return FALSE;
  }
    
  return TRUE;
}

/** 
 * Stop recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_mic_end()
{
  spCloseAudioDevice(audio);
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
  long nread;

#ifdef SP_AUDIO_NONBLOCKING
  nread = sampnum;
#else
  if (sampnum <= buffer_length) {
      nread = sampnum;
  } else {
      nread = buffer_length;
  }
#endif
  nread = spReadAudio(audio, (short *)buf, nread);
  
  return nread;
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
  return("SP default device");
}
