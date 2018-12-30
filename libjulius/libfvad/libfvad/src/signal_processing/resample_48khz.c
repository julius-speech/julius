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
 * This file contains resampling functions between 48 kHz and nb/wb.
 * The description header can be found in signal_processing_library.h
 *
 */

#include "signal_processing_library.h"
#include "resample_by_2_internal.h"
#include <string.h>


////////////////////////////
///// 48 kHz ->  8 kHz /////
////////////////////////////

// 48 -> 8 resampler
void WebRtcSpl_Resample48khzTo8khz(const int16_t* in, int16_t* out,
                                   WebRtcSpl_State48khzTo8khz* state, int32_t* tmpmem)
{
    ///// 48 --> 24 /////
    // int16_t  in[480]
    // int32_t out[240]
    /////
    WebRtcSpl_DownBy2ShortToInt(in, 480, tmpmem + 256, state->S_48_24);

    ///// 24 --> 24(LP) /////
    // int32_t  in[240]
    // int32_t out[240]
    /////
    WebRtcSpl_LPBy2IntToInt(tmpmem + 256, 240, tmpmem + 16, state->S_24_24);

    ///// 24 --> 16 /////
    // int32_t  in[240]
    // int32_t out[160]
    /////
    // copy state to and from input array
    memcpy(tmpmem + 8, state->S_24_16, 8 * sizeof(int32_t));
    memcpy(state->S_24_16, tmpmem + 248, 8 * sizeof(int32_t));
    WebRtcSpl_Resample48khzTo32khz(tmpmem + 8, tmpmem, 80);

    ///// 16 --> 8 /////
    // int32_t  in[160]
    // int16_t out[80]
    /////
    WebRtcSpl_DownBy2IntToShort(tmpmem, 160, out, state->S_16_8);
}

// initialize state of 48 -> 8 resampler
void WebRtcSpl_ResetResample48khzTo8khz(WebRtcSpl_State48khzTo8khz* state)
{
    memset(state->S_48_24, 0, 8 * sizeof(int32_t));
    memset(state->S_24_24, 0, 16 * sizeof(int32_t));
    memset(state->S_24_16, 0, 8 * sizeof(int32_t));
    memset(state->S_16_8, 0, 8 * sizeof(int32_t));
}
