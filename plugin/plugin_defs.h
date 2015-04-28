/**
 * @file   plugin_defs.h
 * 
 * <EN>
 * @brief  Definitions for JPI Plugin 
 * </EN>
 * 
 * <JA>
 * @brief  JPI プラグイン用定義
 * </JA>
 * 
 * @author Akinobu Lee
 * @date   Sat Aug  9 23:46:32 2008
 * 
 * $Revision: 1.4 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __JULIUS_PLUGIN_DEFS__
#define __JULIUS_PLUGIN_DEFS__

typedef unsigned char boolean;
typedef short SP16;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/// Return code of adin_read()
#define ADIN_EOF -1
#define ADIN_ERROR -2
#define ADIN_SEGMENT -3

#endif /* __JULIUS_PLUGIN_DEFS__ */
