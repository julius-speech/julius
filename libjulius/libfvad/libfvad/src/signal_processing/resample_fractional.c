/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


/*
 * This file contains the resampling functions between 48, 44, 32 and 24 kHz.
 * The description headers can be found in signal_processing_library.h
 *
 */

#include "signal_processing_library.h"

// interpolation coefficients
static const int16_t kCoefficients48To32[2][8] = {
        {778, -2050, 1087, 23285, 12903, -3783, 441, 222},
        {222, 441, -3783, 12903, 23285, 1087, -2050, 778}
};


//   Resampling ratio: 2/3
// input:  int32_t (normalized, not saturated) :: size 3 * K
// output: int32_t (shifted 15 positions to the left, + offset 16384) :: size 2 * K
//      K: number of blocks

void WebRtcSpl_Resample48khzTo32khz(const int32_t *In, int32_t *Out, size_t K)
{
    /////////////////////////////////////////////////////////////
    // Filter operation:
    //
    // Perform resampling (3 input samples -> 2 output samples);
    // process in sub blocks of size 3 samples.
    int32_t tmp;
    size_t m;

    for (m = 0; m < K; m++)
    {
        tmp = 1 << 14;
        tmp += kCoefficients48To32[0][0] * In[0];
        tmp += kCoefficients48To32[0][1] * In[1];
        tmp += kCoefficients48To32[0][2] * In[2];
        tmp += kCoefficients48To32[0][3] * In[3];
        tmp += kCoefficients48To32[0][4] * In[4];
        tmp += kCoefficients48To32[0][5] * In[5];
        tmp += kCoefficients48To32[0][6] * In[6];
        tmp += kCoefficients48To32[0][7] * In[7];
        Out[0] = tmp;

        tmp = 1 << 14;
        tmp += kCoefficients48To32[1][0] * In[1];
        tmp += kCoefficients48To32[1][1] * In[2];
        tmp += kCoefficients48To32[1][2] * In[3];
        tmp += kCoefficients48To32[1][3] * In[4];
        tmp += kCoefficients48To32[1][4] * In[5];
        tmp += kCoefficients48To32[1][5] * In[6];
        tmp += kCoefficients48To32[1][6] * In[7];
        tmp += kCoefficients48To32[1][7] * In[8];
        Out[1] = tmp;

        // update pointers
        In += 3;
        Out += 2;
    }
}

