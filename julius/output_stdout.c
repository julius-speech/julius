/**
 * @file   output_stdout.c
 * 
 * <JA>
 * @brief  認識結果を標準出力へ出力する. 
 * </JA>
 * 
 * <EN>
 * @brief  Output recoginition result to standard output
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Tue Sep 06 17:18:46 2005
 *
 * $Revision: 1.14 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include "app.h"

extern boolean separate_score_flag;

static boolean have_progout = FALSE;

/* for short pause segmentation and successive decoding */
static WORD_ID confword[MAXSEQNUM];
static int confwordnum;

#ifdef CHARACTER_CONVERSION
#define MAXBUFLEN 4096 ///< Maximum line length of a message sent from a client
static char inbuf[MAXBUFLEN];
static char outbuf[MAXBUFLEN];
void
myprintf(char *fmt, ...)
{
  va_list ap;
  int ret;
  
  va_start(ap,fmt);
  ret = vsnprintf(inbuf, MAXBUFLEN, fmt, ap);
  va_end(ap);
  if (ret > 0) {		/* success */
    printf("%s", charconv(inbuf, outbuf, MAXBUFLEN));
  }
}
#else
#define myprintf printf
#endif

/**
 * Assumed tty width for graph view output
 * 
 */
#define TEXTWIDTH 70

/**
 * tty width for short-pause segmentation output
 * 
 */

#define SPTEXTWIDTH 72
#define SPTEXT_FULLWIDTH 76

/**********************************************************************/
/* process online/offline status  */

/** 
 * <JA>
 * 起動が終わったとき，あるいは中断状態から復帰したときに
 * メッセージを表示する. 
 * 
 * </JA>
 * <EN>
 * Output message when processing was started or resumed.
 * 
 * </EN>
 */
static void
status_process_online(Recog *recog, void *dummy)
{
  /* nop */
}
/** 
 * <JA>
 * プロセスが中断状態へ移行したときにメッセージを表示する. モジュールモード
 * で呼ばれる. 
 * 
 * </JA>
 * <EN>
 * Output message when the process paused by external command.
 * 
 * </EN>
 */
static void
status_process_offline(Recog *recog, void *dummy)
{
  /* nop */
}

/**********************************************************************/
/* output recording status changes */

/** 
 * <JA>
 * 準備が終了して、認識可能状態（入力待ち状態）に入ったときの出力
 * 
 * </JA>
 * <EN>
 * Output when ready to recognize and start waiting speech input.
 * 
 * </EN>
 */
static void
status_recready(Recog *recog, void *dummy)
{
  if (recog->jconf->input.speech_input == SP_MIC || recog->jconf->input.speech_input == SP_NETAUDIO) {
    if (!recog->process_segment) {
      fprintf(stderr, "<<< please speak >>>");
    }
  }
}
/** 
 * <JA>
 * 入力の開始を検出したときの出力
 * 
 * </JA>
 * <EN>
 * Output when input starts.
 * 
 * </EN>
 */
static void
status_recstart(Recog *recog, void *dummy)
{
  if (recog->jconf->input.speech_input == SP_MIC || recog->jconf->input.speech_input == SP_NETAUDIO) {
    if (!recog->process_segment) {
      fprintf(stderr, "\r                    \r");
    }
  }
}
/** 
 * <JA>
 * 入力終了を検出したときの出力
 * 
 * </JA>
 * <EN>
 * Output when input ends.
 * 
 * </EN>
 */
static void
status_recend(Recog *recog, void *dummy)
{
  /* nop */
}
/** 
 * <JA>
 * 入力長などの入力パラメータ情報を出力. 
 * 
 * @param param [in] 入力パラメータ構造体
 * </JA>
 * <EN>
 * Output input parameter status such as length.
 * 
 * @param param [in] input parameter structure
 * </EN>
 */
static void
status_param(Recog *recog, void *dummy)
{
  if (verbose_flag) {
    //    put_param_info(stdout, recog->param);
  }
}

/**********************************************************************/
/* recognition begin / end */

/** 
 * <JA>
 * 音声入力が検知され認識処理を開始した時点でメッセージを表示する. 
 * ショートポーズセグメンテーション時は，最初のセグメント開始時点で出力される. 
 * 
 * </JA>
 * <EN>
 * Output message when recognition is just started for an incoming input.
 * When short-pause segmentation mode, this will be called for the first input.
 * segment.
 * 
 * </EN>
 */
static void
status_recognition_begin(Recog *recog, void *dummy) 
{
  if (recog->jconf->decodeopt.segment) { /* short pause segmentation */
    if (have_progout) {
      confwordnum = 0;
    }
  }
}

