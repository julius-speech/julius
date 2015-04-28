/**
 * @file   julius.h
 * 
 * <JA>
 * @brief  Julius 用のトップヘッダファイル
 * </JA>
 * 
 * <EN>
 * @brief  Top common header for Julius
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Mar 17 21:08:21 2005
 *
 * $Revision: 1.9 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __J_JULIUS_H__
#define __J_JULIUS_H__

/* read configurable definitions */
#if defined(_WIN32) && !defined(__CYGWIN32__) && !defined(__MINGW32__)
# include <config-msvc-libjulius.h>
# include <config-msvc-libsent.h>
#else
#include <julius/config.h>
#include <sent/config.h>
#endif
/* read built-in definitions */
#include <julius/define.h>

/* read libsent includes */
#include <sent/stddefs.h>
#include <sent/tcpip.h>
#include <sent/speech.h>
#include <sent/mfcc.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/hmm_calc.h>
#include <sent/vocabulary.h>
#include <sent/ngram2.h>
#include <sent/dfa.h>

/* read Julius/Julian includes */
#ifdef ENABLE_PLUGIN
#include <julius/plugin.h>
#endif
#include <julius/multi-gram.h>
#include <julius/wchmm.h>
#include <julius/trellis.h>
#include <julius/graph.h>
#include <julius/beam.h>
#include <julius/search.h>
#include <julius/misc.h>
#include <julius/jconf.h>
#include <julius/recog.h>
#include <julius/global.h>
#include <julius/jfunc.h>
#include <julius/callback.h>
#include <julius/useropt.h>
#include <julius/extern.h>

#endif /* __J_JULIUS_H__ */
