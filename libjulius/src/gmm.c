/**
 * @file   gmm.c
 * 
 * <JA>
 * @brief  GMM による入力棄却およびVAD
 *
 * Gaussian Mixture Model (GMM) が起動時に指定された場合，Julius/Julian は
 * 入力発話に対してフレームごとにスコアを計算し，その累積スコアを算出する. 
 * これはGMMに基づく入力音声の発話検証および棄却に用いられる. 実際の計算は
 * 第1パスの認識処理と並行してリアルタイムに行なわれ，第1パス終了と同時に
 * 結果が出力される. 
 *
 * GMMのスコア計算には Gaussian pruning の safe algorithm が用いられ，
 * 各フレームにおいて上位 N 個だけが正しく得られるように計算される. 
 * ただし通常の認識用音響モデルの場合と異なり，直前フレームの順位情報は
 * 用いていない. 
 *
 * GMM_VAD 定義時は，上記の入力棄却に加えて，short-pause segmentation と
 * 同じ枠組にを用いた VAD が行われる. 
 * </JA>
 * 
 * <EN>
 * @brief  Input rejection and VAD using GMM
 *
 * When a Gaussian Mixture Model (GMM) is specified on startup, Julius/Julian
 * will compute the frame-wise likelihoods of each GMM for given inputs,
 * and produces the accumulated scores for each.  Then the input rejection is
 * determined from the value.  Actually, the recognition will be computed
 * on-line concurrently with the 1st pass, and the result will be got as
 * soon as the 1st pass ends.
 *
 * Gaussian pruning is performed using the safe algorithm in the computation
 * of GMM scores.  In each frame, pruning will be done to fully compute only
 * the top N Gaussians.  The algorithm is slightly simpler than AM computation,
 * i.e. the score order of the previous frame is not used here.
 *
 * When GMM_VAD is defined, a GMM-based VAD will be enabled in addition to
 * the input rejection, using the scheme of short-pause segmentation.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Tue Mar 15 05:14:10 2005
 *
 * $Revision: 1.7 $
 * 
 */

/*
 * Copyright (c) 2003-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius/julius.h>

#undef MES

/** 
 * <JA>
 * Gaussianのスコアを計算済みGaussianリストのどの位置に挿入すべきかを返す. 
 *
 * @param gc [i/o] GMM計算用ワークエリア
 * @param score [in] 挿入したいスコア
 * @param len [in] 現在のリストの長さ
 * 
 * @return リスト内の挿入位置
 * </JA>
 * <EN>
 * Return insertion point where a computed Gaussian score should be
 * inserted in current list of computed Gaussians.
 *
 * @param gc [i/o] work area for GMM calculation
 * @param score [in] a score to be inserted
 * @param len [in] current length of the list
 * 
 * @return index to insert the value at the list.
 * </EN>
 */
