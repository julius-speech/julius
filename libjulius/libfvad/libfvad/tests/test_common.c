/*
 * Copyright (c) 2016 Daniel Pirch
 *
 * Use of this source code is governed by a BSD-style license
 * that can be found in the LICENSE file in the root of the source
 * tree. An additional intellectual property rights grant can be found
 * in the file PATENTS.  All contributing project authors may
 * be found in the AUTHORS file in the root of the source tree.
 */

#include "test_common.h"
#include <stdio.h>

void assert_fail(const char *s, const char *file, int line)
{
    printf("failed ASSERT: %s (%s:%d)\n", s, file, line);
    exit(1);
}

static bool expect_failed = false;

void expect_fail(const char *s, const char *file, int line)
{
    printf("failed EXPECT: %s (%s:%d)\n", s, file, line);
    expect_failed = true;
}

// function to be defined by individual tests
void test_main(void);

int main()
{
    test_main();
    if (expect_failed) exit(2);
}
