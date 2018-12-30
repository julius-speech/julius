/*
 * Copyright (c) 2016 Daniel Pirch
 *
 * Use of this source code is governed by a BSD-style license
 * that can be found in the LICENSE file in the root of the source
 * tree. An additional intellectual property rights grant can be found
 * in the file PATENTS.  All contributing project authors may
 * be found in the AUTHORS file in the root of the source tree.
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

// replacements for WebRTC's various assert-like macros
#define RTC_COMPILE_ASSERT(expr) static_assert(expr, #expr)
#define RTC_DCHECK(expr) assert(expr)
#define RTC_DCHECK_GT(a,b) assert((a) > (b))
#define RTC_DCHECK_LT(a,b) assert((a) < (b))
#define RTC_DCHECK_LE(a,b) assert((a) <= (b))

// from webrtc/base/sanitizer.h
#ifdef __has_attribute
#if __has_attribute(no_sanitize)
#define RTC_NO_SANITIZE(what) __attribute__((no_sanitize(what)))
#endif
#endif
#ifndef RTC_NO_SANITIZE
#define RTC_NO_SANITIZE(what)
#endif

#define arraysize(a) (sizeof (a) / sizeof *(a))


#endif // COMMON_H_
