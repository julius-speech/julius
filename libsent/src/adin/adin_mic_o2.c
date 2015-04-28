/**
 * @file   adin_mic_o2.c
 * 
 * <JA>
 * @brief  マイク入力 (SGI IRIX)
 *
 * SGI IRIX のマイク入力を使用する低レベル音声入力関数です．
 * IRIXマシンではデフォルトでこれが使用されます．
 *
 * SGI O2 Workstation (IRIX6.3) で動作確認をしています．
 *
 * 起動後オーディオ入力はマイクに自動的に切り替わりますが，
 * ボリュームは自動調節されません．apanelコマンドで別途調節してください． 
 * </JA>
 * <EN>
 * @brief  Microphone input on SGI IRIX machine
 *
 * Low level I/O functions for microphone input on a SGI IRIX machine.
 * This file is used as default on IRIX machines.
 *
 * Tested on IRIX 6.3, SGI O2 Workstation.
 *
 * The microphone input device will be automatically selected by Julius
 * on startup.  Please note that the recoding volue will not be
 * altered by Julius, and appropriate value should be set by another tool
 * such as apanel.
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Sun Feb 13 18:42:22 2005
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

#include <sent/stddefs.h>
#include <sent/adin.h>

/* sound header */
#include <audio.h>
static ALconfig ac;		///< Local port settings
static ALport aport;		///< Audio port

/** 
 * Initialize global audio interface to use microphone input.
 * 
 * @param rate [in] sampling rate in Hz
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
adin_o2_setup_global(double rate)
{
  int device;
  ALpv setPVbuf[4];

  setPVbuf[0].param   = AL_INTERFACE;
  setPVbuf[0].value.i = alGetResourceByName(AL_SYSTEM, "Microphone", AL_INTERFACE_TYPE);
  setPVbuf[1].param   = AL_MASTER_CLOCK;
  setPVbuf[1].value.i = AL_CRYSTAL_MCLK_TYPE;
  setPVbuf[2].param   = AL_RATE;
  setPVbuf[2].value.ll= alDoubleToFixed(rate);
  device = alGetResourceByName(AL_SYSTEM, "Microphone", AL_DEVICE_TYPE);
  if (alSetParams(device, setPVbuf, 3) < 0) {
    return FALSE;
  } else {
    return TRUE;
  }
}

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
  long rate;
  long prec = AL_SAMPLE_16;
  long encd = AL_SAMPFMT_TWOSCOMP;
  long chan = AL_MONO;

  /* global setup */
  rate = sfreq;
  if (adin_o2_setup_global((double)rate) == FALSE) { /* failed */
    jlog("Error: adin_o2: cannot setup microphone device (global)\n");
    return(FALSE);
  }

  /* local parameter setup */
  if ((ac = ALnewconfig()) == 0) {
    jlog("Error: adin_o2: cannot config microphone device (local)\n");
    return(FALSE);
  }
  ALsetqueuesize(ac, rate * 2 * 1); /* 2 sec. of mono. */
  ALsetwidth(ac, prec);
  ALsetchannels(ac, chan);
  ALsetsampfmt(ac, encd);

  jlog("Stat: adin_o2: local microphone port successfully initialized\n");
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
  /* open audio port */
  if (pathname != NULL) {
    jlog("Stat: adin_o2: opening audio device \"%s\"\n", pathname);
    aport = ALopenport(pathname,"r",ac);
  } else {
    aport = ALopenport("mic","r",ac);
  }
  if (aport == (ALport)(0)) {
    jlog("Error: adin_o2: cannot open microphone audio port for reading\n");
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
  /* close audio port */
  ALcloseport(aport);
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
  long cnt;

  cnt = ALgetfilled(aport);	/* get samples currently stored in queue */
  if (cnt > sampnum) cnt = sampnum;
  if (ALreadsamps(aport, buf, cnt) < 0) { /* get them */
    jlog("Error: adin_o2: failed to read sample\n");
    return(-2);			/* return negative on error */
  }
  return cnt;
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
  return("Microphone");
}

/* end of file */
