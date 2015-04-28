/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* next_word functions */

#include "common.h"
#include "gen_next.h"

extern WORD_INFO *winfo;
extern DFA_INFO *dfa;

NEXTWORD **
nw_malloc()
{
  NEXTWORD **nw;
  NEXTWORD *nwtmp;
  int i;
  int maxnw;

  maxnw = winfo->num * 2;	/* NOISEを飛ばす分 */
  /* 連続領域を配列に割り当てる */
  nw = (NEXTWORD **)malloc(maxnw * sizeof(NEXTWORD *));
  nwtmp = (NEXTWORD *)malloc(maxnw * sizeof(NEXTWORD));
  nw[0] = nwtmp;
  for (i=1;i<maxnw; i++) {
    nw[i] = &(nwtmp[i]);
  }
  return nw;
}

/* 予測次単語格納領域の解放 */
void
nw_free(NEXTWORD **nw)
{
  free(nw[0]);
  free(nw);
}



/* 初期状態から遷移しうる単語集合を返す */
/* 返り値: 単語数*/
/* NOISE: ここには来ない仕様 */
int
dfa_firstwords(NEXTWORD **nw)
{
  DFA_ARC *arc;
  int i, cate, iw, ns;
  int num = 0;

  for (i=0;i<dfa->state_num;i++) {
    if ((dfa->st[i].status & INITIAL_S) != 0) { /* 初期状態から */
      for (arc = dfa->st[i].arc; arc; arc = arc->next) {	/* 全ての遷移 */
	cate = arc->label;
	ns = arc->to_state;
	/* 遷移に対応するカテゴリ内の全単語を展開 */
	for (iw=0;iw<dfa->term.wnum[cate];iw++) {
	  nw[num]->id = dfa->term.tw[cate][iw];
	  nw[num]->next_state = ns;
	  nw[num]->can_insert_sp = FALSE;
	  num++;
	}
      }
    }
  }

  return num;
}
int
dfa_firstterms(NEXTWORD **nw)
{
  DFA_ARC *arc;
  int i, cate, ns;
  int num = 0;

  for (i=0;i<dfa->state_num;i++) {
    if ((dfa->st[i].status & INITIAL_S) != 0) { /* 初期状態から */
      for (arc = dfa->st[i].arc; arc; arc = arc->next) {	/* 全ての遷移 */
	cate = arc->label;
	ns = arc->to_state;
	/* 遷移に対応するカテゴリ内の1単語を展開 */
	if (dfa->term.wnum[cate] == 0) continue;
	nw[num]->id = dfa->term.tw[cate][0];
	nw[num]->next_state = ns;
	nw[num]->can_insert_sp = FALSE;
	num++;
      }
    }
  }

  return num;
}

/* 次に接続し得る単語群を返す */
/* 返り値:単語数 */
/* NOISE: 先まで見て，can_insert_sp=TRUEで返す */
int
dfa_nextwords(NODE *hypo, NEXTWORD **nw)
{
  DFA_ARC *arc, *arc2;
  int iw,cate,ns,cate2,ns2;
  int num = 0;

  for (arc = dfa->st[hypo->state].arc; arc; arc = arc->next) {
    cate = arc->label;
    ns = arc->to_state;
    if (dfa->is_sp[cate]) {
      /* 先まで見る。自分は展開しない */
      for (arc2 = dfa->st[ns].arc; arc2; arc2 = arc2->next) {
	cate2 = arc2->label;
	ns2 = arc2->to_state;
	for (iw=0;iw<dfa->term.wnum[cate2];iw++) {
	  nw[num]->id = dfa->term.tw[cate2][iw];
	  nw[num]->next_state = ns2;
	  nw[num]->can_insert_sp = TRUE;
	  num++;
	}
      }
    } else {
      /* 遷移に対応するカテゴリ内の全単語を展開 */
      for (iw=0;iw<dfa->term.wnum[cate];iw++) {
	nw[num]->id = dfa->term.tw[cate][iw];
	nw[num]->next_state = ns;
	nw[num]->can_insert_sp = FALSE;
	num++;
      }
    }
  }
  return num;
}
int
dfa_nextterms(NODE *hypo, NEXTWORD **nw)
{
  DFA_ARC *arc, *arc2;
  int cate,ns,cate2,ns2;
  int num = 0;

  for (arc = dfa->st[hypo->state].arc; arc; arc = arc->next) {
    cate = arc->label;
    ns = arc->to_state;
    if (dfa->is_sp[cate]) {
      /* 先まで見る。自分は展開しない */
      for (arc2 = dfa->st[ns].arc; arc2; arc2 = arc2->next) {
	cate2 = arc2->label;
	ns2 = arc2->to_state;
	if (dfa->term.wnum[cate2] == 0) continue;
	nw[num]->id = dfa->term.tw[cate2][0];
	nw[num]->next_state = ns2;
	nw[num]->can_insert_sp = TRUE;
	num++;
      }
    } else {
      /* 遷移に対応するカテゴリ内の全単語を展開 */
      if (dfa->term.wnum[cate] == 0) continue;
      nw[num]->id = dfa->term.tw[cate][0];
      nw[num]->next_state = ns;
      nw[num]->can_insert_sp = FALSE;
      num++;
    }
  }
  return num;
}

/* 仮説が文として受理可能であるかどうかを返す */
/* NOISE: ここにはこない仕様 */
boolean
dfa_acceptable(NODE *hypo)
{
  /* 受理状態なら */
  if (dfa->st[hypo->state].status & ACCEPT_S) {
    return TRUE;
  } else {
    return FALSE;
  }
}
