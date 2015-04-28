/**
 * @file   mbr.c
 * 
 * <JA>
 * @brief  認識された文をMBRの枠組みでリスコア
 *
 * 認識された文をMBRの枠組みでリスコアする．
 * </JA>
 *
 * <EN>
 * @brief  Rescoring N-best sentences using MBR framework
 * </EN>
 * 
 * @author Hiroaki NANJO, Ryo FURUTANI
 * @date   28 March 2011
 *
 * $Revision: 1.3 $
 * 
 */
/*
 * Copyright (c) 2011-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius/julius.h>

#ifdef USE_MBR

/** 
 * <JA>
 * MBRスコアでソートするための qsort コールバック関数. 
 * 
 * @param a [in] 要素1
 * @param b [in] 要素2
 * 
 * @return 演算の結果の符合を返す. 
 * </JA>
 */

static int
mbr_cmp(NODE **a, NODE **b)
{
  if ((*a)->score_mbr > (*b)->score_mbr) return 1;
  if ((*a)->score_mbr < (*b)->score_mbr) return -1;
  return 0;
}


/** 
 * <JA>
 * DPマッチングの結果を出力するデバッグ関数．
 * 
 * @param d [in] DPマッチングの結果
 * @param len1 [in] 要素1の要素数
 * @param len2 [in] 要素2の要素数
 * 
 * @return 最短距離
 * </JA>
 */

static int
dp_print(DP *d, int len1, int len2)
{
  int i, j;

  jlog("Distance\n");
  for(i = 0; i < len1; i++){
    for(j = 0; j < len2; j++){
      jlog("%d ", d[j * len1 + i].d);
    }
    jlog("\n");
  }

  jlog("\nTransition from\n");
  for(i = 0; i < len1; i++){
    for(j = 0; j < len2; j++){
      jlog("%d ", d[j * len1 + i].r);
    }
    jlog("\n");
  }

  jlog("\nCost\n");
  for(i = 0; i < len1; i++){
    for(j = 0; j < len2; j++){
      jlog("%d ", d[j * len1 + i].c);
    }
    jlog("\n");
  }

  return d[len1 * len2 - 1].d;
}


/** 
 * <JA>
 * 
 * 最もコストが小さいパスを返す．
 * 
 * @param a [in] 値1
 * @param b [in] 値2
 * @param c [in] 値3
 * @param cost [in] 遷移コスト
 * 
 * @return コストと遷移元．
 * </JA>
 */

static DP
dppath(int a, int b, int c, int cost)
{
  DP min;

  if (a < b){
    min.d = a;
    min.r = 1;
  }
  else{
    min.d = b;
    min.r = 2;
  }

  if (c < min.d){
    min.d = c;
    min.r = 3;
  }

  min.c = cost;

  return min;
}


/** 
 * <JA>
 * 
 * DPマッチングを行う．
 * 
 * @param a [in] 要素1
 * @param b [in] 要素2
 * @param w [in] 言語モデル
 * 
 * @return DPマッチングの結果．
 * </JA>
 */

static DP*
dpmatch(NODE *a, NODE *b, WORD_INFO *winfo)
{
  int len1, len2;
  DP *d;
  int i, j;
  int cost;

  char *c1, *c2;

  len1 = a->seqnum + 1;
  len2 = b->seqnum + 1;

  d = (DP *)mymalloc(sizeof(DP) * len1 * len2);

  d[0].d = 0;
  d[0].r = 0;
  d[0].c = 0;

  for(i = 1; i < len1; i++){

    d[i].d = i;
    d[i].r = 1;
    d[i].c = 0;
  }

  for(i = 1; i < len2; i++){

    d[i * len1].d = i;
    d[i * len1].r = 2;
    d[i * len1].c = 0;
  }

  for(i = 1; i < len1; i++){

    c1 = winfo->woutput[a->seq[i - 1]];

    for(j = 1; j < len2; j++){

      c2 = winfo->woutput[b->seq[j - 1]];

      if (strmatch(c1, c2)) {

	cost = 0;
      }
      else {

	cost = 1;
      }

      d[j * len1 + i] = dppath(d[j * len1 + (i - 1)].d + 1,
			       d[(j - 1) * len1 + i].d + 1,
			       d[(j - 1) * len1 + (i - 1)].d + cost,
			       cost);
     }
  }

  if(debug2_flag){
    dp_print(d, len1, len2);
  }

  return d;
}

