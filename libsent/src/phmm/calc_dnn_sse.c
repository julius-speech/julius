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

#ifdef HAS_SIMD_SSE
#include <immintrin.h>
#endif

void
calc_dnn_sse(float *dst, float *src, float *w, float *b, int out, int in, float *fstore)
{
#ifdef HAS_SIMD_SSE

  float *s;
  int i, j;

  int n = in / 4;
  for (i = 0; i < out; i++) {
    __m128 x = _mm_setzero_ps();
    s = src;
    for (j = 0; j < n; j++) {
      __m128 v1 = _mm_load_ps(w);
      __m128 v2 = _mm_load_ps(s);
      v2 = _mm_mul_ps(v1, v2);
      x = _mm_add_ps(x, v2);
      w += 4;
      s += 4;
    }
    _mm_store_ps(fstore, x);
    *(dst++) = fstore[0] + fstore[1] + fstore[2] + fstore[3] + *(b++);
  }

#endif	/* HAS_SIMD_SSE */
}
