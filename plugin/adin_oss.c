/**
 * @file   adin_oss.c
 * 
 * <EN>
 * @brief  A reference sample of A/D-in plugin
 *
 * This file describes the specifications of plugin functions to be
 * defined to make an A/D-in plugin.  An A/D-in plugin will extend a
 * new audio sream input into Julius by addin a new choice to the
 * "-input" option.
 *
 * The recording format should be 16 bit (signed short), and sampling
 * rate should be set to the given value at adin_standby().
 * 
 * </EN>
 * 
 * <JA>
 * @brief  オーディオ入力プラグインのひな形
 *
 * このファイルは，オーディオ入力プラグインを作成する際に定義すべきプ
 * ラグイン関数について解説している．オーディオ入力プラグインは，
 * Julius に新たな音声入力デバイスを追加する．"-input" に新たな選択肢
 * が追加され，実行時に Julius に対してそれを指定することで，このプラ
 * グイン経由で音声を取り込み認識することができる．
 *
 * オーディオ入力プラグインで取り込むべきデータのフォーマットは 16bit で
 * あること．さらに，サンプリングレートを adin_standby() 呼び出し時に
 * 与えられるレートに合わせること．
 * 
 * </JA>
 *
 * Common functions that can be defined in any type of plugin:
 *   - get_plugin_info()
 *   - initialize()
 * 
 * A/D-in plugin functions:
 * 
 * Required:
 *   - adin_get_optname()
 *   - adin_get_configuration()
 *   - adin_standby()
 *   - adin_open()
 *   - adin_read()
 *   - adin_close()
 *
 * Optional:
 *   - adin_terminate()
 *   - adin_pause()
 *   - adin_resume()
 *   - adin_input_name()
 * 
 * 
 * @author Akinobu Lee
 * @date   Thu Aug  7 14:28:37 2008
 * 
 * $Revision: 1.3 $
 * 
 */

/***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plugin_defs.h"

/**
 * <EN>
 * Description string of this plugin file.
 * </EN>
 * <JA>
 * このプラグインファイルの説明文字列．
 * </JA>
 * 
 */
#define PLUGIN_TITLE "A/D-in plugin for Julius"

/**
 * <EN>
 * string to be specified at "-input" option at Julius to use this plugin
 * as input module.
 * </EN>
 * <JA>
 * このプラグインを使用して音声入力を行う際に，Juliusの "-input" オプション
 * に与えるべき文字列.
 * </JA>
 * 
 */
#define INPUT_OPT "myadin"

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
/* A/D-in plugin functions */

