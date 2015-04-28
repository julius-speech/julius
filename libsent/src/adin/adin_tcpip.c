/**
 * @file   adin_tcpip.c
 * 
 * <JA>
 * @brief  ネットワーク入力：adinnet クライアントからの音声入力
 *
 * 入力ソースとして Julius の adinnet クライアントを使用する低レベル関数です．
 * 
 * adinnet は ネットワーク上で音声データを送信する Julius 独自の方式です．
 * この入力が選ばれた場合，Julius はadinnetサーバとなり，起動時に
 * クライアントの接続を待ちます．サンプルのadinnetクライアントとして，
 * adintool が Julius に付属していますので参考にしてください．
 * 
 * Linux, Windows, その他サポートされているほとんどの OS で動作します．
 *
 * 他にネットワーク上で音声データをやりとりする方法として, Linux では
 * EsounD を使う方法があります．adin_esd.c をご覧ください．
 *
 * @attention サーバ側とクライアント側でサンプリングレート
 * の設定を一致させる必要があります．接続時に両者間でチェックは行われません．
 *
 * @bug マシンバイトオーダーの異なるマシン同士で正しく通信できません．
 * </JA>
 * <EN>
 * @brief  Audio input from adinnet client
 *
 * Low level I/O functions for audio input from adinnet client.
 * The adinnet server/client is a scheme to transfer speech data via tcp/ip
 * network with segmentation information, implemented for Julius.
 * When this input is selected, Julius becomes adinnet server and
 * wait for a client to connect.
 * The sample client "adintool" is also included in Julius package.
 *
 * This input works on Linux, Windows and most OS where Julius can run.
 *
 * "EsounD" is an another method of obtaining audio input from network
 * on Linux.  See adin_esd.c for details.
 *
 * @attention The sampling rate setting on both side (server and client)
 * should be the same.  They will not be checked when connected.
 *
 * @bug Does not work between different machine byte order.
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Mon Feb 14 14:55:03 2005
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
#include <sent/tcpip.h>

static int adinnet_sd = -1;	///< Listen socket for adinserv
static int adinnet_asd = -1;	///< Accept socket for adinserv

#ifdef FORK_ADINNET
static pid_t child;		/* child process ID (0 if myself is child) */
#endif

/** 
 * Initialize as adinnet server: prepare to become server.
 * 
 * @param freq [in] required sampling frequency
 * @param port_str [in] port number in string
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_tcpip_standby(int freq, void *port_str)
{
  int port;

  port = atoi((char *)port_str);

  if ((adinnet_sd = ready_as_server(port)) < 0) {
    jlog("Error: adin_tcpip: cannot ready for server\n");
    return FALSE;
  }

  jlog("Stat: adin_tcpip: ready for server\n");

  return TRUE;
}

/** 
 * Wait for connection from adinnet client and begin audio input stream.
 *
 * @param pathname [in] path name to open or NULL for default
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_tcpip_begin(char *pathname)
{
#ifdef FORK_ADINNET
    /***********************************/
    /*** server infinite loop here!! ***/
    /***********************************/
    for (;;) {
      /* wait connection */
      jlog("Stat: adin_tcpip: waiting connection...\n");
      if ((adinnet_asd = accept_from(adinnet_sd)) < 0) {
	return FALSE;
      }
      jlog("Stat: adin_tcpip: connected\n");
      /* fork self */
      child = fork();
      if (child < 0) {		/* error */
	jlog("Error: adin_tcpip: fork failed\n");
	return FALSE;
      }
      /* child thread should handle this request */
      if (child == 0) {		/* child thread */
	break;			/* proceed */
      } else {			/* parent thread */
	jlog("Stat: adin_tcpip: forked process [%d] handles this request\n", child);
      }
    }
#else  /* ~FORK_ADINNET */
    jlog("Stat: adin_tcpip: waiting connection...\n");
    if ((adinnet_asd = accept_from(adinnet_sd)) < 0) {
      return FALSE;
    }
    jlog("Stat: adin_tcpip: connected\n");
#endif /* FORK_ADINNET */

  return TRUE;
}

/** 
 * @brief  End recording.
 *
 * If last input segment was segmented by end of connection, close socket
 * and wait for next connection.  Otherwise, it means that the last input
 * segment was segmented by either end-of-segment signal
 * from client side or speech detection function in server side, so just
 * wait for a next input in the current socket.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_tcpip_end()
{
    /* end of connection */
    close_socket(adinnet_asd);
