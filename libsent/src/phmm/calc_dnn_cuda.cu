/*

 * Copyright (c) 1991-2020 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2020 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/hmm_calc.h>

#ifdef __NVCC__

#include <cuda_runtime.h>

// from common.h
#include <sys/time.h>

#define CHECK(call)                                                            \
{                                                                              \
    const cudaError_t error = call;                                            \
    if (error != cudaSuccess)                                                  \
    {                                                                          \
        fprintf(stderr, "Error: %s:%d, ", __FILE__, __LINE__);                 \
        fprintf(stderr, "code: %d, reason: %s\n", error,                       \
                cudaGetErrorString(error));                                    \
        exit(1);                                                               \
    }                                                                          \
}

/* define this to test disabling expsum computation at softmax */
#undef NO_SUM_COMPUTATION

/***********************************************************************/
/*
 * global mode: not use shared memory, block size is BLOCK_SIZE
 *
 * shared mode: use [BLOCK_SIZE_X][BLOCK_SIZE_Y] threads
 *
 */

#define BLOCK_SIZE 128    /// Block size in global mode
#define BLOCK_SIZE_X 16   /// Block size X in shared mode
#define BLOCK_SIZE_Y 8    /// Block size Y in shared mode

typedef struct {
  int stride;
  float *elements_src;
  float *elements_dst;
} Matrix;

/***********************************************************************/
/* should be same value as calc_dnn.c */
#define LOGISTIC_TABLE_FACTOR 20000
#define LOGISTIC_TABLE_MAX (16 * LOGISTIC_TABLE_FACTOR)
#define LOGISTIC_MIN 0.000334
#define LOGISTIC_MAX 0.999666

static float *d_logistic;  /* GPU entry point of logistic value table */

/* build logistic function value table on GPU */
void cuda_copy_logistic_table(float *table, int len)
{
  // copy logistic_table to GPU
  CHECK(cudaMalloc((void **)&d_logistic, sizeof(float) * len));
  CHECK(cudaMemcpy(d_logistic, table, sizeof(float) * len, cudaMemcpyHostToDevice));
}

/***********************************************************************/
// allocate GPU memory per DNN layer
void cuda_layer_load(DNNLayer *l)
{
  CHECK(cudaMalloc((void **)&l->dw, sizeof(float) * l->out * l->in));
  CHECK(cudaMemcpy(l->dw, l->w, sizeof(float) * l->out * l->in, cudaMemcpyHostToDevice));
  CHECK(cudaMalloc((void **)&l->db, sizeof(float) * l->out));
  CHECK(cudaMemcpy(l->db, l->b, sizeof(float) * l->out, cudaMemcpyHostToDevice));
}

// free GPU memory per DNN layer
void cuda_layer_free(DNNLayer *l)
{
  if (l->dw != NULL) CHECK(cudaFree(l->dw));
  if (l->db != NULL) CHECK(cudaFree(l->db));
}

// clear GPU part of DNN structure
void cuda_dnn_clear(DNNData *dnn)
{
  int i;

  if (dnn->ddst) {
    for (i = 0; i < dnn->hnum; i++) {
      if (dnn->ddst[i]) {
	CHECK(cudaFree(dnn->ddst[i]));
      }
    }
    free(dnn->ddst);
  }
  if (dnn->dout) CHECK(cudaFree(dnn->dout));
  if (dnn->dinvec) CHECK(cudaFree(dnn->dinvec));
}

// set up
void cuda_dnn_setup(DNNData *dnn)
{
  int i;

  dnn->ddst = (float **)mymalloc(sizeof(float *) * dnn->hnum);
  for (i = 0; i < dnn->hnum; i++) {
    CHECK(cudaMalloc((void **)&(dnn->ddst[i]), sizeof(float) * dnn->hiddennodenum));
  }
  CHECK(cudaMalloc((void **)&dnn->dout, sizeof(float) * dnn->outputnodenum));
  CHECK(cudaMalloc((void **)&dnn->dinvec, sizeof(float) * dnn->inputnodenum));

  if (dnn->use_cuda_shared == FALSE) {
    if (dnn->blocksize1 == 0) {
      dnn->blocksize1 = BLOCK_SIZE;
    }
  } else {
    if (dnn->blocksize1 == 0) {
      dnn->blocksize1 = BLOCK_SIZE_X;
    }
    if (dnn->blocksize2 == 0) {
      dnn->blocksize2 = BLOCK_SIZE_Y;
    }
  }
}

