/**
 * @file   tcpip.h
 *
 * <EN>
 * @brief  Definitions relating TCP/IP Networking
 * </EN>
 * <JA>
 * @brief  TCP/IP ネットワーク関連の定義
 * </JA>
 * 
 * @author Akinobu LEE
 * @date   Sat Feb 12 12:26:15 2005
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

#ifndef __SENT_SPEECH_TCPIP__
#define __SENT_SPEECH_TCPIP__

#include	<errno.h>
#if defined(_WIN32) && !defined(__CYGWIN32__)
/* win32 */
#include <winsock2.h>
/// Define if WinSock library should be used in Win32 environment
#define WINSOCK
#else
/* unixen */
#include	<netdb.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<sys/un.h>
#include	<arpa/inet.h>
#include	<sys/file.h>
#include	<sys/types.h>
#include	<fcntl.h>
#endif

/// Default port number of A/D-in server (adinnet)
#define         ADINNET_PORT 5530

/// Default port number of feature server (vecin_net)
#define         VECINNET_PORT 5531

/// Default Host/unit name for NetAudio/DatLink input
#define         NETAUDIO_DEVNAME "localhost:0"

/// Retry time when client connection failed */
#define		CONNECTION_RETRY_TIMES    5 /* retry times */
/// Interval time between retry in seconds
#define		CONNECTION_RETRY_INTERVAL 2 /* sec. */


#ifdef __cplusplus
extern "C" {
#endif

/* net/rdwt.c */
int rd(int fd, char *data, int *len, int maxlen);
int wt(int fd, char *data, int len);
/* net/server_client.c */
int ready_as_server(int port_num);
int accept_from(int sd);
int make_connection(char *hostname, int port_num);
int make_connection_unix(char *address);
int close_socket(int sd);
void cleanup_socket();

/* vecin_net.c */
boolean vecin_standby();
boolean vecin_open();
int vecin_get_configuration(int opcode);
int vecin_read(float *vecbuf, int veclen);
boolean vecin_close();
boolean vecin_terminate();
boolean vecin_pause();
boolean vecin_resume();
char *vecin_input_name();

#ifdef __cplusplus
}
#endif

#endif /* __SENT_SPEECH_TCPIP__ */
