/**
 * @file   feature_postprocess.c
 * 
 * <EN>
 * @brief  A sample plugin for feature vector postprocessing
 * </EN>
 * 
 * <JA>
 * @brief  特徴量の後処理プラグインのサンプル
 * </JA>
 * 
 * @author Akinobu Lee
 * @date   Sun Aug 10 15:14:19 2008
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
 * Feature vector input postprocessing functions
 * 
 * Required:
 *   - fvin_postprocess()
 *   
 */

/***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "plugin_defs.h"

#define PLUGIN_TITLE "feature vector postprocess plugin for Julius"

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
 * @brief  Post-processing function for a feature vector
 *
 * When defined, this function will be called at every input vector
 * before recognition.  This will be called successively for every input
 * at each frame.
 *
 * You can monitor the feature vector to be recognized, and also can
 * modify or overwrite the content to do some transformation like a
 * feature-space adaptation.
 *
 * If multiple plugins have this functions, they are all executed in order
 * of loading.
 * 
 * @param vecbuf [i/o] a feature vector
 * @param veclen [in] length of @a vecbuf
 * @param nframe [in] frame number in a recognition, staring with 0.
 * 
 * </EN>
 * <JA>
 * @brief  特徴量ベクトルに対する後処理関数
 *
 * この関数が定義された場合，Julius は個々の特徴量ベクトルについて，
 * 認識が行われる前にこの関数を呼び出す．この関数は，入力が進むたびに
 * その各フレームの特徴量ベクトルについて呼ばれる．
 *
 * この関数を使って入力の特徴量ベクトルをモニタできるほか，バッファ上の
 * データを直接書き換えることもできる．音声認識はこの関数が終わったあとの
 * データに対して行われるので，例えば話者適応や話者正規化のような処理を
 * ここで行うことも可能である．
 *
 * 複数のプラグインでこの関数が指定されている場合，それらは読み込み順に
 * 実行される．
 * 
 * @param vecbuf [i/o] 特徴量ベクトル
 * @param veclen [in] @a vecbuf の長さ
 * @param nframe [in] フレーム番号
 * 
 * </JA>
 * 
 */
void
fvin_postprocess(float *vecbuf, int veclen, int nframe)
{
  int i;
  /* just output the vectors to stdout */
  printf("%d:", nframe);
  for(i=0;i<veclen;i++) {
    printf(" %f", vecbuf[i]);
  }
  printf("\n");
}
/* end of file */
