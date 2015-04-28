/**
 * @file   guess_cdHMM.c
 * 
 * <JA>
 * @brief  %HMM 定義がコンテキスト依存モデルかどうかを推定する
 *
 * %HMMの名前付けルールから判定が行われます．
 * </JA>
 * 
 * <EN>
 * @brief  Guess whether the %HMM definition data is a context-dependent model
 *
 * The naming rule of %HMM data will be used to determine whether this
 * is a context-dependent model.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Tue Feb 15 22:30:37 2005
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

/** 
 * Guess whether the given %HMM definition contains context-dependent modeling,
 * just by the naming rule.
 * 
 * @param hmminfo [in] target %HMM definition
 * 
 * @return TRUE if the result is context-dependent model, FALSE if context-dependent model.
 */
boolean
guess_if_cd_hmm(HTK_HMM_INFO *hmminfo)
{
  HMM_Logical *dt;
  int rnum, lnum, totalnum;

  /* check if there is a logical HMM whose name includes either HMM_RC_DLIM
     or HMM_LC_DLIM */
  rnum = lnum = totalnum = 0;
  for (dt = hmminfo->lgstart; dt; dt = dt->next) {
    if (strstr(dt->name, HMM_RC_DLIM) != NULL) rnum++;
    if (strstr(dt->name, HMM_LC_DLIM) != NULL) lnum++;
    totalnum++;
  }
  if (rnum > 0) {
    if (lnum == 0) {
      jlog("Warning: guess_cdHMM: cannot handle right-context dependency correctly\n");
      return(FALSE);
    } else {
      return(TRUE);
    }
  }
  return(FALSE);
}
