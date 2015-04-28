/**
 * @file   plugin.h
 * 
 * <EN>
 * @brief  Plugin related header
 * </EN>
 * 
 * <JA>
 * @brief  プラグイン用ヘッダ
 * </JA>
 * 
 * @author Akinobu Lee
 * @date   Sat Aug  2 13:04:15 2008
 * 
 * $Revision: 1.5 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __JULIUS_PLUGIN__
#define __JULIUS_PLUGIN__

#include <sent/stddefs.h>

/**
 * Plug-in file suffix
 * 
 */
#define PLUGIN_SUFFIX ".jpi"

/**
 * List of plugin function names
 * 
 */
#define PLUGIN_FUNCTION_NAMELIST { \
 "adin_get_optname", \
 "adin_get_configuration", "adin_standby", \
 "adin_open",  "adin_read", "adin_close", \
 "adin_resume", "adin_pause", "adin_terminate", \
 "adin_postprocess", "adin_postprocess_triggered", \
 "fvin_get_optname", \
 "fvin_get_configuration", "fvin_standby", \
 "fvin_open", "fvin_read", "fvin_close", \
 "fvin_resume", "fvin_pause", "fvin_terminate", \
 "fvin_input_name",\
 "fvin_postprocess", \
 "calcmix_get_optname", "calcmix", "calcmix_init", "calcmix_free", \
 "result_best_str", \
 "startup"}

/**
 * Typedef for loaded module
 * 
 */
#if defined(_WIN32) && !defined(__CYGWIN32__)
typedef HMODULE PLUGIN_MODULE;
#else
typedef void* PLUGIN_MODULE;
#endif

/**
 * define for "none"
 * 
 */
#define PLUGIN_NONE NULL

/**
 * Function definition
 * 
 */
typedef void (*FUNC_VOID)();
typedef char * (*FUNC_STR)();
typedef int (*FUNC_INT)();

/**
 * Plugin function entry
 * 
 */
typedef struct __j_plugin_entry__ {
  int id;
  int source_id;
  FUNC_VOID func;
  struct __j_plugin_entry__ *next;
} PLUGIN_ENTRY;

/* include headers for dynamic loading */
/* unix, cygwin = dlopen */
/* mingw, VS = non (should emulate using win32 func.) */
#ifdef _WIN32
# ifdef __CYGWIN32__
#  include <dlfcn.h>
# else
#  include <windows.h>
#  include <errno.h>
#  define dlopen(P,G) (void *)LoadLibrary(P)
#  define dlsym(D, F) (void *)GetProcAddress((HMODULE)D, F)
#  define dlclose(D)  FreeLibrary((HMODULE)D)
/* dlerror() is defined in plugins.c */
#  define RTLD_LAZY 0		/* dummy */
# endif
#else  /* UNIX */
# include <dlfcn.h>
#endif

#endif /* __JULIUS_PLUGIN__ */
