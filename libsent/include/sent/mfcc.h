/**
 * @file   mfcc.h
 *
 * <JA>
 * @brief MFCC計算のための定義
 *
 * このファイルには，音声波形データからMFCC形式の特徴量ベクトル系列を
 * 計算するための構造体の定義およびデフォルト値が含まれています．
 * デフォルト値は Julius とともに配布されている音響モデルで使用している
 * 値であり，HTKのデフォルトとは値が異なる部分がありますので注意して下さい．
 * </JA>
 * <EN>
 * @brief Definitions for MFCC computation
 *
 * This file contains structures and default values for extracting speech
 * parameter vectors of Mel-Frequency Cepstral Cefficients (MFCC).
 * The default values here are the ones used in the standard acoustic models
 * distributed together with Julius, and some of them have different value from
 * HTK defaults.  So be careful of the default values.
 * </EN>
 *
 * @sa libsent/src/wav2mfcc/wav2mfcc.c
 * @sa libsent/src/wav2mfcc/wav2mfcc-pipe.c
 * @sa julius/wav2mfcc.c
 * @sa julius/realtime-1stpass.c
 * 
 * @author Akinobu LEE
 * @date   Fri Feb 11 03:40:52 2005
 *
 * $Revision: 1.8 $
 * 
 */


/************************************************************************/
/*    mfcc.h                                                            */
/*                                                                      */
/*    Author    : Yuichiro Nakano                                       */
/************************************************************************/

#ifndef __MFCC_H__
#define __MFCC_H__

/// DEBUG: define if you want to enable debug messages for sin/cos table operation
#undef MFCC_TABLE_DEBUG

#define CPMAX 500		///< Maximum number of frames to store ceptral mean for realtime CMN update
#define CPSTEP 5		///< allocate step of cmean list per sentence

#include <sent/stddefs.h>
#include <sent/htk_defs.h>
#include <sent/htk_param.h>
#include <ctype.h>

#define DEF_SMPPERIOD   625	///< Default sampling period in 100ns (625 = 16kHz)
#define DEF_FRAMESIZE   400	///< Default Window size in samples, similar to WINDOWSIZE in HTK (unit is different)
#define DEF_FFTNUM      512	///< Number of FFT steps
#define DEF_FRAMESHIFT  160	///< Default frame shift length in samples
#define DEF_PREENPH     0.97	///< Default pre-emphasis coefficient, corresponds to PREEMCOEF in HTK
#define DEF_MFCCDIM     12	///< Default number of MFCC dimension, corresponds to NUMCEPS in HTK
#define DEF_CEPLIF      22	///< Default cepstral Liftering coefficient, corresponds to CEPLIFTER in HTK
#define DEF_FBANK       24	///< Default number of filterbank channels, corresponds to NUMCHANS in HTK
#define DEF_DELWIN      2	///< Default delta window size, corresponds to DELTAWINDOW in HTK
#define DEF_ACCWIN      2	///< Default acceleration window size, corresponds to ACCWINDOW in HTK
#define DEF_SILFLOOR    50.0	///< Default energy silence floor in dBs, corresponds to SILFLOOR in HTK
#define DEF_ESCALE      1.0	///< Default scaling coefficient of log energy, corresponds to ESCALE in HTK

#define DEF_SSALPHA     2.0	///< Default alpha coefficient for spectral subtraction
#define DEF_SSFLOOR     0.5	///< Default flooring coefficient for spectral subtraction

/* version 2 ... ss_floor and ss_alpha removed */
/* version 3 add usepower */
#define VALUE_VERSION 3	///< Integer version number of Value, for embedding