/** 
 * <JA>
 * 入力終了し認識処理が終了した時点でメッセージを表示する. 
 * ショートポーズセグメンテーション時は，１入力の最後のセグメントの終了時に
 * 呼ばれる. 
 * </JA>
 * <EN>
 * Output message when the whole recognition procedure was just finished for
 * an input.  When short-pause segmentation mode, this will be called
 * after all the segmentd input fragments are recognized.
 * </EN>
 */
static void
status_recognition_end(Recog *recog, void *dummy) 
{
  if (recog->process_segment) {
    if (verbose_flag) {
      printf("Segmented by short pause, continue to next...\n");
    } else {
      //printf("-->\n");
    }
  }
  if (recog->jconf->decodeopt.segment) { /* short pause segmentation */
    if (have_progout) {
      if (confwordnum > 0) {
	printf("\n");
      }
    }
  }
}

/** 
 * <JA>
 * ショートポーズセグメンテーション時に，区切られたある入力断片に対して
 * 認識を開始したときにメッセージを出力する. 
 * </JA>
 * <EN>
 * Output a message when recognition was just started for a segment in a
 * input.
 * </EN>
 */
static void
status_segment_begin(Recog *recog, void *dummy)
{
  /* no output */
}

/** 
 * <JA>
 * ショートポーズセグメンテーション時に，区切られたある入力断片に対して
 * 認識を終了したときにメッセージを出力する. 
 * </JA>
 * <EN>
 * Output a message when recognition was just finished for a segment in a
 * input.
 * </EN>
 */
static void
status_segment_end(Recog *recog, void *dummy)
{
  /* nop */
}

/**********************************************************************/
/* 1st pass output */

static int wst;			///< Number of words at previous output line
static int writelen;		///< written string length on this tty line


/** 
 * <JA>
 * 第1パス：音声認識を開始する際の出力（音声入力開始時に呼ばれる）. 
 * 
 * </JA>
 * <EN>
 * 1st pass: output when recognition begins (will be called at input start).
 * 
 * </EN>
 */
static void
status_pass1_begin(Recog *recog, void *dummy)
{
  if (!recog->jconf->decodeopt.realtime_flag) {
    VERMES("### Recognition: 1st pass (LR beam)\n");
  }

  wst = 0;
  
  if (recog->jconf->decodeopt.segment) { /* short pause segmentation */
    if (have_progout) {
      writelen = 0;
    }
  }

}


/** 
 * <JA>
 * 第1パス：途中結果を出力する（第1パスの一定時間ごとに呼ばれる）
 * 
 * @param t [in] 現在の時間フレーム
 * @param seq [in] 現在の一位候補単語列
 * @param num [in] @a seq の長さ
 * @param score [in] 上記のこれまでの累積スコア
 * @param LMscore [in] 上記の最後の単語の信頼度
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * 1st pass: output current result while search (called periodically while 1st pass).
 * 
 * @param t [in] current time frame
 * @param seq [in] current best word sequence at time @a t.
 * @param num [in] length of @a seq.
 * @param score [in] accumulated score of the current best sequence at @a t.
 * @param LMscore [in] confidence score of last word on the sequence
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
result_pass1_current(Recog *recog, void *dummy)
{
  int i, j, bgn;
  int len, num;
  WORD_INFO *winfo;
  WORD_ID *seq;
  RecogProcess *r;

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    if (! r->have_interim) continue;

    winfo = r->lm->winfo;
    seq = r->result.pass1.word;
    num = r->result.pass1.word_num;

    /* update output line with folding */
    printf("\r");
    
    if (r->config->successive.enabled) { /* short pause segmentation */

      if (have_progout) {

	/* progressive output */
	/* first, print already confirmed words */
	len = 0;
	for(i=0;i<confwordnum;i++) {
	  if (len + strlen(winfo->woutput[confword[i]]) > SPTEXTWIDTH) {
	    for(j=len;j<writelen;j++) printf(" ");
	    printf("\n");
	    for(j=i;j<confwordnum;j++) confword[j-i] = confword[j];
	    confwordnum -= i;
	    len = 0;
	    i = 0;
	    writelen = 0;
	  }
	  myprintf("%s", winfo->woutput[confword[i]]);
	  len += strlen(winfo->woutput[confword[i]]);
	}

	/* output nothing if we are in the first pause area */
	if (!r->pass1.first_sparea) {
	  printf("|");
	  len++;
	  /* the first word of a segment is the same as last segment, so do not output */
	  if (confword[confwordnum-1] == seq[0]) {
	    bgn = 1;
	  } else {
	    bgn = 0;
	  }
	  /* next, print current candidate words */
	  for(i=bgn;i<num;i++) {
	    if (len + strlen(winfo->woutput[seq[i]]) > SPTEXT_FULLWIDTH) {
	      if (i < num - 1) continue;
	      myprintf("*");
	      len += 1;
	    } else {
	      myprintf("%s", winfo->woutput[seq[i]]);
	      len += strlen(winfo->woutput[seq[i]]);
	    }
	  }
	  printf("|");
	  len++;
	}
	
	fflush(stdout);
	/* store maximum written length */
	if (writelen < len) writelen = len;
	
	return;
      }
    }
    
    len = 0;
    if (wst == 0) {		/* first line */
      len += 11;
      printf("pass1_best:");
    }
    
    bgn = wst;			/* output only the last line */
    for (i=bgn;i<num;i++) {
      len += strlen(winfo->woutput[seq[i]]) + 1;
      if (len > FILLWIDTH) {	/* fold line */
	wst = i;
	printf("\n");
	len = 0;
      }
      myprintf(" %s",winfo->woutput[seq[i]]);
    }
    if (writelen < len) writelen = len;
    
  }

  fflush(stdout);		/* flush */
}


