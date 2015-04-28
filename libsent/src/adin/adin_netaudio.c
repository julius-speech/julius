/**
 * @file   adin_netaudio.c
 *
 * <JA>
 * @brief  ネットワーク入力：NetAudio/DatLink サーバからの音声入力
 *
 * 入力ソースとして，DatLink に附属の NetAudio サーバを使用する
 * 低レベル関数です．これを用いることで DatLink の入力を
 * 直接認識することができます．
 * NetAudio がインストールしてあるホストで configure することで
 * コンパイルされます．
 *
 * 関数の実体は adin_na.c で定義されています．
 * </JA>
 * <EN>
 * @brief  Audio input from NetAudio/DatLink server
 *
 * Low level I/O functions for audio input via the NetAudio server.
 * NetAudio is a part of DatLink product, and this feature enables
 * direct live input recognition via DatLink.  This file will be
 * compiled if NetAudio headers and libraries are located on the machine.
 * 
 * The actual procedure are defined in adin_na.c.
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Sun Feb 13 19:50:55 2005
 *
 * $Revision: 1.7 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* Tested with DAT deck, DatLink hardware and NetAudio library
   on Solaris2.5.1  */

/* because of `boolean' type conflict in sentlib and datlink includes, */
/* there are only wrappers. The core functions are defined in adin_na.c */


#include <sent/stddefs.h>
#include <sent/adin.h>

static char server_devname[MAXPATHLEN];  ///< Device name

/** 
 * Connection initialization: check connectivity and open for recording.
 * 
 * @param sfreq [in] required sampling frequency
 * @param arg [in] server device name string to connect
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_netaudio_standby(int sfreq, void *arg)
{
  strncpy(server_devname, arg, MAXPATHLEN);
  if (NA_standby(sfreq, server_devname) == 0) return(FALSE); /* error */
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
adin_netaudio_begin(char *pathname)
{
  NA_start();
  return(TRUE);
}

/** 
 * Stop recording.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_netaudio_end()
{
  NA_stop();
  return(TRUE);
}

/**
 * @brief  Read samples from the daemon.
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
adin_netaudio_read(SP16 *buf, int sampnum)
{
  int cnt;
  cnt = NA_read(buf, sampnum);
  if (cnt < 0) {
    jlog("Error: adin_na: failed to read sample\n");
    return(-2);			/* return negative on error */
  }
  return(cnt);
}

/** 
 * 
 * Function to return current input source device name
 * 
 * @return string of current input device name.
 * 
 */
char *
adin_netaudio_input_name()
{
  return(server_devname);
}
