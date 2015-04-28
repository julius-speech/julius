/**
 * @file   mfcc-core.c
 * 
 * <JA>
 * @brief  MFCC 特徴量の計算
 *
 * ここでは，窓をかけて取り出された音声波形データから MFCC 特徴量を
 * 算出するコア関数が納められています．
 * </JA>
 * 
 * <EN>
 * @brief  Compute MFCC parameter vectors
 *
 * These are core functions to compute MFCC vectors from windowed speech data.
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Mon Aug  7 11:55:45 2006
 *
 * $Revision: 1.9 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/mfcc.h>

#ifdef MFCC_SINCOS_TABLE

/** 
 * Generate table for hamming window.
 * 
 * @param w [i/o] MFCC calculation work area
 * @param framesize [in] window size
 */
static void
make_costbl_hamming(MFCCWork *w, int framesize)
{
  int i;
  float a;

  w->costbl_hamming = (double *)mymalloc(sizeof(double) * framesize);
  a = 2.0 * PI / (framesize - 1);
  for(i=1;i<=framesize;i++) {
    /*costbl_hamming[i-1] = 0.54 - 0.46 * cos(2 * PI * (i - 1) / (float)(framesize - 1));*/
    w->costbl_hamming[i-1] = 0.54 - 0.46 * cos(a * (i - 1));
  }
  w->costbl_hamming_len = framesize;
#ifdef MFCC_TABLE_DEBUG
  jlog("Stat: mfcc-core: generated Hamming cos table (%d bytes)\n",
       w->costbl_hamming_len * sizeof(double));
#endif
}

/** 
 * Build tables for FFT.
 * 
 * @param w [i/o] MFCC calculation work area
 * @param n [in] 2^n = FFT point
 */
static void
make_fft_table(MFCCWork *w, int n)
{
  int m;
  int me, me1;
  
  w->costbl_fft = (double *)mymalloc(sizeof(double) * n);
  w->sintbl_fft = (double *)mymalloc(sizeof(double) * n);
  for (m = 1; m <= n; m++) {
    me = 1 << m;
    me1 = me / 2;
    w->costbl_fft[m-1] =  cos(PI / me1);
    w->sintbl_fft[m-1] = -sin(PI / me1);
  }
  w->tbllen = n;
#ifdef MFCC_TABLE_DEBUG
  jlog("Stat: mfcc-core: generated FFT sin/cos table (%d bytes)\n", w->tbllen * sizeof(double));
#endif
}

/** 
 * Generate table for DCT operation to make mfcc from fbank.
 * 
 * @param w [i/o] MFCC calculation work area
 * @param fbank_num [in] number of filer banks
 * @param mfcc_dim [in] number of dimensions in MFCC
 */
static void
make_costbl_makemfcc(MFCCWork *w, int fbank_num, int mfcc_dim)
{
  int size;
  int i, j, k;
  float B, C;

  size = fbank_num * mfcc_dim;
  w->costbl_makemfcc = (double *)mymalloc(sizeof(double) * size);

  B = PI / fbank_num;
  k = 0;
  for(i=1;i<=mfcc_dim;i++) {
    C = i * B;
    for(j=1;j<=fbank_num;j++) {
      w->costbl_makemfcc[k] = cos(C * (j - 0.5));
      k++;
    }
  }
  w->costbl_makemfcc_len = size;
#ifdef MFCC_TABLE_DEBUG
  jlog("Stat: mfcc-core: generated MakeMFCC cos table (%d bytes)\n",
       w->costbl_makemfcc_len * sizeof(double));
#endif
}

/** 
 * Generate table for weighing cepstrum.
 * 
 * @param w [i/o] MFCC calculation work area
 * @param lifter [in] cepstral liftering coefficient
 * @param mfcc_dim [in] number of dimensions in MFCC
 */
