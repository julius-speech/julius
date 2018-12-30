/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


// This header file includes the inline functions in
// the fix point signal processing library.

#ifndef COMMON_AUDIO_SIGNAL_PROCESSING_INCLUDE_SPL_INL_H_
#define COMMON_AUDIO_SIGNAL_PROCESSING_INCLUDE_SPL_INL_H_

#include "../common.h"

extern const int8_t kWebRtcSpl_CountLeadingZeros32_Table[64];

// Don't call this directly except in tests!
static __inline int WebRtcSpl_CountLeadingZeros32_NotBuiltin(uint32_t n) {
  // Normalize n by rounding up to the nearest number that is a sequence of 0
  // bits followed by a sequence of 1 bits. This number has the same number of
  // leading zeros as the original n. There are exactly 33 such values.
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;

  // Multiply the modified n with a constant selected (by exhaustive search)
  // such that each of the 33 possible values of n give a product whose 6 most
  // significant bits are unique. Then look up the answer in the table.
  return kWebRtcSpl_CountLeadingZeros32_Table[(n * 0x8c0b2891) >> 26];
}

// Returns the number of leading zero bits in the argument.
static __inline int WebRtcSpl_CountLeadingZeros32(uint32_t n) {
#ifdef __GNUC__
  RTC_COMPILE_ASSERT(sizeof(unsigned int) == sizeof(uint32_t));
  return n == 0 ? 32 : __builtin_clz(n);
#else
  return WebRtcSpl_CountLeadingZeros32_NotBuiltin(n);
#endif
}


static __inline int16_t WebRtcSpl_GetSizeInBits(uint32_t n) {
  return 32 - WebRtcSpl_CountLeadingZeros32(n);
}

// Return the number of steps a can be left-shifted without overflow,
// or 0 if a == 0.
static __inline int16_t WebRtcSpl_NormW32(int32_t a) {
  return a == 0 ? 0 : WebRtcSpl_CountLeadingZeros32(a < 0 ? ~a : a) - 1;
}

// Return the number of steps a can be left-shifted without overflow,
// or 0 if a == 0.
static __inline int16_t WebRtcSpl_NormU32(uint32_t a) {
  return a == 0 ? 0 : WebRtcSpl_CountLeadingZeros32(a);
}


#endif  // COMMON_AUDIO_SIGNAL_PROCESSING_INCLUDE_SPL_INL_H_
