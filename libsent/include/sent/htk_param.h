/**
 * @file   htk_param.h
 *
 * <EN>
 * @brief Structures for storing input speech parameters
 *
 * This file defines a structure for holding a vector sequence of
 * input speech parameters.  The speech parameter sequence will be
 * stored in HTK_Param.  The HTK_Param also holds information about
 * the extraction condition, i.e., frame shift length, window size and
 * so on.
 *
 * The speech input vector can be read from HTK parameter file, or
 * Julius extracts the parameters directly from input speech.  Julius
 * supports extraction of only MFCC format of fixed dimension.
 * More precisely, only parameter type of MFCC_{0|E}_D[_Z][_N] with {25|26}
 * dimensions is supported.
 *
 * When recognition, the parameter types of both acoustic model and
 * input parameter should be the same.  Please note that only the
 * parameter type is checked, and other parameters such as source sampling
 * rate, frame shift length and window sizes will not be checked.
 * </EN>
 * <JA>
 * @brief HTKの特徴パラメータを保持する構造体に関連する定義
 *
 * このファイルには，音声特徴量のベクトル系列を保持する構造体が
 * 定義されています．入力音声から計算されたMFCC等の音声特徴量は，
 * ここで定義される構造体 HTK_Param に保存されます．HTK_Paramには
 * また，特徴量抽出時のフレームシフト幅やウィンドウ長などの情報が
 * 保持されます．
 *
 * 音声特徴量は外部で HTK などによって抽出されたHTK形式の特徴量ファイルを
 * 読み込むことができます．また，MFCC 形式であれば Julius 内で
 * 直接音声波形から抽出することができます．実際にJuliusが内部で抽出する
 * することができる特徴量は {25|26} 次元の MFCC_{0|E}_D[_Z][_N] のみです．
 *
 * 使用する音響モデル(%HMM)が学習時に用いた特徴量と認識対象とする入力の
 * 特徴量の形式は一致させる必要があります．認識実行時には，音響モデルと入力
 * ファイルの特徴量形式がチェックされ，適合しない場合はエラーとなります．
 * ただし，入力音声のサンプリング周波数やフレームシフト幅，ウィンドウ長の
 * 情報はHTK形式の音響モデルには保持されていないため，チェックできません．
 * 注意して下さい．
 * </JA>
 *
 * @sa htk_defs.h
 *
 * @author Akinobu LEE
 * @date   Fri Feb 11 02:52:52 2005
 *
 * $Revision: 1.7 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __SENT_HTK_PARAM_H__
#define __SENT_HTK_PARAM_H__

#include <sent/stddefs.h>
#include <sent/htk_defs.h>

/// Parameter types and extraction conditions
typedef struct {
  unsigned int samplenum;	///< Number of samples (or frames)
  unsigned int wshift;		///< Window shift (unit: 100ns) 
  unsigned short sampsize;	///< Bytes per sample 
  short samptype;		///< Parameter type, see also htk_defs.h
} HTK_Param_Header;

/// Input speech parameter
typedef struct {
  HTK_Param_Header header;	///< Parameter header information
  unsigned int samplenum;	///< Number of sample (same in header.samplenum) 
  short veclen;			///< Vector length of a sample
  VECT **parvec;		///< Actual parameter vectors [0..samplenum-1][0..veclen-1]
  short veclen_alloc;		///< Allocated vector length of a sample
  unsigned int samplenum_alloc;	///< Alllocated number of samples
  BMALLOC_BASE *mroot;		///< Pointer for block memory allocation
  boolean is_outprob;		///< TRUE if this is outprob vector
} HTK_Param;

/**
 * Increment step of HTK Parameter holder in frames
 * 
 */
#define HTK_PARAM_INCREMENT_STEP_FRAME 200

#ifdef __cplusplus
extern "C" {
#endif

boolean rdparam(char *, HTK_Param *);
HTK_Param *new_param();
void free_param(HTK_Param *);
short param_qualstr2code(char *);
short param_str2code(char *);
char *param_qualcode2str(char *, short, boolean);
char *param_code2str(char *, short, boolean);
int guess_basenum(HTK_Param *p, short qualtype);
boolean param_strip_zero(HTK_Param *param);

void param_init_content(HTK_Param *p);
boolean param_alloc(HTK_Param *p, unsigned int samplenum, short veclen);
void param_free_content(HTK_Param *p);


/* hmminfo/put_htkdata_info.c */
void put_param_head(FILE *fp, HTK_Param_Header *h);
void put_vec(FILE *fp, VECT **p, int num, short veclen);
void put_param(FILE *fp, HTK_Param *pinfo);
void put_param_info(FILE *fp, HTK_Param *pinfo);

#ifdef __cplusplus
}
#endif

#endif /* __SENT_HTK_PARAM_H__ */
