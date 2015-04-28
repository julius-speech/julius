/**
 * @file   generic_callback.c
 * 
 * <EN>
 * @brief  An example plugin using callback.
 * </EN>
 * 
 * <JA>
 * @brief  コールバックを使うプラグインのサンプル
 * </JA>
 * 
 * @author Akinobu Lee
 * @date   Wed Aug 13 23:50:27 2008
 * 
 * $Revision: 1.1 $
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
 * using plugin function:
 * 
 *   - engine_startup()
 * 
 */


// #include "plugin_defs.h"
#include <julius/juliuslib.h>

#define PLUGIN_TITLE "An example plugin using callback"

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

/** 
 * <EN>
 * A sample callback function to output RECREADY message.
 * 
 * @param recog [in] engine instance
 * @param dummy [in] callback argument (dummy)
 * </EN>
 * <JA>
 * RECREADY を出力するコールバック用関数（サンプル）
 *
 * @param recog [in] エンジンインスタンス
 * @param dummy [in] コールバック引数（ダミー）
 * </JA>
 * 
 */
static void
status_recready(Recog *recog, void *dummy)
{
  printf("<<<RECREADY>>>\n");
}

/** 
 * <EN>
 * @brief  plugin function that will be called after engine startup.
 *
 * When the function of this name is defined in a plugin, this will
 * be called just after Julius finished all startup sequence and before
 * input and recognition start.
 *
 * In this example, this function registers the local function
 * status_recready() as a CALLBACK_EVENT_SPEECH_READY callback.
 * This callback will be called on every time Julius is ready for
 * recognition for the next incoming input.
 * 
 * @param data [in] a data pointer, actually a pointer to an engine instance.
 * 
 * @return 0 on success, -1 on error.  On error, Julius will exit immediately.
 * </EN>
 * <JA>
 * @brief  認識エンジン起動完了時に呼び出されるプラグイン関数
 *
 * この名前の関数が定義された場合，その関数は，Julius が全ての初期化を
 * 完了して起動プロセスを終えた直後，実際に音声入力を開いて認識が始ま
 * る前に呼ばれます．
 *
 * ここでは，この関数を使って，上記の関数 status_recready() を
 * CALLBACK_EVENT_SPEECH_READY コールバックとして登録しています．
 * このコールバックは Julius が入力ストリームからの次の音声入力待ち
 * 状態になったときに呼ばれます．
 * 
 * @param data [in] データへのポインタ．実体はエンジンインスタンスへの
 * ポインタが渡される．
 * 
 * @return 成功時 0 ，エラー時 -1 を返す．エラーの場合 Julius は異常終了する．
 * </JA>
 * 
 */
int
startup(void *data)
{
  Recog *recog = data;
  callback_add(recog, CALLBACK_EVENT_SPEECH_READY, status_recready, NULL);
  return 0;
}