static void
make_sintbl_wcep(MFCCWork *w, int lifter, int mfcc_dim)
{
  int i;
  float a, b;

  w->sintbl_wcep = (double *)mymalloc(sizeof(double) * mfcc_dim);
  if (lifter > 0) {
    a = PI / lifter;
    b = lifter / 2.0;
    for(i=0;i<mfcc_dim;i++) {
      w->sintbl_wcep[i] = 1.0 + b * sin((i+1) * a);
    }
  } else {
    for(i=0;i<mfcc_dim;i++) {
      w->sintbl_wcep[i] = 1.0;
    }
  }
  w->sintbl_wcep_len = mfcc_dim;
#ifdef MFCC_TABLE_DEBUG
  jlog("Stat: mfcc-core: generated WeightCepstrum sin table (%d bytes)\n",
       w->sintbl_wcep_len * sizeof(double));
#endif
}

#endif /* MFCC_SINCOS_TABLE */

/** 
 * Return mel-frequency.
 * 
 * @param k [in] channel number of filter bank
 * @param fres [in] constant value computed by "1.0E7 / (para.smp_period * fb.fftN * 700.0)"
 * 
 * @return the mel frequency.
 */
float Mel(int k, float fres)
{
  return(1127 * log(1 + (k-1) * fres));
}

/**
 * Create fbank center frequency for VTLN.
 *
 * @param cf [i/o] center frequency of channels in Mel, will be changed considering VTLN
 * @param para [in] analysis parameter
 * @param mlo [in] fbank lower bound in Mel
 * @param mhi [in] fbank upper bound in Mel
 * @param maxChan [in] maximum number of channels
 * 
 */
static boolean
VTLN_recreate_fbank_cf(float *cf, Value *para, float mlo, float mhi, int maxChan)
{
  int chan;
  float minf, maxf, cf_orig, cf_new;
  float scale, cu, cl, au, al;

  /* restore frequency range to non-Mel */
  minf = 700.0 * (exp(mlo / 1127.0) - 1.0);
  maxf = 700.0 * (exp(mhi / 1127.0) - 1.0);

  if (para->vtln_upper > maxf) {
    jlog("Error: VTLN upper cut-off greater than upper frequency bound: %.1f > %.1f\n", para->vtln_upper, maxf);
    return FALSE;
  }
  if (para->vtln_lower < minf) {
    jlog("Error: VTLN lower cut-off smaller than lower frequency bound: %.1f < %.1f\n", para->vtln_lower, minf);
    return FALSE;
  }
  
  /* prepare variables for warping */
  scale = 1.0 / para->vtln_alpha;
  cu = para->vtln_upper * 2 / ( 1 + scale);
  cl = para->vtln_lower * 2 / ( 1 + scale);
  au = (maxf - cu * scale) / (maxf - cu);
  al = (cl * scale - minf) / (cl - minf);
  
  for (chan = 1; chan <= maxChan; chan++) {
    /* get center frequency, restore to non-Mel */
    cf_orig = 700.0 * (exp(cf[chan] / 1127.0) - 1.0);
    /* do warping */
    if( cf_orig > cu ){
      cf_new = au * (cf_orig - cu) + scale * cu;
    } else if ( cf_orig < cl){
      cf_new = al * (cf_orig - minf) + minf;
    } else {
      cf_new = scale * cf_orig;
    }
    /* convert the new center frequency to Mel and store */
    cf[chan] = 1127.0 * log (1.0 + cf_new / 700.0);
  }
  return TRUE;
}

/** 
 * Build filterbank information and generate tables for MFCC comptutation.
 * 
 * @param w [i/o] MFCC calculation work area
 * @param para [in] configuration parameters
 * 
 * @return the generated filterbank information. 
 */