/** 
 * <EN>
 * @brief  Return option string to select at option. (required)
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
 * @brief  音声入力選択用のオプション文字列を返す（必須）
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
adin_get_optname(char *buf, int buflen)
{
  strncpy(buf, INPUT_OPT, buflen);
}

/** 
 * <EN>
 * @brief  Return decoder parameter values related to this adin plugin (required)
 * 
 * This function should return configuration values about how to set up
 * decoder to use this adin plugin.  The return value is dependent on
 * the given opcode, as described below:
 *
 * opcode = 0:  return whether real-time processing of 1st pass
 * should be enabled by default.
 *
 * if returns 0 (disabled) , Julius will do buffered input, spooling
 * the incoming input till EOF or silence cut segmentation, extract
 * feature vector, then recognize the whole.  If returns 1 (enabled),
 * on-the-fly decoding will be performed, reading input and decoding
 * it concurrently.
 *
 * A real-time decoding uses some approximation on feature extraction
 * related to sentence-based normalization i.e. CMN or energy normalization.  
 * This value is typically 0 on off-line recognition, and 1 for on-line
 * recognition.
 *
 * This value is device-dependent default value, and can be overridden by
 * user option "-realtime" and "-norealtime".
 *
 * opcode = 1: return whether silence cut segmentation should be
 * enabled by default
 *
 * return 0 to disable, 1 to enable.
 * 
 * On file input, you can choose whether silence detection and
 * segmentation should be performed before recognition.  On live input
 * like microphone, where input stream is infinite, you would perfer
 * choose 1 to enable it.
 * 
 * This value is device-dependent default value, and can be overridden by
 * user option "-cutsilence" and "-nocutsilence".
 *
 * opcode = 2: return whether input threading is necessary or not.
 * 
 * On Unix, when set to 1, Julius forks a separate thread for A/D-in
 * input.  It can be useful when recognition is slow and some audio
 * inputs are dropped.  Note that this should be enabled only for
 * infinite input like microphone or line input, since EOF handling on
 * threaded mode is not supported yet.  Recommended value is 1 for
 * microphone input, 0 for file and network (tcp/ip) input.
 * Ignored on Win32.
 * 
 * @param opcode [in] requested operation code
 * 
 * @return values required for the opcode as described.
 * </EN>
 * 
 * <JA>
 * @brief  入力の扱いに関するパラメータ設定を返す（必須）
 *
 * Julius がこの入力プラグインをどう扱うべきかについて，設定パラメータを
 * 返す．与えられた以下の opcode ごとに，値を返す．
 *
 * opcode = 0: リアルタイム認識を行うかどうかのデフォルト値
 *
 * 1 を返すと，Julius は入力に対して特徴抽出と認識処理を平行して行う
 * リアルタイム認識を行う．0 の場合，いったん入力を終端（あるいは区切り）
 * まで受け取ってから，特徴抽出を行い，その後認識を開始する．
 * リアルタイム処理では，CMN やエネルギー平均など，発話全体を用いた
 * 特徴量の正規化が近似される．
 *
 * 通常，マイク入力などリアルタイムな結果が欲しい場合は 1，
 * ファイル入力などオフライン認識の場合は 0 を返すことが多い．
 *
 * なお，ここの値は，この入力が規定するデフォルト値であり，
 * Juliusの実行時オプション "-realtime", "-norealtime" でも変更できる．
 * オプションが指定された場合はその指定が優先される．
 * 
 * opcode = 1: 無音区間検出による入力区切りのデフォルト値
 *
 * Julius は入力音声に対して振幅と零交差による入力判定を行い，振幅が一
 * 定レベル以下の部分をスキップし，そこで区切って入力とすることができ
 * る．この無音での自動区切りのデフォルトを，返値 1 で有効， 0 で無効
 * とできる．
 *
 * 通常，マイクなどの直接入力では 1，１発話ごとの音声ファイルでは 0 を
 * 返すことが多い．
 *
 * なお，ここの値は，この入力が規定するデフォルト値であり，
 * Juliusの実行時オプション "-cutsilence", "-nocutsilence" でも変更できる．
 * オプションが指定された場合はその指定が優先される．
 *
 * opcode = 2: 音声入力をスレッド化するかのデフォルト値
 *
 * 音声入力取り込み部を別スレッドにするかどうかを選択する．
 * 音声認識の速度が遅く，音声データの取りこぼしが発生する場合に有効である．
 * ただし，現在のJuliusでは，EOF による認識終了を正しく扱えないので，
 * マイク入力などの入力長が有限でない入力についてのみスレッド化を有効に
 * すべきである．
 *
 * 通常，マイク UDP などでは 1 にし，ファイルや TCP/IP ソケットでは
 * 0 にする．
 *
 * @param opcode [in] 要求動作コード (現在 0 のみ実装)
 * 
 * @return opcode ごとに要求された値を返す．
 * </JA>
 */
int
adin_get_configuration(int opcode)
{
  /* For your convenience, UNCOMMENT ONE OF THEM BELOW that match your needs */

  /* typical values for live microphone/line input */
  switch(opcode) {
  case 0:	
    return 1;
  case 1:
    return 1;
  case 2:
    return 1;
  }
  /* typical values for offline file input */
  /* 
   * switch(opcode) {
   * case 0:	   
   *   return 0;
   * case 1:
   *   return 0;
   * case 2:
   *   return 0;
   * }
   */
  /* typical setting for tcpip input */
  /* assuming speech to be segmented at sender */
  /* 
   * switch(opcode) {
   * case 0:	   
   *   return 1;
   * case 1:
   *   return 0;
   * case 2:
   *   return 0;
   * }
   */
  /* typical setting for tcpip input */
  /* assuming receiving continous speech stream and segmented
     should be done at Julius side */
  /* 
   * switch(opcode) {
   * case 0:	   
   *   return 1;
   * case 1:
   *   return 1;
   * case 2:
   *   return 0;
   * }
   */
}
 

/************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/soundcard.h>
static int audio_fd;
static int freq;

/**
 * <EN>
 * @brief  Initialize input device (required)
 *
 * This will be called only once at start up of Julius.  You can
 * initialize the device, check if the device exists or prepare a socket
 * for connection.
 *
 * If this function returns FALSE, Julius will exit.
 * 
 * JuliusLib: this function will be called at j_adin_init().
 *
 * @param sfreq [in] required sampling frequency
 * @param dummy [in] a dummy data
 * 
 * @return TRUE on success, FALSE on failure.
 * </EN>
 * <JA>
 * @brief  デバイスを初期化する（必須）
 *
 * この関数は起動時に一回だけ呼ばれる．ここではデバイスのチェックや
 * ソケットの用意といった，音声入力のための準備を行うのに使う．
 *
 * FALSE を返した場合，Julius は終了する．
 * 
 * JuliusLib: この関数は j_adin_init() で呼ばれる．
 *
 * @param sfreq [in] サンプリングレート
 * @param dummy [in] ダミーデータ（未使用）
 * 
 * @return 成功時 TRUE，失敗時 FALSE を返す．
 * </JA>
 */
