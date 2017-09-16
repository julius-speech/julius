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

#ifdef HAS_SIMD_AVX
#include <immintrin.h>
#endif

void
calc_dnn_avx(float *dst, float *src, float *w, float *b, int out, int in, float *fstore)
{
#ifdef HAS_SIMD_AVX

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
      __m256 v1 = _mm256_load_ps(w);
      __m256 v2 = _mm256_load_ps(w2);
      __m256 v3 = _mm256_load_ps(w3);
      __m256 v4 = _mm256_load_ps(w4);
      v1 = _mm256_mul_ps(vs, v1);
      v2 = _mm256_mul_ps(vs, v2);
      v3 = _mm256_mul_ps(vs, v3);
      v4 = _mm256_mul_ps(vs, v4);
      x1 = _mm256_add_ps(x1, v1);
      x2 = _mm256_add_ps(x2, v2);
      x3 = _mm256_add_ps(x3, v3);
      x4 = _mm256_add_ps(x4, v4);
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
      __m256 vs = _mm256_load_ps(w);
      __m256 v1 = _mm256_load_ps(s);
      v1 = _mm256_mul_ps(vs, v1);
      x = _mm256_add_ps(x, v1);
      w += 8;
      s += 8;
    }
    _mm256_store_ps(fstore, x);
    *(dst++) = fstore[0] + fstore[1] + fstore[2] + fstore[3] + fstore[4] + fstore[5] + fstore[6] + fstore[7] + *(b++);
  }

#endif	/* HAS_SIMD_AVX */
}