boolean
InitFBank(MFCCWork *w, Value *para)
{
  float mlo, mhi, ms, melk;
  int k, chan, maxChan, nv2;

  /* Calculate FFT size */
  w->fb.fftN = 2;  w->fb.n = 1;
  while(para->framesize > w->fb.fftN){
    w->fb.fftN *= 2; w->fb.n++;
  }

  nv2 = w->fb.fftN / 2;
  w->fb.fres = 1.0E7 / (para->smp_period * w->fb.fftN * 700.0);
  maxChan = para->fbank_num + 1;
  w->fb.klo = 2;   w->fb.khi = nv2;
  mlo = 0;      mhi = Mel(nv2 + 1, w->fb.fres);

  /* lo pass filter */
  if (para->lopass >= 0) {
    mlo = 1127*log(1+(float)para->lopass/700.0);
    w->fb.klo = ((float)para->lopass * para->smp_period * 1.0e-7 * w->fb.fftN) + 2.5;
    if (w->fb.klo<2) w->fb.klo = 2;
  }
  /* hi pass filter */
  if (para->hipass >= 0) {
    mhi = 1127*log(1+(float)para->hipass/700.0);
    w->fb.khi = ((float)para->hipass * para->smp_period * 1.0e-7 * w->fb.fftN) + 0.5;
    if (w->fb.khi>nv2) w->fb.khi = nv2;
  }

  /* Create vector of fbank centre frequencies */
  w->fb.cf = (float *)mymalloc((maxChan + 1) * sizeof(float));
  ms = mhi - mlo;
  for (chan = 1; chan <= maxChan; chan++) 
    w->fb.cf[chan] = ((float)chan / maxChan)*ms + mlo;

  if (para->vtln_alpha != 1.0) {
    /* Modify fbank center frequencies for VTLN */
    if (VTLN_recreate_fbank_cf(w->fb.cf, para, mlo, mhi, maxChan) == FALSE) {
      return FALSE;
    }
  }

  /* Create loChan map, loChan[fftindex] -> lower channel index */
  w->fb.loChan = (short *)mymalloc((nv2 + 1) * sizeof(short));
  for(k = 1, chan = 1; k <= nv2; k++){
    if (k < w->fb.klo || k > w->fb.khi) w->fb.loChan[k] = -1;
    else {
      melk = Mel(k, w->fb.fres);
      while (w->fb.cf[chan] < melk && chan <= maxChan) ++chan;
      w->fb.loChan[k] = chan - 1;
    }
  }

  /* Create vector of lower channel weights */   
  w->fb.loWt = (float *)mymalloc((nv2 + 1) * sizeof(float));
  for(k = 1; k <= nv2; k++) {
    chan = w->fb.loChan[k];
    if (k < w->fb.klo || k > w->fb.khi) w->fb.loWt[k] = 0.0;
    else {
      if (chan > 0) 
	w->fb.loWt[k] = (w->fb.cf[chan + 1] - Mel(k, w->fb.fres)) / (w->fb.cf[chan + 1] - w->fb.cf[chan]);
      else
	w->fb.loWt[k] = (w->fb.cf[1] - Mel(k, w->fb.fres)) / (w->fb.cf[1] - mlo);
    }
  }
  
  /* Create workspace for fft */
  w->fb.Re = (float *)mymalloc((w->fb.fftN + 1) * sizeof(float));
  w->fb.Im = (float *)mymalloc((w->fb.fftN + 1) * sizeof(float));

  w->sqrt2var = sqrt(2.0 / para->fbank_num);

  return TRUE;
}

/** 
 * Free FBankInfo.
 * 
 * @param fb [in] filterbank information
 */
void
FreeFBank(FBankInfo *fb)
{
  free(fb->cf);
  free(fb->loChan);
  free(fb->loWt);
  free(fb->Re);
  free(fb->Im);
}

/** 
 * Remove DC offset per frame
 * 
 * @param wave [i/o] waveform data in the current frame
 * @param framesize [in] frame size
 * 
 */
void
ZMeanFrame(float *wave, int framesize)
{		   
  int i;
  float mean;

  mean = 0.0;
  for(i = 1; i <= framesize; i++) mean += wave[i];
  mean /= framesize;
  for(i = 1; i <= framesize; i++) wave[i] -= mean;
}

/** 
 * Calculate Log Raw Energy.
 * 
 * @param wave [in] waveform data in the current frame
 * @param framesize [in] frame size
 * 
 * @return the calculated log raw energy.
 */
float CalcLogRawE(float *wave, int framesize)
{		   
  int i;
  double raw_E = 0.0;
  float energy;

  for(i = 1; i <= framesize; i++)
    raw_E += wave[i] * wave[i];
  energy = (float)log(raw_E);

  return(energy);
}

/** 
 * Apply pre-emphasis filter.
 * 
 * @param wave [i/o] waveform data in the current frame
 * @param framesize [i/o] frame size in samples
 * @param preEmph [in] pre-emphasis coef.
 */
void PreEmphasise (float *wave, int framesize, float preEmph)
{
  int i;
   
  for(i = framesize; i >= 2; i--)
    wave[i] -= wave[i - 1] * preEmph;
  wave[1] *= 1.0 - preEmph;  
}