/***********************************************************************/
/* sigmoid computation on GPU*/

__global__ void _cuda_sigmoid(float *dst, float *logistic, int out)
{
  int idx = blockIdx.x * blockDim.x + threadIdx.x;

  if (idx < out) {
    if (dst[idx] <= -8.0f) {
      dst[idx] = LOGISTIC_MIN;
    } else if (dst[idx] >=  8.0f) {
      dst[idx] = LOGISTIC_MAX;
    } else {
      dst[idx] = logistic[(int)((dst[idx] + 8.0f) * LOGISTIC_TABLE_FACTOR + 0.5)];
    }
  }
}

/* calc DNN on GPU (global) */
__global__ void _cuda_calc_dnn(float *src, float *dst, float *w, float *b, int in, int out)
{
  int idx = blockIdx.x * blockDim.x + threadIdx.x;

  if (idx < out) {
    float x = 0.0f;
    for (int k = 0; k < in; ++k) {
      x += src[k] * w[idx * in + k];
    }
    dst[idx] = x + b[idx];
  }
}

/* global version */
static void local_calc_outprob_global(HMMWork *wrk)
{
  DNNData *dnn = wrk->OP_dnn;
  DNNLayer *h;

  // define <grid, block> for layer computation
  dim3 block(dnn->blocksize1);
  dim3 grid((dnn->hiddennodenum + block.x - 1) / block.x);

  // define <grid2, block2> for output layer computation
  dim3 block2(dnn->blocksize1);
  dim3 grid2((dnn->outputnodenum + block2.x - 1) / block2.x);

  // transfer input vectors to GPU
  CHECK(cudaMemcpy(dnn->dinvec,
                   &(wrk->OP_param->parvec[wrk->OP_time][0]),
                   sizeof(float) * dnn->inputnodenum,
                   cudaMemcpyHostToDevice));

  // do calculation on GPU
  h = &(dnn->h[0]);
  _cuda_calc_dnn<<<grid, block>>>(dnn->dinvec, dnn->ddst[0], h->dw, h->db, h->in, h->out);
  _cuda_sigmoid<<<grid, block>>>(dnn->ddst[0], d_logistic, h->out);

  for (int hidx = 1; hidx < dnn->hnum; hidx++) {
    h = &(dnn->h[hidx]);
    _cuda_calc_dnn<<<grid, block>>>(dnn->ddst[hidx-1], dnn->ddst[hidx], h->dw, h->db, h->in, h->out);
    _cuda_sigmoid<<<grid, block>>>(dnn->ddst[hidx], d_logistic, h->out);
  }

  _cuda_calc_dnn<<<grid2, block2>>>(dnn->ddst[dnn->hnum-1], dnn->dout, dnn->o.dw, dnn->o.db, dnn->o.in, dnn->o.out);

  /* transfer result from GPU to cpu */
  CHECK(cudaMemcpy(wrk->last_cache, dnn->dout, sizeof(float) * dnn->outputnodenum, cudaMemcpyDeviceToHost));

}

/***********************************************************************/

/* return first point of partial matrix that corresponds to the block */
__device__ Matrix GetSubMatrix(float *A, int row, int col, int in)
{
  Matrix Asub;

  Asub.elements_src = &A[in * BLOCK_SIZE_Y * row + BLOCK_SIZE_X * col];
  Asub.elements_dst = &A[in * BLOCK_SIZE_X * row + BLOCK_SIZE_Y * col];

  return Asub;
}

