/**
 * @file   check_hmm_restriction.c
 * 
 * <JA>
 * @brief  与えられた %HMM の遷移が使用可能な形式かどうかチェックする
 * </JA>
 * 
 * <EN>
 * @brief  Check if the given %HMM definition file can be used
 * </EN>
 *
 * TRANSITION RESTRICTIONS:
 *
 *   - for HTK and Julius:
 *      - no arc to initial state
 *      - no arc from final state
 * 
 *   - Normal version of Julius:
 *      - should have at least one output state
 *      - allow only one arc from initial state
 *      - allow only one arc to final state
 *        (internal skip/loop is allowed)
 *
 *   - Multipath version of Julius:
 *      - should have at least one output state
 *
 * In multipath version, all the transitions including model-skipping
 * transition is allowed.  However, in normal version, their transition
 * is restricted as above.
 * 
 * If such transition is found, Julius output warning and
 * proceed by modifying transition to suite for the restriction.
 *
 * @author Akinobu LEE
 * @date   Tue Feb 15 19:00:58 2005
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
#include <sent/htk_param.h>
#include <sent/hmm.h>

/** 
 * Return TRUE if it has more than one arc from initial state, or
 * to the final state.  In such case, Julius should be run in
 * multi-path version.
 * 
 * @param hmminfo [in] HMM definition
 * 
 * @return TRUE if has, or FALSE if not exist.
 * </EN>
 */
boolean
htk_hmm_has_several_arc_on_edge(HTK_HMM_INFO *hmminfo)
{
  HTK_HMM_Data *dt;
  HTK_HMM_Trans *t;
  boolean flag;
  int i;

  for (dt = hmminfo->start; dt; dt = dt->next) {
    t = dt->tr;
    flag = FALSE;
    for (i=0;i<t->statenum;i++) {
      if (t->a[0][i] != LOG_ZERO) {
	if (flag == FALSE) {
	  flag = TRUE;
	} else {
	  jlog("Stat: check_hmm_restriction: an HMM with several arcs from initial state found: \"%s\"\n", dt->name);
	  return (TRUE);
	}
      }
    }
    flag = FALSE;
    for (i=0;i<t->statenum;i++) {
      if (t->a[i][t->statenum-1] != LOG_ZERO) {
	if (flag == FALSE) {
	  flag = TRUE;
	} else {
	  jlog("Stat: check_hmm_restriction: an HMM with several arcs to final state found: \"%s\"\n", dt->name);
	  return (TRUE);
	}
      }
    }
  }

  return FALSE;
}

/** 
 * Scan the transition matrix to test the ristrictions.
 * 
 * @param t [in] a transition matrix to be tested
 * 
 * @return 0 if it conforms, 1 if unacceptable transition was found and
 * modification forced, 3 if totally unsupported transition as included and
 * cannot by handled.
 */
static boolean
trans_ok_p(HTK_HMM_Trans *t)
{
  int i;
  int tflag;
  int retflag = TRUE;

  /* check arc to initial state */
  tflag = FALSE;
  for (i=0;i<t->statenum;i++) {
    if (t->a[i][0] != LOG_ZERO) {
      tflag = TRUE;
      break;
    }
  }
  if (tflag) {
    jlog("Error: check_hmm_restriction: transition to initial state is not allowed\n");
    retflag = FALSE;
  }
  /* check arc from final state */
  tflag = FALSE;
  for (i=0;i<t->statenum;i++) {
    if (t->a[t->statenum-1][i] != LOG_ZERO) {
      tflag = TRUE;
      break;
    }
  }
  if (tflag) {
    jlog("Error: check_hmm_restriction: transition from final state is not allowed\n");
    retflag = FALSE;
  }
  /* check if arc from/to initial/final state exist */
  tflag = FALSE;
  for (i=0;i<t->statenum;i++) {
    if (t->a[0][i] != LOG_ZERO) {
      tflag = TRUE;
      break;
    }
  }
  if (tflag == FALSE) {
    jlog("Error: check_hmm_restriction: no transition from initial state\n");
    retflag = FALSE;
  }
  tflag = FALSE;
  for (i=0;i<t->statenum;i++) {
    if (t->a[i][t->statenum-1] != LOG_ZERO) {
      tflag = TRUE;
      break;
    }
  }
  if (tflag == FALSE) {
    jlog("Error: check_hmm_restriction: no transition to final state\n");
    retflag = FALSE;
  }
    
  return(retflag);
}

/** 
 * Check if the transition matrix conforms the ristrictions of Julius.
 * 
 * @param dt [in] HTK %HMM model to check.
 * 
 * @return TRUE on success, FALSE if the check failed.
 */
boolean
check_hmm_limit(HTK_HMM_Data *dt)
{
  boolean return_flag = TRUE;

  if (trans_ok_p(dt->tr) == FALSE) {
    return_flag = FALSE;
    jlog("Error: check_hmm_restriction: HMM \"%s\" has unsupported arc.\n", dt->name);
    put_htk_trans(jlog_get_fp(), dt->tr);
  }
  if (dt->tr->statenum < 3) {
    return_flag = FALSE;
    jlog("Error: HMM \"%s\" has no output state (statenum=%d)\n", dt->name, dt->tr->statenum);
  }
  return(return_flag);
}

/** 
 * Check all the %HMM definitions in a HTK %HMM definition data.
 * 
 * @param hmminfo [in] HTK %HMM data to check.
 * 
 * @return TRUE if there was no bad models, FALSE if at least one model is bad.
 */
boolean
check_all_hmm_limit(HTK_HMM_INFO *hmminfo)
{
  HTK_HMM_Data *dt;
  boolean return_flag = TRUE;

  for (dt = hmminfo->start; dt; dt = dt->next) {
    if (check_hmm_limit(dt) == FALSE) {
      return_flag = FALSE;
    }
  }
  return(return_flag);
}


/** 
 * <JA>
 * モデルが，出力状態を経由せずに入力状態から出力状態へ直接遷移するような
 * 遷移を持つかどうかをチェックする．
 * 
 * @param d [in] 論理HMM
 * 
 * @return 入力から出力への直接遷移を持つ場合 TRUE, 持たない場合 FALSE を返す．
 * </JA>
 * <EN>
 * Check if the model has direct transition from initial state to final state,
 * skipping all the output state.
 * 
 * @param d [in] logical HMM
 * 
 * @return TRUE if it has direct transition from initial state to final state,
 * that is, this is a "skippable" model.  Otherwise, return FALSE.
 * </EN>
 */
boolean
is_skippable_model(HTK_HMM_Data *d)
{
  if (d->tr->a[0][d->tr->statenum-1] != LOG_ZERO) {
    return TRUE;
  }
  return FALSE;
}