/** 
 * Apply hamming window.
 * 
 * @param wave [i/o] waveform data in the current frame
 * @param framesize [in] frame size
 * @param w [i/o] MFCC calculation work area
 */
void Hamming(float *wave, int framesize, MFCCWork *w)
{
  int i;
#ifdef MFCC_SINCOS_TABLE
  for(i = 1; i <= framesize; i++)
    wave[i] *= w->costbl_hamming[i-1];
#else
  float a;
  a = 2 * PI / (framesize - 1);
  for(i = 1; i <= framesize; i++)
    wave[i] *= 0.54 - 0.46 * cos(a * (i - 1));
#endif
}

/** 
 * Apply FFT
 * 
 * @param xRe [i/o] real part of waveform
 * @param xIm [i/o] imaginal part of waveform
 * @param p [in] 2^p = FFT point
 * @param w [i/o] MFCC calculation work area
 */
void FFT(float *xRe, float *xIm, int p, MFCCWork *w)
{
  int i, ip, j, k, m, me, me1, n, nv2;
  double uRe, uIm, vRe, vIm, wRe, wIm, tRe, tIm;
  
  n = 1<<p;
  nv2 = n / 2;
  
  j = 0;
  for(i = 0; i < n-1; i++){
    if(j > i){
      tRe = xRe[j];      tIm = xIm[j];
      xRe[j] = xRe[i];   xIm[j] = xIm[i];
      xRe[i] = tRe;      xIm[i] = tIm;
    }
    k = nv2;
    while(j >= k){
      j -= k;      k /= 2;
    }
    j += k;
  }

  for(m = 1; m <= p; m++){
    me = 1<<m;                me1 = me / 2;
    uRe = 1.0;                uIm = 0.0;
#ifdef MFCC_SINCOS_TABLE
    wRe = w->costbl_fft[m-1];    wIm = w->sintbl_fft[m-1];
#else
    wRe = cos(PI / me1);      wIm = -sin(PI / me1);
#endif
    for(j = 0; j < me1; j++){
      for(i = j; i < n; i += me){
	ip = i + me1;
	tRe = xRe[ip] * uRe - xIm[ip] * uIm;
	tIm = xRe[ip] * uIm + xIm[ip] * uRe;
	xRe[ip] = xRe[i] - tRe;   xIm[ip] = xIm[i] - tIm;
	xRe[i] += tRe;            xIm[i] += tIm;
      }
      vRe = uRe * wRe - uIm * wIm;   vIm = uRe * wIm + uIm * wRe;
      uRe = vRe;                     uIm = vIm;
    }
  }
}


/** 
 * Convert wave -> (spectral subtraction) -> mel-frequency filterbank
 * 
 * @param wave [in] waveform data in the current frame
 * @param w [i/o] MFCC calculation work area
 * @param para [in] configuration parameters
 */
