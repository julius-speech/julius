/**
 * @file   server-client.c
 * 
 * <JA>
 * @brief  サーバ・クライアント接続
 * </JA>
 * 
 * <EN>
 * @brief  Server client connection
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 07:18:13 2005
 *
 * $Revision: 1.6 $
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

#ifdef WINSOCK
boolean winsock_initialized = FALSE; ///< TRUE if once initialized
#endif

/** 
 * Prepare as a server creating a socket for client connection.
 * 
 * @param port_num [in] network port to listen.
 * 
 * @return socket descriptor, or -1 if failed to create socket,
 * -2 if failed to bind, or -3 if failed to listen.
 */
int
ready_as_server(int port_num)
{
  struct sockaddr_in sin;
  int sd;
  int optval;
  int optlen;

#ifdef WINSOCK
  /* init winsock */
  if (!winsock_initialized) {
    WSADATA data;
    WSAStartup(MAKEWORD(2,0), &data);
    winsock_initialized = TRUE;
  }
#endif

  /* create socket */
#ifdef WINSOCK
  if((sd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0)) == INVALID_SOCKET){
    jlog("Error: server-client: socket() error\n");
    jlog("Error: server-client: error code = %d\n", WSAGetLastError());
    switch(WSAGetLastError()) {
    case WSANOTINITIALISED: jlog("Error: server-client: reason: A successful WSAStartup must occur before using this function.\n"); break;
    case WSAENETDOWN: jlog("Error: server-client: reason: The network subsystem or the associated service provider has failed.\n"); break;
    case WSAEAFNOSUPPORT: jlog("Error: server-client: reason: The specified address family is not supported. \n"); break;
     case WSAEINPROGRESS: jlog("Error: server-client: reason: A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function. \n"); break;
    case WSAEMFILE: jlog("Error: server-client: reason: No more socket descriptors are available. \n"); break;
    case WSAENOBUFS: jlog("Error: server-client: reason: No buffer space is available. The socket cannot be created. \n"); break;
    case WSAEPROTONOSUPPORT: jlog("Error: server-client: reason: The specified protocol is not supported. \n"); break;
    case WSAEPROTOTYPE: jlog("Error: server-client: reason: The specified protocol is the wrong type for this socket. \n"); break;
    case WSAESOCKTNOSUPPORT: jlog("Error: server-client: reason: The specified socket type is not supported in this address family. \n"); break;
    }
    return -1;
  }
