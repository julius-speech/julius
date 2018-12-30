/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef COMMON_AUDIO_VAD_VAD_UNITTEST_H_
#define COMMON_AUDIO_VAD_VAD_UNITTEST_H_

#include "test_common.h"

// Modes we support
static const int kModes[] = { 0, 1, 2, 3 };
static const size_t kModesSize = sizeof(kModes) / sizeof(*kModes);

// Rates we support.
static const int kRates[] = { 8000, 12000, 16000, 24000, 32000, 48000 };
static const size_t kRatesSize = sizeof(kRates) / sizeof(*kRates);

// Frame lengths we support.
#define kMaxFrameLength 1440
static const size_t kFrameLengths[] = { 80, 120, 160, 240, 320, 480, 640, 960,

    kMaxFrameLength };
static const size_t kFrameLengthsSize = sizeof(kFrameLengths) / sizeof(*kFrameLengths);


  // Returns true if the rate and frame length combination is valid.
  bool ValidRatesAndFrameLengths(int rate, size_t frame_length);

#endif  // COMMON_AUDIO_VAD_VAD_UNITTEST_H_
