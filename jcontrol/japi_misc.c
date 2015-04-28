/**
 * @file   japi_misc.c
 * 
 * <JA>
 * @brief  モジュールコマンド送信部
 * </JA>
 * 
 * <EN>
 * @brief  Sending module commands
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Mar 24 11:24:18 2005
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

/** 
 * <JA>
 * コマンド DIE: 認識サーバを終了させる．
 * 
 * @param sd [in] 送信ソケット
 * </JA>
 * <EN>
 * Command "DIE": kill the recognition server.
 * 
 * @param sd [in] socket to send data
 * </EN>
 */
void
japi_die(int sd)
{
  do_send(sd, "DIE\n");
}

/** 
 * <JA>
 * コマンド VERSION: バージョン情報を送信させる．
 * 
 * @param sd [in] 送信ソケット
 * </JA>
 * <EN>
 * Command "VERSION": let the server send version information.
 * 
 * @param sd [in] socket to send data
 * </EN>
 */
void
japi_get_version(int sd)
{
  do_send(sd, "VERSION\n");
}

/** 
 * <JA>
 * コマンド STATUS: 認識サーバの現在の状態(認識中/停止中)を送信させる．
 * 
 * @param sd [in] 送信ソケット
 * </JA>
 * <EN>
 * Command "STATUS": ask server about its current status (run/stop)
 * 
 * @param sd [in] socket to send data
 * </EN>
 */
void
japi_get_status(int sd)
{
  do_send(sd, "STATUS\n");
}

/** 
 * <JA>
 * コマンド PAUSE: 認識サーバを一時停止する．サーバが認識中の場合，その入力
 * が終わってから停止する．
 * 
 * @param sd [in] 送信ソケット
 * </JA>
 * <EN>
 * Command "PAUSE": tell server to pause recognition.  If audio input is
 * processing at that time, recognition will stop after the current input
 * has ended.
 * 
 * @param sd [in] socket to send data
 * </EN>
 */
void
japi_pause_recog(int sd)
{
  do_send(sd, "PAUSE\n");
}

/** 
 * <JA>
 * コマンド TERMINATE: 認識サーバを一時停止する．サーバが認識中の場合，
 * その入力を捨てて即時停止する．
 * 
 * @param sd [in] 送信ソケット
 * </JA>
 * <EN>
 * Command "TERMINATE": tell server to pause recognition immediately,
 * even if audio input is processing at that time.
 * 
 * @param sd [in] socket to send data
 * </EN>
 */
void
japi_terminate_recog(int sd)
{
  do_send(sd, "TERMINATE\n");
}

/** 
 * <JA>
 * コマンド RESUME: PAUSEやTERMINATEによって一時停止した認識サーバを
 * 再開させる．
 * 
 * @param sd [in] 送信ソケット
 * </JA>
 * <EN>
 * Command "RESUME": tell server to restart recognition.
 * 
 * @param sd [in] socket to send data
 * </EN>
 */
void
japi_resume_recog(int sd)
{
  do_send(sd, "RESUME\n");
}

/** 
 * <JA>
 * コマンド INPUTONCHANGE: 文法切り替え指示時に認識中であった場合の動作を
 * 指定する．
 * 
 * @param sd [in] 送信ソケット
 * @param arg [in] "TERMINATE" for immediate rejection of current input,
 * "PAUSE" for immediate input segmentation followed by recognition, or
 * "WAIT" for waiting the input to be segmented.
 * </JA>
 * <EN>
 * Command "INPUTONCHANGE": specify grammar changing timing policy when
 * input is being recognized.
 * 
 * @param sd [in] socket to send data
 * @param arg [in] "TERMINATE" for immediate rejection of current input,
 * "PAUSE" for immediate input segmentation followed by recognition, or
 * "WAIT" for waiting the input to be segmented.
 * </EN>
 */
void
japi_set_input_handler_on_change(int sd, char *arg)
{
  /* argument should be checked here... */
  /* one of TERMINATE, PAUSE, WAIT */

  /* send */
  do_sendf(sd, "INPUTONCHANGE\n%s\n", arg);
}