#else  /* ~WINSOCK */
  if((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
    jlog("Error: server-client: socket() error\n");
    return -1;
  }
#endif /* ~WINSOCK */

  /* set socket to allow reuse of local address at bind() */
  /* this option prevent from "error: Address already in use" */
  optval = 1;
  optlen = sizeof(int);
  if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, optlen) != 0) {
    jlog("Error: server-client: socketopt() error\n");
    return -2;
  }

  /* assign name(address) to socket */
  memset((char *)&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons((unsigned short)port_num);
  if(bind(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0){
    jlog("Error: server-client: bind() error\n");
    return -2;
  }
  /* begin to listen */
  if (listen(sd, 5) < 0) {
    jlog("Error: server-client: listen() error\n");
    return -3;
  }

  jlog("Stat: server-client: socket ready as server\n");

  return(sd);
}

/** 
 * @brief  Wait for a request from client.
 *
 * This function blocks until a connection request comes.
 * 
 * @param sd [in] listening server socket descpritor
 * 
 * @return the connected socket descriptor.
 */
int
accept_from(int sd)
{
  static struct sockaddr_in from;
#ifdef HAVE_SOCKLEN_T
  static socklen_t nbyte;
#else  
  static int nbyte;
#endif /* HAVE_SOCKLEN_T */
  int asd;

  nbyte = sizeof(struct sockaddr_in);
  asd = accept(sd, (struct sockaddr *)&from, &nbyte);
  if (asd < 0) {              /* error */
    jlog("Error: server-client: accept() error\n");
    jlog("Error: server-client: failed to accept connection\n");
#ifdef WINSOCK
    switch(WSAGetLastError()) {
    case WSANOTINITIALISED: jlog("Error: server-client: reason: A successful WSAStartup must occur before using this FUNCTION. \n"); break;
    case WSAENETDOWN: jlog("Error: server-client: reason:  The network subsystem has failed. \n"); break;
    case WSAEFAULT: jlog("Error: server-client: reason:  The addrlen parameter is too small or addr is not a valid part of the user address space. \n"); break;
    case WSAEINTR: jlog("Error: server-client: reason:  A blocking Windows Sockets 1.1 call was canceled through WSACancelBlockingCall. \n"); break;
    case WSAEINPROGRESS: jlog("Error: server-client: reason:  A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function. \n"); break;
    case WSAEINVAL: jlog("Error: server-client: reason:  The listen function was not invoked prior to accept. \n"); break;
    case WSAEMFILE: jlog("Error: server-client: reason:  The queue is nonempty upon entry to accept and there are no descriptors available. \n"); break;
    case WSAENOBUFS: jlog("Error: server-client: reason:  No buffer space is available. \n"); break;
    case WSAENOTSOCK: jlog("Error: server-client: reason:  The descriptor is not a socket. \n"); break;
    case WSAEOPNOTSUPP: jlog("Error: server-client: reason:  The referenced socket is not a type that supports connection-oriented service. \n"); break;
    case WSAEWOULDBLOCK: jlog("Error: server-client: reason:  The socket is marked as nonblocking and no connections are present to be accepted. \n"); break;
    }
#endif
    return -1;
  }
  jlog("Stat: server-client: connect from %s\n", inet_ntoa(from.sin_addr));
  return asd;
}
  
/** 
 * Make a connection to a server.
 * 
 * @param hostname [in] server host (host name or numeric IP address)
 * @param port_num [in] port number
 * 
 * @return new socket descriptor, -1 if fails to prepare socket, -2 if fails to connect, -3 if host name is wrong.
 */
int
make_connection(char *hostname, int port_num)
{
  static struct hostent *hp;
  static struct sockaddr_in	sin;
  int sd;
  int trynum;

#ifdef WINSOCK
  /* init winsock */
  if (!winsock_initialized) {
    WSADATA data;
    WSAStartup(0x1010, &data);
    winsock_initialized = TRUE;
  }
#endif

  /* host existence check */
  if ((hp  = gethostbyname(hostname)) == NULL) {
    jlog("Error: server-client: target host not found: %s\n", hostname);
    return -3;
  }

  /* create socket */
#ifdef WINSOCK
  if((sd = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET){
    jlog("Error: server-client: failed to create socket\n") ;
    jlog("Error: server-client: Error code: %d\n", WSAGetLastError());
    switch(WSAGetLastError()) {
    case WSANOTINITIALISED: jlog("Error: server-client: reason: A successful WSAStartup must occur before using this function.\n"); break;
    case WSAENETDOWN: jlog("Error: server-client: reason: The network subsystem or the associated service provider has failed.\n"); break;
    case WSAEAFNOSUPPORT: jlog("Error: server-client: reason: The specified address family is not supported. \n"); break;
    case WSAEINPROGRESS: jlog("Error: server-client: reason: A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function. \n"); break;
    case WSAEMFILE: jlog("Error: server-client: reason: No more socket descriptors are available. \n"); break;
    case WSAENOBUFS: jlog("Error: server-client: reason: No buffer space is available. The socket cannot be created. \n"); break;
    case WSAEPROTONOSUPPORT: jlog("Error: server-client: reason: The specified protocol is not supported. \n"); break;
    case WSAEPROTOTYPE: jlog("Error: server-client: reason: The specified protocol is the wrong type for this socket. \n"); break;
    case WSAESOCKTNOSUPPORT: jlog("Error: server-client: reason: The specified socket type is not supported in this address family. \n"); break;
    }
    return -1;
  }
#else  /* ~WINSOCK */
  if((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
    jlog("Error: server-client: failed to create socket\n") ;
    return -1;
  }
#endif /* ~WINSOCK */

  /* try to connect */
  for (trynum = 0; trynum < CONNECTION_RETRY_TIMES; trynum++) {
    memset((char *)&sin, 0, sizeof(sin));
    memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
    sin.sin_family = hp->h_addrtype;
    sin.sin_port = htons((unsigned short)port_num);
    if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) >= 0) {
      /* success */
      break;
    } else {
      /* failure */
      jlog("Stat: server-client: conection failed\n") ;
      /* retry */
      jlog("Stat: server-client: retry after %d second...\n", CONNECTION_RETRY_INTERVAL);
      sleep(CONNECTION_RETRY_INTERVAL);
    }
  }
  if (trynum == CONNECTION_RETRY_TIMES) {
    /* finally failed */
    jlog("Error: server-client: failed to connect to %s:%d\n", hostname, port_num);
    return -2;
  }

  return sd;
}

#ifndef WINSOCK
/** 
 * Make a connection to a server using unix domain socket.
 * 
 * @param address [in] unix domain socket address (path)
 * 
 * @return new socket descriptor, -1 if fails to prepare socket,
 * -2 if fails to connect.
 */
int
make_connection_unix(char *address)
{
  struct sockaddr_un	ps;
  int len;
  int sd;

  ps.sun_family = PF_UNIX;
  strcpy(ps.sun_path, address);
  len = sizeof(ps.sun_family) + strlen(ps.sun_path);

  if((sd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0){
    jlog("Error: server-client: failed to create socket\n");
    return -1;
  }
  while(connect(sd, (struct sockaddr *)&ps, len) < 0){
    jlog("Error: server-client: failed to conect to %s\n", address);
    /* retry */
    jlog("Error: server-client: retry after %d sec...\n",CONNECTION_RETRY_INTERVAL);
    sleep(CONNECTION_RETRY_INTERVAL);
  }

  jlog("Stat: server-client: connected to unix socket %s\n", address);
  
  return sd;
}
#endif /* ~WINSOCK */

/** 
 * Close socket.
 * 
 * @param sd [in] socket descriptor to close
 * 
 * @return 0 on success, -1 on failure.
 */
int
close_socket(int sd)
{
  int ret;
#ifdef WINSOCK
  ret = closesocket(sd);
#else
  ret = close(sd);
#endif
  return(ret);
}
  
/** 
 * Clean up socket data at program exit.
 * 
 */
void
cleanup_socket()
{
#ifdef WINSOCK
  if (winsock_initialized) {
    WSACleanup();
  }
#endif
}