/** 
 * <JA>
 * 第1パス：終了時に第1パスの結果を出力する（第1パス終了後、第2パスが
 * 始まる前に呼ばれる. 認識に失敗した場合は呼ばれない）. 
 * 
 * @param seq [in] 第1パスの1位候補の単語列
 * @param num [in] 上記の長さ
 * @param score [in] 1位の累積仮説スコア
 * @param LMscore [in] @a score のうち言語スコア
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * 1st pass: output final result of the 1st pass (will be called just after
 * the 1st pass ends and before the 2nd pass begins, and will not if search
 * failed).
 * 
 * @param seq [in] word sequence of the best hypothesis at the 1st pass.
 * @param num [in] length of @a seq.
 * @param score [in] accumulated hypothesis score of @a seq.
 * @param LMscore [in] language score in @a score.
 * @param winfo [in] word dictionary.
 * </EN>
 */
static void
result_pass1(Recog *recog, void *dummy)
{
  int i,j;
  static char buf[MAX_HMMNAME_LEN];
  WORD_INFO *winfo;
  WORD_ID *seq;
  int num;
  RecogProcess *r;
  boolean multi;
  int len;

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    if (r->result.status < 0) continue;	/* search already failed  */
    if (have_progout && r->config->successive.enabled) continue; /* short pause segmentation */
    if (r->config->output.progout_flag) printf("\r");

    winfo = r->lm->winfo;
    seq = r->result.pass1.word;
    num = r->result.pass1.word_num;

    /* words */
    if (multi) printf("[#%d %s]\n", r->config->id, r->config->name);
    printf("pass1_best:");
    if (r->config->output.progout_flag) {
      len = 0;
      for (i=0;i<num;i++) {
	len += strlen(winfo->woutput[seq[i]]) + 1;
	myprintf(" %s",winfo->woutput[seq[i]]);
      }
      for(j=len;j<writelen;j++) printf(" ");
    } else {
      for (i=0;i<num;i++) {
	myprintf(" %s",winfo->woutput[seq[i]]);
      }
    }
    printf("\n");
    
    if (verbose_flag) {		/* output further info */
      /* N-gram entries */
      printf("pass1_best_wordseq:");
      for (i=0;i<num;i++) {
	myprintf(" %s",winfo->wname[seq[i]]);
      }
      printf("\n");
      /* phoneme sequence */
      printf("pass1_best_phonemeseq:");
      for (i=0;i<num;i++) {
	for (j=0;j<winfo->wlen[seq[i]];j++) {
	  center_name(winfo->wseq[seq[i]][j]->name, buf);
	  myprintf(" %s", buf);
	}
	if (i < num-1) printf(" |");
      }
      printf("\n");
      if (debug2_flag) {
	/* logical HMMs */
	printf("pass1_best_HMMseq_logical:");
	for (i=0;i<num;i++) {
	  for (j=0;j<winfo->wlen[seq[i]];j++) {
	    myprintf(" %s", winfo->wseq[seq[i]][j]->name);
	  }
	  if (i < num-1) printf(" |");
	}
	printf("\n");
      }
      /* score */
      printf("pass1_best_score: %f", r->result.pass1.score);
      if (r->lmtype == LM_PROB) {
	if (separate_score_flag) {
	  printf(" (AM: %f  LM: %f)", 
		 r->result.pass1.score_am,
		 r->result.pass1.score_lm);
	}
      }
      printf("\n");
    }
    //printf("\n");
  }
}

#ifdef WORD_GRAPH
static void
result_pass1_graph(Recog *recog, void *dummy)
{
  WordGraph *wg;
  WORD_INFO *winfo;
  RecogProcess *r;
  boolean multi;
  int n;
  int tw1, tw2, i;

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    if (r->result.wg1 == NULL) continue;
    if (multi) printf("[#%d %s]\n", r->config->id, r->config->name);
    printf("--- begin wordgraph data pass1 ---\n");

    winfo = r->lm->winfo;
    /* debug: output all graph word info */
    wordgraph_dump(stdout, r->result.wg1, winfo);
    for(wg=r->result.wg1;wg;wg=wg->next) {
      tw1 = (TEXTWIDTH * wg->lefttime) / r->peseqlen;
      tw2 = (TEXTWIDTH * wg->righttime) / r->peseqlen;
      printf("%4d:", wg->id);
      for(i=0;i<tw1;i++) printf(" ");
      myprintf(" %s\n", winfo->woutput[wg->wid]);
      printf("%4d:", wg->lefttime);
      for(i=0;i<tw1;i++) printf(" ");
      printf("|");
      for(i=tw1+1;i<tw2;i++) printf("-");
      printf("|\n");
    }
    printf("--- end wordgraph data pass1 ---\n");

  }
}

