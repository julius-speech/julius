/**
 * @file   calcmix.c
 * 
 * <EN>
 * @brief  A sample plugin for calculating Gaussians
 *
 * This sample uses Julius libraries.
 * </EN>
 * 
 * <JA>
 * @brief  ガウス分布計算プラグインのサンプル
 *
 * このサンプルは julius のライブラリを使用します．
 * </JA>
 * 
 * @author Akinobu Lee
 * @date   Mon Aug 11 15:29:45 2008
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
 * Gaussian mixture calculation plugin:
 * 
 * Required:
 *   - calcmix_get_optname()
 *   - calcmix()
 *   - calcmix_init()
 *   - calcmix_free()
 * 
 */

/***************************************************************************/

/* we refer to Julius libsent header */
#include <sent/stddefs.h>
#include <sent/hmm_calc.h>

//#include "plugin_defs.h"

#define PLUGIN_TITLE "Gaussian calculation plugin for Julius"
#define GPRUNE_OPT "mycalc"

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

/** 
 * <EN>
 * @brief  Return option string to select at option. (required)
 *
 * This function should return option string which should be specified
 * as an argument "-gprune" option, to be used on Julius.  The returning
 * string should not be the same with any existing value.
 *
 * This function will be called several times at option parsing at startup.
 *
 * @param buf [out] buffer to store the return string
 * @param buflen [in] maximum length of buf
 * </EN>
 * <JA>
 * @brief  計算方法選択用オプションのための文字列を返す（必須）
 *
 * Julius で起動時に "-gprune ここで返す値" と指定するとこのプラグイン
 * が使用される．この関数では，上記の "-gprune" に与えるべき文字列を格
 * 納して返す．返す文字は，システムの "-gprune" オプションにすでにある
 * ものや，他のプラグインが使用しているものと同じでないこと．（もし同
 * じだった場合システム側が優先される）
 *
 * この関数は，起動時のオプション解析時に何度か呼ばれる．
 *
 * @param buf [out] 値を格納して返すバッファ
 * @param buflen [in] buf の最大長
 * </JA>
 * 
 */
void
calcmix_get_optname(char *buf, int buflen)
{
  strncpy(buf, GPRUNE_OPT, buflen);
}

/**
 * <EN>
 * @brief  A basic implementaion of computing Gaussians
 *
 * This function should compute output probabilities for each
 * Gaussians.  after this function returns, Julius will do addlog to
 * get the final output log probability.
 *
 * The input vector to be computed is located at wrk->OP_vec[], at a
 * length of wrk->OP_veclen.  Gaussians are given by g[], at a number
 * of num.  last_id and lnum is for internal use for pruning, just ignore
 * them.
 *
 * The scores for each Gaussians computed in this function should be
 * stored in OP_calced_score[], with their corresponding Gaussian ids
 * to OP_calced_id.  The total number of calculated mixtures shuold
 * also stored in OP_calced_num.
 * 
 * @param wrk [i/o] HMM computation work area to store data
 * @param g [in] set of Gaussian densities to compute the output probability.
 * @param num [in] length of above
 * @param last_id [in] ID list of N-best mixture in previous input frame,
 * or NULL if not exist
 * @param lnum [in] length of last_id
 * </EN>
 * <JA>
 * @brief  ガウス分布計算関数
 *
 * この関数では，与えられた複数のガウス分布に対して入力ベクトルの
 * 出力確率を求める．この関数が行うのは，複数のガウス分布それぞれの
 * 出力確率の算出と格納のみであり，混合分布としての重み計算や addlog
 * はこの関数が返ったあとに Julius 側で行われる．
 *
 * 入力ベクトルは wrk->OP_vec[] に格納されており，長さは wrk->OP_veclen
 * である．ガウス分布定義は g[] に配列として複数渡され，その数は num である．
 * 
 * なお，last_id と lnum はこのガウス分布集合 g[] において直前の入力フ
 * レームで計算されたものの id が入っている．Julius の内部処理用なので，
 * 使わなくても差し支えない．
 *
 * 各ガウス分布に対する入力ベクトルの対数出力確率は，そのガウス分布の
 * ID (0 から始まる配列の添え字) を wrk->OP_calced_id に，値を
 * wrk->OP_calced_score に格納すること．また，実際に計算された
 * ガウス分布の数を wrk->OP_calced_num に格納すること．
 * （これは Gaussian pruning を想定した実装である）
 *
 * 以下は，pruning 等を行わない単純な出力確率計算を実装したものである．
 * ガウス分布は対角共分散を仮定している．なお Julius では読み込み時に
 * HTK でいうところの gconst 値はあらかじめ計算される．このため，計算時に
 * 下記の dens->gconst のように利用できる．
 * 
 * @param wrk [i/o] HMM計算用ワークエリア
 * @param g [in] 出力確率を計算するガウス分布の列
 * @param num [in] @a g のガウス分布の数
 * @param last_id [in] 直前入力フレームで上位だったガウス分布のIDリスト，
 * または内場合は NULL
 * @param lnum [in] @a last_id の長さ
 * </JA>
 */
