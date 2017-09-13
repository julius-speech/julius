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
#ifdef _OPENMP
#include <omp.h>
#endif /* _OPENMP */
#endif

void
calc_dnn_fma(float *dst, float *src, float *ww, float *b, int out, int in, float *fstore)
{
#ifdef HAS_SIMD_FMA

  int n = in / 8;

#ifdef _OPENMP
  int thread_num = omp_get_max_threads();
  int num = out / thread_num;
  num = ((num + 3) / 4) * 4;
#endif
  
#ifdef _OPENMP
#pragma omp parallel
  {
#endif /* _OPENMP */
    float *s;
    int i, j;

#ifdef _OPENMP
    int thread_id = omp_get_thread_num();
    int begin = num * thread_id;
    int end = begin + num;
    if (end > out) end = out;
    float *w = ww + begin * in;
    float *thread_dst = dst + begin;
    float *thread_b = b + begin;
    float *thread_fstore = fstore + 8 * thread_id;
#else /* ~_OPENMP */
    int begin = 0;
    int end = out;
    float *w = ww;
    float *thread_dst = dst;
    float *thread_b = b;
    float *thread_fstore = fstore;
#endif /* _OPENMP */
    
    for (i = begin; i + 3 < end; i += 4) {
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
      _mm256_store_ps(thread_fstore, x1);
      *(thread_dst++) = thread_fstore[0] + thread_fstore[1] + thread_fstore[2] + thread_fstore[3] + thread_fstore[4] + thread_fstore[5] + thread_fstore[6] + thread_fstore[7] + *(thread_b++);
      _mm256_store_ps(thread_fstore, x2);
      *(thread_dst++) = thread_fstore[0] + thread_fstore[1] + thread_fstore[2] + thread_fstore[3] + thread_fstore[4] + thread_fstore[5] + thread_fstore[6] + thread_fstore[7] + *(thread_b++);
      _mm256_store_ps(thread_fstore, x3);
      *(thread_dst++) = thread_fstore[0] + thread_fstore[1] + thread_fstore[2] + thread_fstore[3] + thread_fstore[4] + thread_fstore[5] + thread_fstore[6] + thread_fstore[7] + *(thread_b++);
      _mm256_store_ps(thread_fstore, x4);
      *(thread_dst++) = thread_fstore[0] + thread_fstore[1] + thread_fstore[2] + thread_fstore[3] + thread_fstore[4] + thread_fstore[5] + thread_fstore[6] + thread_fstore[7] + *(thread_b++);
      w = w4;
    }

    /* process last <4 nodes */
    for (; i < end; i++) {
      __m256 x = _mm256_setzero_ps();
      s = src;
      for (j = 0; j < n; j++) {
	__m256 vs = _mm256_load_ps(s);
	__m256 v = _mm256_load_ps(w);
	x = _mm256_fmadd_ps(vs, v, x);
	s  += 8;
	w  += 8;
      }
      _mm256_store_ps(thread_fstore, x);
      *(thread_dst++) = thread_fstore[0] + thread_fstore[1] + thread_fstore[2] + thread_fstore[3] + thread_fstore[4] + thread_fstore[5] + thread_fstore[6] + thread_fstore[7] + *(thread_b++);
    }

#ifdef _OPENMP
  }
#endif /* _OPENMP */
  
#endif	/* HAS_SIMD_FMA */
}
