/**
 * @file   vecin_net.c
 * 
 * @brief  Feature input from network
 *
 *
 * $Revision: 1.1 $
 * 
 */

/**
 * feature input functions
 * 
 * Required:
 *   - vecin_standby()
 *   - vecin_open()
 *   - vecin_get_configuration()
 *   - vecin_read()
 *   - vecin_close()
 *   - vecin_terminate()
 *   - vecin_pause()
 *   - vecin_resume()
 *   - vecin_input_name()
 * 
 */

#include <sent/stddefs.h>
#include <sent/tcpip.h>

/// Return code of vecin_read()
#define ADIN_NOERROR 0
#define ADIN_EOF -1
#define ADIN_ERROR -2
#define ADIN_SEGMENT -3

/// Return code of local_read_data()
#define LRD_NOERROR 0
#define LRD_ENDOFSEGMENT 1
#define LRD_ENDOFSTREAM 2
#define LRD_ERROR 3

static int vecin_sd = -1;	///< Listening socket
static int vecin_asd = -1;	///< Accepted  socket

typedef struct {
  int veclen;		      ///< (4 byte)Vector length of an input
  int fshift;		      ///< (4 byte) Frame shift in msec of the vector
  char outprob_p;	      ///< (1 byte) TRUE if input is outprob vector
} ConfigurationHeader;

ConfigurationHeader conf;

/************************************************************************/
int
local_read_data(int sd, void *buf, int bytes)
{
  int len;
  int ret;
  int toread, offset;

  /* get header */
  toread = sizeof(int);
  offset = 0;
  while (toread > 0) {
    ret = recv(sd, ((char *)&len) + offset, toread, 0);
    if (ret < 0) {
      /* error */
      jlog("Error: vecin_net: failed to read length data %d/%d\n", offset, sizeof(int));
      return LRD_ERROR;
    }
    toread -= ret;
    offset += ret;
  }
  if (len == 0) {
    /* end of segment mark */
    return LRD_ENDOFSEGMENT;
  }
  if (len < 0) {
    /* end of input, mark */
    return LRD_ENDOFSTREAM;
  }

  if (len != bytes) {
    jlog("Error: vecin_net: protocol error: length not match: %d, %d\n", bytes, len);
    return LRD_ERROR;
  }

  /* get body */
  toread = len;
  offset = 0;
  while (toread > 0) {
    ret = recv(sd, ((char *)buf) + offset, toread, 0);
    if (ret < 0) {
      /* error */
      jlog("Error: vecin_net: failed to read data: %d / %d\n", offset, len);
      return LRD_ERROR;
    }
    toread -= ret;
    offset += ret;
  }
  
  return LRD_NOERROR;
}


/************************************************************************/

/**
 * @brief  Initialize input device (required)
 *
 * This will be called only once at start up of Julius.  You can
 * check if the input file exists or prepare a socket for connection.
 *
 * If this function returns FALSE, Julius will exit.
 * 
 * JuliusLib: this function will be called at j_adin_init().
 *
 * @return TRUE on success, FALSE on failure.
 * </EN>
 */
boolean
vecin_standby()
{
  vecin_sd = -1;
  vecin_asd = -1;
  conf.veclen = 0;
  conf.fshift = 0;
  conf.outprob_p = FALSE;

  if ((vecin_sd = ready_as_server(VECINNET_PORT)) < 0) {
    jlog("Error: vecin_net: cannot listen port %d to be a server\n", VECINNET_PORT);
    return FALSE;
  }

  jlog("Stat: vecin_net: listening port %d\n", VECINNET_PORT);

  return TRUE;
}

/**
 * @brief  Open an input (required)
 *
 * This function should open a new input.  You may open a feature
 * vector file, or wait for connection at this function.
 *
 * If this function returns FALSE, Julius will exit recognition loop.
 * 
 * JuliusLib: this will be called at j_open_stream().
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
vecin_open()
{
  if (vecin_sd == -1) {
    jlog("Error: vecin_net: socket not ready\n");
    return FALSE;
  }
  if (vecin_asd != -1) {
    vecin_close();
  }
  jlog("Stat: vecin_net: waiting connection...\n");
  if ((vecin_asd = accept_from(vecin_sd)) < 0) {
    jlog("Error: vecin_net: failed to accept connection\n");
    return FALSE;
  }
  jlog("Stat: vecin_net: connected\n");

  /* receive configuration parameters from client */
  if (local_read_data(vecin_asd, &conf, sizeof(ConfigurationHeader)) != LRD_NOERROR) {
    jlog("Error: vecin_net: failed to receive first configuration data\n");
    return FALSE;
  }
  
  return TRUE;
}

/** 
 * @brief  Return configuration parameters for this input (required)
 * 
 * This function should return configuration parameters about the input.
 *
 * When opcode = 0, return the dimension (length) of input vector.
 * 
 * When opcode = 1, return the frame interval (time between frames) in
 * milliseconds.
 * 
 * When opcode = 2, parameter type code can be returned.  The code should
 * the same format as used in HTK parameter file header.  This is for
 * checking the input parameter type against acousitc model, and
 * you can disable the checking by returning "0xffff" to this opcode.
 *
 * When opcode = 3, should return 0 if the input vector is feature
 * vector, and 1 if the input is outprob vector.
 * 
 * @param opcode [in] requested operation code
 * 
 * @return values required for the opcode as described.
 */