static float
get_weight(WORD_INFO *winfo, WORD_ID id)
{
  float val;

  if (winfo->weight) {
    /* word-level weight exist, return the value */
    val = winfo->weight[id];
  } else {
    /* no word-level weight, return default value */
    val = 1.0;
  }
  return val;
}


/** 
 * <JA>
 * Weighed Levenstein distanceを計算する．
 * 
 * @param a [in] 要素1
 * @param b [in] 要素2
 * @param w [in] 言語モデル
 * 
 * @return Weighed Levenstein distanceの値を返す．
 * </JA>
 */

static float
calc_wld(NODE *a, NODE *b, WORD_INFO *winfo)
{
  float weight, error1, error2;
  DP *d;
  int i, j, now;

  /* DPマッチングのパスを求める */
  d = dpmatch(a, b, winfo);

  weight = 0.0;
  i = a->seqnum;
  j = b->seqnum;

  /* バックトレースしつつ重みを確定 */
  if(d[i * j - 1].d > 0){

    error1 = 0.0;
    error2 = 0.0;

    while(i > 0 || j > 0){

      now = j * (a->seqnum + 1) + i;

      if(d[now].r == 1){
	/* Deletion error */
	error1 += get_weight(winfo, a->seq[i - 1]);
	i--;
      }
      else if(d[now].r == 2){
	/* Insertion error */
	error2 += get_weight(winfo, b->seq[j - 1]);
	j--;
      }
      else if(d[now].r == 3){
	if(d[now].c == 1){
	  /* Substitution error */
	  error1 += get_weight(winfo, a->seq[i - 1]);
	  error2 += get_weight(winfo, b->seq[j - 1]);
	}
	else if(d[now].c == 0){
	  /* Correct word */
	  weight += error1 > error2 ? error1 : error2;
	  error1 = 0.0;
	  error2 = 0.0;
	}
	else{
	  jlog("Error: calc_wld: cannot calculation Weighted Levenstein distance: cost error\n");
	  return -1.0;
	}

	i--;
	j--;
      }
      else{
	jlog("Error: calc_wld: cannot calculation Weighted Levenstein distance: table error: i = %d, j = %d\n", i, j);
	return -1.0;
      }
    }

    weight += error1 > error2 ? error1 : error2;
  }

  free(d);

  return weight;
}


/** 
 * <JA>
 * Levenstein distanceを計算する．
 * 
 * @param a [in] 要素1
 * @param b [in] 要素2
 * @param w [in] 言語モデル
 * 
 * @return Levenstein distanceの値を返す．
 * </JA>
 */

static int
calc_ld(NODE *a, NODE *b, WORD_INFO *winfo)
{
  int distance;
  DP *d;

  /* DPマッチングのパスを求める */
  d = dpmatch(a, b, winfo);

  distance = d[(a->seqnum + 1) * (b->seqnum + 1) - 1].d;

  free(d);

  return distance;
}


/** 
 * <JA>
 * 音声認識スコアを正規化する．
 * MBRスコアの初期化も同時に行う．
 * 
 * @param table [in] スタックテーブル
 * @param r_stacknum [in] スタックのデータ数
 * @param r [in] 認識インスタンス
 * 
 * @return 正規化されたスコアの配列を返す．
 * </JA>
 */

