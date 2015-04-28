/**
 * @file   check_hmmtype.c
 * 
 * <JA>
 * @brief  %HMM と特徴ファイルのパラメータ型野整合性をチェックする
 * </JA>
 * 
 * <EN>
 * @brief  Check the parameter types between %HMM and input
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Tue Feb 15 19:11:50 2005
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

#include <sent/stddefs.h>
#include <sent/htk_defs.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>

/** 
 * Check if the required parameter type in this %HMM can be handled by Julius.
 * 
 * @param hmm [in] HMM definition data
 * 
 * @return TRUE if Julius supports it, otherwise return FALSE.
 */
boolean
check_hmm_options(HTK_HMM_INFO *hmm)
{
  boolean ret_flag = TRUE;
  
  /* 
   * if (hmm->opt.stream_info.num > 1) {
   *   jlog("Error: check_hmmtype: Input stream must be single\n");
   *   ret_flag = FALSE;
   * }
   */
/* 
 *   if (hmm->opt.dur_type != D_NULL) {
 *     jlog("Error: check_hmmtype: Duration types other than NULLD are not supported.\n");
 *     ret_flag = FALSE;
 *   }
 */
  if (hmm->opt.cov_type != C_DIAG_C) {
    jlog("Error: check_hmmtype: Covariance matrix type must be DIAGC, others not supported.\n");
    ret_flag = FALSE;
  }

  return(ret_flag);
}

/** 
 * Check if an input parameter type exactly matches that of %HMM.
 * 
 * @param hmm [in] HMM definition data
 * @param pinfo [in] input parameter
 * 
 * @return TRUE if matches, FALSE if differs.
 */
boolean
check_param_coherence(HTK_HMM_INFO *hmm, HTK_Param *pinfo)
{
  boolean ret_flag;

  ret_flag = TRUE;

  /* HMM type check */
  if (hmm->opt.param_type
      != (pinfo->header.samptype & ~(F_COMPRESS | F_CHECKSUM))) {
/* 
 *     jlog("Error: check_hmmtype: incompatible parameter type\n");
 *     jlog("Error: check_hmmtype: HMM trained by %s\n", param_code2str(buf, hmm->opt.param_type, FALSE));
 *     jlog("Error: check_hmmtype: input parameter is %s\n", param_code2str(buf, pinfo->header.samptype, FALSE));
 */
    ret_flag = FALSE;
  }

  /* vector length check */
  if (hmm->opt.vec_size != pinfo->veclen) {
/* 
 *     jlog("Error: check_hmmtype: vector length differ.\n");
 *     jlog("Error: check_hmmtype: HMM=%d, param=%d\n", hmm->opt.vec_size, pinfo->veclen);
 */
    ret_flag = FALSE;
  }
  
  return(ret_flag);
}

/** 
 * Check if the base type of input parameter matches that of %HMM.
 * 
 * @param hmm [in] HMM definition data
 * @param pinfo [in] input parameter
 * 
 * @return TRUE if matches, FALSE if differs.
 */
boolean
check_param_basetype(HTK_HMM_INFO *hmm, HTK_Param *pinfo)
{
  if ((hmm->opt.param_type & F_BASEMASK)
      != (pinfo->header.samptype & F_BASEMASK)) {
    return FALSE;
  } else {
    return TRUE;
  }
} 