boolean
adin_standby(int sfreq, void *dummy)
{
  /* store the frequency */
  freq = sfreq;
  return TRUE;
}
 
/**
 * <EN>
 * @brief  Open an input stream (required)
 *
 * This function should open a new audio stream for input.
 * You may open a capture device, open an audio file, or wait for
 * connection with other network client at this function.
 *
 * If this function returns FALSE, Julius will exit recognition loop.
 * 
 * JuliusLib: this will be called at j_open_stream().
 *
 * @param pathname [in] file / device name to open or NULL for default
 * 
 * @return TRUE on success, FALSE on failure.
 * </EN>
 * <JA>
 * @brief  入力音声ストリームを開く（必須）
 *
 * 入力音声ストリームを新規に開く．通常，デバイスやファイルのオープン，
 * ネットワーククライアントからの接続などをここで行う．
 *
 * FALSE を返したとき，Julius は認識ループを抜ける．
 * 
 * JuliusLib: この関数は j_open_stream() 内で呼ばれる．
 * 
 * @param pathname [in] 開くファイルあるいはデバイス名，NULL ならデフォルト
 * 
 * @return 成功時 TRUE，失敗時 FALSE を返す．
 * </JA>
 */
boolean
adin_open(char *pathname)
{
  /* do open the device */
  int fmt;
  int stereo;
  int ret;
  int s;
  char buf[2];

  if ((audio_fd = open(pathname ? pathname : "/dev/dsp", O_RDONLY)) == -1) {
    printf("Error: cannot open %s\n", pathname ? pathname : "/dev/dsp");
    return FALSE;
  }
  fmt = AFMT_S16_LE;               /* 16bit signed (little endian) */
  if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &fmt) == -1) {
    printf("Error: failed set format to 16bit signed\n");
    return FALSE;
  }
  stereo = 0;			/* mono */
  ret = ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo);
  if (ret == -1 || stereo != 0) {
    stereo = 1;
    ret = ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &stereo);
    if (ret == -1 || stereo != 1) {
      printf("Error: failed to set monoral channel\n");
      return FALSE;
    }
  }
  s = freq;
  if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &s) == -1) {
    printf("Erorr: failed to set sample rate to %dHz\n", freq);
    return FALSE;
  }

  /* start recording... */
  read(audio_fd, buf, 2);

  return(TRUE);
}

