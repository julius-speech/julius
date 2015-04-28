/**
 * @file   useropt.h
 * 
 * <JA>
 * @brief  ユーザ指定の jconf オプション拡張
 * </JA>
 * 
 * <EN>
 * @brief  User-defined jconf options
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Sun Sep 02 03:09:12 2007
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

#ifndef __J_USEROPT_H__
#define __J_USEROPT_H__

/**
 * User-defined option
 * 
 */
typedef struct __j_useropt__ {
  char *optstr;			///< Option string
  char *desc;			///< Description that will be output on help
  int argnum;			///< Number of arguments
  int reqargnum;		///< Number of optional arguments in argnum
  boolean (*func)(Jconf *jconf, char *arg[], int argnum); ///< Handling function
  struct __j_useropt__ *next;	///< Pointer to next data
} USEROPT;

boolean j_add_option(char *fmt, int argnum, int reqargnum, char *desc, boolean (*func)(Jconf *jconf, char *arg[], int argnum));
void useropt_free_all();
int useropt_exec(Jconf *jconf, char *argv[], int argc, int *n);
void useropt_show_desc(FILE *fp);


#endif /* __J_USEROPT_H__ */
