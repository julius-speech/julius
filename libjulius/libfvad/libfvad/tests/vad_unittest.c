/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *  Copyright (c) 2016 Daniel Pirch.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vad_unittest.h"
#include "../include/fvad.h"


// Returns true if the rate and frame length combination is valid.
bool ValidRatesAndFrameLengths(int rate, size_t frame_length) {

  if (rate == 8000) {
    if (frame_length == 80 || frame_length == 160 || frame_length == 240) {
      return true;
    }
    return false;
  } else if (rate == 16000) {
    if (frame_length == 160 || frame_length == 320 || frame_length == 480) {
      return true;
    }
    return false;
  } else if (rate == 32000) {
    if (frame_length == 320 || frame_length == 640 || frame_length == 960) {
      return true;
    }
    return false;
  } else if (rate == 48000) {
    if (frame_length == 480 || frame_length == 960 || frame_length == 1440) {
      return true;
    }
    return false;
  }

  return false;
}


#ifdef TEST_VAD_API
void test_main() {
  // This API test runs through the APIs for all possible valid and invalid
  // combinations.

  Fvad* handle = fvad_new();
  int16_t zeros[kMaxFrameLength] = { 0 };

  // Construct a speech signal that will trigger the VAD in all modes. It is
  // known that (i * i) will wrap around, but that doesn't matter in this case.
  int16_t speech[kMaxFrameLength];
  for (size_t i = 0; i < kMaxFrameLength; i++) {
    speech[i] = i * i;
  }

  // fvad_new()
  EXPECT_TRUE(handle);

  // fvad_set_mode() invalid modes tests. Tries smallest supported value
  // minus one and largest supported value plus one.
  EXPECT_EQ(-1, fvad_set_mode(handle, -1));
  EXPECT_EQ(-1, fvad_set_mode(handle, 4));

  // Invalid sampling rate
  EXPECT_EQ(-1, fvad_set_sample_rate(handle, 9999));

  // fvad_process() tests
  // All zeros as input should work
  EXPECT_EQ(0, fvad_set_sample_rate(handle, kRates[0]));
  EXPECT_EQ(0, fvad_process(handle, zeros, kFrameLengths[0]));
  for (size_t k = 0; k < kModesSize; k++) {
    // Test valid modes
    EXPECT_EQ(0, fvad_set_mode(handle, kModes[k]));
    // Loop through sampling rate and frame length combinations
    for (size_t i = 0; i < kRatesSize; i++) {
      for (size_t j = 0; j < kFrameLengthsSize; j++) {
        if (ValidRatesAndFrameLengths(kRates[i], kFrameLengths[j])) {
          EXPECT_EQ(0, fvad_set_sample_rate(handle, kRates[i]));
          EXPECT_EQ(1, fvad_process(handle, speech, kFrameLengths[j]));
        } else if (ValidRatesAndFrameLengths(kRates[i], kRates[i] / 100)) {
          EXPECT_EQ(0, fvad_set_sample_rate(handle, kRates[i]));
          EXPECT_EQ(-1, fvad_process(handle, speech, kFrameLengths[j]));
        } else {
          EXPECT_EQ(-1, fvad_set_sample_rate(handle, kRates[i]));
        }
      }
    }
  }

  fvad_free(handle);
}
#endif // TEST_VAD_API

// TODO(bjornv): Add a process test, run on file.

