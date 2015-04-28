/**
 * @file   audio_postprocess.c
 * 
 * <EN>
 * @brief  A sample audio postprocessing plugin
 * </EN>
 * 
 * <JA>
 * @brief  オーディオ入力の後処理プラグインのサンプル
 * </JA>
 * 
 * @author Akinobu Lee
 * @date   Sun Aug 10 15:12:50 2008
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
 * A/D-in postprocessing functions
 * 
 * Required:
 *   - adin_postprocess()
 *   
 */

/***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "plugin_defs.h"

#define PLUGIN_TITLE "audio postprocess plugin for Julius"

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
 * @brief  Post-processing function for captured audio
 *
 * When defined, this function will be called at every audio input
 * fragments before any feature analysis.  @a buf contains the small
 * fragment of captured audio input at a length of @a len, and this
 * will be called successively as input goes.
 *
 * You can monitor the incoming audio stream, and also can modify or
 * overwrite the content of @a buf to do some audio processing for the
 * incoming data like noise supression etc.
 *
 * If multiple plugins have this functions, they are all executed in order
 * of loading.
 * 
 * @param buf [i/o] a fragment of audio inputs
 * @param len [in] length of @a buf (in samples)
 * 
 * </EN>
 * <JA>
 * @brief  音声入力に対する後処理
 *
 * この関数が定義された場合，Julius は入力された音声データに対して，特
 * 徴量抽出を行う前にこの関数を呼び出す．@a buf には @a len の長さの音
 * 声入力データ断片が入っている．この関数は，入力が進むたびにその短い
 * 断片ごとに繰り返し呼ばれる．
 *
 * この関数を使って入力音声データをモニタできるほかに，バッファ上の
 * データを直接書き換えることもできる．音声認識はこの関数が終わったあとの
 * データに対して行われるので，例えば雑音抑圧処理などをここで行う
 * ことも可能である．
 *
 * 複数のプラグインでこの関数が指定されている場合，それらは読み込み順に
 * 実行される．
 * 
 * @param buf [i/o] 音声入力データ断片の入ったバッファ
 * @param len [in] @a buf の長さ（サンプル数）
 * 
 * </JA>
 * 
 */
void
adin_postprocess(SP16 *buf, int len)
{
  //printf("%d\n", len);
}
/* end of file */