/**
 * <EN>
 * @brief  Read samples from device (required)
 *
 * This function is for reading samples to be recognized from input stream.
 * This will be called repeatedly at each time the read samples are fully
 * processed.
 *
 * The sampling format should be 16bit, 1 channel.
 *
 * @a sampnum is the maximum number of samples that can be read into @a buf.
 * The actual number of read samples should be returned.
 *
 * Impotant notes about I/O blocking:
 *  - Do not wait until all the @a sampnum samples are read.
 *    Blocking inside this function will block the whole recognition process.
 *    If device allows, it is better to read only the available data
 *    in the stream and return immediately.
 *  - Avoid returning value of 0 when no data is available, wait for some
 *    data to come inside this function.  When you are using non-blocking
 *    operation, you may want to return 0 when no data is available.
 *    However, returning 0 will cause Julius to call again this function
 *    immediately, and cause busy loop to make CPU load to reach 100%.
 *
 * So the ideal operation will be first wait_for_some_data_to_come, and
 * if any data becomes available, read them at most @a sampnum samples
 * and return the number of read samples.
 *
 * Positive return value should be the number of read samples, or one
 * of ADIN_EOF, ADIN_SEGMENT or ADIN_ERROR.  Return value of ADIN_EOF
 * tells end of stream, which causes Julius to finish current
 * recognition and close stream.  ADIN_SEGMENT requests Julius to
 * segment the current input.  The current recognition will be stopped
 * at this point, recognition result will be output, and then Julius
 * continues to the next input.  The behavior of ADIN_SEGMENT is
 * similar to ADIN_EOF except that ADIN_SEGMENT does not close/open
 * stream, but just stop and restart the recognition.  At last, return
 * value should be ADIN_ERROR on error, in which Julius exits itself
 * immediately.
 * 
 * @param buf [out] output buffer to store samples obtained.
 * @param sampnum [in] maximum number of samples that can be stored in @a buf.
 * 
 * @return actural number of read samples, ADIN_EOF on end of stream,
 * ADIN_SEGMENT to request segmentation to Julius, or ADIN_ERROR on error.
 * </EN>
 * <JA>
 * @brief  デバイスからサンプルを読み込む（必須）
 *
 * この関数は入力ストリームから音声サンプルを読み込む．
 *
 * バッファに格納して返す音声データの形式は 16bit, 1 チャンネルであること．
 * 
 * @a sampnum は @a buf に格納することのできる最大のサンプル数である．
 * 返り値として，実際に読み込まれたサンプル数，あるいは以下で説明する
 * エラーコードを返す．
 * 
 * この関数は認識中に何度も呼ばれ，ここで読まれたデータが Julius によっ
 * て 認識処理される．読み込んだ分の処理が終了すると，次の入力を読み込
 * むためにこの関数が再度呼ばれる．
 * 
 * この関数内での I/O blocking については以下の注意が必要である：
 * 
 *  - 長時間のブロックは避けること（@a sampnum は要求サンプル数ではな
 *  く@a buf に格納可能な最大数である）．この関数内でブロックすると認
 *  識処理全体がブロックする．読み込みが長時間ブロックしないよう，数百
 *  サンプル程度だけ読み込んで返すか，あるいは最初にバッファ内にあるブ
 *  ロックせずに読み込み可能なデータサンプル数を取得し，その分だけ読み
 *  込むようにするのがよい．
 *    
 *  - non-blocking モードを用いる場合， 0 を返さないこと．
 *    バッファにデータが存在しないとき，0 を返すと Julius はサンプル
 *    無しのためまた即座にこの関数を呼び出す．これがビジーウェイトを
 *    発生させ，CPUロードがあがってしまう．バッファにデータが無いとき，
 *    即座に 0 を返さず，数十msec でよいのでこの関数内で待つ
 *    ことが望ましい．
 *
 * 返り値は，実際に読み込んだサンプル数を正の値として返すか，あるいは
 * ADIN_EOF, ADIN_SEGMENT, ADIN_ERROR のどれかを返す．ADIN_EOF はスト
 * リームが終端まで達したことを表す，これを返すと，Julius は現在の認識
 * 処理を終了させ，ストリームを閉じる．ADIN_ERROR はこの関数内で深刻な
 * エラーが生じた場合に返す．これが返された場合，Julius はその場で異常
 * 終了する．
 *
 * ADIN_SEGMENT を返すことで，Julius に現在の認識を現時点で区切ること
 * を要求することができる．現在の認識処理はこの時点でいったん区切られ，
 * そこまでの認識結果が確定・出力されたあと，次の認識処理が始まりこの
 * 関数が呼ばれる．ADIN_SEGMENT は ADIN_EOF と動作が似ているが，
 * ADIN_EOF が adin_close(), adin_open() を呼んでストリームを終了させ
 * るのに対して，ADIN_SEGMENT はこれらを呼ばずに入力を続行する．この機
 * 能は，たとえばネットワーク経由で音声データを受信しているときに，送
 * 信側から音声認識のON/OFFやVADをコントロールしたい場合などに
 * 使うことができる．
 * 
 * @param buf [out] 得られたサンプルを格納するバッファ
 * @param sampnum [in] @a buf 内に格納できる最大サンプル数
 * 
 * @return 実際に読み込まれたサンプル数，あるいは end of stream 時に ADIN_EOF,
 * Julius に区切り要求を出すときには ADIN_SEGMENT, エラー時はADIN_ERROR を
 * 返す．
 * </JA>
 */
int
adin_read(SP16 *buf, int sampnum)
{
  audio_buf_info info;
  int size, cnt;

  /* get sample num that can be read without blocking */
  if (ioctl(audio_fd, SNDCTL_DSP_GETISPACE, &info) == -1) {
    printf("Error: adin_oss: failed to get number of samples in the buffer\n");
    return(ADIN_ERROR);
  }
  /* get them as much as possible */
  size = sampnum * sizeof(SP16);
  if (size > info.bytes) size = info.bytes;
  size &= ~ 1;		/* Force 16bit alignment */
  cnt = read(audio_fd, buf, size);
  if ( cnt < 0 ) {
    printf("Error: adin_oss: failed to read samples\n");
    return (ADIN_ERROR);
  }
  cnt /= sizeof(short);
    
  return(cnt);
}

/**
 * <EN>
 * @brief  Close the current input stream (required)
 *
 * This function will be called when the input stream has reached
 * end of file (i.e. the last call of adin_read() returns ADIN_EOF)
 *       
 * You may close a capture device, close an audio file, or
 * disconnect network client.
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
 * @brief  入力ストリームを閉じる（必須）
 *
 * 現在のストリームを閉じる．この関数は，入力ストリームが終端（EOF）
 * に達したとき（すなわち adin_read() が ADIN_EOF を返したとき）に
 * 呼ばれる．デバイスを閉じる，ファイルを閉じる，あるいはネットワーク接続を
 * 切断するのに使うことができる．
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
adin_close()
{
  close(audio_fd);
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
adin_terminate()
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
 * 中断要求 (adin_terminate) との違いは，認識途中に要求を受けたときの動作が
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
adin_pause()
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
adin_resume()
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
adin_input_name()
{
  printf("input name function was called\n");
  return("default");
}


/* end of file */
