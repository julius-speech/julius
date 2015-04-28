/**
 * @file   wav2mfcc-buffer.c
 * 
 * <JA>
 * @brief  音声波形から MFCC 特徴量へ変換する(発話単位)
 *
 * ここでは音声波形全体を単位として MFCC ベクトル系列へ変換する関数が定義
 * されています．フレーム単位で抽出を行う関数は wav2mfcc-pipe.c に
 * 記述されています
 * 
 * ここで抽出できるのは MFCC[_0][_E][_D][_A][_Z] の形式です．
 * </JA>
 * 
 * <EN>
 * @brief  Convert speech inputs into MFCC parameter vectors (per utterance)
 *
 * This file contains functions to convert the whole speech input
 * to MFCC vector array.  The frame-wise MFCC computation needed for
 * real-time recognition is defined in wav2mfcc-pipe.c.
 *
 * The supported format is MFCC[_0][_E][_D][_A][_Z].
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 17:43:35 2005
 *
 * $Revision: 1.7 $
 * 
 */

/************************************************************************/
/*    wav2mfcc.c   Convert Speech file to MFCC_E_D_(Z) file             */
/*----------------------------------------------------------------------*/
/*    Author    : Yuichiro Nakano                                       */
/*                                                                      */
/*    Copyright(C) Yuichiro Nakano 1996-1998                            */
/*----------------------------------------------------------------------*/
/************************************************************************/


#include <sent/stddefs.h>
#include <sent/mfcc.h>

/** 
 * Convert wave data to MFCC.  Also does spectral subtraction
 * if @a ssbuf specified.
 * 
 * @param wave [in] waveform data
 * @param mfcc [out] buffer to store the resulting MFCC parameter vector [t][0..veclen-1], should be already allocated
 * @param para [in] configuration parameters
 * @param nSamples [in] length of waveform data
 * @param w [i/o] MFCC calculation work area
 * 
 * @return the number of processed frames.
 */
int
Wav2MFCC(SP16 *wave, float **mfcc, Value *para, int nSamples, MFCCWork *w, CMNWork *c)
{
  int i, k, t;
  int end = 0, start = 1;
  int frame_num;                    /* Number of samples in output file */

  /* set noise spectrum if any */
  if (w->ssbuf != NULL) {
    /* check ssbuf length */
    if (w->ssbuflen != w->bflen) {
      jlog("Error: mfcc-core: noise spectrum length not match\n");
      return FALSE;
    }
  }

  frame_num = (int)((nSamples - para->framesize) / para->frameshift) + 1;
  
  for(t = 0; t < frame_num; t++){
    if(end != 0) start = end - (para->framesize - para->frameshift) - 1;

    k = 1;
    for(i = start; i <= start + para->framesize; i++){
      w->bf[k] = (float)wave[i - 1];  k++;
    }
    end = i;
    
    /* Calculate base MFCC coefficients */
    WMP_calc(w, mfcc[t], para);
  }
  
  /* Normalise Log Energy */
  if (para->energy && para->enormal) NormaliseLogE(mfcc, frame_num, para);
  
  /* Delta (consider energy suppress) */
  if (para->delta) Delta(mfcc, frame_num, para);

  /* Acceleration */
  if (para->acc) Accel(mfcc, frame_num, para);

  /* Cepstrum Mean and/or Variance Normalization */
  if (para->cmn && ! para->cvn) CMN(mfcc, frame_num, para->mfcc_dim + (para->c0 ? 1 : 0), c);
  else if (para->cmn || para->cvn) MVN(mfcc, frame_num, para, c);

  return(frame_num);
}

/** 
 * Normalise log energy
 * 
 * @param mfcc [i/o] array of MFCC vectors
 * @param frame_num [in] number of frames
 * @param para [in] configuration parameters
 */
void NormaliseLogE(float **mfcc, int frame_num, Value *para)
{  
  float max, min, f;
  int t;
  int l;

  l = para->mfcc_dim;
  if (para->c0) l++;

  /* find max log energy */
  max = mfcc[0][l];
  for(t = 0; t < frame_num; t++)
    if(mfcc[t][l] > max) max = mfcc[t][l];

  /* set the silence floor */
  min = max - (para->silFloor * LOG_TEN) / 10.0;  

  /* normalise */
  for(t = 0; t < frame_num; t++){
    f = mfcc[t][l];
    if (f < min) f = min;
    mfcc[t][l] = 1.0 - (max - f) * para->escale;
  }
}

/** 
 * Calculate delta coefficients
 * 
 * @param c [i/o] MFCC vectors, in which the delta coeff. will be appended.
 * @param frame [in] number of frames
 * @param para [in] configuration parameters
 */
void Delta(float **c, int frame, Value *para)
{
  int theta, t, n, B = 0;
  float A1, A2, sum;

  for(theta = 1; theta <= para->delWin; theta++)
    B += theta * theta;

  for(n = para->baselen - 1; n >=0; n--){
    for(t = 0; t < frame; t++){
      sum = 0;
      for(theta = 1; theta <= para->delWin; theta++){
	/* Replicate the first or last vector */
	/* at the beginning and end of speech */
	if (t - theta < 0) A1 = c[0][n];
	else A1 = c[t - theta][n];
	if (t + theta >= frame) A2 = c[frame - 1][n];
	else A2 = c[t + theta][n];
	sum += theta * (A2 - A1);
      }
      sum /= (2.0 * B);
      if (para->absesup) {
	c[t][para->baselen + n - 1] = sum;
      } else {
	c[t][para->baselen + n] = sum;
      }
    }
  }
}


