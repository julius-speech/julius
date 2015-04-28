/**
 * @file   jcontrol.c
 * 
 * <JA>
 * @brief  サンプルモジュールクライアント jcontrol メイン
 * </JA>
 * 
 * <EN>
 * @brief  Main routine for sample module client 'jcontrol'
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Mar 24 11:49:27 2005
 *
 * $Revision: 1.7 $
 * 
 */
/*
 * Copyright (c) 2002-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2002-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */
#include "japi.h"

#define strmatch !strcmp	///< String matching function macro

static char sbuf[MAXLINELEN]; ///< Local workarea for message string handling
static char rbuf[MAXLINELEN]; ///< Local workarea for message string handling

/** 
 * <JA>
 * コマンド送信: sbuf バッファに格納されているユーザコマンドを処理する．
 * 
 * @param sd [in] 送信ソケット
 * </JA>
 * <EN>
 * Send command: process user command string in sbuf buffer.
 * 
 * @param sd [in] socket to send data
 * </EN>
 */
void
do_command(int sd)
{
  char *p, *com, *arg1, *arg2;

    com = strtok(sbuf, " \t\n");
    if (com == NULL) return;    /* avoid segv when no command given */
    arg1 = arg2 = NULL;
    if ((p = strtok(NULL, " \t\n")) != NULL) {
      arg1 = p;
      if ((p = strtok(NULL, " \t\n")) != NULL) {
	arg2 = p;
      }
    }
    if (strmatch(com, "die")) {
      japi_die(sd);
    } else if (strmatch(com, "version")) {
      japi_get_version(sd);
    } else if (strmatch(com, "status")) {
      japi_get_status(sd);
    } else if (strmatch(com, "graminfo")) {
      japi_get_graminfo(sd);
    } else if (strmatch(com, "pause")) {
      japi_pause_recog(sd);
    } else if (strmatch(com, "terminate")) {
      japi_terminate_recog(sd);
    } else if (strmatch(com, "resume")) {
      japi_resume_recog(sd);
    } else if (strmatch(com, "inputparam")) {
      japi_set_input_handler_on_change(sd, arg1);
    } else if (strmatch(com, "changegram")) {
      japi_change_grammar(sd, arg1);
    } else if (strmatch(com, "addgram")) {
      japi_add_grammar(sd, arg1);
    } else if (strmatch(com, "deletegram")) {
      japi_delete_grammar(sd, arg1);
    } else if (strmatch(com, "activategram")) {
      japi_activate_grammar(sd, arg1);
    } else if (strmatch(com, "deactivategram")) {
      japi_deactivate_grammar(sd, arg1);
    } else if (strmatch(com, "syncgram")) {
      japi_sync_grammar(sd);
    } else if (strmatch(com, "listprocess")) {
      japi_list_process(sd);
    } else if (strmatch(com, "currentprocess")) {
      japi_current_process(sd, arg1);
    } else if (strmatch(com, "shiftprocess")) {
      japi_shift_process(sd);
    } else if (strmatch(com, "addprocess")) {
      japi_add_process(sd, arg1);
    } else if (strmatch(com, "delprocess")) {
      japi_del_process(sd, arg1);
    } else if (strmatch(com, "activateprocess")) {
      japi_activate_process(sd, arg1);
    } else if (strmatch(com, "deactivateprocess")) {
      japi_deactivate_process(sd, arg1);
    } else if (strmatch(com, "addword")) {
      japi_add_words(sd, arg1, arg2);
    } else {
      fprintf(stderr,"No such command: [%s]\n", com);
    }
}

/** 
 * <JA>
 * データ受信: サーバからの受信メッセージを標準出力にダンプする
 * 
 * @param sd [in] 送信ソケット
 * </JA>
 * <EN>
 * Receive data: dump a message from server to standard out.
 * 
 * @param sd [in] socket to send data
 * </EN>
 */
void
do_output(int sd)
{
  while(do_receive(sd, rbuf, MAXLINELEN) != NULL) {
    if (rbuf[0] == '.' && rbuf[1] == '\0') break;
    printf("> %s\n", rbuf);
  }
  fflush(stdout);
}

/* handle input event */

#if defined(_WIN32) && !defined(__CYGWIN32__)
#include <conio.h>
#endif

