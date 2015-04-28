/**
 * @file   wrsamp.c
 *
 * <JA>
 * @brief  音声波形列を big endian のバイトオーダーで書き込む
 * </JA>
 * <EN>
 * @brief  Write waveform data in big endian
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Tue Feb 15 00:58:47 2005
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

#include <sent/stddefs.h>
#include <sent/speech.h>

/** 
 * Write waveform data in big endian to a file descriptor
 * 
 * @param fd [in] file descriptor
 * @param buf [in] array of speech data
 * @param len [in] length of above
 * 
 * @return number of bytes written, -1 on error.
 */
int
wrsamp(int fd, SP16 *buf, int len)
{
  int ret;
#ifndef WORDS_BIGENDIAN
  /* swap byte order to BIG ENDIAN */
  swap_sample_bytes(buf, len);
#endif
  ret = write(fd, buf, len * sizeof(SP16));
#ifndef WORDS_BIGENDIAN
  /* undo byte swap */
  swap_sample_bytes(buf, len);
#endif
  return(ret);
}