/// mfcc configuration parameter values
typedef struct {
  short basetype;       ///< Parameter basetype (F_MFCC/F_FBANK/F_MELSPEC)/
  int smp_period;       ///< Sampling period in 100ns units
  int smp_freq;	        ///< Sampling frequency
  int framesize;        ///< Window size in samples, similar to WINDOWSIZE in HTK (unit is different)
  int frameshift;       ///< Frame shift length in samples
  float preEmph;        ///< Pre-emphasis coefficient, corresponds to PREEMCOEF in HTK
  int lifter;           ///< Cepstral liftering coefficient, corresponds to CEPLIFTER in HTK
  int fbank_num;        ///< Number of filterbank channels, corresponds to NUMCHANS in HTK
  int delWin;           ///< Delta window size, corresponds to DELTAWINDOW in HTK
  int accWin;           ///< Acceleration window size, corresponds to ACCWINDOW in HTK
  float silFloor;       ///< Energy silence floor in dBs, corresponds to SILFLOOR in HTK
  float escale;         ///< Scaling coefficient of log energy, corresponds to ESCALE in HTK
  int hipass;		///< High frequency cut-off in fbank analysis, -1 if disabled, corresponds to HIFREQ in HTK
  int lopass;		///< Low frequency cut-off in fbank analysis, -1 if disabled, corresponds to LOFREQ in HTK
  int enormal;          ///< 1 if normalise raw energy, 0 if disabled, corresponds to ENORMALISE in HTK
  int raw_e;            ///< 1 if using raw energy, 0 if disabled, corresponds to RAWENERGY in HTK
  int zmeanframe;	///< 1 if apply zero mean frame like ZMEANSOURCE in HTK
  int usepower;		///< 1 if use power instead of magnitude in filterbank analysis
  float vtln_alpha;	///< warping factor for VTLN, corresponds to WARPFREQ in HTK
  float vtln_upper;	///< hi freq. cut off for VTLN, corresponds to WARPUCUTOFF in HTK
  float vtln_lower;	///< low freq. cut off for VTLN, corresponds to WARPLCUTOFF in HTK

  /* items below does not need to be embedded, because they can be
     detemined from the acoustic model header, or should be computed
     from run-time variables */
  int delta;            ///< 1 if delta coef. needs to be computed
  int acc;              ///< 1 if acceleration coef. needs to be computed
  int energy;		///< 1 if energy coef. needs to be computed
  int c0;		///< 1 if use 0'th cepstral parameter, 0 if disabled, corresponds to _0 qualifier in HTK
  int absesup;		///< 1 if absolute energy should be suppressed
  int cmn;              ///< 1 if use Cepstrum Mean Normalization, 0 if disabled, corresponds to _Z qualifier in HTK
  int cvn;		///< 1 if use cepstral variance normalization, else 0 */
  int mfcc_dim;         ///< Number of MFCC dimensions
  int baselen;		///< Number of base MFCC dimension with energies
  int vecbuflen;	///< Vector length needed for computation
  int veclen;		///< Resulting length of vector

  int loaded;		///< 1 if these parameters were loaded from HTK config file or binhmm header
}Value;

/// Workspace for filterbank analysis
typedef struct {
   int fftN;            ///< Number of FFT point
   int n;               ///< log2(fftN)
   int klo;             ///< FFT indices of lopass cut-off
   int khi;             ///< FFT indices of hipass cut-off
   float fres;          ///< Scaled FFT resolution
   float *cf;           ///< Array[1..pOrder+1] of centre freqs
   short *loChan;       ///< Array[1..fftN/2] of loChan index
   float *loWt;         ///< Array[1..fftN/2] of loChan weighting
   float *Re;           ///< Array[1..fftN] of fftchans (real part)
   float *Im;           ///< Array[1..fftN] of fftchans (imag part)
} FBankInfo;

/// Cycle buffer for delta computation
typedef struct {
  float **mfcc;			///< MFCC buffer
  int veclen;			///< Vector length of above
  float *vec;			///< Points to the current MFCC
  int win;			///< Delta window length
  int len;			///< Length of the buffer (= win*2+1)
  int store;			///< Current next storing point
  boolean *is_on;		///< TRUE if data filled
  int B;			///< B coef. for delta computation
} DeltaBuf;

/// Work area for MFCC computation
typedef struct {
  float *bf;			///< Local buffer to hold windowed waveform 
  double *fbank;   ///< Local buffer to hold filterbank
  FBankInfo fb;	///< Local buffer to hold filterbank information
  int bflen;			///< Length of above
  boolean fbank_only;		///< True if output is filterbank
  boolean log_fbank;		///< True if use log filterbank
#ifdef MFCC_SINCOS_TABLE
  double *costbl_hamming; ///< Cos table for hamming window
  int costbl_hamming_len; ///< Length of above
  /* cos/-sin table for FFT */
  double *costbl_fft; ///< Cos table for FFT
  double *sintbl_fft; ///< Sin table for FFT
  int tbllen; ///< Length of above
  /* cos table for MakeMFCC */
  double *costbl_makemfcc; ///< Cos table for DCT
  int costbl_makemfcc_len; ///< Length of above
  /* sin table for WeightCepstrum */
  double *sintbl_wcep; ///< Sin table for cepstrum weighting
  int sintbl_wcep_len; ///< Length of above
#endif /* MFCC_SINCOS_TABLE */
  float sqrt2var; ///< Work area that holds value of sqrt(2.0) / fbank_num
  float *ssbuf;			///< Pointer to noise spectrum for SS
  int ssbuflen;			///< length of @a ssbuf
  float ss_floor;		///< flooring value for SS
  float ss_alpha;		///< alpha scaling value for SS
} MFCCWork;

/**
 * Structure to hold sentence sum of MFCC for realtime CMN
 * 
 */
typedef struct {
  float *mfcc_sum;		///< Sum of MFCC parameters
  float *mfcc_var;		///< Variance sum of MFCC parameters
  int framenum;			///< summed number of frames
} CMEAN;

/**
 * Work area for real-time CMN
 * 
 */