/** 
 * <JA>
 * @brief メインイベントループ
 * 
 * サーバからのメッセージ受信イベントおよびキーボードからのユーザ入力
 * イベントを 監視し，対応する処理を行うメイン関数．
 * 
 * @param sd [in] 送信ソケット
 * </JA>
 * <EN>
 * @brief Main event loop
 *
 * This is main loop to watch events from server (message of recognition
 * results etc.) and tty (user keyboard input), catch and process them.
 * 
 * @param sd [in] socket to send data
 * </EN>
 */
void
command_loop(int sd)
{
#if defined(_WIN32) && !defined(__CYGWIN32__)
  /* win32 version: read console input using conio.h */
  fd_set readfds;
  struct timeval tv;
  int status;
  int i, nfd;
  int ch;
  int slen;

  slen = 0;

  for(;;) {

    /* watch socket by select() and check keyboard input by _kbhit() if timeout */
    FD_ZERO(&readfds);
    FD_SET(sd, &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 50000;

    nfd = select(sd+1, &readfds, NULL, NULL, &tv);

    if (nfd < 0) { /* winsock error */
      switch(WSAGetLastError()) {
      case WSANOTINITIALISED: printf(" A successful WSAStartup must occur before using this function. \n"); break;
      case WSAEFAULT: printf(" The Windows Sockets implementation was unable to allocate needed resources for its internal operations, or the readfds, writefds, exceptfds, or timeval parameters are not part of the user address space.  \n"); break;
      case WSAENETDOWN: printf(" The network subsystem has failed.  \n"); break;
      case WSAEINVAL: printf(" The timeout value is not valid, or all three descriptor parameters were NULL.  \n"); break;
      case WSAEINTR: printf(" A blocking Windows Socket 1.1 call was canceled through WSACancelBlockingCall.  \n"); break;
      case WSAEINPROGRESS: printf(" A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.  \n"); break;
      case WSAENOTSOCK: printf(" One of the descriptor sets contains an entry that is not a socket.  \n"); break;
      }
      perror("Error: select");
      exit(1);
    }
    if (FD_ISSET(sd, &readfds)) {      /* from server */
      do_output(sd);
    } else {  /* timeout, check for keyboard input */
      if (_kbhit()) {
	ch = _getche();
	if (ch == '\r') ch = '\n';
	sbuf[slen] = (char)ch;
	slen++;
	if (ch == '\n') {
	  sbuf[slen] = '\0';
	  do_command(sd);	/* execute command */
	  slen = 0;
	}
      }
    }
  }

#else

  /* unix version: watch both stdin and socket */
  fd_set readfds;
  int nfd;

  for(;;) {

    /* watch socket (fd = sd) and stdin (fd = 0) */
    FD_ZERO(&readfds);
    FD_SET(sd, &readfds);
    FD_SET(0, &readfds);

    nfd = select(sd+1, &readfds, NULL, NULL, NULL);

    if (nfd < 0) {
      perror("Error: select");
      exit(1);
    }
    if (FD_ISSET(0, &readfds)) {
      /* stdin */
      if (fgets(sbuf, MAXLINELEN, stdin) != NULL) {
	do_command(sd);
      }
    }
    if (FD_ISSET(sd, &readfds)) {
      /* from server */
      do_output(sd);
    }
  }
  
#endif /* WIN32 */

}

/** 
 * <JA>
 * 使用方法を標準出力に出力する．
 * 
 * </JA>
 * <EN>
 * Output usage to stdout.
 * 
 * </EN>
 */
void usage()
{
  printf("usage: jcontrol host [portnum (def=%d)]\n", DEFAULT_PORT);
}

/** 
 * <JA>
 * メイン
 * 
 * @param argc [in] 引数の数
 * @param argv [in] 引数列
 * 
 * @return 正常終了時 0, エラー終了時 1 を返す．
 * </JA>
 * <EN>
 * Main function.
 * 
 * @param argc [in] number of arguments
 * @param argv [in] argument array
 * 
 * @return 0 on normal exit, 1 on error exit.
 * </EN>
 */
int
main(int argc, char *argv[])
{
  int port;
  int sd;
  
  if (argc < 2) {
    usage();
    return 1;
  }
  if (argc < 3) {
    port = DEFAULT_PORT;
  } else {
    port = atoi(argv[2]);
  }
  sd = do_connect(argv[1], port);
  command_loop(sd);

  return 0;
}