#endif

/** 
 * <JA>
 * 第1パス：終了時の出力（第1パスの終了時に必ず呼ばれる）
 * 
 * </JA>
 * <EN>
 * 1st pass: end of output (will be called at the end of the 1st pass).
 * 
 * </EN>
 */
static void
status_pass1_end(Recog *recog, void *dummy)
{
  if (recog->jconf->decodeopt.segment) { /* short pause segmentation */
    if (have_progout) return;
  }
  /* no op */
  //printf("\n");
}

/**********************************************************************/
/* 2nd pass output */

/** 
 * <JA>
 * 仮説中の単語情報を出力する
 * 
 * @param hypo [in] 仮説
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * Output word sequence of a hypothesis.
 * 
 * @param hypo [in] sentence hypothesis
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
put_hypo_woutput(WORD_ID *seq, int n, WORD_INFO *winfo)
{
  int i;

  if (seq != NULL) {
    for (i=0;i<n;i++) {
      myprintf(" %s",winfo->woutput[seq[i]]);
    }
  }
  printf("\n");  
}

/** 
 * <JA>
 * 仮説のN-gram情報（Julianではカテゴリ番号列）を出力する. 
 * 
 * @param hypo [in] 文仮説
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * Output LM word sequence (N-gram entry/DFA category) of a hypothesis.
 * 
 * @param hypo [in] sentence hypothesis
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
put_hypo_wname(WORD_ID *seq, int n, WORD_INFO *winfo)
{
  int i;

  if (seq != NULL) {
    for (i=0;i<n;i++) {
      myprintf(" %s",winfo->wname[seq[i]]);
    }
  }
  printf("\n");  
}

/** 
 * <JA>
 * 仮説の音素系列を出力する. 
 * 
 * @param hypo [in] 文仮説
 * @param winfo [in] 単語情報
 * </JA>
 * <EN>
 * Output phoneme sequence of a hypothesis.
 * 
 * @param hypo [in] sentence hypothesis
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
put_hypo_phoneme(WORD_ID *seq, int n, WORD_INFO *winfo)
{
  int i,j;
  WORD_ID w;
  static char buf[MAX_HMMNAME_LEN];

  if (seq != NULL) {
    for (i=0;i<n;i++) {
      if (i > 0) printf(" |");
      w = seq[i];
      for (j=0;j<winfo->wlen[w];j++) {
	center_name(winfo->wseq[w][j]->name, buf);
	myprintf(" %s", buf);
      }
    }
  }
  printf("\n");  
}
#ifdef CONFIDENCE_MEASURE
/** 
 * <JA>
 * 仮説の単語ごとの信頼度を出力する. 
 * 
 * @param hypo [in] 文仮説
 * </JA>
 * <EN>
 * Output confidence score of words in a sentence hypothesis.
 * 
 * @param hypo 
 * </EN>
 */
#ifdef CM_MULTIPLE_ALPHA
static void
put_hypo_cmscore(NODE *hypo, int id)
{
  int i;
  int j;
  
  if (hypo != NULL) {
    for (i=hypo->seqnum-1;i>=0;i--) {
      printf(" %5.3f", hypo->cmscore[i][id]);
    }
  }
  printf("\n");  
}
#else
static void
put_hypo_cmscore(LOGPROB *cmscore, int n)
{
  int i;
  
  if (cmscore != NULL) {
    for (i=0;i<n;i++) {
      printf(" %5.3f", cmscore[i]);
    }
  }
  printf("\n");
}
#endif
#endif /* CONFIDENCE_MEASURE */

