/**
 * @file   ss.c
 * 
 * <JA>
 * @brief  スペクトル減算
 *
 * 実際のスペクトル減算は wav2mfcc-buffer.c および wav2mfcc-pipe.c で
 * 行われます．ここでは平均スペクトルの推定とファイルI/Oのみ定義されています．
 * </JA>
 * 
 * <EN>
 * @brief  Spectral subtraction
 *
 * The actual subtraction will be performed in wav2mfcc-buffer.c and
 * wav2mfcc-pipe.c.  These functions are for estimating average spectrum
 * of audio input, and file I/O for that.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 17:19:54 2005
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

#include <sent/stddefs.h>
#include <sent/mfcc.h>


/** 
 * Binary read function with byte swaping (assume file is BIG ENDIAN)
 * 
 * @param buf [out] read data
 * @param unitbyte [in] size of a unit in bytes
 * @param unitnum [in] number of unit to be read
 * @param fp [in] file pointer
 */
static boolean
myread(void *buf, size_t unitbyte, int unitnum, FILE *fp)
{
  size_t tmp;
  if ((tmp = myfread(buf, unitbyte, unitnum, fp)) < (size_t)unitnum) {
    return(FALSE);
  }
#ifndef WORDS_BIGENDIAN
  swap_bytes(buf, unitbyte, unitnum);
#endif
  return(TRUE);
}

/** 
 * Load a noise spectrum from file.
 * 
 * @param filename [in] path name of noise spectrum file
 * @param slen [out] length of the returned buffer
 * 
 * @return a newly allocated buffer that holds the loaded noise spectrum.
 */
float *
new_SS_load_from_file(char *filename, int *slen)
{
  FILE *fp;
  int num;
  float *sbuf;

  /* open file */
  jlog("Stat: ss: reading Noise Spectrum for SS\n");
  if ((fp = fopen_readfile(filename)) == NULL) {
    jlog("Error: ss: failed to open \"%s\"\n", filename);
    return(NULL);
  }
  /* read length */
  if (myread(&num, sizeof(int), 1, fp) == FALSE) {
    jlog("Error: ss: failed to read \"%s\"\n", filename);
    return(NULL);
  }
  /* allocate */
  sbuf = (float *)mymalloc(sizeof(float) * num);
  /* read data */
  if (myread(sbuf, sizeof(float), num, fp) == FALSE) {
    jlog("Error: ss: failed to read \"%s\"\n", filename);
    return(NULL);
  }
  /* close file */
  fclose_readfile(fp);

  *slen = num;
  jlog("Stat: ss: done\n");
  return(sbuf);
}

/** 
 * Compute average spectrum of audio input.
 * This is used to estimate a noise spectrum from input samples.
 * 
 * @param wave [in] input audio data sequence
 * @param wavelen [in] length of above
 * @param slen [out] length of returned buffer
 * @param w [i/o] MFCC calculation work area
 * @param para [in] parameter
 * 
 * @return a newly allocated buffer that contains the calculated spectrum.
 */
float *
new_SS_calculate(SP16 *wave, int wavelen, int *slen, MFCCWork *w, Value *para)
{
  float *spec;
  int t, framenum, start, end, k, i;
  double x, y;
  
  /* allocate work area */
  spec = (float *)mymalloc((w->fb.fftN + 1) * sizeof(float));
  for(i=0;i<w->fb.fftN;i++) spec[i] = 0.0;
  
  /* Caluculate sum of noise power spectrum */
  framenum = (int)((wavelen - para->framesize) / para->frameshift) + 1;
  if (framenum < 1) {
    jlog("Error: too short to get noise spectrum: length < 1 frame\n");
    jlog("Error: no SS will be performed\n");
    *slen = w->fb.fftN;
    return spec;
  }
    
  start = 1;
  end = 0;
  for (t = 0; t < framenum; t++) {
    if (end != 0) start = end - (para->framesize - para->frameshift) - 1;
    k = 1;
    for (i = start; i <= start + para->framesize; i++) {
      w->bf[k] = (float)wave[i-1];
      k++;
    }
    end = i;

    if (para->zmeanframe) {
      ZMeanFrame(w->bf, para->framesize);
    }

    /* Pre-emphasis */
    PreEmphasise(w->bf, para->framesize, para->preEmph);
    /* Hamming Window */
    Hamming(w->bf, para->framesize, w);
    /* FFT Spectrum */
    for (i = 1; i <= para->framesize; i++) {
      w->fb.Re[i-1] = w->bf[i]; w->fb.Im[i-1] = 0.0;
    }
    for (i = para->framesize + 1; i <= w->fb.fftN; i++) {
      w->fb.Re[i-1] = 0.0;   w->fb.Im[i-1] = 0.0;
    }
    FFT(w->fb.Re, w->fb.Im, w->fb.n, w);
    /* Sum noise spectrum */
    for(i = 1; i <= w->fb.fftN; i++){
      x = w->fb.Re[i - 1];  y = w->fb.Im[i - 1];
      spec[i - 1] += sqrt(x * x + y * y);
    }
  }

  /* Calculate average noise spectrum */
  for(t=0;t<w->fb.fftN;t++) {
    spec[t] /= (float)framenum;
  }

  /* return the new spec[] */
  *slen = w->fb.fftN;
  return(spec);
}
