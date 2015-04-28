/*
 * Define this to send parameter file as outprob vector
 *
 */
#define OUTPROBVECTOR

/*
 * Define this to send 26 dim. data as 25 dim. excluding abs. power
 * for feature vector
 *
 */
#define SEND26TO25

#include <stdio.h>
#include <stdlib.h>
#if defined(_WIN32) && !defined(__CYGWIN32__)
#define WINSOCK
#endif

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/** 
 * Generic byte-swapping functions for any size of unit.
 * 
 * @param buf [i/o] data buffer
 * @param unitbyte [in] size of unit in bytes
 * @param unitnum [in] number of unit in the buffer
 */
void
swap_bytes(char *buf, size_t unitbyte, size_t unitnum)
{
  char *p, c;
  int i, j;

  p = buf;
  while (unitnum > 0) {
    i=0; j=unitbyte-1;
    while(i<j) {
      c = p[i]; p[i] = p[j]; p[j] = c;
      i++;j--;
    }
    p += unitbyte;
    unitnum--;
  }
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* generic network functions for connection (win32/unixen) */

#ifdef WINSOCK
#include <winsock2.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/file.h>
#include <sys/types.h>
#include <fcntl.h>
#endif

#ifdef WINSOCK
boolean winsock_initialized = FALSE; ///< TRUE if once initialized
#endif

/* make connection to server */
/* return socket descriptor, -1 on socket error, -2 on connection error, -3 on invalid host name */
int make_connection(char *hostname, int port_num)
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
    fprintf(stderr, "Error: failed to resolve host: %s\n", hostname);
    return -3;
  }
  
  /* create socket */
#ifdef WINSOCK
  if((sd = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET){
    fprintf(stderr, "Error: failed to create socket\n") ;
    fprintf(stderr, "Error: error code: %d\n", WSAGetLastError());
    switch(WSAGetLastError()) {
    case WSANOTINITIALISED: fprintf(stderr, "Error: reason: A successful WSAStartup must occur before using this function.\n"); break;
    case WSAENETDOWN: fprintf(stderr, "Error: reason: The network subsystem or the associated service provider has failed.\n"); break;
    case WSAEAFNOSUPPORT: fprintf(stderr, "Error: reason: The specified address family is not supported. \n"); break;
    case WSAEINPROGRESS: fprintf(stderr, "Error: reason: A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function. \n"); break;
    case WSAEMFILE: fprintf(stderr, "Error: reason: No more socket descriptors are available. \n"); break;
    case WSAENOBUFS: fprintf(stderr, "Error: reason: No buffer space is available. The socket cannot be created. \n"); break;
    case WSAEPROTONOSUPPORT: fprintf(stderr, "Error: reason: The specified protocol is not supported. \n"); break;
    case WSAEPROTOTYPE: fprintf(stderr, "Error: reason: The specified protocol is the wrong type for this socket. \n"); break;
    case WSAESOCKTNOSUPPORT: fprintf(stderr, "Error: reason: The specified socket type is not supported in this address family. \n"); break;
    }
    return -1;
  }

#else
  if((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
    fprintf(stderr, "Error: failed to create socket\n") ;
    return -1;
  }
#endif

  /* make connection */
  memset((char *)&sin, 0, sizeof(sin));
  memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
  sin.sin_family = hp->h_addrtype;
  sin.sin_port = htons((unsigned short)port_num);
  if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    /* failure */
    fprintf(stderr, "Error: failed to connect to %s:%d\n", hostname, port_num);
    return -2;
  }

  return sd;
}

/* close socket */
/* return -1 on failure, 0 on success */
int close_socket(int sd)
{
  int ret;

#ifdef WINSOCK
  ret = closesocket(sd);
#else
  ret = close(sd);
#endif

  return(ret);
}

