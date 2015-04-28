/**
 * @file   word_align.c
 * 
 * <JA>
 * @brief  単語・音素・状態単位のアラインメント
 *
 * ここでは，認識結果に対する入力音声のアラインメントを出力するための
 * 関数が定義されています. 
 *
 * Julius/Julian では，認識結果においてその単語や音素，あるいはHMMの状態が
 * それぞれ入力音声のどの区間にマッチしたのかを知ることができます. 
 * より正確なアラインメントを求めるために，Julius/Julian では認識中の
 * 近似を含む情報は用いずに，認識が終わった後に得られた認識結果の単語列に
 * 対して，あらためて forced alignment を実行しています. 
 * </JA>
 * 
 * <EN>
 * @brief  Forced alignment by word / phoneme / state unit.
 *
 * This file defines functions for performing forced alignment of
 * recognized words.  The forced alignment is implimented in Julius/Julian
 * to get the best matching segmentation of recognized word sequence
 * upon input speech.  Word-level, phoneme-level and HMM state-level
 * alignment can be obtained.
 *
 * Julius/Julian performs the forced alignment as a post-processing of
 * recognition process.  Recomputation of Viterbi path on the recognized
 * word sequence toward input speech will be done after the recognition
 * to get better alignment.
 *
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Sat Sep 24 16:09:46 2005
 *
 * $Revision: 1.8 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius/julius.h>

/** 
 * <JA>
 * 与えられた単語列からHMMを連結して文全体のHMMを構築する. 
 * 
 * @param wseq [in] 単語列
 * @param num [in] @a wseq の数
 * @param has_sp_ret [out] ショートポーズを後続に挿入しうるユニットの情報
 * @param num_ret [out] 構築されたHMMに含まれる音素HMMの数
 * @param end_ret [out] アラインメントの区切りとなる状態番号の列
 * @param per_what [in] 単語・音素・状態のどの単位でアラインメントを取るかを指定
 * @param r [in] 認識処理インスタンス
 * 
 * @return あらたに割り付けられた文全体をあらわすHMMモデル列へのポインタを返す. 
 * </JA>
 * <EN>
 * Make the whole sentence HMM from given word sequence by connecting
 * each phoneme HMM.
 * 
 * @param wseq [in] word sequence to align
 * @param num [in] number of @a wseq
 * @param has_sp_ret [out] unit information of whether it can be followed by a short-pause
 * @param num_ret [out] number of HMM contained in the generated sentence HMM
 * @param end_ret [out] sequence of state location as alignment unit
 * @param per_what [in] specify the alignment unit (word / phoneme / state)
 * @param r [in] recognition process instance
 * 
 * @return newly malloced HMM sequences.
 * </EN>
 */