/** 
 * <JA>
 * 第2パス：得られた文仮説候補を1つ出力する. 
 * 
 * @param hypo [in] 得られた文仮説
 * @param rank [in] @a hypo の順位
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * 2nd pass: output a sentence hypothesis found in the 2nd pass.
 * 
 * @param hypo [in] sentence hypothesis to be output
 * @param rank [in] rank of @a hypo
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
//ttyout_pass2(NODE *hypo, int rank, Recog *recog)
result_pass2(Recog *recog, void *dummy)
{
  int i, j;
  int len;
  char ec[5] = {0x1b, '[', '1', 'm', 0};
  WORD_INFO *winfo;
  WORD_ID *seq;
  int seqnum;
  int n, num;
  Sentence *s;
  RecogProcess *r;
  boolean multi;
  HMM_Logical *p;
  SentenceAlign *align;

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    if (multi) printf("[#%d %s]\n", r->config->id, r->config->name);

    if (r->config->successive.enabled) { /* short pause segmentation */
      if (r->result.status < 0 && r->config->output.progout_flag) {
	/* search failed */
	
	printf("\r");
	winfo = r->lm->winfo;

	if (r->result.status == J_RESULT_STATUS_FAIL) {
 	  /* search fail */
 	  /* output pass1 result as final */
	  seq = r->result.pass1.word;
	  seqnum = r->result.pass1.word_num;
	  j = 0;
 	  /* skip output if head word is the same as previous segment */
	  if (confword[confwordnum-1] == seq[0]) j++;

	  /* store 1st pass result as final */
	  for (i=j;i<seqnum;i++) {
	    confword[confwordnum++] = seq[i];
	  }
	} /* else (rejection), output nothing new */

	len = 0;
	/* output all confirmed words */
	for(i=0;i<confwordnum;i++) {
	  if (len + strlen(winfo->woutput[confword[i]]) > SPTEXTWIDTH) {
	    for(j=len;j<writelen;j++) printf(" ");
	    printf("\n");
	    for(j=i;j<confwordnum;j++) confword[j-i] = confword[j];
	    confwordnum -= i;
	    len = 0;
	    i = 0;
	    writelen = 0;
	  }
	  myprintf("%s", winfo->woutput[confword[i]]);
	  len += strlen(winfo->woutput[confword[i]]);
	}
	for(i=len;i<writelen;i++) printf(" ");
	fflush(stdout);
	
	continue;
      }
    }

    if (r->result.status < 0) {
      switch(r->result.status) {
      case J_RESULT_STATUS_REJECT_POWER:
	printf("<input rejected by power>\n");
	break;
      case J_RESULT_STATUS_TERMINATE:
	printf("<input teminated by request>\n");
	break;
      case J_RESULT_STATUS_ONLY_SILENCE:
	printf("<input rejected by decoder (silence input result)>\n");
	break;
      case J_RESULT_STATUS_REJECT_GMM:
	printf("<input rejected by GMM>\n");
	break;
      case J_RESULT_STATUS_REJECT_SHORT:
	printf("<input rejected by short input>\n");
	break;
      case J_RESULT_STATUS_REJECT_LONG:
	printf("<input rejected by long input>\n");
	break;
      case J_RESULT_STATUS_FAIL:
	printf("<search failed>\n");
	break;
      }
      continue;
    }

    winfo = r->lm->winfo;
    num = r->result.sentnum;

    for(n=0;n<num;n++) {
      s = &(r->result.sent[n]);
      seq = s->word;
      seqnum = s->word_num;
      
      if (r->config->successive.enabled) { /* short pause segmentation */
	if (r->config->output.progout_flag) {
	  printf("\r");

	  j = 0;
 	  /* skip output if head word is the same as previous segment */
	  if (confword[confwordnum-1] == seq[0]) j++;
	  for (i=j;i<seqnum;i++) {
	    confword[confwordnum++] = seq[i];
	  }

	  /* output all confirmed words */
	  len = 0;
	  for(i=0;i<confwordnum;i++) {
	    if (len + strlen(winfo->woutput[confword[i]]) > SPTEXTWIDTH) {
	      for(j=len;j<writelen;j++) printf(" ");
	      printf("\n");
	      for(j=i;j<confwordnum;j++) confword[j-i] = confword[j];
	      confwordnum -= i;
	      len = 0;
	      i = 0;
	      writelen = 0;
	    }
	    myprintf("%s", winfo->woutput[confword[i]]);
	    len += strlen(winfo->woutput[confword[i]]);
	  }
	  for(i=len;i<writelen;i++) printf(" ");
	  
	  break;
	}
      }

      if (debug2_flag) {
	printf("\n%s",ec);		/* newline & bold on */
      }
      printf("sentence%d:", n+1);
      put_hypo_woutput(seq, seqnum, winfo);
      if (verbose_flag) {
	printf("wseq%d:", n+1);
	put_hypo_wname(seq, seqnum, winfo);
	printf("phseq%d:", n+1);
	put_hypo_phoneme(seq, seqnum, winfo);
#ifdef CONFIDENCE_MEASURE
#ifdef CM_MULTIPLE_ALPHA
	{
	  int i;
	  for(i=0;i<r->config->annotate.cm_alpha_num;i++) {
	    printf("cmscore%d[%f]:", rank, r->config->annotate.cm_alpha_bgn + i * r->config->annotate.cm_alpha_step);
	    put_hypo_cmscore(hypo, i);
	  }
	}
#else
	printf("cmscore%d:", n+1);
	put_hypo_cmscore(s->confidence, seqnum);
#endif
#endif /* CONFIDENCE_MEASURE */
      }
      if (debug2_flag) {
	ec[2] = '0';
	printf("%s\n",ec);		/* bold off & newline */
      }
      if (verbose_flag) {
#ifdef USE_MBR
	if(r->config->mbr.use_mbr == TRUE){
	  printf("MBRscore%d: %f\n", n+1, s->score_mbr);
	}
#endif
	printf("score%d: %f", n+1, s->score);
	if (r->lmtype == LM_PROB) {
	  if (separate_score_flag) {
	    printf(" (AM: %f  LM: %f)", s->score_am, s->score_lm);
	  }
	}
	printf("\n");
	if (r->lmtype == LM_DFA) {
	  /* output which grammar the hypothesis belongs to on multiple grammar */
	  /* determine only by the last word */
	  if (multigram_get_all_num(r->lm) > 1) {
	    printf("grammar%d: %d\n", n+1, s->gram_id);
	  }
	}
      }
      
      /* output alignment result if exist */
      for (align = s->align; align; align = align->next) {
	printf("=== begin forced alignment ===\n");
	switch(align->unittype) {
	case PER_WORD:
	  printf("-- word alignment --\n"); break;
	case PER_PHONEME:
	  printf("-- phoneme alignment --\n"); break;
	case PER_STATE:
	  printf("-- state alignment --\n"); break;
	}
	printf(" id: from  to    n_score    unit\n");
	printf(" ----------------------------------------\n");
	for(i=0;i<align->num;i++) {
	  printf("[%4d %4d]  %f  ", align->begin_frame[i], align->end_frame[i], align->avgscore[i]);
	  switch(align->unittype) {
	  case PER_WORD:
	    myprintf("%s\t[%s]\n", winfo->wname[align->w[i]], winfo->woutput[align->w[i]]);
	    break;
	  case PER_PHONEME:
	    p = align->ph[i];
	    if (p->is_pseudo) {
	      printf("{%s}\n", p->name);
	    } else if (strmatch(p->name, p->body.defined->name)) {
	      printf("%s\n", p->name);
	    } else {
	      printf("%s[%s]\n", p->name, p->body.defined->name);
	    }
	    break;
	  case PER_STATE:
	    p = align->ph[i];
	    if (p->is_pseudo) {
	      printf("{%s}", p->name);
	    } else if (strmatch(p->name, p->body.defined->name)) {
	      printf("%s", p->name);
	    } else {
	      printf("%s[%s]", p->name, p->body.defined->name);
	    }
	    if (r->am->hmminfo->multipath) {
	      if (align->is_iwsp[i]) {
		printf(" #%d (sp)\n", align->loc[i]);
	      } else {
		printf(" #%d\n", align->loc[i]);
	      }
	    } else {
	      printf(" #%d\n", align->loc[i]);
	    }
	    break;
	  }
	}
	
	printf("re-computed AM score: %f\n", align->allscore);

	printf("=== end forced alignment ===\n");
      }
    }
  }

  fflush(stdout);

}

