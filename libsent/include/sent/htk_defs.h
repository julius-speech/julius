/**
 * @file   htk_defs.h
 *
 * <EN>
 * @brief Symbol definitions for HTK HMM and HTK parameter
 *
 * This file defines symbols for HMM parameter definition needed
 * for both HMM definition file and HMM parameter file.
 *
 * @sa htk_hmm.h
 * @sa htk_param.h
 * </EN>
 * <JA>
 * @brief HTKの特徴パラメータの形式に関する定義
 *
 * このファイルには, HTK形式のHMM定義ファイル,あるいはHTK形式の
 * パラメータファイルを読み込む際に必要な,パラメータ型に関連する
 * 定義が納められています．
 *
 * @sa htk_hmm.h
 * @sa htk_param.h
 * </JA>
 *
 * @author Akinobu LEE
 * @date   Thu Feb 10 19:36:47 2005
 *
 * $Revision: 1.6 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __SENT_HTK_DEFS_H__
#define __SENT_HTK_DEFS_H__

/// Definition of input parameter types derived from HTK
enum parameter_type {
  F_WAVEFORM,			///< Waveform format
  F_LPC,			///< LPC --- linear prediction coef. 
  F_LPREFC,			///< linear prediction refrection coef. 
  F_LPCEPSTRA,			///< LPC cepstrum 
  F_LPDELCEP,
  F_IREFC,
  F_MFCC,			///< mel-frequency cepstral coef. 
  F_FBANK,			///< log-scale filterbank parameter 
  F_MELSPEC,			///< mel-scale filterbank parameter 
  F_USER,
  F_DISCRETE,			///< discrete 
  F_ERR_INVALID			///< ERROR 
};

/* Additional parameter qualifiers */
#define F_ENERGY     0x0040	///< @c _E log energy coef. 
#define F_ENERGY_SUP 0x0080	///< @c _N (with _E) suppress absolute energy 
#define F_DELTA      0x0100 	///< @c _D delta (first-order regression) coef. 
#define F_ACCL       0x0200	///< @c _A (with _D) acceleration (second-order) coef. 
#define F_COMPRESS   0x0400	///< @c _C compressed 
#define F_CEPNORM    0x0800	///< @c _Z cepstral mean normalization 
#define F_CHECKSUM   0x1000	///< @c _K CRC checksum added 
#define F_ZEROTH     0x2000	///< @c _0 (with MFCC) 0'th cepstral parameter 

#define F_BASEMASK   0x003f	///< Mask to extract qualifiers

/// Covariance matrix types: only C_INV_DIAG is supported in Julius
enum {
  C_DIAG_C,			///< (not supported) Diagonal covariance
  C_INV_DIAG,			///< Inversed diagonal covaritance
  C_FULL,			///< (not supported) Full covariance
  C_LLT,			///< (not supported) 
  C_XFORM};			///< (not supported) 

/// Duration model types: No duration model is supported in Julius, so only D_NULL is acceptable
enum {
  D_NULL,			///< No duration model
  D_POISSON,			///< (not supported) 
  D_GAMMA,			///< (not supported) 
  D_GEN};			///< (not supported) 

/**
 * @brief Structure for decoding/encoding parameter type code
 *
 * @sa libsent/src/anlz/paramtypes.c
 * @sa libsent/src/hmminfo/rdhmmdef_options.c
 */
typedef struct {
  char *name;			///< Name string used in HTK hmmdefs
  short type;			///< Type code (one of definitions above)
  char *desc;			///< Brief description for user */
  boolean supported;		///< TRUE if this is supported in Julius
} OptionStr;

/// Header string to detect binary HMM file
#define BINHMM_HEADER "JBINHMM\n"

/// Header string for binary HMM file V2 (parameter embedded)
#define BINHMM_HEADER_V2 "JBINHMMV2"

/// A header qualifier string for V2: acoustic analysis parameter embedded
#define BINHMM_HEADER_V2_EMBEDPARA 'P'

/// A header qualifier string for V2: variance inversed
#define BINHMM_HEADER_V2_VARINV 'V'

/// A header qualifier string for V2: has mixture pdf macro def
#define BINHMM_HEADER_V2_MPDFMACRO 'M'

/// Maximum number of input stream
#define MAXSTREAMNUM 50

#ifdef ENABLE_MSD
#define LZERO (-1.0E10) 		///< log(0) value of void dimension for MSD-HMM */
#endif

#endif /* __SENT_HTK_DEFS_H__ */