static HMM_Logical **
make_phseq(WORD_ID *wseq, short num, boolean **has_sp_ret, int *num_ret, int **end_ret, int per_what, 
	   RecogProcess *r)
{
  HMM_Logical **ph;		/* phoneme sequence */
  boolean *has_sp;
  int k;
  int phnum;			/* num of above */
  WORD_ID tmpw, w;
  int i, j, pn, st, endn;
  HMM_Logical *tmpp, *ret;
  WORD_INFO *winfo;
  HTK_HMM_INFO *hmminfo;
  boolean enable_iwsp;		/* for multipath */

  winfo = r->lm->winfo;
  hmminfo = r->am->hmminfo;
  if (hmminfo->multipath) enable_iwsp = r->lm->config->enable_iwsp;

  /* make ph[] from wseq[] */
  /* 1. calc total phone num and malloc */
  phnum = 0;
  for (w=0;w<num;w++) phnum += winfo->wlen[wseq[w]];
  ph = (HMM_Logical **)mymalloc(sizeof(HMM_Logical *) * phnum);
  
  if (hmminfo->multipath && enable_iwsp) {
    has_sp = (boolean *)mymalloc(sizeof(boolean) * phnum);
  } else {
    has_sp = NULL;
  }
  /* 2. make phoneme sequence */
  st = 0;
  if (hmminfo->multipath) st++;
  pn = 0;
  endn = 0;
  for (w=0;w<num;w++) {
    tmpw = wseq[w];
    for (i=0;i<winfo->wlen[tmpw];i++) {
      tmpp = winfo->wseq[tmpw][i];
      /* handle cross-word context dependency */
      if (r->ccd_flag) {
	if (w > 0 && i == 0) {	/* word head */
	  
	  if ((ret = get_left_context_HMM(tmpp, ph[pn-1]->name, hmminfo)) != NULL) {
	    tmpp = ret;
	  }
	  /* if triphone not found, fallback to bi/mono-phone  */
	  /* use pseudo phone when no bi-phone found in alignment... */
	}
	if (w < num-1 && i == winfo->wlen[tmpw] - 1) { /* word tail */
	  if ((ret = get_right_context_HMM(tmpp, winfo->wseq[wseq[w+1]][0]->name, hmminfo)) != NULL) {
	    tmpp = ret;
	  }
	}
      }
      ph[pn] = tmpp;
      if (hmminfo->multipath && enable_iwsp) {
	if (i == winfo->wlen[tmpw] - 1) {
	  has_sp[pn] = TRUE;
	} else {
	  has_sp[pn] = FALSE;
	}
      }
      if (per_what == PER_STATE) {
	for (j=0;j<hmm_logical_state_num(tmpp)-2;j++) {
	  (*end_ret)[endn++] = st + j;
	}
	if (hmminfo->multipath && enable_iwsp && has_sp[pn]) {
	  for (k=0;k<hmm_logical_state_num(hmminfo->sp)-2;k++) {
	    (*end_ret)[endn++] = st + j + k;
	  }
	}
      }
      st += hmm_logical_state_num(tmpp) - 2;
      if (hmminfo->multipath && enable_iwsp && has_sp[pn]) {
	st += hmm_logical_state_num(hmminfo->sp) - 2;
      }
      if (per_what == PER_PHONEME) (*end_ret)[endn++] = st - 1;
      pn++;
    }
    if (per_what == PER_WORD) (*end_ret)[endn++] = st - 1;
  }
  *num_ret = phnum;
  *has_sp_ret = has_sp;
  return ph;
}


/** 
 * <JA>
 * 文全体のHMMを構築し，Viterbiアラインメントを実行し，結果を出力する. 
 * 
 * @param words [in] 文仮説をあらわす単語列
 * @param wnum [in] @a words の長さ
 * @param param [in] 入力特徴パラメータ列
 * @param per_what [in] 単語・音素・状態のどの単位でアラインメントを取るかを指定
 * @param align [out] アラインメント結果を格納するSentence構造体
 * @param r [i/o] 認識処理インスタンス
 * </JA>
 * <EN>
 * Build sentence HMM, call viterbi_segment() and output result.
 * 
 * @param words [in] word sequence of the sentence
 * @param wnum [in] number of words in @a words
 * @param param [in] input parameter vector
 * @param per_what [in] specify the alignment unit (word / phoneme / state)
 * @param s [out] Sentence data area to store the alignment result
 * @param r [i/o] recognition process instance
 * </EN>
 */