int
vecin_get_configuration(int opcode)
{
  if (vecin_asd == -1) {
    jlog("Error: vecin_net: vecin_get_configuration() called without connection\n");
    return 0;
  }
  switch(opcode) {
  case 0:		   /* return number of elements in a vector */
    return(conf.veclen);
  case 1:/* return msec per frame */
    return(conf.fshift);
  case 2:/* return parameter type specification in HTK format */
    /* return 0xffff to disable checking */
    return(0xffff);
  case 3:/* return 0 if feature vector input, 1 if outprob vector input */
    return(conf.outprob_p ? 1 : 0);
  }
}

/**
 * @brief  Read a vector from input (required)
 *
 * This will be called repeatedly at each frame, and the read vector
 * will be processed immediately, and then this function is called again.
 *
 * Return value of ADIN_EOF tells end of stream to Julius, which
 * causes Julius to finish current recognition and close stream.
 * ADIN_SEGMENT requests Julius to segment the current input.  The
 * current recognition will be stopped at this point, recognition
 * result will be output, and then Julius continues to the next input.
 * The behavior of ADIN_SEGMENT is similar to ADIN_EOF except that
 * ADIN_SEGMENT does not close/open input, but just stop and restart
 * the recognition.  At last, return value should be ADIN_ERROR on
 * error, in which Julius exits itself immediately.
 * 
 * @param vecbuf [out] store a vector obtained in this function
 * @param veclen [in] vector length
 * 
 * @return 0 on success, ADIN_EOF on end of stream, ADIN_SEGMENT to
 * request segmentation to Julius, or ADIN_ERROR on error.
 */
int
vecin_read(float *vecbuf, int veclen)
{
  int ret;

  if (vecin_asd == -1) {
    jlog("Error: vecin_net: vecin_read() called without connection\n");
    return ADIN_ERROR;
  }

  ret = local_read_data(vecin_asd, vecbuf, sizeof(float) * veclen);

  switch(ret) {
  case LRD_ENDOFSEGMENT:	/* received an end of segment */
    jlog("Stat: vecin_net: received end of segment\n");
    return ADIN_SEGMENT;
  case LRD_ENDOFSTREAM:		/* received an end of stream */
    jlog("Stat: vecin_net: received end of stream\n");
    return ADIN_EOF;
  case LRD_ERROR:		/* some error has occured */
    jlog("Error: vecin_net: error in receiving data\n");
    return ADIN_ERROR;
  }

  return(ADIN_NOERROR);			/* success */
}

/**
 * @brief  Close the current input (required)
 *
 * This function will be called when the input has reached end of file
 * (i.e. the last call of vecin_read() returns ADIN_EOF)
 *       
 * You may close a file or disconnect network client here.
 *
 * If this function returns TRUE, Julius will go again to adin_open()
 * to open another stream.  If returns FALSE, Julius will exit
 * the recognition loop.
 * 
 * JuliusLib: This will be called at the end of j_recognize_stream().
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
vecin_close()
{
  if (vecin_asd == -1) return TRUE;

  /* end of connection */
  close_socket(vecin_asd);
  vecin_asd = -1;

  jlog("Stat: vecin_net: connection closed\n");

  return TRUE;
}

/************************************************************************/

/**
 * @brief  A hook for Termination request (optional)
 *
 * This function will be called when Julius receives a Termination
 * request to stop running.  This can be used to synchronize input
 * facility with Julius's running status.
 * 
 * Termination will occur when Julius is running on module mode and
 * received TERMINATE command from client, or j_request_terminate()
 * is called inside application.  On termination, Julius will stop
 * recognition immediately (discard current input if in process),
 * and wait until received RESUME command or call of j_request_resume().
 *
 * This hook function will be called just after a Termination request.
 * Please note that this will be called when Julius receives request,
 * not on actual termination.
 * 
 * @return TRUE on success, FALSE on failure.
 * 
 */
boolean
vecin_terminate()
{
  printf("terminate request\n");
  return TRUE;
}

/**
 * @brief  A hook for Pause request (optional)
 *
 * This function will be called when Julius receives a Pause request
 * to stop running.  This can be used to synchronize input facility
 * with Julius's running status.
 * 
 * Pause will occur when Julius is running on module mode and
 * received PAUSE command from client, or j_request_pause()
 * is called inside application.  On pausing, Julius will 
 * stop recognition and then wait until it receives RESUME command
 * or j_request_resume() is called.  When pausing occurs while recognition is
 * running, Julius will process it to the end before stops.
 *
 * This hook function will be called just after a Pause request.
 * Please note that this will be called when Julius receives request,
 * not on actual pause.
 *
 * @return TRUE on success, FALSE on failure.
 * 
 */
boolean
vecin_pause()
{
  printf("pause request\n");
  return TRUE;
}

/**
 * @brief  A hook for Resume request (optional)
 *
 * This function will be called when Julius received a resume request
 * to recover from pause/termination status.
 * 
 * Resume will occur when Julius has been stopped by receiving RESUME
 * command from client on module mode, or j_request_resume() is called
 * inside application.
 *
 * This hook function will be called just after a resume request.
 * This can be used to make this A/D-in plugin cooperate with the
 * pause/resume status, for example to tell audio client to restart
 * audio streaming.
 *
 * This function is totally optional.
 * 
 * @return TRUE on success, FALSE on failure.
 * 
 */
boolean
vecin_resume()
{
  printf("resume request\n");
  return TRUE;
}

/**
 * @brief  A function to return current device name for information (optional)
 *
 * This function is totally optional.
 * 
 * @return pointer to the device name string
 * 
 */
char *
vecin_input_name()
{
  return("vector input");
}
/* end of file */