/** 
 * <JA>
 * 第2パス：音声認識結果の出力を開始する際の出力. 認識結果を出力する際に、
 * 一番最初に出力される. 
 * 
 * </JA>
 * <EN>
 * 2nd pass: output at the start of result output (will be called before
 * all the result output in the 2nd pass).
 * 
 * </EN>
 */
static void
status_pass2_begin(Recog *recog, void *dummy)
{
  VERMES("### Recognition: 2nd pass (RL heuristic best-first)\n");
  //if (verbose_flag) printf("samplenum=%d\n", recog->param->samplenum);
  //if (debug2_flag) VERMES("getting %d candidates...\n", recog->jconf->search.pass2.nbest);
}

/** 
 * <JA>
 * 第2パス：終了時
 * 
 * </JA>
 * <EN>
 * 2nd pass: end output
 * 
 * </EN>
 */
static void
status_pass2_end(Recog *recog, void *dummy)
{
  fflush(stdout);
}

/**********************************************************************/
/* word graph output */

#define TEXTWIDTH 70

/** 
 * <JA>
 * 得られた単語グラフ全体を出力する. 
 * 
 * @param root [in] グラフ単語集合の先頭要素へのポインタ
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * Output the whole word graph.
 * 
 * @param root [in] pointer to the first element of graph words
 * @param winfo [in] word dictionary
 * </EN>
 */
