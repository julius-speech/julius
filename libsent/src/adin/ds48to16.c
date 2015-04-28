/**
 * @file   ds48to16.c
 *
 * <JA>
 * @brief  48kHz -> 16kHz ダウンサンプリング
 *
 * </JA>
 * <EN>
 * @brief  Down sampling from 48kHz to 16kHz
 *
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Sun Feb 13 16:18:26 2005
 *
 * $Revision: 1.5 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/adin.h>

/// TRUE if use embedded values on header
#define USE_HEADER_COEF

/// USB device sampling rate

#ifdef USE_HEADER_COEF
/// filter parameters in header
#include "lpfcoef_3to4.h"
#include "lpfcoef_2to1.h"
#endif

/* work area for down sampling */
#define	mod(x)	((x) & (DS_RBSIZE -1)) ///< Buffer index cycler


#ifdef USE_HEADER_COEF
/** 
 * Set 1/2 filter coefficients from header values.
 * 
 * @param f [out] filter info
 */
static void
load_filter_from_header_2to1(DS_FILTER *f)
{
  int i;
 
  /* read the filter coefficients from header file */
  for(i=0;i<DS_RBSIZE + 1; i++) {
    if (i >= lpfcoef_2to1_num) break;
    f->hdn[i] = lpfcoef_2to1[i];
  }
  f->hdn_len = i - 1;
}

/** 
 * Set 4/2 filter coefficients from header values.
 * 
 * @param f [out] filter info
 */
static void
load_filter_from_header_3to4(DS_FILTER *f)
{
  int i;
 
  /* read the filter coefficients from header file */
  for(i=0;i<DS_RBSIZE + 1; i++) {
    if (i >= lpfcoef_3to4_num) break;
    f->hdn[i] = lpfcoef_3to4[i];
  }
  f->hdn_len = i - 1;
}

#else  /* ~USE_HEADER_COEF */

/** 
 * Read filter coefficients from file.
 * 
 * @param f [out] filter info
 * @param coeffile [in] filename
 */
static boolean
load_filter(DS_FILTER *f, char *coeffile)
{
  FILE *fp;
  static char buf[512];
  int i;
 
  /* read the filter coefficients */
  if ((fp = fopen(coeffile, "r")) == NULL) {
    jlog("Error: ds48to16: failed to open filter coefficient file \"%s\"\n", coeffile);
    return FALSE;
  }
  for(i=0;i<DS_RBSIZE + 1; i++) {
    if (fgets(buf, 512, fp) == NULL) break;
    f->hdn[i] = atof(buf);
  }
  fclose(fp);
  if (i <= 0) {
    jlog("Error: ds48to16: failed to read filter coefficient from \"%s\"\n", coeffile);
    return FALSE;
  }
  f->hdn_len = i - 1;

  return TRUE;
}

#endif

/** 
 * Initialize filter values
 * 
 * @param f [i/o] filter info
 * @param u [in] up sampling rate
 * @param d [in] down sampling rate
 */
static void
init_filter(DS_FILTER *f, int d, int u)
{
 
  f->decrate = d;
  f->intrate = u;

  /* set filter starting point */
  f->delay = f->hdn_len / (2 * f->decrate);

  /* reset index */
  f->indx = 0;

  /* reset pointer */
  f->bp = 0;

  /* reset output counter */
  f->count = 1;
}

/** 
 * Store input for FIR filter
 * 
 * @param f [i/o] filter info
 * @param in [in] an input sample
 * </EN>
 */
static void
firin(DS_FILTER *f, double in)
{
  f->indx = mod(f->indx - 1);
  f->rb[f->indx] = in;
}

/** 
 * Get filtered output from FIR filter
 * 
 * @param f [i/o] filter info
 * @param os [in] point
 * 
 * @return output value
 */
static double
firout(DS_FILTER *f, int os)
{
  double out;
  int k, l;
  
  out = 0.0;
  for(k = os, l = f->indx ; k <= f->hdn_len; k += f->intrate, l = mod(l + 1)) {
    out += f->rb[l] * f->hdn[k];
  }
  return(out);
}

/** 
 * Perform down sampling of input samples.
 * 
 * @param f [i/o] filter info
 * @param dst [out] store the resulting samples
 * @param src [in] input samples
 * @param len [in] number of input samples
 * @param maxlen [in] maximum length of dst
 * 
 * @return the number of samples written to dst, or -1 on errror.
 * </EN>
 */