static void
do_align(WORD_ID *words, short wnum, HTK_Param *param, int per_what, SentenceAlign *align, RecogProcess *r)
{
  HMM_Logical **phones;		/* phoneme sequence */
  boolean *has_sp;		/* whether phone can follow short pause */
  int k;
  int phonenum;			/* num of above */
  HMM *shmm;			/* sentence HMM */
  int *end_state;		/* state number of word ends */
  int *end_frame;		/* segmented last frame of words */
  LOGPROB *end_score;		/* normalized score of each words */
  LOGPROB allscore;		/* total score of this word sequence */
  WORD_ID w;
  int i, rlen;
  int end_num = 0;
  int *id_seq, *phloc = NULL, *stloc = NULL;
  int j,n,p;
  WORD_INFO *winfo;
  HTK_HMM_INFO *hmminfo;
  boolean enable_iwsp;		/* for multipath */

  winfo = r->lm->winfo;
  hmminfo = r->am->hmminfo;
  if (hmminfo->multipath) enable_iwsp = r->lm->config->enable_iwsp;

  /* initialize result storage buffer */
  switch(per_what) {
  case PER_WORD:
    jlog("ALIGN: === word alignment begin ===\n");
    end_num = wnum;
    phloc = (int *)mymalloc(sizeof(int)*wnum);
    i = 0;
    for(w=0;w<wnum;w++) {
      phloc[w] = i;
      i += winfo->wlen[words[w]];
    }
    break;
  case PER_PHONEME:
    jlog("ALIGN: === phoneme alignment begin ===\n");
    end_num = 0;
    for(w=0;w<wnum;w++) end_num += winfo->wlen[words[w]];
    break;
  case PER_STATE:
    jlog("ALIGN: === state alignment begin ===\n");
    end_num = 0;
    for(w=0;w<wnum;w++) {
      for (i=0;i<winfo->wlen[words[w]]; i++) {
	end_num += hmm_logical_state_num(winfo->wseq[words[w]][i]) - 2;
      }
      if (hmminfo->multipath && enable_iwsp) {
	end_num += hmm_logical_state_num(hmminfo->sp) - 2;
      }
    }
    phloc = (int *)mymalloc(sizeof(int)*end_num);
    stloc = (int *)mymalloc(sizeof(int)*end_num);
    {
      n = 0;
      p = 0;
      for(w=0;w<wnum;w++) {
	for(i=0;i<winfo->wlen[words[w]]; i++) {
	  for(j=0; j<hmm_logical_state_num(winfo->wseq[words[w]][i]) - 2; j++) {
	    phloc[n] = p;
	    stloc[n] = j + 1;
	    n++;
	  }
	  if (hmminfo->multipath && enable_iwsp && i == winfo->wlen[words[w]] - 1) {
	    for(k=0;k<hmm_logical_state_num(hmminfo->sp)-2;k++) {
	      phloc[n] = p;
	      stloc[n] = j + 1 + k + end_num;
	      n++;
	    }
	  }
	  p++;
	}
      }
    }
    
    break;
  }
  end_state = (int *)mymalloc(sizeof(int) * end_num);

  /* make phoneme sequence word sequence */
  phones = make_phseq(words, wnum, &has_sp, &phonenum, &end_state, per_what, r);
  /* build the sentence HMMs */
  shmm = new_make_word_hmm(hmminfo, phones, phonenum, has_sp);
  if (shmm == NULL) {
    j_internal_error("Error: failed to make word hmm for alignment\n");
  }

  /* call viterbi segmentation function */
  allscore = viterbi_segment(shmm, param, r->wchmm->hmmwrk, hmminfo->multipath, end_state, end_num, &id_seq, &end_frame, &end_score, &rlen);

  /* store result to s */
  align->num = rlen;
  align->unittype = per_what;
  align->begin_frame = (int *)mymalloc(sizeof(int) * rlen);
  align->end_frame   = (int *)mymalloc(sizeof(int) * rlen);
  align->avgscore    = (LOGPROB *)mymalloc(sizeof(LOGPROB) * rlen);
  for(i=0;i<rlen;i++) {
    align->begin_frame[i] = (i == 0) ? 0 : end_frame[i-1] + 1;
    align->end_frame[i]   = end_frame[i];
    align->avgscore[i]    = end_score[i];
  }
  switch(per_what) {
  case PER_WORD:
    align->w = (WORD_ID *)mymalloc(sizeof(WORD_ID) * rlen);
    for(i=0;i<rlen;i++) {
      align->w[i] = words[id_seq[i]];
    }
    break;
  case PER_PHONEME:
    align->ph = (HMM_Logical **)mymalloc(sizeof(HMM_Logical *) * rlen);
    for(i=0;i<rlen;i++) {
      align->ph[i] = phones[id_seq[i]];
    }
    break;
  case PER_STATE:
    align->ph = (HMM_Logical **)mymalloc(sizeof(HMM_Logical *) * rlen);
    align->loc = (short *)mymalloc(sizeof(short) * rlen);
    if (hmminfo->multipath) align->is_iwsp = (boolean *)mymalloc(sizeof(boolean) * rlen);
    for(i=0;i<rlen;i++) {
      align->ph[i]  = phones[phloc[id_seq[i]]];
      if (hmminfo->multipath) {
	if (enable_iwsp && stloc[id_seq[i]] > end_num) {
	  align->loc[i] = stloc[id_seq[i]] - end_num;
	  align->is_iwsp[i] = TRUE;
	} else {
	  align->loc[i] = stloc[id_seq[i]];
	  align->is_iwsp[i] = FALSE;
	}
      } else {
	align->loc[i] = stloc[id_seq[i]];
      }
    }
    break;
  }

  align->allscore = allscore;

  free_hmm(shmm);
  free(id_seq);
  free(phones);
  if (has_sp) free(has_sp);
  free(end_score);
  free(end_frame);
  free(end_state);

  switch(per_what) {
  case PER_WORD:
    free(phloc);
    break;
  case PER_PHONEME:
    break;
  case PER_STATE:
    free(phloc);
    free(stloc);
  }
  
}