static void
result_graph(Recog *recog, void *dummy)
{
  WordGraph *wg;
  int tw1, tw2, i;
  WORD_INFO *winfo;
  RecogProcess *r;
  boolean multi;

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    if (r->result.wg == NULL) continue;	/* no graphout specified */
    if (multi) printf("[#%d %s]\n", r->config->id, r->config->name);

    winfo = r->lm->winfo;

    /* debug: output all graph word info */
    wordgraph_dump(stdout, r->result.wg, winfo);

    printf("-------------------------- begin wordgraph show -------------------------\n");
    for(wg=r->result.wg;wg;wg=wg->next) {
      tw1 = (TEXTWIDTH * wg->lefttime) / r->peseqlen;
      tw2 = (TEXTWIDTH * wg->righttime) / r->peseqlen;
      printf("%4d:", wg->id);
      for(i=0;i<tw1;i++) printf(" ");
      myprintf(" %s\n", winfo->woutput[wg->wid]);
      printf("%4d:", wg->lefttime);
      for(i=0;i<tw1;i++) printf(" ");
      printf("|");
      for(i=tw1+1;i<tw2;i++) printf("-");
      printf("|\n");
    }
    printf("-------------------------- end wordgraph show ---------------------------\n");
  }
}

/** 
 * <JA>
 * 得られたコンフュージョンネットワークを出力する. 
 * 
 * </JA>
 * <EN>
 * Output the obtained confusion network.
 * 
 * </EN>
 */
static void
result_confnet(Recog *recog, void *dummy)
{
  CN_CLUSTER *c;
  int i;
  RecogProcess *r;
  boolean multi;

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    if (r->result.confnet == NULL) continue;	/* no confnet obtained */
    if (multi) printf("[#%d %s]\n", r->config->id, r->config->name);

    printf("---- begin confusion network ---\n");
    for(c=r->result.confnet;c;c=c->next) {
      for(i=0;i<c->wordsnum;i++) {
	myprintf("(%s:%.3f)", (c->words[i] == WORD_INVALID) ? "-" : r->lm->winfo->woutput[c->words[i]], c->pp[i]);
	if (i == 0) printf("  ");
      }
      printf("\n");
#if 0
      /* output details - break down all words clustered into this class */
      for(i=0;i<c->wgnum;i++) {
	printf("    ");
	put_wordgraph(stdout, c->wg[i], r->lm->winfo);
      }
#endif
    }
    printf("---- end confusion network ---\n");
  }
}

/********************* RESULT OUTPUT FOR GMM *************************/
/** 
 * <JA>
 * GMMの計算結果を標準出力に出力する. ("-result tty" 用)
 * </JA>
 * <EN>
 * Output result of GMM computation to standard out.
 * (for "-result tty" option)
 * </EN>
 */
static void
result_gmm(Recog *recog, void *dummy)
{
  HTK_HMM_Data *d;
  GMMCalc *gc;
  int i;

  gc = recog->gc;

  if (debug2_flag) {
    printf("--- GMM result begin ---\n");
    i = 0;
    for(d=recog->gmm->start;d;d=d->next) {
      myprintf("  [%8s: total=%f avg=%f]\n", d->name, gc->gmm_score[i], gc->gmm_score[i] / (float)gc->framecount);
      i++;
    }
    myprintf("  max = \"%s\"", gc->max_d->name);
#ifdef CONFIDENCE_MEASURE
    printf(" (CM: %f)", gc->gmm_max_cm);
#endif
    printf("\n");
    printf("--- GMM result end ---\n");
  } else if (verbose_flag) {
    myprintf("GMM: max = \"%s\"", gc->max_d->name);
#ifdef CONFIDENCE_MEASURE
    printf(" (CM: %f)", gc->gmm_max_cm);
#endif
    printf("\n");
  } else {
    if (recog->jconf->decodeopt.segment) { /* short pause segmentation */
      if (!have_progout) {
	myprintf("[GMM: %s]\n", gc->max_d->name);
      }
    } else {
      myprintf("[GMM: %s]\n", gc->max_d->name);
    }
  }
}

/** 
 * <JA>
 * 現在保持している文法のリストを標準出力に出力する. 
 * 
 * </JA>
 * <EN>
 * Output current list of grammars to stdout.
 * 
 * </EN>
 */
void 
print_all_gram(Recog *recog)
{
  MULTIGRAM *m;
  RecogProcess *r;
  boolean multi;
  char buf[1024];

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  for(r=recog->process_list;r;r=r->next) {
    if (! r->live) continue;
    if (multi) printf("[#%d %s]\n", r->config->id, r->config->name);
    if (r->lmtype == LM_PROB) {
      printf("NOT A GRAMMAR-BASED LM\n");
      continue;
    }
    printf("[grammars]\n");
    for(m=r->lm->grammars;m;m=m->next) {
      buf[0] = '\0';
      if (m->dfa) {
	snprintf(buf, 1024, ", %3d categories, %4d nodes",
		 m->dfa->term_num, m->dfa->state_num);
      }
      if (m->newbie) strcat(buf, " (new)");
      if (m->hook != 0) {
	strcat(buf, " (next:");
	if (m->hook & MULTIGRAM_DELETE) {
	  strcat(buf, " delete");
	}
	if (m->hook & MULTIGRAM_ACTIVATE) {
	  strcat(buf, " activate");
	}
	if (m->hook & MULTIGRAM_DEACTIVATE) {
	  strcat(buf, " deactivate");
	}
	if (m->hook & MULTIGRAM_MODIFIED) {
	  strcat(buf, " modified");
	}
	strcat(buf, ")");
      }
      myprintf("  #%2d: [%-11s] %4d words%s \"%s\"\n",
	       m->id,
	       m->active ? "active" : "inactive",
	       m->winfo->num,
	       buf,
	       m->name);
    }
    if (r->lm->dfa != NULL) {
      printf("  Global:            %4d words, %3d categories, %4d nodes\n", r->lm->winfo->num, r->lm->dfa->term_num, r->lm->dfa->state_num);
    }
  }
}

