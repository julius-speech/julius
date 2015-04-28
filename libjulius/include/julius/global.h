/**
 * @file   global.h
 * 
 * <JA>
 * @brief  大域変数の定義
 *
 * </JA>
 * 
 * <EN>
 * @brief  Global variables
 *
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Sun Sep 18 23:53:17 2005
 *
 * $Revision: 1.7 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __J_GLOBAL_H__
#define __J_GLOBAL_H__

#include <sent/stddefs.h>
#include <sent/vocabulary.h>
#include <julius/wchmm.h>
#include <julius/search.h>
#include <julius/plugin.h>

/**
 * If GLOBAL_VARIABLE_DEFINE is defined, global variables are actually made.
 * Else, these are external definition.
 * 
 */
#ifdef GLOBAL_VARIABLE_DEFINE
#define GLOBAL /*  */
#define GLOBAL_VAL(v) = (v)
#else
#define GLOBAL extern
#define GLOBAL_VAL(v) /*  */
#endif

/* global variables */
GLOBAL boolean verbose_flag GLOBAL_VAL(TRUE);
GLOBAL boolean debug2_flag GLOBAL_VAL(FALSE);
GLOBAL boolean callback_debug_flag GLOBAL_VAL(FALSE);

/* function list for adin process callback */
GLOBAL PLUGIN_ENTRY **global_plugin_list GLOBAL_VAL(NULL);
GLOBAL int global_plugin_loaded_file_num GLOBAL_VAL(0);

#endif /* __J_GLOBAL_H__ */