/* clean up socket at end of program */
void cleanup_socket()
{
#ifdef WINSOCK
  if (winsock_initialized) {
    WSACleanup();
  }
#endif
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* data structure and global variables */

typedef struct {
  int veclen;                 ///< (4 byte)Vector length of an input
  int fshift;                 ///< (4 byte) Frame shift in msec of the vector
  char outprob_p;             ///< (1 byte) != 0 if input is outprob vector
} ConfigurationHeader;

#define VECINNET_PORT 5531	///< Port number to send the data

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* data transfer function */
/* return -1 on error, 0 on success */
int send_data(int sd, void *buf, int bytes)
{
  /* send data size header (4 byte) */
  if (send(sd, &bytes, sizeof(int), 0) != sizeof(int)) {
    fprintf(stderr, "Error: failed to send %lu bytes\n", sizeof(int));
    return -1;
  }

  /* send data body */
  if (send(sd, buf, bytes, 0) != bytes) {
    fprintf(stderr, "Error: failed to send %lu bytes\n", sizeof(int));
    return -1;
  }

  return 0;
}

/* end-of-utterance transfer function */
/* return -1 on error, 0 on success */
int send_end_of_utterance(int sd)
{
  int i;

  /* send header value of '0' as an end-of-utterance marker */
  i = 0;
  if (send(sd, &i, sizeof(int), 0) != sizeof(int)) {
    fprintf(stderr, "Error: failed to send %lu bytes\n", sizeof(int));
    return -1;
  }

  return 0;
}

/* end-of-session transfer function */
/* return -1 on error, 0 on success */
int send_end_of_session(int sd)
{
  int i;

  /* send negative header value as an end-of-session marker */
  i = -1;
  if (send(sd, &i, sizeof(int), 0) != sizeof(int)) {
    fprintf(stderr, "Error: failed to send %lu bytes\n", sizeof(int));
    return -1;
  }

  return 0;
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/* usage: exename paramfile host [port] */
int main(int argc, char *argv[])
{
  int portnum;			///< Port number
  int sd;
  ConfigurationHeader conf;	///< Configuration of vector to be sent

  FILE *fp;
  int framelen;			///< Number of frames in source
  int fshift;			///< Frame shift of source
  unsigned short vecdim;	///< Vector dimension


  /* application argument check */
  if (argc < 3) {
    fprintf(stderr, "usage: %s paramfile hostname [portnum]\n", argv[0]);
    return -1;
  }

  /* open HTK parameter file and read header */
  if ((fp = fopen(argv[1], "r")) == NULL) {
    fprintf(stderr, "Error: failed to open %s\n", argv[1]);
    close_socket(sd); cleanup_socket(); return -1;
  }
  if (fread(&framelen, sizeof(int), 1, fp) != 1) {
    fprintf(stderr, "Error: failed to read header in %s\n", argv[1]);
    close_socket(sd); cleanup_socket(); return -1;
  }
  swap_bytes((char *)&framelen, sizeof(int), 1);
  if (fread(&fshift, sizeof(int), 1, fp) != 1) {
    fprintf(stderr, "Error: failed to read header in %s\n", argv[1]);
    close_socket(sd); cleanup_socket(); return -1;
  }
  swap_bytes((char *)&fshift, sizeof(int), 1);
  if (fread(&vecdim, sizeof(unsigned short), 1, fp) != 1) {
    fprintf(stderr, "Error: failed to read header in %s\n", argv[1]);
    close_socket(sd); cleanup_socket(); return -1;
  }
  swap_bytes((char *)&vecdim, sizeof(unsigned short), 1);
  vecdim /= sizeof(float);
  fseek(fp, sizeof(short), SEEK_CUR);

  printf("%d frames, %d fshift, %d vecdim\n", framelen, fshift, vecdim);

  /* prepare configuration header */
  /* Warning: no parameter type/size check performed at server side */
  conf.veclen = vecdim;			 /* dimension */
  conf.fshift = (float)fshift / 10000.0; /* msec/frame */
#ifdef OUTPROBVECTOR
  conf.outprob_p = 1;			 /* outprob vector */
#else
  conf.outprob_p = 0;			 /* feature vector */
#ifdef SEND26TO25
  conf.veclen--;
#endif
#endif

  /* port number: use VECINNET_PORT (= Julius default) if not specified */
  portnum = (argc >= 4) ? atoi(argv[3]) : VECINNET_PORT;


  /* connect to server */
  if ((sd = make_connection(argv[2], portnum)) < 0) {
    return -1;
  }

  /* send configuration header */
  if (send_data(sd, &conf, sizeof(ConfigurationHeader)) == -1) {
    return -1;
  }
    
  /* send data, frame by frame*/
  {
    int i;
    float *buf;

    buf = (float *)malloc(sizeof(float) * vecdim);
    for (i = 0; i < framelen; i++) {
      if (fread(buf, sizeof(float), vecdim, fp) != vecdim) {
	fprintf(stderr, "Error: failed to read vectors in %s\n", argv[1]);
	close_socket(sd); cleanup_socket(); return -1;
      }
      swap_bytes((char *)buf, sizeof(float), vecdim);

#if !defined(OUTPROBVECTOR) && defined(SEND26TO25)
      /* send 26 dim data as 25 dim */
      {
	int j;
	for (j = 13; j < vecdim; j++) {
	  buf[j-1] = buf[j];
	}
	if (send_data(sd, buf, sizeof(float) * (vecdim - 1)) == -1) {
	  close_socket(sd); cleanup_socket(); return -1;
	}
      }
#else
      if (send_data(sd, buf, sizeof(float) * vecdim) == -1) {
	close_socket(sd); cleanup_socket(); return -1;
      }
#endif
      printf("frame %d\n", i);
    }
  }
  /* send end-of-utterance at each end of utterance unit */
  if (send_end_of_utterance(sd) == -1) {
    close_socket(sd); cleanup_socket(); return -1;
  }

  /* send end-of-session when input was ended */
  if (send_end_of_session(sd) == -1) {
    close_socket(sd); cleanup_socket(); return -1;
  }

  /* close socket */
  close_socket(sd);
  cleanup_socket();

  return 0;
}