static int
gmm_find_insert_point(GMMCalc *gc, LOGPROB score, int len)
{
  /* binary search on score */
  int left = 0;
  int right = len - 1;
  int mid;

  while (left < right) {
    mid = (left + right) / 2;
    if (gc->OP_calced_score[mid] > score) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  return(left);
}

/** 
 * <JA>
 * あるGaussianの計算結果を計算済みGaussianリストに格納する. 
 * 
 * @param gc [i/o] GMM計算用ワークエリア
 * @param id [in] Gaussian の GMM 内での番号
 * @param score [in] その Gaussian の計算された音響尤度
 * @param len [in] 現在のリストの長さ（現在格納されている Gaussian の数）
 * 
 * @return 格納後のリストの長さ. 
 * </JA>
 * <EN>
 * Store a Gaussian likelihood to the list of computed Gaussians.
 * 
 * @param gc [i/o] work area for GMM calculation
 * @param id [in] id of a Gaussian in the GMM to be stored
 * @param score [in] the likelihood of the Gaussian to be stored
 * @param len [in] current list length (= current number of Gaussians in cache)
 * 
 * @return the current length of list after the storing.
 * </EN>
 */
static int
gmm_cache_push(GMMCalc *gc, int id, LOGPROB score, int len)
{
  int insertp;

  if (len == 0) {               /* first one */
    gc->OP_calced_score[0] = score;
    gc->OP_calced_id[0] = id;
    return(1);
  }
  if (gc->OP_calced_score[len-1] >= score) { /* bottom */
    if (len < gc->OP_gprune_num) {          /* append to bottom */
      gc->OP_calced_score[len] = score;
      gc->OP_calced_id[len] = id;
      len++;
    }
    return len;
  }
  if (gc->OP_calced_score[0] < score) {
    insertp = 0;
  } else {
    insertp = gmm_find_insert_point(gc, score, len);
  }
  if (len < gc->OP_gprune_num) {
    memmove(&(gc->OP_calced_score[insertp+1]), &(gc->OP_calced_score[insertp]), sizeof(LOGPROB)*(len - insertp));
    memmove(&(gc->OP_calced_id[insertp+1]), &(gc->OP_calced_id[insertp]), sizeof(int)*(len - insertp));    
  } else if (insertp < len - 1) {
    memmove(&(gc->OP_calced_score[insertp+1]), &(gc->OP_calced_score[insertp]), sizeof(LOGPROB)*(len - insertp - 1));
    memmove(&(gc->OP_calced_id[insertp+1]), &(gc->OP_calced_id[insertp]), sizeof(int)*(len - insertp - 1));
  }
  gc->OP_calced_score[insertp] = score;
  gc->OP_calced_id[insertp] = id;
  if (len < gc->OP_gprune_num) len++;
  return(len);
}

/** 
 * <JA>
 * 現在のフレームの入力ベクトルに対する Gaussian の出力確率を計算する. 
 * Gaussian pruning は行なわない. 
 * 
 * @param gc [i/o] GMM計算用ワークエリア
 * @param binfo [in] Gaussian
 * 
 * @return 出力確率の対数値
 * </JA>
 * <EN>
 * Compute an output probability of a Gaussian for the input vector of
 * current frame.  No Gaussian pruning is performed in this function.
 * 
 * @param gc [i/o] work area for GMM calculation
 * @param binfo [in] Gaussian
 * 
 * @return the log output probability.
 * </EN>
 */
static LOGPROB
gmm_compute_g_base(GMMCalc *gc, HTK_HMM_Dens *binfo)
{
  VECT tmp, x;
  VECT *mean;
  VECT *var;
  VECT *vec = gc->OP_vec;
  short veclen = gc->OP_veclen;

  if (binfo == NULL) return(LOG_ZERO);
  mean = binfo->mean;
  var = binfo->var->vec;
  tmp = 0.0;
  for (; veclen > 0; veclen--) {
    x = *(vec++) - *(mean++);
    tmp += x * x * *(var++);
  }
  return((tmp + binfo->gconst) * -0.5);
}

/** 
 * <JA>
 * 現在のフレームの入力ベクトルに対する Gaussian の出力確率を計算する. 
 * 計算時には固定しきい値による safe pruning を行なう. 
 * 
 * @param gc [i/o] GMM計算用ワークエリア
 * @param binfo [in] Gaussian
 * @param thres [in] safe pruning のための枝刈りしきい値
 * 
 * @return 出力確率の対数値
 * </JA>
 * <EN>
 * Compute an output probability of a Gaussian for the input vector of
 * current frame.  Safe pruning is performed in this function.
 * 
 * @param gc [i/o] work area for GMM calculation
 * @param binfo [in] Gaussian
 * @param thres [in] pruning threshold for safe pruning
 * 
 * @return the log output probability.
 * </EN>
 */
static LOGPROB
gmm_compute_g_safe(GMMCalc *gc, HTK_HMM_Dens *binfo, LOGPROB thres)
{
  VECT tmp, x;
  VECT *mean;
  VECT *var;
  VECT *vec = gc->OP_vec;
  short veclen = gc->OP_veclen;
  VECT fthres = thres * (-2.0);

  if (binfo == NULL) return(LOG_ZERO);
  mean = binfo->mean;
  var = binfo->var->vec;
  tmp = binfo->gconst;
  for (; veclen > 0; veclen--) {
    x = *(vec++) - *(mean++);
    tmp += x * x * *(var++);
    if (tmp > fthres)  return LOG_ZERO;
  }
  return(tmp * -0.5);
}

/** 
 * <JA>
 * GMM計算における Gaussian pruning のためのワークエリアを確保する
 * 
 * @param gc [i/o] GMM計算用ワークエリア
 * @param hmminfo [in] HMM 構造体
 * @param prune_num [in] Gaussian pruning において計算する上位ガウス分布数
 * </JA>
 * <EN>
 * Allocate work area for Gaussian pruning for GMM calculation.
 * 
 * @param gc [i/o] work area for GMM calculation
 * @param hmminfo [in] HMM structure
 * @param prune_num [in] number of top Gaussians to be computed at the pruning
 * </EN>
 */
static void
gmm_gprune_safe_init(GMMCalc *gc, HTK_HMM_INFO *hmminfo, int prune_num)
{
  /* store the pruning num to local area */
  gc->OP_gprune_num = prune_num;
  /* maximum Gaussian set size = maximum mixture size * nstream */
  gc->OP_calced_maxnum = hmminfo->maxmixturenum * gc->OP_nstream;
  /* allocate memory for storing list of currently computed Gaussian in a frame */
  gc->OP_calced_score = (LOGPROB *)mymalloc(sizeof(LOGPROB) * gc->OP_calced_maxnum);
  gc->OP_calced_id = (int *)mymalloc(sizeof(int) * gc->OP_calced_maxnum);
}

/** 
 * <JA>
 * @brief  ガウス分布集合内の各ガウス分布の現フレームに対する出力確率を計算する. 
 *
 * Gaussian pruning により，実際には上位 N 個のみを保証する枝刈りが行なわれ，
 * スコアの低いガウス分布は計算されない. 
 *
 * 計算結果は計算済みGaussianリスト (OP_calced_score, OP_calced_id) に
 * 格納される. 
 * 
 * @param gc [i/o] GMM計算用ワークエリア
 * @param g [in] ガウス分布集合
 * @param gnum [in] @a g の長さ
 * </JA>
 * <EN>
 * @brief  Compute scores for a set of Gaussians with Gaussian pruning for
 * the current frame.
 *
 * Gaussian pruning will be performed to guarantee only the top N Gaussians
 * to be fully computed.  The results will be stored in the list of
 * computed Gaussians in OP_calced_score and OP_calced_id.
 * 
 * @param gc [i/o] work area for GMM calculation
 * @param g [in] set of Gaussians
 * @param gnum [in] length of @a g
 * </EN>
 */
static void
gmm_gprune_safe(GMMCalc *gc, HTK_HMM_Dens **g, int gnum)
{
  int i, num = 0;
  LOGPROB score, thres;

  thres = LOG_ZERO;
  for (i = 0; i < gnum; i++) {
    if (num < gc->OP_gprune_num) {
      score = gmm_compute_g_base(gc, g[i]);
    } else {
      score = gmm_compute_g_safe(gc, g[i], thres);
      if (score <= thres) continue;
    }
    num = gmm_cache_push(gc, i, score, num);
    thres = gc->OP_calced_score[num-1];
  }
  gc->OP_calced_num = num;
}

/** 
 * <JA>
 * あるGMM状態の現フレームに対する出力確率を計算する. 
 * 
 * @param gc [i/o] GMM計算用ワークエリア
 * @param state [in] GMM 状態
 * 
 * @return 出力確率の対数スコア
 * </JA>
 * <EN>
 * Compute the output probability of a GMM state for the current frame.
 * 
 * @param gc [i/o] work area for GMM calculation
 * @param state [in] GMM state
 * 
 * @return the log probability.
 * </EN>
 */
static LOGPROB
gmm_calc_mix(GMMCalc *gc, HTK_HMM_State *state)
{
  int i;
  LOGPROB logprob, logprobsum;
  int s;
  PROB stream_weight;


  /* compute Gaussian set */
  logprobsum = 0.0;
  for(s=0;s<gc->OP_nstream;s++) {
    /* set stream weight */
    if (state->w) stream_weight = state->w->weight[s];
    else stream_weight = 1.0;
    /* setup storage pointer for this mixture pdf */
    gc->OP_vec = gc->OP_vec_stream[s];
    gc->OP_veclen = gc->OP_veclen_stream[s];
    /* compute output probabilities */
    gmm_gprune_safe(gc, state->pdf[s]->b, state->pdf[s]->mix_num);
    /* computed Gaussians will be set in:
       score ... OP_calced_score[0..OP_calced_num]
       id    ... OP_calced_id[0..OP_calced_num] */
  /* sum */
    for(i=0;i<gc->OP_calced_num;i++) {
      gc->OP_calced_score[i] += state->pdf[s]->bweight[gc->OP_calced_id[i]];
    }
    /* add log probs */
    logprob = addlog_array(gc->OP_calced_score, gc->OP_calced_num);
    /* if outprob of a stream is zero, skip this stream */
    if (logprob <= LOG_ZERO) continue;
    /* sum all the obtained mixture scores */
    logprobsum += logprob * stream_weight;

  }
  if (logprobsum == 0.0) return(LOG_ZERO); /* no valid stream */
  if (logprobsum <= LOG_ZERO) return(LOG_ZERO);	/* lowest == LOG_ZERO */
  return (logprob * INV_LOG_TEN);
}

/** 
 * <JA>
 * 入力の指定フレームにおけるGMM状態のスコアを求めるメイン関数. 
 * 
 * @param gc [i/o] GMM計算用ワークエリア
 * @param t [in] 計算するフレーム
 * @param stateinfo [in] GMM状態
 * @param param [in] 入力ベクトル系列
 * 
 * @return 出力確率の対数スコア
 * </JA>
 * <EN>
 * Main function to compute the output probability of a GMM state for
 * the specified input frame.
 * 
 * @param gc [i/o] work area for GMM calculation
 * @param t [in] time frame on which the output probability should be computed
 * @param stateinfo [in] GMM state
 * @param param [in] input vector sequence
 * 
 * @return the log output probability.
 * </EN>
 */
static LOGPROB
outprob_state_nocache(GMMCalc *gc, int t, HTK_HMM_State *stateinfo, HTK_Param *param)
{
  int d, i;
  /* set global values for outprob functions to access them */
  for(d=0,i=0;i<gc->OP_nstream;i++) {
    gc->OP_vec_stream[i] = &(param->parvec[t][d]);
    d += gc->OP_veclen_stream[i];
  }
  return(gmm_calc_mix(gc, stateinfo));
}

/************************************************************************/
/* global functions */

/** 
 * <JA>
 * GMMの計算のための初期化. 起動時に一度だけ呼ばれる. 
 * 
 * @param recog [i/o] エンジンインスタンス
 * </JA>
 * <EN>
 * Initialization for computing GMM likelihoods.  This will be called
 * once on startup.
 * 
 * @param recog [i/o] engine instance
 * </EN>
 *
 * @callgraph
 * @callergraph
 * 
 */
boolean
gmm_init(Recog *recog)
{
  HTK_HMM_INFO *gmm;
  HTK_HMM_Data *d;
  GMMCalc *gc;
  int i;

  gmm = recog->gmm;

  /* check GMM format */
  /* tied-mixture GMM is not supported */
  if (gmm->is_tied_mixture) {
    jlog("ERROR: gmm_init: tied-mixture GMM is not supported\n");
    return FALSE;
  }
  /* assume 3 state GMM (only one output state) */
  for(d=gmm->start;d;d=d->next) {
    if (d->state_num > 3) {
      jlog("ERROR: gmm_init: more than three states (one output state) defined in GMM [%s]\n", d->name);
      return FALSE;
    }
  }

  /* check if CMN needed */

  /* allocate work area */
  if (recog->gc == NULL) {
    gc = (GMMCalc *)mymalloc(sizeof(GMMCalc));
    recog->gc = gc;
  } else {
    gc = recog->gc;
  }
  
  /* allocate buffers */
  gc->gmm_score = (LOGPROB *)mymalloc(sizeof(LOGPROB) * gmm->totalhmmnum);

#ifdef GMM_VAD
  gc->nframe = recog->jconf->detect.gmm_margin;
  gc->rates = (LOGPROB *)mymalloc(sizeof(LOGPROB) * gc->nframe);
#endif

  gc->is_voice = (boolean *)mymalloc(sizeof(boolean) * gmm->totalhmmnum);
  i = 0;
  if (recog->jconf->reject.gmm_reject_cmn_string) {
    for(d=recog->gmm->start;d;d=d->next) {
      if (strstr(recog->jconf->reject.gmm_reject_cmn_string, d->name)) {
	gc->is_voice[i] = FALSE;
      } else {
	gc->is_voice[i] = TRUE;
      }
      i++;
    }
  } else {
    for(d=recog->gmm->start;d;d=d->next) {
      gc->is_voice[i] = TRUE;
      i++;
    }
  }

  /* initialize work area */
  gc->OP_nstream = gmm->opt.stream_info.num;
  for(i=0;i<gc->OP_nstream;i++) {
    gc->OP_veclen_stream[i] = gmm->opt.stream_info.vsize[i];
  }
  gmm_gprune_safe_init(gc, gmm, recog->jconf->reject.gmm_gprune_num);

  /* check if variances are inversed */
  if (!gmm->variance_inversed) {
    /* here, inverse all variance values for faster computation */
    htk_hmm_inverse_variances(gmm);
    gmm->variance_inversed = TRUE;
  }

  return TRUE;
}

/** 
 * <JA>
 * GMM計算のための準備を行なう. １入力開始ごとに呼ばれる. 
 * 
 * @param recog [i/o] エンジンインスタンス
 * </JA>
 * <EN>
 * Prepare for the next GMM computation.  This will be called just before
 * an input begins.
 * 
 * @param recog [i/o] engine instance
 * </EN>
 * 
 * @callgraph
 * @callergraph
 */
void
gmm_prepare(Recog *recog)
{
  HTK_HMM_Data *d;
  int i;

  /* initialize score buffer and frame count */
  i = 0;
  for(d=recog->gmm->start;d;d=d->next) {
    recog->gc->gmm_score[i] = 0.0;
    i++;
  }
#ifdef GMM_VAD
  for(i=0;i<recog->gc->nframe;i++) recog->gc->rates[i] = 0.0;
  recog->gc->framep = 0;
  recog->gc->filled = FALSE;
  recog->gc->in_voice = FALSE;
#endif

  recog->gc->framecount = 0;

#ifdef GMM_VAD_DEBUG
  printf("GMM_VAD: init\n");
#endif
}

/** 
 * <JA>
 * 与えられた入力ベクトル列上のあるフレームについて，全GMMのスコアを計算し，
 * 計算結果を gmm_score に積算する. 
 *
 * GMM_VAD 定義時は，後で VAD 判定するために，過去 jconf->detect.gmm_margin
 * フレーム分の VAD スコア （音声GMMの最大スコア - 雑音GMMの最大スコア）が
 * 保存される. 
 * 
 * @param recog [i/o] エンジンインスタンス
 * </JA>
 * <EN>
 * Compute output probabilities of all GMM for a given input vector, and
 * accumulate the results to the gmm_score buffer.
 *
 * When GMM_VAD is defined, VAD scores,
 * "(maximum score of speech GMMs) - (maximum score of noise GMMs)" of
 * last frames (jconf->detect.gmm_margin) will be stored for later VAD
 * decision.
 * 
 * @param recog [i/o] engine instance
 * </EN>
 *
 * @callgraph
 * @callergraph
 */
void
gmm_proceed(Recog *recog)
{
  HTK_HMM_Data *d;
  GMMCalc *gc;
  int i;
  MFCCCalc *mfcc;
  LOGPROB score;
#ifdef GMM_VAD
  LOGPROB max_n;
  LOGPROB max_v;
#endif

  mfcc = recog->gmmmfcc;
  gc = recog->gc;

  if (!mfcc->valid) return;

  gc->framecount++;

#ifdef GMM_VAD
  max_n = max_v = LOG_ZERO;
#endif

  i = 0;
  for(d=recog->gmm->start;d;d=d->next) {
    score = outprob_state_nocache(gc, mfcc->f, d->s[1], mfcc->param);
    gc->gmm_score[i] += score;
#ifdef GMM_VAD
    if (gc->is_voice[i]) {
      if (max_v < score) max_v = score;
    } else {
      if (max_n < score) max_n = score;
    }
#endif
#ifdef MES
    jlog("DEBUG: [%s: total=%f avg=%f]\n", d->name, gc->gmm_score[i], gc->gmm_score[i] / (float)gc->framecount);
#endif
    i++;
  }
#ifdef GMM_VAD
#ifdef GMM_VAD_DEBUG
  //printf("GMM_VAD: max_v = %f, max_n = %f, rate = %f\n", max_v, max_n, max_v - max_n, gc->framep);
#endif
  /* set rate of this frame */
  gc->rates[gc->framep] = max_v - max_n;
#ifdef GMM_VAD_DEBUG
  printf("GMM_VAD: %f\n", max_v - max_n);
#endif
  /* increment current frame pointer */
  gc->framep++;
  /* if reached end, go to start point */
  if (gc->framep >= gc->nframe) {
    gc->filled = TRUE;
    gc->framep = 0;
  }
#endif
}

/** 
 * <JA>
 * @brief  GMMの計算を終了し，結果を出力する. 
 *
 * gmm_proceed() によって累積された各フレームごとのスコアから，
 * 最大スコアのGMMを決定する. その事後確率に基づく信頼度を計算し
 * 最終的な結果を result_gmm() によって出力する. 
 * 
 * @param recog [i/o] エンジンインスタンス
 * </JA>
 * <EN>
 * @brief  Finish the GMM computation for an input, and output the result.
 *
 * The GMM of the maximum score is finally determined from the accumulated
 * scores computed by gmm_proceed(), and compute the confidence score of the
 * maximum GMM using posterior probability.  Then the result will be output
 * using result_gmm().
 * 
 * @param recog [i/o] engine instance
 * </EN>
 * 
 * @callgraph
 * @callergraph
 */
void
gmm_end(Recog *recog)
{
  HTK_HMM_INFO *gmm;
  LOGPROB *score;
  HTK_HMM_Data *d;
  LOGPROB maxprob;
  HTK_HMM_Data *dmax;
#ifdef CONFIDENCE_MEASURE
  LOGPROB sum;
#endif
  int i;
  int maxid;

  if (recog->gc->framecount == 0) return;

  gmm = recog->gmm;
  score = recog->gc->gmm_score;

  /* get max score */
  i = 0;
  maxprob = LOG_ZERO;
  dmax = NULL;
  maxid = 0;
  for(d=gmm->start;d;d=d->next) {
    if (maxprob < score[i]) {
      dmax = d;
      maxprob = score[i];
      maxid = i;
    }
    i++;
  }
  recog->gc->max_d = dmax;
  recog->gc->max_i = maxid;

#ifdef CONFIDENCE_MEASURE
  /* compute CM */
  sum = 0.0;
  i = 0;
  for(d=gmm->start;d;d=d->next) {
    //sum += pow(10, recog->jconf->annotate.cm_alpha * (score[i] - maxprob));
    sum += pow(10, 0.05 * (score[i] - maxprob));
    i++;
  }
  recog->gc->gmm_max_cm = 1.0 / sum;
#endif
  
  /* output result */
  callback_exec(CALLBACK_RESULT_GMM, recog);

}


/** 
 * <JA>
 * GMMの識別結果，最後の入力が音声入力として有効であったか
 * 無効であったかを返す. 
 *
 * @param recog [i/o] エンジンインスタンス
 * 
 * @return 一位のGMMの名前が gmm_reject_cmn_string 内に無ければ valid として
 * TRUE, あれば invalid として FALSE を返す. 
 * </JA>
 * <EN>
 * Return whether the last input was valid or invalid, from the result of
 * GMM computation.
 *
 * @param recog [i/o] engine instance
 * 
 * @return TRUE if input is valid, i.e. the name of maximum GMM is not included
 * in gmm_reject_cmn_string, or FALSE if input is invalid, i.e. the name is
 * included in that string.
 * </EN>
 * 
 * @callgraph
 * @callergraph
 */
boolean
gmm_valid_input(Recog *recog)
{
  if (recog->gc->max_d == NULL) return FALSE;
  if (recog->gc->is_voice[recog->gc->max_i]) {
    return TRUE;
  }
  return FALSE;
}

/** 
 * <EN>
 * Free work area used for GMM calculation.
 * </EN>
 * <JA>
 * GMM計算に用いたワークエリアを開放する. 
 * </JA>
 * 
 * @param recog [i/o] engine instance
 * 
 * @callgraph
 * @callergraph
 * 
 */
void
gmm_free(Recog *recog)
{
  if (recog->gc) {
    free(recog->gc->OP_calced_score);
    free(recog->gc->OP_calced_id);
    free(recog->gc->is_voice);
#ifdef GMM_VAD
    free(recog->gc->rates);
#endif
    free(recog->gc->gmm_score);
    free(recog->gc);
    recog->gc = NULL;
  }
}

#ifdef GMM_VAD

/** 
 * <EN>
 * Compute score of voice activity from the last (jconf->detect.gmm_margin)
 * frames.  Positive value designates speech, and negative means noise.
 * </EN>
 * <JA>
 * 直前の (jconf->detect.gmm_margin) フレーム分のスコアから
 * voice activity のスコアを計算する. 正の値は音声，負の値は雑音を表す. 
 * </JA>
 * 
 * @param gc [i/o] work area for GMM calculation
 * @param mean_ret [out] mean value of last (jconf->detect.gmm_margin) frames
 * @param var_ret [out] variance of last (jconf->detect.gmm_margin) frames
 * @param count_ret [out] count of speech frames in last (jconf->detect.gmm_margin) frames
 * 
 */
static void
voice_activity_score(GMMCalc *gc, float *mean_ret, float *var_ret, int *count_ret)
{
  int i, len;
  LOGPROB mean;
  LOGPROB var;
  LOGPROB x;
  int count;

  if (!gc->filled) {
    /* cycle buffer not filled yet */
    *mean_ret = 0.0;
    *var_ret = 0.0;
    *count_ret = 0;
    return;
  }

  if (gc->filled) {
    len = gc->nframe;
  } else {
    len = gc->framep;
  }

  mean = 0;
  count = 0;
  for(i=0;i<len;i++) {
    mean += gc->rates[i];
    if (gc->rates[i] > 0.0) count++;
  }
  mean /= (float)len;
  var = 0.0;
  for(i=0;i<len;i++) {
    x = mean - gc->rates[i];
    var += x * x;
  }
  var /= (float)len;

  *mean_ret = mean;
  *var_ret = var;
  *count_ret = count;
}

/** 
 * <EN>
 * Check if trigger of speech / noise segment.  If we are in noise segment
 * and some speech input begins at this frame, recog->gc->up_trigger will
 * be set to TRUE.  If current is in speech segment and it ended at
 * this frame, recog->gc->down_trigger will be set to FALSE.
 * </EN>
 * <JA>
 * 音声/非音声区間の区切りを検知する. これまでが非音声区間でこのフレームで
 * 音声トリガを検知したとき，recog->gc->up_trigger を TRUE にセットする. 現在
 * 音声区間で区間終了を検知したとき，recog->gc->down_trigger を TRUE に
 * セットする. 
 * </JA>
 * 
 * @param recog [i/o] engine instance
 * 
 * @callgraph
 * @callergraph
 */
void
gmm_check_trigger(Recog *recog)
{
  GMMCalc *gc;
  gc = recog->gc;
  float mean;
  float var;
  int count;

  gc->up_trigger = gc->down_trigger = FALSE;

  voice_activity_score(gc, &mean, &var, &count);

  if (gc->in_voice) {
    if (mean <= recog->jconf->detect.gmm_downtrigger_thres) {
      gc->down_trigger = TRUE;
      gc->in_voice = FALSE;
    }
  } else {
    if (mean >= recog->jconf->detect.gmm_uptrigger_thres) {
      gc->up_trigger = TRUE;
      gc->in_voice = TRUE;
    }
  }

#ifdef GMM_VAD_DEBUG
  printf("GMM_VAD: %s: %f %f %d", gc->in_voice ? "VOICE" : "NOISE", mean, var, count);
  if (gc->up_trigger) printf(": BEGIN");
  if (gc->down_trigger) printf(": END");
  printf("\n");
#endif

}
#endif /* GMM_VAD */

/* end of file */
