/*
 * Copyright (c) 1991-2016 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2016 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/hmm_calc.h>

#ifdef HAS_SIMD_FMA
#include <immintrin.h>
#endif

void
calc_dnn_fma(float *dst, float *src, float *w, float *b, int out, int in, float *fstore)
{
#ifdef HAS_SIMD_FMA

  float *s;
  int i, j;
  int n = in / 8;

  for (i = 0; i + 3 < out; i += 4) {
    float *w2, *w3, *w4;
    __m256 x1 = _mm256_setzero_ps();
    __m256 x2 = _mm256_setzero_ps();
    __m256 x3 = _mm256_setzero_ps();
    __m256 x4 = _mm256_setzero_ps();
    w2 = w + in;
    w3 = w2 + in;
    w4 = w3 + in;
    s = src;
    for (j = 0; j < n; j++) {
      __m256 vs = _mm256_load_ps(s);
      __m256 vw1 = _mm256_load_ps(w);
      __m256 vw2 = _mm256_load_ps(w2);
      __m256 vw3 = _mm256_load_ps(w3);
      __m256 vw4 = _mm256_load_ps(w4);
      x1 = _mm256_fmadd_ps(vs, vw1, x1);
      x2 = _mm256_fmadd_ps(vs, vw2, x2);
      x3 = _mm256_fmadd_ps(vs, vw3, x3);
      x4 = _mm256_fmadd_ps(vs, vw4, x4);
      s  += 8;
      w  += 8;
      w2 += 8;
      w3 += 8;
      w4 += 8;
    }
    _mm256_store_ps(fstore, x1);
    *(dst++) = fstore[0] + fstore[1] + fstore[2] + fstore[3] + fstore[4] + fstore[5] + fstore[6] + fstore[7] + *(b++);
    _mm256_store_ps(fstore, x2);
    *(dst++) = fstore[0] + fstore[1] + fstore[2] + fstore[3] + fstore[4] + fstore[5] + fstore[6] + fstore[7] + *(b++);
    _mm256_store_ps(fstore, x3);
    *(dst++) = fstore[0] + fstore[1] + fstore[2] + fstore[3] + fstore[4] + fstore[5] + fstore[6] + fstore[7] + *(b++);
    _mm256_store_ps(fstore, x4);
    *(dst++) = fstore[0] + fstore[1] + fstore[2] + fstore[3] + fstore[4] + fstore[5] + fstore[6] + fstore[7] + *(b++);
    w = w4;
  }

  /* process last <4 nodes */
  for (; i < out; i++) {
    __m256 x = _mm256_setzero_ps();
    s = src;
    for (j = 0; j < n; j++) {
      __m256 vs = _mm256_load_ps(s);
      __m256 v = _mm256_load_ps(w);
      x = _mm256_fmadd_ps(vs, v, x);
      s  += 8;
      w  += 8;
    }
    _mm256_store_ps(fstore, x);
    *(dst++) = fstore[0] + fstore[1] + fstore[2] + fstore[3] + fstore[4] + fstore[5] + fstore[6] + fstore[7] + *(b++);
  }
  
#endif	/* HAS_SIMD_FMA */
}
