/**
 * @file   server.c
 * 
 * <JA>
 * @brief  Julius サーバと通信を行うための低レベル関数群
 * </JA>
 * 
 * <EN>
 * @brief  Low-level functions for send/receive data to/from Julius server
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Mar 24 12:07:24 2005
 *
 * $Revision: 1.5 $
 * 
 */
/*
 * Copyright (c) 2002-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2002-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include "japi.h"

#ifdef WINSOCK
int winsock_initialized = 0;   ///< 1 of winsock has been initialized 
#endif

/** 
 * <JA>
 * Juliusサーバへの接続を行う．
 * 
 * @param hostname [in] 接続先のホスト名
 * @param portnum [in] 接続するポート番号
 * 
 * @return ソケットデスクリプタ
 * </JA>
 * <EN>
 * Establish a connection to Julius server.
 * 
 * @param hostname [in] host name to connect
 * @param portnum [in] port number to connect
 * 
 * @return the socket descriptor
 * </EN>
 */
int
do_connect(char *hostname, int portnum)
{
  static struct hostent *hp;
  static struct sockaddr_in sin;
  int sd;

#ifdef WINSOCK
  /* if not initialized yet, initialize winsock here */
  if (winsock_initialized == 0) {
     WSADATA data;
     WSAStartup(0x1010, &data);
     winsock_initialized = 1;
  }
#endif
  
  /* get host entry */
  if ((hp = gethostbyname(hostname)) == NULL) {
    fprintf(stderr, "Error: host \"%s\" not found\n", hostname);
    exit(1);
  }

#ifdef WINSOCK
  if((sd = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET){
    perror("Error: socket()") ;
    printf("Error code: %d\n", WSAGetLastError());
    switch(WSAGetLastError()) {
    case WSANOTINITIALISED: printf("A successful WSAStartup must occur before using this function.\n"); break;
    case WSAENETDOWN: printf("The network subsystem or the associated service provider has failed.\n"); break;
    case WSAEAFNOSUPPORT: printf("The specified address family is not supported. \n"); break;
    case WSAEINPROGRESS: printf("A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function. \n"); break;
    case WSAEMFILE: printf("No more socket descriptors are available. \n"); break;
    case WSAENOBUFS: printf("No buffer space is available. The socket cannot be created. \n"); break;
    case WSAEPROTONOSUPPORT: printf("The specified protocol is not supported. \n"); break;
    case WSAEPROTOTYPE: printf("The specified protocol is the wrong type for this socket. \n"); break;
    case WSAESOCKTNOSUPPORT: printf("The specified socket type is not supported in this address family. \n"); break;
    }
    exit(1);
  }
#else  /* ~WINSOCK */
  /* create socket */
  if((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
    perror("Error: socket()") ;
    exit(1);
  }
#endif /* ~WINSOCK */
  
  /* connect */
  memset((char *)&sin, 0, sizeof(sin));
  memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
  sin.sin_family = hp->h_addrtype;
  sin.sin_port = htons(portnum);
  fprintf(stderr, "connecting to %s:%d...", hostname, portnum);
  if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    perror("Error");
    exit(1);
  }
  fprintf(stderr, "done\n");
  return(sd);
}

/** 
 * <JA>
 * 接続を切断する．
 * 
 * @param sd [in] 送信ソケット
 * </JA>
 * <EN>
 * Disconnect the server.
 * 
 * @param sd [in] socket descriptor
 * </EN>
 */
void
do_disconnect(int sd)
{
#ifdef WINSOCK
  closesocket(sd);
#else
  if (close(sd) < 0) {
    fprintf(stderr,"Error: close() failed");
  }
#endif
}

/** 
 * <JA>
 * サーバーに文字列を送信する汎用関数(printf 形式)
 * 
 * @param sd [in] 送信ソケット
 * @param fmt [in] フォーマット
 * @param  ... [in] フォーマットに対する引数
 * </JA>
 * <EN>
 * General function to send string to server (printf style).
 * 
 * @param sd [in] socket to send data
 * @param fmt [in] format
 * @param  ... [in] arguments to the format
 * </EN>
 */
void
do_sendf(int sd, char *fmt, ...)
{
  static char buf[MAXLINELEN];
  va_list ap;
  int n;
  
  va_start(ap, fmt);
  vsnprintf(buf, MAXLINELEN, fmt, ap);
#ifdef WINSOCK
  n = send(sd, buf, strlen(buf), 0);
#else
  n = write(sd, buf, strlen(buf));
#endif
  if (n < 0) {
    perror("Error: do_sendf");
  }
  va_end(ap);
}

/** 
 * <JA>
 * サーバにバッファの内容を送信する．
 * 
 * @param sd [in] 送信ソケット
 * @param buf [in] 送信内容の文字列
 * </JA>
 * <EN>
 * Send content of the buffer to server.
 * 
 * @param sd [in] socket to send data
 * @param buf [in] string to send
 * </EN>
 */
void
do_send(int sd, char *buf)
{
  int n;
  
#ifdef WINSOCK
  n = send(sd, buf, strlen(buf), 0);
#else
  n = write(sd, buf, strlen(buf));
#endif
  if (n < 0) {
    perror("Error: do_send");
  }
}

/** 
 * <JA>
 * サーバーからメッセージを一行読み込みバッファに格納する．
 * 末尾の改行コードは削除される．
 * 
 * @param sd [in] 受信ソケット
 * @param buf [out] 受信したメッセージを格納するバッファ
 * @param maxlen [in] @a buf の最大長
 * 
 * @return @a buf へのポインタ, あるいはエラー時はNULLを返す．
 * </JA>
 * <EN>
 * Receive message from server for one line, and store it to buffer.
 * The newline character at end will be stripped.
 * 
 * @param sd [in] socket descriptor to receive data
 * @param buf [out] buffer to store the received message string.
 * @param maxlen [in] maximum allowed length of @a buf
 * 
 * @return pointer equal to @a buf, or NULL if error.
 * </EN>
 */
char *
do_receive(int sd, char *buf, int maxlen)
{
  int cnt;
  char *p;
  
  p = buf;
  while(1) {
#ifdef WINSOCK
    cnt = recv(sd, p, 1, 0);
#else
    cnt = read(sd, p, 1);
#endif
    if (cnt <= 0) return NULL;		/* eof or error */
    if (*p == '\n' && p > buf) {
      *p = '\0';
      break;
    } else {
      if (++p >= buf + maxlen) {
	fprintf(stderr,"Error: do_receive: line too long (> %d)\n", maxlen);
	exit(1);
      }
    }
  }
  return buf;
}
