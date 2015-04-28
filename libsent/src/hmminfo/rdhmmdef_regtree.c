/**
 * @file   rdhmmdef_regtree.c
 * 
 * <JA>
 * @brief  HTK %HMM 定義ファイルの読み込み：Regression tree
 *
 * Regression tree は保存されず，読み飛ばされます．
 * </JA>
 * 
 * <EN>
 * @brief  Read HTK %HMM definition file: Regression tree
 *
 * The regression tree informations are not saved, just skipped.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 02:30:28 2005
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

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>

extern char *rdhmmdef_token;	///< Current token

/** 
 * Read in the regression tree to skip till next macro.
 * 
 * @param fp [in] file pointer
 */
static void
regtree_read(FILE *fp)
{
  int num;

  read_token(fp);
  NoTokErr("missing REGTREE terminal node num");
  num = atoi(rdhmmdef_token);
  read_token(fp);
  for(;;) {
    if (currentis("NODE")) {	/* skip 3 arguments */
      read_token(fp);
      read_token(fp);
      read_token(fp);
      read_token(fp);
    } else if (currentis("TNODE")) { /* skip 2 argument */
      read_token(fp);
      read_token(fp);
      read_token(fp);
    } else {
      break;
    }
  }
}

/** 
 * Skip a regression tree data or its macro reference.
 * 
 * @param name [in] macro name
 * @param fp [in] file pointer
 * @param hmm [in] %HMM definition data
 */
void
def_regtree_macro(char *name, FILE *fp, HTK_HMM_INFO *hmm)
{
  if (currentis("~r")) {	/* macro reference */
    /* ignore silently */
  } else if (currentis("REGTREE")) { /* definition */
    /* do not define actually, just read forward till next macro */
    regtree_read(fp);
  } else {
    rderr("no regtree data\n");
  }
  return;
}
