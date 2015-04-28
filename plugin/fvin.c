/**
 * @file   fvin.c
 * 
 * <EN>
 * @brief  A skeleton code of feature input plugin
 * </EN>
 * 
 * <JA>
 * @brief  特徴量入力プラグインのひな形
 * </JA>
 * 
 * @author Akinobu Lee
 * @date   Mon Aug 11 17:05:17 2008
 * 
 * $Revision: 1.4 $
 * 
 */

/**
 * Required for a file
 *   - get_plugin_info()
 *
 * Optional for a file
 *   - initialize()
 * 
 */

/**
 * feature input plugin functions
 * 
 * Required:
 *   - fvin_get_optname()
 *   - fvin_get_configuration()
 *   - fvin_standby()
 *   - fvin_open()
 *   - fvin_read()
 *   - fvin_close()
 *
 * Optional:
 *   - fvin_terminate()
 *   - fvin_pause()
 *   - fvin_resume()
 *   - fvin_input_name()
 * 
 */

#include <stdio.h>
#include <string.h>
#include "plugin_defs.h"

#define PLUGIN_TITLE "Feature vector input plugin for Julius"
#define INPUT_OPT "myfvin"

/************************************************************************/

/** 
 * <EN>
 * @brief  Initialization at loading time (optional)
 * 
 * If defined, this will be called just before this plugin is loaded to Julius.
 * if this returns -1, the whole functions in this file will not be loaded.
 *
 * This function is OPTIONAL.
 * </EN>
 * <JA>
 * @brief  読み込み時の初期化（任意）
 *
 * 起動時，Julius がこのプラグインを読み込む際に最初に呼ばれる．
 * -1 を返すと，このプラグイン全体が読み込まれなくなる．
 * 実行可能性のチェックに使える．
 *
 * </JA>
 * 
 * 
 * @return 0 on success, -1 on failure.
 * 
 */
int
initialize()
{
  return 0;
}

/** 
 * <EN>
 * @brief  Get information of this plugin (required)
 *
 * This function should return informations of this plugin file.
 * The required info will be specified by opcode:
 *  - 0: return description string of this file into buf
 *
 * This will be called just after Julius find this file and after
 * initialize().
 *
 * @param opcode [in] requested operation code
 * @param buf [out] buffer to store the return string
 * @param buflen [in] maximum length of buf
 *
 * @return 0 on success, -1 on failure.  On failure, Julius will ignore this
 * plugin.
 * 
 * </EN>
 * <JA>
 * @brief  プラグイン情報取得（必須）
 *
 * このプラグインに関する情報を返す．与えられた opcode によって動作する．
 *  - 0 の場合，このプラグインファイルの名称を与えられたバッファに格納する
 *
 * この関数は，Julius がこのプラグインを読み込んだ直後に呼ばれる．
 * 
 * @param opcode [in] 要求動作コード (現在 0 のみ実装)
 * @param buf [out] 値を格納するバッファ
 * @param buflen [in] buf の最大長
 * 
 * @return エラー時 -1, 成功時 0 を返す．エラーとして -1 を返した場合，
 * このプラグイン全体は読み込まれない．
 * </JA>
 * 
 */
int
get_plugin_info(int opcode, char *buf, int buflen)
{
  switch(opcode) {
  case 0:
    /* plugin description string */
    strncpy(buf, PLUGIN_TITLE, buflen);
    break;
  }

  return 0;
}

/************************************************************************/
/************************************************************************/
/* Feature-vector input plugin functions */

/** 
 * <EN>
 * @brief  Return option string to select this input at option. (required)
 *
 * This function should return option string which should be specified
 * as an argument "-input" option, to be used on Julius.  The returning
 * string should not be the same with any existing value.
 *
 * This function will be called several times at option parsing at startup.
 *
 * @param buf [out] buffer to store the return string
 * @param buflen [in] maximum length of buf
 * </EN>
 * <JA>
 * @brief  入力選択用のオプション文字列を返す（必須）
 *
 * このプラグインを入力として選択する際に，"-input" オプションで指定す
 * べき文字列を格納して返す．返す文字は，システムにすでにあるものや，
 * 他のプラグインが使用しているものと同じでないこと．
 * （もし同じだった場合システム側が優先される）
 *
 * この関数は，起動時のオプション解析時に何度か呼ばれる．
 *
 * @param buf [out] 値を格納して返すバッファ
 * @param buflen [in] buf の最大長
 * </JA>
 * 
 */
void
fvin_get_optname(char *buf, int buflen)
{
  strncpy(buf, INPUT_OPT, buflen);
}

