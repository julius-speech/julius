/*
 * Copyright (c) 2016 Daniel Pirch
 *
 * Use of this source code is governed by a BSD-style license
 * that can be found in the LICENSE file in the root of the source
 * tree. An additional intellectual property rights grant can be found
 * in the file PATENTS.  All contributing project authors may
 * be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TEST_COMMON_H_
#define TEST_COMMON_H_

#include "../src/common.h"

#define ASSERT_TRUE(cond) ((cond) ? (void)0 : assert_fail(#cond, __FILE__, __LINE__))
#define EXPECT_TRUE(cond) ((cond) ? (void)0 : expect_fail(#cond, __FILE__, __LINE__))

#define ASSERT_EQ(val1, val2) ASSERT_TRUE((val1) == (val2))
#define EXPECT_EQ(val1, val2) EXPECT_TRUE((val1) == (val2))

void assert_fail(const char *s, const char *file, int line);
void expect_fail(const char *s, const char *file, int line);

#define nullptr NULL

#endif // TEST_COMMON_H_