/** 
 * Calculate acceleration coefficients.
 * 
 * @param c [i/o] MFCC vectors, in which the delta coeff. will be appended.
 * @param frame [in] number of frames
 * @param para [in] configuration parameters
 */
void Accel(float **c, int frame, Value *para)
{
  int theta, t, n, B = 0;
  int src, dst;
  float A1, A2, sum;

  for(theta = 1; theta <= para->accWin; theta++)
    B += theta * theta;

  for(t = 0; t < frame; t++){
    src = para->baselen * 2 - 1;
    if (para->absesup) src--;
    dst = src + para->baselen;
    for(n = 0; n < para->baselen; n++){
      sum = 0;
      for(theta = 1; theta <= para->accWin; theta++){
	/* Replicate the first or last vector */
	/* at the beginning and end of speech */
	if (t - theta < 0) A1 = c[0][src];
	else A1 = c[t - theta][src];
	if (t + theta >= frame) A2 = c[frame - 1][src];
	else A2 = c[t + theta][src];
	sum += theta * (A2 - A1);
      }
      c[t][dst] = sum / (2 * B);
      src--;
      dst--;
    }
  }
}

/** 
 * Cepstrum Mean Normalization (buffered)
 * Cepstral mean will be computed within the given MFCC vectors.
 * 
 * @param mfcc [i/o] array of MFCC vectors
 * @param frame_num [in] number of frames
 * @param dim [in] total dimension of MFCC vectors
 */
void CMN(float **mfcc, int frame_num, int dim, CMNWork *c)
{
  int i, t;
  float *mfcc_ave, *sum;

  if (c != NULL && c->cmean_init_set) {
    /* has initial param, use it permanently */
    for(t = 0; t < frame_num; t++){
      for(i = 0; i < dim; i++)
	mfcc[t][i] -= c->cmean_init[i];
    }
  } else {
    /* compute from current input */
    mfcc_ave = (float *)mycalloc(dim, sizeof(float));
    sum = (float *)mycalloc(dim, sizeof(float));
    for(i = 0; i < dim; i++){
      sum[i] = 0.0;
      for(t = 0; t < frame_num; t++)
	sum[i] += mfcc[t][i];
      mfcc_ave[i] = sum[i] / frame_num;
    }
    for(t = 0; t < frame_num; t++){
      for(i = 0; i < dim; i++)
	mfcc[t][i] = mfcc[t][i] - mfcc_ave[i];
    }
    free(sum);
    free(mfcc_ave);
  }
}

/** 
 * Cepstrum Mean/Variance Normalization (buffered)
 * 
 * @param mfcc [i/o] array of MFCC vectors
 * @param frame_num [in] number of frames
 * @param para [in] configuration parameters
 */
void MVN(float **mfcc, int frame_num, Value *para, CMNWork *c)
{
  int i, t;
  float *mfcc_mean, *mfcc_sd;
  float x;
  int basedim;

  basedim = para->mfcc_dim + (para->c0 ? 1 : 0);

  if (c != NULL && c->cmean_init_set) {
    /* has initial param, use it permanently */
    for(t = 0; t < frame_num; t++){
      if (para->cmn) {
	/* mean normalization (base MFCC only) */
	for(i = 0; i < basedim; i++) mfcc[t][i] -= c->cmean_init[i];
      }
      if (para->cvn) {
	/* variance normalization (full MFCC) */
	for(i = 0; i < para->veclen; i++) mfcc[t][i] /= sqrt(c->cvar_init[i]);
      }
    }
    return;
  }

  mfcc_mean = (float *)mycalloc(para->veclen, sizeof(float));
  if (para->cvn) mfcc_sd = (float *)mycalloc(para->veclen, sizeof(float));

  /* get mean */
  for(i = 0; i < para->veclen; i++){
    mfcc_mean[i] = 0.0;
    for(t = 0; t < frame_num; t++)
      mfcc_mean[i] += mfcc[t][i];
    mfcc_mean[i] /= (float)frame_num;
  }
  if (para->cvn) {
    /* get standard deviation */
    for(i = 0; i < para->veclen; i++){
      mfcc_sd[i] = 0.0;
      for(t = 0; t < frame_num; t++) {
	x = mfcc[t][i] - mfcc_mean[i];
	mfcc_sd[i] += x * x;
      }
      mfcc_sd[i] = sqrt(mfcc_sd[i] / (float)frame_num);
    }
  }
  for(t = 0; t < frame_num; t++){
    if (para->cmn) {
      /* mean normalization (base MFCC only) */
      for(i = 0; i < basedim; i++) mfcc[t][i] -= mfcc_mean[i];
    }
    if (para->cvn) {
      /* variance normalization (full MFCC) */
      for(i = 0; i < para->veclen; i++) mfcc[t][i] /= mfcc_sd[i];
    }
  }

  if (para->cvn) free(mfcc_sd);
  free(mfcc_mean);
}