static void
levelmeter(Recog *recog, SP16 *buf, int len, void *dummy)
{
  float d;
  int level;
  int i, n;

  level = 0;
  for(i=0;i<len;i++) {
    if (level < buf[i]) level = buf[i];
  }

  d = log((float)(level+1)) / 10.3971466; /* 10.3971466 = log(32767) */

  n = d * 8.0;
  fprintf(stderr, "\r");
  for(i=0;i<n;i++) fprintf(stderr, ">");

}


static void
frame_indicator(Recog *recog, void *dummy)
{
  RecogProcess *r;

  if (recog->jconf->decodeopt.segment) {
    for(r=recog->process_list;r;r=r->next) {
      if (! r->live) continue;
      if (r->pass1.in_sparea) {
	fprintf(stderr, ".");
	break;
      }
    }
    if (!r) {
      fprintf(stderr, "-");
    }
  } else {
    fprintf(stderr, ".");
  }
}
  

void
setup_output_tty(Recog *recog, void *data)
{
  callback_add(recog, CALLBACK_EVENT_PROCESS_ONLINE, status_process_online, data);
  callback_add(recog, CALLBACK_EVENT_PROCESS_OFFLINE, status_process_offline, data);
  callback_add(recog, CALLBACK_EVENT_SPEECH_READY, status_recready, data);
  callback_add(recog, CALLBACK_EVENT_SPEECH_START, status_recstart, data);
  callback_add(recog, CALLBACK_EVENT_SPEECH_STOP, status_recend, data);
  callback_add(recog, CALLBACK_EVENT_RECOGNITION_BEGIN, status_recognition_begin, data);
  callback_add(recog, CALLBACK_EVENT_RECOGNITION_END, status_recognition_end, data);
  if (recog->jconf->decodeopt.segment) { /* short pause segmentation */
    callback_add(recog, CALLBACK_EVENT_SEGMENT_BEGIN, status_segment_begin, data);
    callback_add(recog, CALLBACK_EVENT_SEGMENT_END, status_segment_end, data);
  }
  callback_add(recog, CALLBACK_EVENT_PASS1_BEGIN, status_pass1_begin, data);
  {
    JCONF_SEARCH *s;
    boolean ok_p;
    ok_p = TRUE;
    for(s=recog->jconf->search_root;s;s=s->next) {
      if (s->output.progout_flag) ok_p = FALSE;
    }
    if (ok_p) {      
      have_progout = FALSE;
    } else {
      have_progout = TRUE;
    }
  }
  if (!recog->jconf->decodeopt.realtime_flag && verbose_flag && ! have_progout) {
    callback_add(recog, CALLBACK_EVENT_PASS1_FRAME, frame_indicator, data);
  }
  callback_add(recog, CALLBACK_RESULT_PASS1_INTERIM, result_pass1_current, data);
  callback_add(recog, CALLBACK_RESULT_PASS1, result_pass1, data);
#ifdef WORD_GRAPH
  callback_add(recog, CALLBACK_RESULT_PASS1_GRAPH, result_pass1_graph, data);
#endif
  callback_add(recog, CALLBACK_EVENT_PASS1_END, status_pass1_end, data);
  callback_add(recog, CALLBACK_STATUS_PARAM, status_param, data);
  callback_add(recog, CALLBACK_EVENT_PASS2_BEGIN, status_pass2_begin, data);
  callback_add(recog, CALLBACK_EVENT_PASS2_END, status_pass2_end, data);
  callback_add(recog, CALLBACK_RESULT, result_pass2, data); // rejected, failed
  callback_add(recog, CALLBACK_RESULT_GMM, result_gmm, data);
  /* below will be called when "-lattice" is specified */
  callback_add(recog, CALLBACK_RESULT_GRAPH, result_graph, data);
  /* below will be called when "-confnet" is specified */
  callback_add(recog, CALLBACK_RESULT_CONFNET, result_confnet, data);

  //callback_add_adin(CALLBACK_ADIN_CAPTURED, levelmeter, data);
}