void
MakeFBank(float *wave, MFCCWork *w, Value *para)
{
  int k, bin, i;
  double Re, Im, A, P, NP, H, temp;

  for(k = 1; k <= para->framesize; k++){
    w->fb.Re[k - 1] = wave[k];  w->fb.Im[k - 1] = 0.0;  /* copy to workspace */
  }
  for(k = para->framesize + 1; k <= w->fb.fftN; k++){
    w->fb.Re[k - 1] = 0.0;      w->fb.Im[k - 1] = 0.0;  /* pad with zeroes */
  }
  
  /* Take FFT */
  FFT(w->fb.Re, w->fb.Im, w->fb.n, w);

  if (w->ssbuf != NULL) {
    /* Spectral Subtraction */
    for(k = 1; k <= w->fb.fftN; k++){
      Re = w->fb.Re[k - 1];  Im = w->fb.Im[k - 1];
      P = sqrt(Re * Re + Im * Im);
      NP = w->ssbuf[k - 1];
      if((P * P -  w->ss_alpha * NP * NP) < 0){
	H = w->ss_floor;
      }else{
	H = sqrt(P * P - w->ss_alpha * NP * NP) / P;
      }
      w->fb.Re[k - 1] = H * Re;
      w->fb.Im[k - 1] = H * Im;
    }
  }

  /* Fill filterbank channels */ 
  for(i = 1; i <= para->fbank_num; i++)
    w->fbank[i] = 0.0;
  
  if (para->usepower) {
    for(k = w->fb.klo; k <= w->fb.khi; k++){
      Re = w->fb.Re[k-1]; Im = w->fb.Im[k-1];
      A = Re * Re + Im * Im;
      bin = w->fb.loChan[k];
      Re = w->fb.loWt[k] * A;
      if(bin > 0) w->fbank[bin] += Re;
      if(bin < para->fbank_num) w->fbank[bin + 1] += A - Re;
    }
  } else {
    for(k = w->fb.klo; k <= w->fb.khi; k++){
      Re = w->fb.Re[k-1]; Im = w->fb.Im[k-1];
      A = sqrt(Re * Re + Im * Im);
      bin = w->fb.loChan[k];
      Re = w->fb.loWt[k] * A;
      if(bin > 0) w->fbank[bin] += Re;
      if(bin < para->fbank_num) w->fbank[bin + 1] += A - Re;
    }
  }

  if (w->log_fbank) {
    /* Take logs */
    for(bin = 1; bin <= para->fbank_num; bin++){ 
      temp = w->fbank[bin];
      if(temp < 1.0) temp = 1.0;
      w->fbank[bin] = log(temp);  
    }
  }
}

/** 
 * Calculate 0'th cepstral coefficient.
 * 
 * @param w [i/o] MFCC calculation work area
 * @param para [in] configuration parameters
 * 
 * @return 
 */
float CalcC0(MFCCWork *w, Value *para)
{
  int i; 
  float S;
  
  S = 0.0;
  for(i = 1; i <= para->fbank_num; i++)
    S += w->fbank[i];
  return S * w->sqrt2var;
}

/** 
 * Apply DCT to filterbank to make MFCC.
 * 
 * @param mfcc [out] output MFCC vector
 * @param para [in] configuration parameters
 * @param w [i/o] MFCC calculation work area
 */
void MakeMFCC(float *mfcc, Value *para, MFCCWork *w)
{
#ifdef MFCC_SINCOS_TABLE
  int i, j, k;
  k = 0;
  /* Take DCT */
  for(i = 0; i < para->mfcc_dim; i++){
    mfcc[i] = 0.0;
    for(j = 1; j <= para->fbank_num; j++)
      mfcc[i] += w->fbank[j] * w->costbl_makemfcc[k++];
    mfcc[i] *= w->sqrt2var;
  }
#else
  int i, j;
  float B, C;
  
  B = PI / para->fbank_num;
  /* Take DCT */
  for(i = 1; i <= para->mfcc_dim; i++){
    mfcc[i - 1] = 0.0;
    C = i * B;
    for(j = 1; j <= para->fbank_num; j++)
      mfcc[i - 1] += w->fbank[j] * cos(C * (j - 0.5));
    mfcc[i - 1] *= w->sqrt2var;     
  }
#endif
}

/** 
 * Re-scale cepstral coefficients.
 * 
 * @param mfcc [i/o] a MFCC vector
 * @param para [in] configuration parameters
 * @param w [i/o] MFCC calculation work area
 */
void WeightCepstrum (float *mfcc, Value *para, MFCCWork *w)
{
#ifdef MFCC_SINCOS_TABLE
  int i;
  for(i=0;i<para->mfcc_dim;i++) {
    mfcc[i] *= w->sintbl_wcep[i];
  }
#else
  int i;
  float a, b, *cepWin;
  
  cepWin = (float *)mymalloc(para->mfcc_dim * sizeof(float));
  a = PI / para->lifter;
  b = para->lifter / 2.0;
  
  for(i = 0; i < para->mfcc_dim; i++){
    cepWin[i] = 1.0 + b * sin((i + 1) * a);
    mfcc[i] *= cepWin[i];
  }
  
  free(cepWin);
#endif
}

/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/
/** 
 * Setup work area for parameters, values, buffers, tables to compute
 * MFCC vectors, with a given parameter configurations
 * 
 * @param para [in] configuration parameters
 *
 * @return pointer to the newly allocated work area.
 */
