/**
 * @file   adin_na.c
 *
 * <JA>
 * @brief  NetAudio入力用のサブルーチン
 *
 * adin_netaudio.c 用のサブ関数が定義されています．
 *
 * NetAudio のライブラリの dat_types.h が
 * libsent/include/sent/stddefs.h での定義と一部衝突するため，
 * このようにサブルーチン部分を分離しています．
 * </JA>
 * <EN>
 * @brief  Sub routines for NetAudio input
 *
 * This file defines sub functions for NetAudio input in adin_netaudio.c
 *
 * These functions are separated from adin_netaudio.c because some definitions
 * in NetAudio header "dat_types.h" conflicts with the 
 * include header "sent/stddefs.h".
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Sun Feb 13 19:40:56 2005
 *
 * $Revision: 1.5 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/config.h>

#include <stdio.h>
#include <stdlib.h>

/* sound header */
#include <netaudio.h>
#include <defaults.h>
#define TRUE 1			///< Should be the same definition in stddefs.h
#define FALSE 0			///< Should be the same definition in stddefs.h
typedef short SP16;		///< Should be the same definition in stddefs.h

static NAport *port;		///< NetAudio port
static int need_swap = FALSE;	///< Incoming data is always BIG ENDIAN

/** 
 * Initialize NetAudio device.
 * 
 * @param sfreq [in] sampling frequency
 * @param server_devname [in] server host name
 * 
 * @return 1 on success, 0 on failure.
 */
int
NA_standby(int sfreq, char *server_devname)
{
  NAinfo info;
  char *buf;
  int cnt;

  /* endian check --- incoming data is BE */
#ifdef WORDS_BIGENDIAN
  need_swap = FALSE;
#else  /* LITTLE ENDIAN */
  need_swap = TRUE;
#endif /* WORDS_BIGENDIAN */

  /* Initialize '.datlinkrc' processing */
  /*InitDefaults(argv[0]);*/

  /* Open connection to DAT-Link server on server_devname */
  /* if NULL, env AUDIO_DEVICE is used instead. */
  /* if AUDIO_DEVICE not specified, local port is used */
  port = NAOpen(server_devname);
  if (port == NULL) {
    jlog("Error: adin_na: failed to open netaudio server on %s\n", server_devname);
    return(FALSE);
  }

  /* setup parameters */
  NAGetDefaultInfo(&info);
  info.source            = DL_ISRC_ALL; /* input source: all */
  info.record.sampleRate = sfreq; /* DAT(48kHz)->some freq */
  info.record.precision  = 16;	/* bits per sample */
  info.record.encoding   = NA_ENCODING_LINEAR;
  info.record.channels   = NA_CHANNELS_LEFT; /* mono */
  NASetInfo(port, &info);

  /* open a data connection for recording */
  if (NAOpenData(port, NA_RECORD) == -1) {
    jlog("Error: adin_na: failed to open data connection\n");
    return(FALSE);
  }

  jlog("Stat: adin_na: connected to netaudio server on %s\n", server_devname);
  return(TRUE);
}

/** 
 * Close port. (actually never used, just for reference...)
 * 
 */
static void
NA_close()
{

  /* Flush (delete) any buffered data for recording */
  NAFlush(port, NA_RECORD);

  /* Close the data connection */
  NACloseData(port, 0);

  /* Close connection */
  NAClose(port);

}  

/** 
 * Begin recording.
 * 
 */
void
NA_start()
{
  NABegin(port, NA_RECORD);
}

/** 
 * Pause the recording.
 * 
 */
void
NA_stop()
{
  NAPause(port, NA_RECORD, 1);
}

/**
 * @brief  Read samples from NetAudio port.
 * 
 * Try to read @a sampnum samples and returns actual number of recorded
 * samples currently available.  This function will block until
 * at least some samples are obtained.
 * 
 * @param buf [out] samples obtained in this function
 * @param sampnum [in] wanted number of samples to be read
 * 
 * @return actural number of read samples, -1 if an error occured.
 */
int
NA_read(SP16 *buf, int sampnum)
{
  int cnt;
  cnt = NARead(port, (char *)buf, sampnum * sizeof(SP16)) / sizeof(SP16);
  if (need_swap) swap_sample_bytes(buf, cnt);
  return(cnt);
}