/* compute a block */
__global__ void _cuda_calc_dnn_shared(float *src, float *dst, float *w, float *b, int in, int out)
{
  int brow = blockIdx.y; /* should be always 0 since this is 1-dim. grid */
  int bcol = blockIdx.x; /* block ID */

  int trow = threadIdx.y; /* 0 ... BLOCK_SIZE_Y-1 */
  int tcol = threadIdx.x; /* 0 ... BKOCK_SIZE_X-1 */

  /* check if this is my part */
  if (bcol * BLOCK_SIZE_Y + trow < out) {

    /* take partial matrix */
    Matrix dst_sub = GetSubMatrix(dst, brow, bcol, in);

    /* MA loop */
    float x = 0.0f;
    for (int l = 0; l * BLOCK_SIZE_X + tcol < in; ++l) {
      /* get partial matrix of src and W */
      Matrix src_sub = GetSubMatrix(src, brow, l, in);
      Matrix W_sub = GetSubMatrix(w, bcol, l, in);

      /* put them to shared memory */
      __shared__ float srcs[BLOCK_SIZE_X];
      __shared__ float Ws[BLOCK_SIZE_Y][BLOCK_SIZE_X];
      srcs[tcol] = src_sub.elements_src[tcol];
      Ws[trow][tcol] = W_sub.elements_src[trow*in+tcol];
      __syncthreads();

      /* do matrix computation */
      for (int k = 0; k < BLOCK_SIZE_X; ++k) {
	x += srcs[k] * Ws[trow][k];
      }
      __syncthreads();
    }
    /* add bias vector */
    dst_sub.elements_dst[trow] = x + b[bcol * BLOCK_SIZE_Y + trow];
  }
}

/* shared version */
static void local_calc_outprob_shared(HMMWork *wrk)
{
  DNNData *dnn = wrk->OP_dnn;
  DNNLayer *h;

  // define <grid, block> for layer computation
  dim3 block(BLOCK_SIZE_X, BLOCK_SIZE_Y);
  dim3 grid((dnn->hiddennodenum + block.y - 1) / block.y, 1);

  // define <grid2, block2> for output layer computation
  dim3 block2(BLOCK_SIZE_X, BLOCK_SIZE_Y);
  dim3 grid2((dnn->outputnodenum + block2.y - 1) / block2.y, 1);

  // transfer input vectors to GPU
  CHECK(cudaMemcpy(dnn->dinvec,
                   &(wrk->OP_param->parvec[wrk->OP_time][0]),
                   sizeof(float)*dnn->inputnodenum,
                   cudaMemcpyHostToDevice));

  // do calculation on GPU
  h = &(dnn->h[0]);
  _cuda_calc_dnn_shared<<<grid, block>>>(dnn->dinvec, dnn->ddst[0], h->dw, h->db, h->in, h->out);
  _cuda_sigmoid<<<grid, block>>>(dnn->ddst[0], d_logistic, h->out);

  for (int hidx = 1; hidx < dnn->hnum; hidx++) {
    h = &(dnn->h[hidx]);
    _cuda_calc_dnn_shared<<<grid, block>>>(dnn->ddst[hidx-1], dnn->ddst[hidx], h->dw, h->db, h->in, h->out);
    _cuda_sigmoid<<<grid, block>>>(dnn->ddst[hidx], d_logistic, h->out);
  }

  _cuda_calc_dnn_shared<<<grid2, block2>>>(dnn->ddst[dnn->hnum-1], dnn->dout, dnn->o.dw, dnn->o.db, dnn->o.in, dnn->o.out);

  /* transfer result from GPU to cpu */
  CHECK(cudaMemcpy(wrk->last_cache, dnn->dout, sizeof(float) * dnn->outputnodenum, cudaMemcpyDeviceToHost));

}

/************************************************************************/
void cuda_calc_outprob(HMMWork *wrk)
{
  DNNData *dnn = wrk->OP_dnn;

  if (dnn->use_cuda_shared == true) {
    local_calc_outprob_shared(wrk);
  } else {
    local_calc_outprob_global(wrk);
  }

  /* do softmax */
  /* INV_LOG_TEN * (x - addlogarray(x)) - log10(state_prior)) */
#ifdef NO_SUM_COMPUTATION
  /* not compute sum */
  for (int i = 0; i < wrk->statenum; i++) {
    wrk->last_cache[i] = INV_LOG_TEN * wrk->last_cache[i] - dnn->state_prior[i];
  }
#else
  /* compute sum */
  {
    int i;
    float logprob = addlog_array(wrk->last_cache, wrk->statenum);
    for (i = 0; i < wrk->statenum; i++) {
      wrk->last_cache[i] = INV_LOG_TEN * (wrk->last_cache[i] - logprob) - dnn->state_prior[i];
    }
  }
#endif /* NO_SUM_COMPUTATION */
}

#endif /* __NVCC__ */
