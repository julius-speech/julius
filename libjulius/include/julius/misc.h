/**
 * @file   misc.h
 * 
 * <JA>
 * @brief  その他の雑多な定義
 * </JA>
 * 
 * <EN>
 * @brief  Some miscellaneous definitions
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Mon May 30 15:58:16 2005
 *
 * $Revision: 1.5 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __J_MISC_H__
#define __J_MISC_H__

/// Defines for selecting message output destination
enum {
  SP_RESULT_TTY,		///< To tty
  SP_RESULT_MSOCK		///< Socket output in XML-like format for module mode
};

/// Switch to specify the grammar changing timing policy.
enum{SM_TERMINATE, SM_PAUSE, SM_WAIT};

/// Switch to specify the alignment unit
enum{PER_WORD, PER_PHONEME, PER_STATE};

#endif /* __J_MISC_H__ */