/** 
 * <JA>
 * 単語ごとの forced alignment を行う. 
 * 
 * @param words [in] 単語列
 * @param wnum [in] @a words の単語数
 * @param param [in] 入力特徴ベクトル列
 * @param align [out] アラインメント結果を格納するSentence構造体
 * @param r [i/o] 認識処理インスタンス
 * </JA>
 * <EN>
 * Do forced alignment per word for the given word sequence.
 * 
 * @param words [in] word sequence
 * @param wnum [in] length of @a words
 * @param param [in] input parameter vectors
 * @param align [out] Sentence data area to store the alignment result
 * @param r [i/o] recognition process instance
 * </EN>
 * @callgraph
 * @callergraph
 */
void
word_align(WORD_ID *words, short wnum, HTK_Param *param, SentenceAlign *align, RecogProcess *r)
{
  do_align(words, wnum, param, PER_WORD, align, r);
}

/** 
 * <JA>
 * 単語ごとの forced alignment を行う（単語が逆順で与えられる場合）
 * 
 * @param revwords [in] 単語列（逆順）
 * @param wnum [in] @a revwords の単語数
 * @param param [in] 入力特徴ベクトル列
 * @param align [out] アラインメント結果を格納するSentence構造体
 * @param r [i/o] 認識処理インスタンス
 * </JA>
 * <EN>
 * Do forced alignment per word for the given word sequence (reversed order).
 * 
 * @param revwords [in] word sequence in reversed direction
 * @param wnum [in] length of @a revwords
 * @param param [in] input parameter vectors
 * @param align [out] Sentence data area to store the alignment result
 * @param r [i/o] recognition process instance
 * </EN>
 * @callgraph
 * @callergraph
 */
void
word_rev_align(WORD_ID *revwords, short wnum, HTK_Param *param, SentenceAlign *align, RecogProcess *r)
{
  WORD_ID *words;		/* word sequence (true order) */
  int w;
  words = (WORD_ID *)mymalloc(sizeof(WORD_ID) * wnum);
  for (w=0;w<wnum;w++) words[w] = revwords[wnum-w-1];
  do_align(words, wnum, param, PER_WORD, align, r);
  free(words);
}

/** 
 * <JA>
 * 音素ごとの forced alignment を行う. 
 * 
 * @param words [in] 単語列
 * @param num [in] @a words の単語数
 * @param param [in] 入力特徴ベクトル列
 * @param align [out] アラインメント結果を格納するSentence構造体
 * @param r [i/o] 認識処理インスタンス
 * </JA>
 * <EN>
 * Do forced alignment per phoneme for the given word sequence.
 * 
 * @param words [in] word sequence
 * @param num [in] length of @a words
 * @param param [in] input parameter vectors
 * @param align [out] Sentence data area to store the alignment result
 * @param r [i/o] recognition process instance
 * </EN>
 * @callgraph
 * @callergraph
 */
void
phoneme_align(WORD_ID *words, short num, HTK_Param *param, SentenceAlign *align, RecogProcess *r)
{
  do_align(words, num, param, PER_PHONEME, align, r);
}

/** 
 * <JA>
 * 音素ごとの forced alignment を行う（単語が逆順で与えられる場合）
 * 
 * @param revwords [in] 単語列（逆順）
 * @param num [in] @a revwords の単語数
 * @param param [in] 入力特徴ベクトル列
 * @param align [out] アラインメント結果を格納するSentence構造体
 * @param r [i/o] 認識処理インスタンス
 * </JA>
 * <EN>
 * Do forced alignment per phoneme for the given word sequence (reversed order).
 * 
 * @param revwords [in] word sequence in reversed direction
 * @param num [in] length of @a revwords
 * @param param [in] input parameter vectors
 * @param align [out] Sentence data area to store the alignment result
 * @param r [i/o] recognition process instance
 * </EN>
 * @callgraph
 * @callergraph
 */
