/**
 * @file   rdwt.c
 * 
 * <JA>
 * @brief  TCP/IPプロセス間通信のための低レベル関数
 * </JA>
 * 
 * <EN>
 * @brief  Low level functions for TCP/IP inter-process communication
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 07:09:25 2005
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

#include <sent/stddefs.h>
#include <sent/tcpip.h>

#define		BUFSZ	4096	///< Buffer size


/** 
 * @brief Read a data segment from a network stream.
 *
 * This function will block until the specified length has been received.
 * 
 * @param fd [in] file descriptor
 * @param data [out] buffer to store the read data
 * @param len [out] received length in bytes
 * @param maxlen [in] maximum length of @a data buffer in bytes
 * 
 * @return received data length in bytes, or -1 on error.
 */
int
rd(int fd, char *data, int *len, int maxlen)
{
  int count=0;
  int tmpbytes, tmplen;
  int toread = sizeof(int), ret, off = 0;
  
  while (toread > 0) {
    ret = 
#ifdef WINSOCK
      recv(fd,((char *)len) + off, toread, 0);
#else
      read(fd,((char *)len) + off, toread);
#endif
    if (ret <= 0) {  
      if (ret < 0) jlog("Error: rdwt: failed to read data at %d / %d\n", count, len);
      return(-1);
    }
    toread -= ret;
    off += ret;
  }
#ifdef WORDS_BIGENDIAN
  swap_bytes((char *)len, sizeof(int), 1);
#endif
  if (*len > maxlen) {
    jlog("Error: rdwt: transfer data length exceeded: %d (>%d)\n", len, maxlen);
    return(-1);
  }
  while (count<(*len)){

    tmplen = (*len) - count;
    if (tmplen > BUFSZ) tmplen = BUFSZ;
    if ((tmpbytes =
#ifdef WINSOCK
	 recv(fd,data+count,tmplen,0)
#else
	 read(fd,data+count,tmplen)
#endif
	 ) < 0) {
      jlog("Error: rdwt: failed to read data at %d / %d\n",count, len);
      return(count);
    }
    count += tmpbytes;
  }
  return(count);
}

/** 
 * @brief Write a data segment to a network stream.
 *
 * @param fd [in] file descriptor
 * @param data [in] buffer that holds data to write
 * @param len [out] received length in bytes
 * 
 * @return actually written data length in bytes, or -1 on error.
 */
int
wt(int fd, char *data, int len)
{
  int tmpbytes;

#ifdef WORDS_BIGENDIAN
  swap_bytes((char *)&len, sizeof(int), 1);
#endif
  /* len == 0 is used to tell end of segment ack */
  if ((tmpbytes=
#ifdef WINSOCK
       send(fd,(char *)&len,sizeof(int),0)
#else
       write(fd,(char *)&len,sizeof(int))
#endif
       ) != sizeof(int)) {
    /*jlog( "failed to write num\n");*/
    return(-1);
  }
#ifdef WORDS_BIGENDIAN
  swap_bytes((char *)&len, sizeof(int), 1);
#endif
  if (len > 0) {
    if ((tmpbytes=
#ifdef WINSOCK
	 send(fd,data,len,0)
#else
	 write(fd,data,len)
#endif
	 ) < 0) {
      jlog("Error: rdwt: failed to write data (%d bytes)\n",len);
      return(-1);
    }
  } else {
    tmpbytes = 0;
  }
  return(tmpbytes);
}