void
calcmix(HMMWork *wrk, HTK_HMM_Dens **g, int num, int *last_id, int lnum)
{
  int i;
  HTK_HMM_Dens *dens;
  LOGPROB *prob = wrk->OP_calced_score;
  int *id = wrk->OP_calced_id;
  VECT tmp, x;
  VECT *mean;
  VECT *var;
  VECT *vec;
  short veclen;

  for(i=0; i<num; i++) {
    /* store ID */
    *(id++) = i;
    /* get Gaussian to compute */
    dens = *(g++);
    if (dens == NULL) {
      /* no Gaussian, set LOG_ZERO as result */
      *(prob++) = LOG_ZERO;
      continue;
    }
    /* compute log outprob probability */
    mean = dens->mean;
    var = dens->var->vec;
    tmp = dens->gconst;
    vec = wrk->OP_vec;
    veclen = wrk->OP_veclen;
    for (; veclen > 0; veclen--) {
      x = *(vec++) - *(mean++);
      tmp += x * x * *(var++);
    }
    tmp *= -0.5;
    /* store it */
    *(prob++) = tmp;
  }
  wrk->OP_calced_num = num;
}

/**
 * <EN>
 * Free work area.
 * You should free all allocated at clacmix_init().
 * 
 * @param wrk [i/o] HMM computation work area
 * </EN>
 * <JA>
 * calcmix_init() で確保されたワークエリアを開放する．
 * 
 * @param wrk [i/o] HMM 計算用ワークエリア
 * </JA>
 * 
 */
void
calcmix_free(HMMWork *wrk)
{
  free(wrk->OP_calced_score);
  free(wrk->OP_calced_id);
}
  
/**
 * <EN>
 * @brief  Initialize and setup work area for Gaussian computation.
 *
 * You should set value for OP_calced_maxnum, and allocate OP_calced_score
 * and OP_calced_id.  Remaining the content below is safe.
 *
 * This will be called once on instance initialization at startup.
 * 
 * @param wrk [i/o] HMM computation work area
 * 
 * @return TRUE on success, FALSE on failure.
 * </EN>
 * <JA>
 * @brief  計算用のワークエリアを確保する．
 *
 * ガウス分布計算用のワークエリアを確保する．下記にすでに書いてある分は，
 * そのまま Julius の内部でも使用しているので，削らないこと．
 *
 * この関数は最初に音響尤度計算インスタンスが作成されるときに呼び出される．
 * 
 * @param wrk [i/o] HMM 計算用ワークエリア
 * 
 * @return 成功時 TRUE，失敗時 FALSE を返す．
 * </JA>
 */
boolean
calcmix_init(HMMWork *wrk)
{
  /* maximum Gaussian set size = maximum mixture size * nstream */
  wrk->OP_calced_maxnum = wrk->OP_hmminfo->maxmixturenum * wrk->OP_nstream;
  wrk->OP_calced_score = (LOGPROB *)malloc(sizeof(LOGPROB) * wrk->OP_calced_maxnum);
  wrk->OP_calced_id = (int *)malloc(sizeof(int) * wrk->OP_calced_maxnum);
  /* force gprune_num to the max number */
  wrk->OP_gprune_num = wrk->OP_calced_maxnum;
  return TRUE;
}

/* end of file */