void
phoneme_rev_align(WORD_ID *revwords, short num, HTK_Param *param, SentenceAlign *align, RecogProcess *r)
{
  WORD_ID *words;		/* word sequence (true order) */
  int p;
  words = (WORD_ID *)mymalloc(sizeof(WORD_ID) * num);
  for (p=0;p<num;p++) words[p] = revwords[num-p-1];
  do_align(words, num, param, PER_PHONEME, align, r);
  free(words);
}

/** 
 * <JA>
 * HMM状態ごとの forced alignment を行う. 
 * 
 * @param words [in] 単語列
 * @param num [in] @a words の単語数
 * @param param [in] 入力特徴ベクトル列
 * @param align [out] アラインメント結果を格納するSentence構造体
 * @param r [i/o] 認識処理インスタンス
 * </JA>
 * <EN>
 * Do forced alignment per HMM state for the given word sequence.
 * 
 * @param words [in] word sequence
 * @param num [in] length of @a words
 * @param param [in] input parameter vectors
 * @param align [out] Sentence data area to store the alignment result
 * @param r [i/o] recognition process instance
 * </EN>
 * @callgraph
 * @callergraph
 */
void
state_align(WORD_ID *words, short num, HTK_Param *param, SentenceAlign *align, RecogProcess *r)
{
  do_align(words, num, param, PER_STATE, align, r);
}

/** 
 * <JA>
 * HMM状態ごとの forced alignment を行う（単語が逆順で与えられる場合）
 * 
 * @param revwords [in] 単語列（逆順）
 * @param num [in] @a revwords の単語数
 * @param param [in] 入力特徴ベクトル列
 * @param align [out] アラインメント結果を格納するSentence構造体
 * @param r [i/o] 認識処理インスタンス
 * </JA>
 * <EN>
 * Do forced alignment per state for the given word sequence (reversed order).
 * 
 * @param revwords [in] word sequence in reversed direction
 * @param num [in] length of @a revwords
 * @param param [in] input parameter vectors
 * @param align [out] Sentence data area to store the alignment result
 * @param r [i/o] recognition process instance
 * </EN>
 * @callgraph
 * @callergraph
 */
void
state_rev_align(WORD_ID *revwords, short num, HTK_Param *param, SentenceAlign *align, RecogProcess *r)
{
  WORD_ID *words;		/* word sequence (true order) */
  int p;
  words = (WORD_ID *)mymalloc(sizeof(WORD_ID) * num);
  for (p=0;p<num;p++) words[p] = revwords[num-p-1];
  do_align(words, num, param, PER_STATE, align, r);
  free(words);
}

/** 
 * <JA>
 * 認識結果に対して必要なアラインメントを全て実行する．
 * 
 * @param r [i/o] 認識処理インスタンス
 * @param param [in] 入力特徴ベクトル列
 * </JA>
 * <EN>
 * Do required forced alignment for the recognition results
 * 
 * @param r [i/o] recognition process instance
 * @param param [in] input parameter vectors
 * </EN>
 * @callgraph
 * @callergraph
 */
void
do_alignment_all(RecogProcess *r, HTK_Param *param)
{
  int n;
  Sentence *s;
  SentenceAlign *now, *prev;

  for(n = 0; n < r->result.sentnum; n++) {
    s = &(r->result.sent[n]);
    /* do forced alignment if needed */
    if (r->config->annotate.align_result_word_flag) {
      now = result_align_new();
      word_align(s->word, s->word_num, param, now, r);
      if (s->align == NULL) s->align = now;
      else prev->next = now;
      prev = now;
    }
    if (r->config->annotate.align_result_phoneme_flag) {
      now = result_align_new();
      phoneme_align(s->word, s->word_num, param, now, r);
      if (s->align == NULL) s->align = now;
      else prev->next = now;
      prev = now;
    }
    if (r->config->annotate.align_result_state_flag) {
      now = result_align_new();
      state_align(s->word, s->word_num, param, now, r);
      if (s->align == NULL) s->align = now;
      else prev->next = now;
      prev = now;
    }
  }
} 

/* end of file */