static int
do_filter(DS_FILTER *f, double *dst, double *src, int len, int maxlen)
{
  int dstlen;
  int s;
  int d;
  int i, k;

  s = 0;
  dstlen = 0;

  while(1) {
    /* fulfill temporal buffer */
    /* at this point, x[0..bp-1] may contain the left samples of last call */
    while (f->bp < DS_BUFSIZE) {
      if (s >= len) break;
      f->x[f->bp++] = src[s++];
    }
    if (f->bp < DS_BUFSIZE) {	
      /* when reached last of sample, leave the rest in x[] and exit */
      break;
    }
    /* do conversion from x[0..bp-1] to y[] */
    d = 0;
    for(k=0;k<f->bp;k++) {
      firin(f, f->x[k]);
      for(i=0;i<f->intrate;i++) {
	f->count--;
	if(f->count == 0) {
	  f->y[d++] = firout(f, i);
	  f->count = f->decrate;
	}
      }
    }
    /* store the result to dst[] */
    if(f->delay) {
      if(d > f->delay) {
	/* input samples > delay, store the overed samples and enter no-delay state */
	d -= f->delay;
	for(i=0;i<d;i++) {
	  if (dstlen >= maxlen) break;
	  dst[dstlen++] = f->y[f->delay + i];
	}
	f->delay = 0;
	if (dstlen >= maxlen) {
	  jlog("Error: ds48to16: buffer overflow in down sampling, inputs may be lost!\n");
	  return -1;
	}
      } else {
	/* input samples < delay, decrease delay and wait */
	f->delay -= d;
      }
    } else {
      /* no-delay state: store immediately */
      for(i=0;i<d;i++) {
	if (dstlen >= maxlen) break;
	dst[dstlen++] = f->y[i];
      }
      if (dstlen >= maxlen) {
	jlog("Error: ds48to16: buffer overflow in down sampling, inputs may be lost!\n");
	return -1;
      }
    }

    /* reset pointer */
    f->bp -= DS_BUFSIZE;
  }

  return dstlen;
}


/** 
 * Setup for down sampling
 * 
 * @return newly allocated buffer for down sampling
 */
DS_BUFFER *
ds48to16_new()
{
  DS_BUFFER *ds;
  int i;
  /* define 3 filters: 
     48kHz --f1(3/4)-> 64kHz --f2(1/2)-> 32kHz --f3(1/2)-> 16kHz */
  
  ds = (DS_BUFFER *)mymalloc(sizeof(DS_BUFFER));
  for(i=0;i<3;i++) ds->fir[i] = (DS_FILTER *)mymalloc(sizeof(DS_FILTER));
#ifdef USE_HEADER_COEF
  /* set from embedded header */
  load_filter_from_header_3to4(ds->fir[0]);
  load_filter_from_header_2to1(ds->fir[1]);
  load_filter_from_header_2to1(ds->fir[2]);
  jlog("Stat: ds48to16: loaded FIR filters for down sampling\n");
#else
  /* read from file */
  if (load_filter(ds->fir[0], "lpfcoef.3to4") == FALSE) return FALSE;
  if (load_filter(ds->fir[1], "lpfcoef.2to1") == FALSE) return FALSE;
  if (load_filter(ds->fir[2], "lpfcoef.2to1") == FALSE) return FALSE;
  jlog("Stat: ds48to16: initialize FIR filters for down sampling\n");
#endif
  init_filter(ds->fir[0], 3, 4);
  init_filter(ds->fir[1], 2, 1);
  init_filter(ds->fir[2], 2, 1);

  ds->buflen = 0;

  return(ds);
}

/** 
 * Free the down sampling buffer.
 * 
 * @param ds [i/o] down sampling buffer to free
 * 
 */
void
ds48to16_free(DS_BUFFER *ds)
{
  int i;

  if (ds->buflen != 0) {
    for(i=0;i<4;i++) free(ds->buf[i]);
  }
  for(i=0;i<3;i++) free(ds->fir[i]);
  
  free(ds);
}

/** 
 * Perform down sampling of input samples to 1/3.
 * 
 * @param dst [out] store the resulting samples
 * @param src [in] input samples
 * @param srclen [in] number of input samples
 * @param maxdstlen [in] maximum length of dst
 * @param ds [i/o] down sampling buffer
 * 
 * @return the number of samples written to dst, or -1 on errror.
 * </EN>
 */
int
ds48to16(SP16 *dst, SP16 *src, int srclen, int maxdstlen, DS_BUFFER *ds)
{
  int i, n, tmplen;

  if (ds->buflen == 0) {
    ds->buflen = srclen * 2;		/* longer buffer required for 3/4 upsamling */
    for(n=0;n<4;n++) {
      ds->buf[n]  = (double *)mymalloc(sizeof(double) * ds->buflen);
    }
  } else if (ds->buflen < srclen * 2) {
    ds->buflen = srclen * 2;
    for(n=0;n<4;n++) {
      ds->buf[n]  = (double *)myrealloc(ds->buf[n], sizeof(double) * ds->buflen);
    }
  }

  for(i=0;i<srclen;i++) ds->buf[0][i] = src[i];

  tmplen = srclen;
  for(n=0;n<3;n++) {
    tmplen = do_filter(ds->fir[n], ds->buf[n+1], ds->buf[n], tmplen, ds->buflen);
  }

  if (maxdstlen < tmplen) {
    jlog("Error: ds48to16: down sampled num > required!\n");
    return -1;
  }
  for(i=0;i<tmplen;i++) dst[i] = ds->buf[3][i];

  return(tmplen);
}
