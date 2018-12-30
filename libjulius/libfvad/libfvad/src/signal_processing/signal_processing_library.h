/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


/*
 * This header file includes all of the fix point signal processing library (SPL) function
 * descriptions and declarations.
 * For specific function calls, see bottom of file.
 */

#ifndef COMMON_AUDIO_SIGNAL_PROCESSING_INCLUDE_SIGNAL_PROCESSING_LIBRARY_H_
#define COMMON_AUDIO_SIGNAL_PROCESSING_INCLUDE_SIGNAL_PROCESSING_LIBRARY_H_

#include "../common.h"

// Macros specific for the fixed point implementation
#define WEBRTC_SPL_WORD16_MAX       32767
#define WEBRTC_SPL_WORD16_MIN       -32768
#define WEBRTC_SPL_WORD32_MAX       (int32_t)0x7fffffff
#define WEBRTC_SPL_WORD32_MIN       (int32_t)0x80000000

#define WEBRTC_SPL_MUL(a, b) \
    ((int32_t) ((int32_t)(a) * (int32_t)(b)))


// inline functions:
#include "spl_inl.h"

int16_t WebRtcSpl_GetScalingSquare(int16_t* in_vector,
                                   size_t in_vector_length,
                                   size_t times);

// Divisions. Implementations collected in division_operations.c and
// descriptions at bottom of this file.
int32_t WebRtcSpl_DivW32W16(int32_t num, int16_t den);
// End: Divisions.

int32_t WebRtcSpl_Energy(int16_t* vector,
                         size_t vector_length,
                         int* scale_factor);


/************************************************************
 *
 * RESAMPLING FUNCTIONS AND THEIR STRUCTS ARE DEFINED BELOW
 *
 ************************************************************/

/*******************************************************************
 * resample_fractional.c
 * Functions for internal use in the other resample functions
 *
 * Includes the following resampling combinations
 * 48 kHz -> 32 kHz
 *
 ******************************************************************/

void WebRtcSpl_Resample48khzTo32khz(const int32_t* In, int32_t* Out, size_t K);

/*******************************************************************
 * resample_48khz.c
 *
 * Includes the following resampling combinations
 * 48 kHz ->  8 kHz
 *
 ******************************************************************/

typedef struct {
  int32_t S_48_24[8];
  int32_t S_24_24[16];
  int32_t S_24_16[8];
  int32_t S_16_8[8];
} WebRtcSpl_State48khzTo8khz;

void WebRtcSpl_Resample48khzTo8khz(const int16_t* in, int16_t* out,
                                   WebRtcSpl_State48khzTo8khz* state,
                                   int32_t* tmpmem);

void WebRtcSpl_ResetResample48khzTo8khz(WebRtcSpl_State48khzTo8khz* state);

/************************************************************
 * END OF RESAMPLING FUNCTIONS
 ************************************************************/


#endif  // COMMON_AUDIO_SIGNAL_PROCESSING_INCLUDE_SIGNAL_PROCESSING_LIBRARY_H_