#ifdef FORK_ADINNET
    /* terminate this child process here */
    jlog("Stat: adin_tcpip: connection end, child process now exit\n");
    exit(0);
#else
    /* wait for the next connection */
    jlog("Stat: adin_tcpip: connection end\n");
#endif

  return TRUE;
}

/** 
 * Try to read @a sampnum samples and returns actual sample num recorded.
 * This function will not block.
 *
 * If data of zero length has been received, it is treated as a end-of-segment
 * marker from the client.  If data below zero length has been received,
 * it means the client has finished the overall input stream transmission and
 * want to disconnect.
 * 
 * @param buf [out] samples obtained in this function
 * @param sampnum [in] wanted number of samples to be read
 * 
 * @return actural number of read samples, -1 if EOF, -2 if error.
 */
int
adin_tcpip_read(SP16 *buf, int sampnum)
{
  int cnt, ret;
  fd_set rfds;
  struct timeval tv;
  int status;

  /* check if some commands are waiting in queue */
  FD_ZERO(&rfds);
  FD_SET(adinnet_asd, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 10000;		/* 10msec */
  status = select(adinnet_asd+1, &rfds, NULL, NULL, &tv);
  if (status < 0) {		/* error */
    jlog("Error: adin_tcpip: failed to poll socket\n");
    return -2;			/* error return */
  }
  if (status > 0) {		/* there are some data */
    /* read one data segment, leave rest even if any for avoid blocking */
    ret = rd(adinnet_asd, (char *)buf, &cnt, sampnum * sizeof(SP16));
    if (ret == 0) {
      /* end of segment mark */
      return -3;
    }
    if (ret < 0) {
      /* end of input, mark */
      return -1;
    }
  } else {			/* time out, no data */
    cnt = 0;
  }
  cnt /= sizeof(SP16);
#ifdef WORDS_BIGENDIAN
  swap_sample_bytes(buf, cnt);
#endif
  return cnt;
}

/** 
 * Tell the adinnet client to pause transfer.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_tcpip_send_pause()
{
   int count;
   char com;
   /* send stop command to adinnet client */
   com = '0';
   count = wt(adinnet_asd, &com, 1);
   if (count < 0) jlog("Warning: adin_tcpip: cannot send pause command to client\n");
   jlog("Stat: adin_tcpip: sent pause command to client\n");
   return TRUE;
}

/** 
 * Tell the adinnet client to resume the paused transfer.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_tcpip_send_resume()
{
  int count;
  char com;
  int cnt, ret;
  fd_set rfds;
  struct timeval tv;
  int status;
  static char *tmpbuf = NULL;

  /* check if some commands are waiting in queue */
  count = 0;
  do {
    FD_ZERO(&rfds);
    FD_SET(adinnet_asd, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    status = select(adinnet_asd+1, &rfds, NULL, NULL, &tv);
    if (status < 0) {		/* error */
      jlog("Error: adin_tcpip: failed to poll socket\n");
      return FALSE;			/* error return */
    }
    if (status > 0) {		/* there are some data */
      if (tmpbuf == NULL) tmpbuf = (char *)mymalloc(MAXSPEECHLEN);
      ret = rd(adinnet_asd, tmpbuf, &cnt, MAXSPEECHLEN * sizeof(SP16));
    }
    if (cnt > 0) count += cnt;
  } while (status != 0);
  if (count > 0) {
    jlog("Stat: %d samples transfered while pause are flushed\n", count);
  }
    
  /* send resume command to adinnet client */
  com = '1';
  count = wt(adinnet_asd, &com, 1);
  if (count < 0) jlog("Warning: adin_tcpip: cannot send resume command to client\n");
  jlog("Stat: adin_tcpip: sent resume command to client\n");

  /* flush current buffer */
  return TRUE;
}

/** 
 * Tell the adinnet client to terminate transfer.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
adin_tcpip_send_terminate()
{
   int count;
   char com;
   /* send terminate command to adinnet client */
   com = '2';
   count = wt(adinnet_asd, &com, 1);
   if (count < 0) jlog("Warning: adin_tcpip: cannot send terminate command to client\n");
   jlog("Stat: adin_tcpip: sent terminate command to client\n");
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
adin_tcpip_input_name()
{
  return("network socket");
}
