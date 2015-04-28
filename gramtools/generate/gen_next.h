/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __NEXT__H__
#define __NEXT__H__

#include <sent/stddefs.h>
#include <sent/vocabulary.h>
#include <sent/dfa.h>
#include <sent/speech.h>

/* ported from julian/dfa_decode.c */
/* 次単語候補 */
typedef struct __nextword__ {
  WORD_ID id;			/* 単語ID */
  int next_state;		/* 遷移後のDFA状態番号 */
  boolean can_insert_sp;	/* 仮説とこの単語の間にspが入る可能性がある場合 TRUE */
} NEXTWORD;
/* 部分文仮説 */
typedef struct __node__ {
  boolean endflag;              /* 探索終了フラグ */
  WORD_ID seq[MAXSEQNUM];       /* 仮説の単語系列 */
  short seqnum;                 /* 仮説の単語の数 */
  int state;                    /* 現在のDFA状態番号 */
} NODE;

NEXTWORD **nw_malloc();
void nw_free(NEXTWORD **nw);
int dfa_firstwords(NEXTWORD **nw);
int dfa_nextwords(NODE *hypo, NEXTWORD **nw);
int dfa_firstterms(NEXTWORD **nw);
int dfa_nextterms(NODE *hypo, NEXTWORD **nw);
boolean dfa_acceptable(NODE *hypo);


#endif /* __NEXT__H__ */