/** 
 * <EN>
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
 * </EN>
 * 
 * <JA>
 * @brief  特徴量のパラメータを返す（必須）
 *
 * この入力プラグインがJuliusに渡す特徴量に関するパラメータを返す．
 * 与えられた以下の opcode ごとに，値を返す．
 *
 * opcode = 0: ベクトルの次元数
 * opcode = 1: １フレームあたりの時間幅（単位：ミリ秒）
 * opcode = 2: パラメータの型
 *
 * opcode = 2 のパラメータの型は，音響モデルの特徴量との型整合性
 * チェックに使われる．値は，HTK の特徴量ファイルのヘッダ形式で
 * エンコードされた値を返す．型チェックを行わない場合は，
 * 0xffff を返すこと．
 *
 * opcode =3 のとき特徴量ベクトル入力なら 0, 出力確率ベクトルなら1を返す．
 *
 * @param opcode [in] 要求動作コード (現在 0 のみ実装)
 * 
 * @return opcode ごとに要求された値を返す．
 * </JA>
 */
int
fvin_get_configuration(int opcode)
{
  switch(opcode) {
  case 0:		   /* return number of elements in a vector */
    return(25);
  case 1:/* return msec per frame */
    return(10);
  case 2:/* return parameter type specification in HTK format */
    /* return 0xffff to disable checking */
    return(0xffff);
  case 3:/* return 0 if feature vector input, 1 if outprob vector input */
    return(0);
  }
}

/************************************************************************/

/**
 * <EN>
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
 * <JA>
 * @brief  デバイスを初期化する（必須）
 *
 * この関数は起動時に一回だけ呼ばれる．ここでは入力ファイルの準備や
 * ソケットの用意といった，入力のための準備を行うのに使う．
 *
 * FALSE を返した場合，Julius は終了する．
 * 
 * JuliusLib: この関数は j_adin_init() で呼ばれる．
 *
 * @return 成功時 TRUE，失敗時 FALSE を返す．
 * </JA>
 */
boolean
fvin_standby()
{

  /* sever socket ready etc... */
  return TRUE;

}

/**
 * <EN>
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
 * </EN>
 * <JA>
 * @brief  入力を開く（必須）
 *
 * 入力を新規に開く．ファイルのオープン，ネットワーククライアントからの
 * 接続などはここで行う．
 *
 * FALSE を返したとき，Julius は認識ループを抜ける．
 * 
 * JuliusLib: この関数は j_open_stream() 内で呼ばれる．
 * 
 * @return 成功時 TRUE，失敗時 FALSE を返す．
 * </JA>
 */
boolean
fvin_open()
{
  /* listen and accept socket, or open a file */
  return TRUE;
}

/**
 * <EN>
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
 * </EN>
 * <JA>
 * @brief 入力からベクトルを読み込む（必須）
 *
 * この関数は入力からベクトルを１つだけ読み込む．この関数は
 * フレームごとに呼ばれ，読み込まれたベクトルはこのあとすぐに認識処理され，
 * また次のフレームのデータを読むためにこの関数が呼ばれる．
 *
 * 入力が終端まで達したとき，ADIN_EOF を返す．このとき，Julius は現在
 * の認識処理を終了させ，入力を閉じる．
 *
 * ADIN_ERROR はこの関数内で深刻なエラーが生じた場合に返す．これが返さ
 * れた場合，Julius はその場で異常終了する．
 *
 * ADIN_SEGMENT を返すことで，Julius に現在の認識を現時点で区切ること
 * を要求することができる．現在の認識処理はこの時点でいったん区切られ，
 * そこまでの認識結果が確定・出力されたあと，次の認識処理が始まりこの
 * 関数が呼ばれる．ADIN_SEGMENT は ADIN_EOF と動作が似ているが，
 * ADIN_EOF が adin_close(), adin_open() を呼んで入力を終了させ
 * るのに対して，ADIN_SEGMENT はこれらを呼ばずに入力を続行する．
 * 
 * @param vecbuf [out] 得られたベクトルを格納するバッファ
 * @param veclen [in] ベクトル長
 * 
 * @return 成功時 0 あるいは end of stream 時に ADIN_EOF, Julius に区
 * 切り要求を出すときには ADIN_SEGMENT, エラー時はADIN_ERROR を返す．
 * </JA>
 */
int
fvin_read(float *vecbuf, int veclen)
{
  /* read one vector from the input */
  if (0/* error */) return ADIN_ERROR;
  if (0/* input should be segmented here */) return ADIN_SEGMENT;
  if (0/* EOF */) return ADIN_EOF;

  return(0);			/* success */
}

/**
 * <EN>
 * @brief  Close the current input (required)
 *
 * This function will be called when the input has reached end of file
 * (i.e. the last call of fvin_read() returns ADIN_EOF)
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
 * </EN>
 * <JA>
 * @brief  入力を閉じる（必須）
 *
 * 現在の入力を閉じる．この関数は，入力が終端（EOF）に達したとき（すな
 * わち fvin_read() が ADIN_EOF を返したとき）に呼ばれる．通常，ここでは
 * ファイルを閉じる，ネットワーク接続を切断するなどの処理を行う．
 *
 * 正常終了としてTRUEを返したとき，Julius は adin_open() に戻って
 * 他のストリームを開こうとする． FALSE を返したときは，Julius は
 * 認識ループを抜ける．
 * 
 * JuliusLib: この関数は j_recognize_stream() の最後で呼ばれる．
 * 
 * @return 成功時 TRUE，失敗時 FALSE を返す．
 * </JA>
 */