typedef struct {
  CMEAN *clist;		///< List of MFCC sum for previous inputs
  int clist_max;		///< Allocated number of CMEAN in clist
  int clist_num;		///< Currentlly filled CMEAN in clist
  float cweight;		///< Weight of initial cepstral mean
  float *cmean_init;	///< Initial cepstral mean for each input
  float *cvar_init;		///< Inisial cepstral standard deviation for each input
  int mfcc_dim;			///< base MFCC dimension (to apply CMN)
  int veclen;			///< full MFCC vector length
  boolean mean;			///< TRUE if CMN is enabled
  boolean var;			///< TRUE if CVN is enabled
  boolean cmean_init_set;	///< TRUE if cmean_init (and cvar_init) was set
  CMEAN now;		///< Work area to hold current cepstral mean and variance
  CMEAN all;		///< Work area to hold all cepstral mean and variance
  boolean loaded_from_file;	///< TRUE if loaded from file
} CMNWork;

/**
 * work area for energy normalization on real time input
 * 
 */
typedef struct {
  LOGPROB max_last;	///< Maximum energy value of last input
  LOGPROB min_last;	///< Minimum floored energy value of last input
  LOGPROB max;	///< Maximum energy value of current input
} ENERGYWork;


#ifdef __cplusplus
extern "C" {
#endif

/**** mfcc-core.c ****/
MFCCWork *WMP_work_new(Value *para);
void WMP_calc(MFCCWork *w, float *mfcc, Value *para);
void WMP_free(MFCCWork *w);
/* Get filterbank information */
boolean InitFBank(MFCCWork *w, Value *para);
void FreeFBank(FBankInfo *fb);
/* Apply hamming window */
void Hamming (float *wave, int framesize, MFCCWork *w);
/* Apply pre-emphasis filter */
void PreEmphasise (float *wave, int framesize, float preEmph);
/* Return mel-frequency */
float Mel(int k, float fres);
/* Apply FFT */
void FFT(float *xRe, float *xIm, int p, MFCCWork *w);
/* Convert wave -> mel-frequency filterbank */
void MakeFBank(float *wave, MFCCWork *w, Value *para);
/* Apply the DCT to filterbank */ 
void MakeMFCC(float *mfcc, Value *para, MFCCWork *w);
/* Calculate 0'th Cepstral parameter*/
float CalcC0(MFCCWork *w, Value *para);
/* Calculate Log Raw Energy */
float CalcLogRawE(float *wave, int framesize);
/* Zero Mean Souce by frame */
void ZMeanFrame(float *wave, int framesize);
/* Re-scale cepstral coefficients */
void WeightCepstrum (float *mfcc, Value *para, MFCCWork *w);

/**** wav2mfcc-buffer.c ****/
/* Convert wave -> MFCC_E_D_(Z) (batch) */
int Wav2MFCC(SP16 *wave, float **mfcc, Value *para, int nSamples, MFCCWork *w, CMNWork *c);
/* Calculate delta coefficients (batch) */
void Delta(float **c, int frame, Value *para);
/* Calculate acceleration coefficients (batch) */
void Accel(float **c, int frame, Value *para);
/* Normalise log energy (batch) */
void NormaliseLogE(float **c, int frame_num, Value *para);
/* Cepstrum Mean Normalization (batch) */
void CMN(float **mfcc, int frame_num, int dim, CMNWork *c);
void MVN(float **mfcc, int frame_num, Value *para, CMNWork *c);

/**** wav2mfcc-pipe.c ****/
DeltaBuf *WMP_deltabuf_new(int veclen, int windowlen);
void WMP_deltabuf_free(DeltaBuf *db);
void WMP_deltabuf_prepare(DeltaBuf *db);
boolean WMP_deltabuf_proceed(DeltaBuf *db, float *new_mfcc);
boolean WMP_deltabuf_flush(DeltaBuf *db);

CMNWork *CMN_realtime_new(Value *para, float weight);
void CMN_realtime_free(CMNWork *c);
void CMN_realtime_prepare(CMNWork *c);
void CMN_realtime(CMNWork *c, float *mfcc);
void CMN_realtime_update(CMNWork *c, HTK_Param *param);
boolean CMN_load_from_file(CMNWork *c, char *filename);
boolean CMN_save_to_file(CMNWork *c, char *filename);

void energy_max_init(ENERGYWork *energy);
void energy_max_prepare(ENERGYWork *energy, Value *para);
LOGPROB energy_max_normalize(ENERGYWork *energy, LOGPROB f, Value *para);

/**** ss.c ****/
/* spectral subtraction */
float *new_SS_load_from_file(char *filename, int *slen);
float *new_SS_calculate(SP16 *wave, int wavelen, int *slen, MFCCWork *w, Value *para);

/**** para.c *****/
void undef_para(Value *para);
void make_default_para(Value *para);
void make_default_para_htk(Value *para);
void apply_para(Value *dst, Value *src);
boolean htk_config_file_parse(char *HTKconffile, Value *para);
void calc_para_from_header(Value *para, short param_type, short vec_size);
void put_para(FILE *fp, Value *para);

#ifdef __cplusplus
}
#endif

#endif /* __MFCC_H__ */
