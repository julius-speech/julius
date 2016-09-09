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

#ifdef HAS_SIMD_NEONV2
#include <arm_neon.h>
#endif

void
calc_dnn_neonv2(float *dst, float *src, float *w, float *b, int out, int in, float *fstore)
{
#ifdef HAS_SIMD_NEONV2

  float *s;
  int i, j;

  int n = in / 4;
  for (i = 0; i < out; i++) {
    float32x4_t x = vdupq_n_f32(0);
    s = src;
    for (j = 0; j < n; j++) {
      float32x4_t v1 = vld1q_f32(w);
      float32x4_t v2 = vld1q_f32(s);
      x = vmlaq_f32(x, v1, v2);
      w += 4;
      s += 4;
    }
    vst1q_f32(fstore, x);
    *(dst++) = fstore[0] + fstore[1] + fstore[2] + fstore[3] + *(b++);
  }

#endif	/* HAS_SIMD_NEONV2 */
}
