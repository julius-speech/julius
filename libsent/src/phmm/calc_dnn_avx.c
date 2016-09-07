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
  for (i = 0; i < out; i++) {
    __m256 x = _mm256_setzero_ps();
    s = src;
    for (j = 0; j < n; j++) {
      __m256 v1 = _mm256_load_ps(w);
      __m256 v2 = _mm256_load_ps(s);
      v2 = _mm256_mul_ps(v1, v2);
      x = _mm256_add_ps(x, v2);
      w += 8;
      s += 8;
    }
    _mm256_store_ps(fstore, x);
    *(dst++) = fstore[0] + fstore[1] + fstore[2] + fstore[3] + fstore[4] + fstore[5] + fstore[6] + fstore[7] + *(b++);
  }

#endif	/* HAS_SIMD_AVX */
}