MFCCWork *
WMP_work_new(Value *para)
{
  MFCCWork *w;

  /* newly allocated area should be cleared */
  w = (MFCCWork *)mymalloc(sizeof(MFCCWork));
  memset(w, 0, sizeof(MFCCWork));

  /* set switches by the parameter type */
  switch(para->basetype) {
  case F_MFCC:
    w->fbank_only = FALSE;
    w->log_fbank = TRUE;
    break;
  case F_FBANK:
    w->fbank_only = TRUE;
    w->log_fbank = TRUE;
    break;
  case F_MELSPEC:
    w->fbank_only = TRUE;
    w->log_fbank = FALSE;
    break;
  default:
    jlog("Error: mfcc-core: unsupported parameter type\n");
    free(w);
    return NULL;
  }

  /* set filterbank information */
  if (InitFBank(w, para) == FALSE) return NULL;

#ifdef MFCC_SINCOS_TABLE
  /* prepare tables */
  make_costbl_hamming(w, para->framesize);
  make_fft_table(w, w->fb.n);
  if (para->mfcc_dim >= 0) {
    make_costbl_makemfcc(w, para->fbank_num, para->mfcc_dim);
    make_sintbl_wcep(w, para->lifter, para->mfcc_dim);
  }
#endif

  /* prepare some buffers */
  w->fbank = (double *)mymalloc((para->fbank_num+1)*sizeof(double));
  w->bf = (float *)mymalloc(w->fb.fftN * sizeof(float));
  w->bflen = w->fb.fftN;

  return w;
}

/** 
 * Calculate MFCC and log energy for one frame.  Perform spectral subtraction
 * if @a ssbuf is specified.
 * 
 * @param w [i/o] MFCC calculation work area
 * @param mfcc [out] buffer to hold the resulting MFCC vector
 * @param para [in] configuration parameters
 */
void
WMP_calc(MFCCWork *w, float *mfcc, Value *para)
{
  float energy = 0.0;
  float c0 = 0.0;
  int p;

  if (para->zmeanframe) {
    ZMeanFrame(w->bf, para->framesize);
  }

  if (para->energy && para->raw_e) {
    /* calculate log raw energy */
    energy = CalcLogRawE(w->bf, para->framesize);
  }
  /* pre-emphasize */
  PreEmphasise(w->bf, para->framesize, para->preEmph);
  /* hamming window */
  Hamming(w->bf, para->framesize, w);
  if (para->energy && ! para->raw_e) {
    /* calculate log energy */
    energy = CalcLogRawE(w->bf, para->framesize);
  }
  /* filterbank */
  MakeFBank(w->bf, w, para);

  if (w->fbank_only) {
    /* return the filterbank */
    for (p = 0; p < para->mfcc_dim; p++) {
      mfcc[p] = w->fbank[p+1];
    }
    return;
  }

  /* 0'th cepstral parameter */
  if (para->c0) c0 = CalcC0(w, para);
  /* MFCC */
  MakeMFCC(mfcc, para, w);
  /* weight cepstrum */
  WeightCepstrum(mfcc, para, w);
  /* set energy to mfcc */
  p = para->mfcc_dim;
  if (para->c0) mfcc[p++] = c0;
  if (para->energy) mfcc[p++] = energy;
}

/** 
 * Free all work area for MFCC computation
 * 
 * @param w [i/o] MFCC calculation work area
 */
void
WMP_free(MFCCWork *w)
{
  if (w->fbank) {
    FreeFBank(&(w->fb));
    free(w->fbank);
    free(w->bf);
    w->fbank = NULL;
    w->bf = NULL;
  }
#ifdef MFCC_SINCOS_TABLE
  if (w->costbl_hamming) {
    free(w->costbl_hamming);
    w->costbl_hamming = NULL;
  }
  if (w->costbl_fft) {
    free(w->costbl_fft);
    w->costbl_fft = NULL;
  }
  if (w->sintbl_fft) {
    free(w->sintbl_fft);
    w->sintbl_fft = NULL;
  }
  if (w->costbl_makemfcc) {
    free(w->costbl_makemfcc);
    w->costbl_makemfcc = NULL;
  }
  if (w->sintbl_wcep) {
    free(w->sintbl_wcep);
    w->sintbl_wcep = NULL;
  }
#endif
  free(w);
}