static float *
normalization_score(NODE **table, int r_stacknum, RecogProcess *r)
{
  float *n_score;
  int i;
  float max;

  n_score = (float *)mymalloc(sizeof(float) * r_stacknum);

  /* 最も高いスコアで正規化する */
  max = table[0]->score;
  n_score[0] = 1.0;
  table[0]->score_mbr = 0.0;

  if(debug2_flag){
    jlog("n_score[0] = %f\n", n_score[0]);
  }

  for(i = 1; i < r_stacknum; i++){

    n_score[i] = pow(10, (table[i]->score - max) * r->config->mbr.score_weight);
    table[i]->score_mbr = 0.0;

    if(debug2_flag){
      jlog("n_score[%d] = %f\n", i, n_score[i]);
    }
  }

  return n_score;
}


/** 
 * <JA>
 * @brief  MBR処理のメイン関数
 *
 * 認識した文をMBRの枠組みでリスコアする．
 * 
 * @param r_start [i/o] 結果格納用スタックの先頭ノードへのポインタ
 * @param r_bottom [i/o] 結果格納用スタックの底ノードへのポインタ
 * @param r_stacknum [in] スタックに格納されているノード数へのポインタ
 * @param r [in] 認識処理インスタンス
 * </JA>
 *
 * @callgraph
 * @callergraph
 * 
 */


void 
candidate_mbr(NODE **r_start, NODE **r_bottom, int r_stacknum, RecogProcess *r)
{
  JCONF_SEARCH *jconf = r->config;
  NODE **table;
  NODE *now;

  int i, j;
  int dist;

  float *n_score;
  float error;

  /* リストのままでは扱いにくいので配列に変換 */
  table = (NODE **)mymalloc(sizeof(NODE *) * r_stacknum);
  i = 0;
  for(now = *r_start; now; now = now->next){
    table[i] = now;
    i++;
  }

  /* 認識スコア（ゆう度）を正規化 */
  n_score = normalization_score(table, r_stacknum, r);

  /* MBRスコアを計算 */
  for(i = 0; i < r_stacknum - 1; i++){
    for(j = i + 1; j < r_stacknum; j++){

      if(jconf->mbr.use_word_weight){
	/* 損失関数はWeighted Levenstein distance */
	if((error = calc_wld(table[i], table[j], r->lm->winfo)) < 0.0){
	  jlog("Error: candidate_mbr: cannot calculation Weighted Levenstein distance\n");
	  free(n_score);
	  free(table);
	  return;
	}

	error = pow(error, jconf->mbr.loss_weight);
      }
      else{
	/* 損失関数はLevenstein distance */
	if((dist = calc_ld(table[i], table[j], r->lm->winfo)) < 0){
	  jlog("Error: candidate_mbr: cannot calculation Levenstein distance\n");
	  free(n_score);
	  free(table);
	  return;
	}

	error = pow(dist, jconf->mbr.loss_weight);
      }

      table[i]->score_mbr += n_score[j] * error;
      table[j]->score_mbr += n_score[i] * error;

      if(debug2_flag){
	jlog("i = %d, j = %d\n", i, j);
	jlog("error = %f\n", error);
	jlog("n_score[%d] * error = %f\n", j, n_score[j] * error);
	jlog("table[%d]->score_mbr = %f\n", i, table[i]->score_mbr);
	jlog("n_score[%d] * error = %f\n", i, n_score[i] * error);
	jlog("table[%d]->score_mbr = %f\n", j, table[j]->score_mbr);
      }
    }
  }

  /* 結果をリスコア */
  qsort(table, r_stacknum, sizeof(NODE *),
	(int (*)(const void *, const void *))mbr_cmp);

  /* 配列をリストに変換 */
  *r_start = table[0];
  (*r_start)->prev = NULL;
  for(i = 1; i < r_stacknum; i++){
    table[i]->prev = table[i - 1];
    table[i - 1]->next = table[i];
  }
  *r_bottom = table[r_stacknum - 1];
  (*r_bottom)->next = NULL;

  free(n_score);
  free(table);

  return;
}

#endif  /* USE_MBR */