boolean
fvin_close()
{
  /* file close, connection close, etc.. */
  return TRUE;
}

/************************************************************************/

/**
 * <EN>
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
 * </EN>
 * <JA>
 * @brief  中断要求用フック（任意）
 *
 * この関数を定義すると，Julius は中断要求を受け取った際にこの関数を呼び出す．
 * これを使って，Julius の中断・再開と同期した入力同期処理を実装することが
 * できる．（例：入力送信元に対して送信中断要求を出すなど）
 *
 * 中断要求は，Julius がアプリケーションやクライアントより受け取る
 * 認識中断の要求である．具体的には，Julius がモジュールモードで動作して
 * いる時に TERMINATE コマンドをクライアントから受け取ったときや，
 * JuliusLibを組み込んだアプリケーションが j_request_terminate() を
 * 呼んだときに発生する．
 *
 * 中断要求を受け取ると，Julius は現在の認識処理を中断する．
 * 認識途中であった場合，その入力を破棄して即時中断する．
 * 処理の再開は，RESUME コマンドか j_request_resume() の呼び出しで行われる．
 *
 * この関数は中断要求を Julius が受け取った時点で呼ばれる．
 * 実際に処理が中断した後で呼ばれるのではないことに注意されたい．
 * 
 * @return 成功時 TRUE, エラー時 FALSE を返す．
 * </JA>
 * 
 */
boolean
fvin_terminate()
{
  printf("terminate request\n");
  return TRUE;
}

/**
 * <EN>
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
 * </EN>
 * <JA>
 * @brief  停止要求用フック（任意）
 *
 * この関数を定義すると，Julius は停止要求を受け取った際にこの関数を呼び出す．
 * これを使って，Julius の中断・再開と同期した入力同期処理を実装することが
 * できる．（例：入力送信元に対して送信中断要求を出すなど）
 *
 * 停止要求は，Julius がアプリケーションやクライアントより受け取る，
 * 認識の一時停止の要求である．具体的には，Julius がモジュールモードで動作して
 * いる時に PAUSE コマンドをクライアントから受け取ったときや，
 * JuliusLibを組み込んだアプリケーションが j_request_pause() を
 * 呼んだときに発生する．
 *
 * 停止要求を受け取ると，Julius は現在の認識処理を中断する．
 * 認識途中であった場合，その認識が終わるまで待ってから中断する．
 * 処理の再開は，RESUME コマンドか j_request_resume() の呼び出しで行われる．
 * 
 * 中断要求 (fvin_terminate) との違いは，認識途中に要求を受けたときの動作が
 * 異なる．中断要求では強制中断するが，停止要求ではその認識が終わるまで
 * 待ってから停止する．
 *
 * この関数は停止要求を Julius が受け取った時点で呼ばれる．
 * 実際に処理が停止した後で呼ばれるのではないことに注意されたい．
 * 
 * @return 成功時 TRUE, エラー時 FALSE を返す．
 * </JA>
 * 
 */
boolean
fvin_pause()
{
  printf("pause request\n");
  return TRUE;
}

/**
 * <EN>
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
 * </EN>
 * <JA>
 * @brief  認識再開要求用フック（任意）
 *
 * この関数を定義すると，Julius は停止状態からの認識再開要求の際に
 * この関数を呼び出す．
 *
 * 認識再開要求は，Julius がモジュールモードで動作して RESUME コマンドを
 * クライアントから受け取ったときや，JuliusLibを組み込んだアプリケーション
 * が j_request_resume() を呼んだときに発生する．この再開要求が発生
 * すると，Julius は停止していた認識を再開する．
 *
 * 注意：この関数は，実際に停止したときに呼ばれるのではなく，Julius が
 * 要求を受け取った時点で，そのたびに呼ばれる．複数回呼ばれることや，
 * すでに動作中である場合にさらにこのコマンドを受け取ったときにも呼ば
 * れることがあることに注意されたい．
 * 
 * @return 成功時 TRUE, エラー時 FALSE を返す．
 * </JA>
 * 
 */
boolean
fvin_resume()
{
  printf("resume request\n");
  return TRUE;
}

/**
 * <EN>
 * @brief  A function to return current device name for information (optional)
 *
 * This function is totally optional.
 * 
 * @return pointer to the device name string
 * </EN>
 * <JA>
 * @brief  入力ファイル・デバイス名を返す関数（任意）
 *
 * @return 入力ファイルあるいはデバイス名の文字列へのポインタ
 * </JA>
 * 
 */
char *
fvin_input_name()
{
  printf("input name function was called\n");
  return("default");
}
/* end of file */
