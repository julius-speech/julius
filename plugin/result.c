/**
 * @file   result.c
 * 
 * <EN>
 * @brief  Plugin to process recognition result
 * </EN>
 * 
 * <JA>
 * @brief  認識結果を処理するプラグイン
 * </JA>
 * 
 * @author Akinobu Lee
 * @date   Fri Aug 22 15:17:59 2008
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
 * Result processing function
 * 
 *   - result_str()
 *   
 */

/***************************************************************************/

#include <stdio.h>
#include <string.h>

#define PLUGIN_TITLE "result process plugin for Julius"

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
 * @brief  Process a recognition result (best string)
 *
 * This function will be called each time after recognition of an
 * utterance is finished.  The best recognition result for the
 * utterance will be passed to this function, as a string in which
 * words are separated by white space.  When the recognition was failed
 * or rejected, string will be NULL.
 *
 * On short-pause segmentation mode or GMM/Decoder-VAD mode, where
 * an input utterance may be segmented into pieces, this funtion will be
 * called for each segment.  On multi decoding, the best hypothesis among
 * all the recognition instance will be given.
 * 
 * @param result_str [in] recognition result, words separated by whitespace,
 * or NULL on failure
 * 
 * </EN>
 * <JA>
 * @brief  認識結果の処理（最尤文字列）
 *
 * この関数は入力の認識が終わるたびに呼び出され，
 * 入力に対する認識結果（最も確率の高い候補）の文字列が渡される．
 * 与えられる文字列は，単語毎にスペースで区切られる．
 * 認識が失敗した場合は， 文字列に NULL が渡される．
 *
 * ショートポーズセグメンテーションや GMM/Decoder ベースのVADを
 * 行う場合，入力は小単位に分割される．この場合，この関数は
 * その分割された小単位ごとに呼ばれる．また，複数モデル認識の場合，
 * 全認識処理中で最もスコアの高い仮説が渡される．
 * 
 * @param result_str [in] 認識結果（単語は空白で区切られている）NULLの
 * 場合，認識失敗．
 * 
 * </JA>
 * 
 */
void
result_best_str(char *result_str)
{
  if (result_str == NULL) {
    printf("[failed]\n");
  } else {
    printf("               <<%s>>\n", result_str);
  }
}
