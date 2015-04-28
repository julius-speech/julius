/**
 * @file   machines.h
 *
 * <EN>
 * @brief  Machine-dependent definitions
 *
 * This file contains machine-dependent or OS-dependent definitions,
 * although most of such runtime dependencies are now handled by
 * configure script.
 * </EN>
 * <JA>
 * @brief 実行環境依存の定義
 *
 * このファイルには実行環境に依存した定義が含まれています．
 * ただし，そのような実行環境に依存した定義の変更や関数の有無の判定は
 * 現在ほとんど configure スクリプトによって行なわれています．
 * </JA>
 *
 * @sa config.h.in
 *
 * @author Akinobu LEE
 * @date   Fri Feb 11 03:38:31 2005
 *
 * $Revision: 1.6 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __SENT_MACHINES_H__
#define __SENT_MACHINES_H__

#include <sent/stddefs.h>

#ifndef HAVE_STRCASECMP
#ifdef __cplusplus
extern "C" {
#endif
int strcasecmp(char *s1, char *s2);
int strncasecmp(char *s1, char *s2, size_t n);
#ifdef __cplusplus
}
#endif
#endif

#endif /* __SENT_MACHINES_H__ */
