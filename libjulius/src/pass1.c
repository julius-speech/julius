/**
 * @file   pass1.c
 * 
 * <JA>
 * @brief  第1パス：フレーム同期ビーム探索
 *
 * 静的木構造辞書を用いて，入力特徴量ベクトル列に対して，Juliusの第１パス
 * であるフレーム同期ビーム探索を行います. 
 *
 * 入力データ全体があらかじめ得られている場合は，一括で計算を
 * 行う関数 get_back_trellis() がメインから呼ばれます. オンライン認識
 * の場合は realtime_1stpass.c から，初期化，フレームごとの計算，
 * 終了処理のそれぞれが入力の進行にあわせて個別に呼ばれます. 
 *
 * 実際の個々の認識処理インスタンスごとの処理は beam.c に記述されています. 
 *
 * </JA>
 * 
 * <EN>
 * @brief  The first pass: frame-synchronous beam search
 *
 * These functions perform a frame-synchronous beam search using a static
 * lexicon tree, as the first pass of Julius/Julian.
 *
 * When the whole input is already obtained, get_back_trellis() simply
 * does all the processing of the 1st pass.  When performing online
 * real-time recognition with concurrent speech input, each function
 * will be called separately from realtime_1stpass.c according on the
 * basis of input processing.
 *
 * The core recognition processing functions for each recognition
 * process instances are written in beam.c.
 *
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Fri Oct 12 23:14:13 2007
 *
 * $Revision: 1.11 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius/julius.h>

/********************************************************************/
/* 第１パスを実行するメイン関数                                     */
/* 入力をパイプライン処理する場合は realtime_1stpass.c を参照のこと */
/* main function to execute 1st pass                                */
/* the pipeline processing is not here: see realtime_1stpass.c      */
/********************************************************************/

/** 
 * <EN>
 * @brief  Process one input frame for all recognition process instance.
 *
 * This function proceeds the recognition for one frame.  All
 * recognition process instance will be processed synchronously.
 * The input frame for each instance is stored in mfcc->f, where mfcc
 * is the MFCC calculation instance assigned to each process instance.
 *
 * If an instance's mfcc->invalid is set to TRUE, its processing will
 * be skipped.
 *
 * When using GMM, GMM computation will also be executed here.
 * If GMM_VAD is defined, GMM-based voice detection will be performed
 * inside this function, by using a scheme of short-pause segmentation.
 *
 * This function also handles segmentation of recognition process.  A
 * segmentation will occur when end of speech is detected by level-based
 * sound detection or GMM-based / decoder-based VAD, or by request from
 * application.  When segmented, it stores current frame and return with
 * that status.
 *
 * The frame-wise callbacks will be executed inside this function,
 * when at least one valid recognition process instances exists.
 * 
 * </EN>
 * <JA>
 * @brief  全ての認識処理インスタンス処理を1フレーム分進める.
 *
 * 全ての認識処理インスタンスについて，割り付けられているMFCC計算インスタンス
 * の mfcc->f をカレントフレームとして処理を1フレーム進める. 
 *
 * なお，mfcc->invalid が TRUE となっている処理インスタンスの処理はスキップ
 * される. 
 *
 * GMMの計算もここで呼び出される. GMM_VAD 定義時は，GMM による
 * 発話区間開始・終了の検出がここで行われる. また，GMMの計算結果，
 * あるいは認識処理内のショートポーズセグメンテーション判定やデバイス・外部
 * からの要求によりセグメンテーションが要求されたかどうかの判定も行う. 
 *
 * フレーム単位で呼び出されるコールバックが登録されている場合は，それらの
 * 呼出しも行う. 
 * </JA>
 * 
 * @param recog [in] engine instance
 * 
 * @return 0 on success, -1 on error, or 1 when an input segmentation
 * occured/requested inside this function.
 *
 * @callgraph
 * @callergraph
 * 
 */
int
decode_proceed(Recog *recog)
{
  MFCCCalc *mfcc;
  boolean break_flag;
  boolean break_decode;
  RecogProcess *p;
  boolean ok_p;
#ifdef GMM_VAD
  GMMCalc *gmm;
  boolean break_gmm;
#endif
  
  break_decode = FALSE;

  for(p = recog->process_list; p; p = p->next) {
#ifdef DETERMINE
    p->have_determine = FALSE;
#endif
    p->have_interim = FALSE;
  }
  for (mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
    mfcc->segmented = FALSE;
  }

#ifdef POWER_REJECT
  for (mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
    if (!mfcc->valid) continue;
    if (mfcc->f == 0) {
      mfcc->avg_power = 0.0;
      if (debug2_flag) jlog("STAT: power_reject: reset\n");
    }
  }
#endif


#ifdef GMM_VAD
  if (recog->gmm != NULL) {
    /* reset flags */
    break_gmm = FALSE;
    recog->gc->want_rewind = FALSE;
  }
#endif
  if (recog->gmm != NULL && recog->gmmmfcc->valid) {
    /* GMM 計算を行う */
    if (recog->gmmmfcc->f == 0) {
      /* GMM 計算の初期化 */
      gmm_prepare(recog);
    }
    /* このフレームに対するGMMの尤度を計算 */
    gmm_proceed(recog);
#ifdef GMM_VAD
    /* Check for GMM-based VAD */
    gmm = recog->gc;
    gmm_check_trigger(recog);
    if (gmm->after_trigger) {
      /* after trigger, in speech area */
      if (gmm->down_trigger) {
	/* down trigger, end segment */
#ifdef GMM_VAD_DEBUG
	printf("GMM_VAD: %d: down trigger\n", recog->gmmmfcc->f);
#endif
	recog->gmmmfcc->sparea_start = recog->gmmmfcc->f + 1 - recog->jconf->detect.gmm_margin;
	if (recog->gmmmfcc->sparea_start < 0) recog->gmmmfcc->sparea_start = 0;
	gmm->after_trigger = FALSE;
	recog->gmmmfcc->segmented = TRUE;
	break_gmm = TRUE;
      } else {
	/* keep recognition */
      }
    } else {
      /* before trigger, in noise area */
      if (gmm->up_trigger) {
	/* start recognition */
	/* request caller to rewind to the backstep point and
	   re-start with normal search */
	if (recog->gmmmfcc->f + 1 < recog->jconf->detect.gmm_margin) {
	  gmm->rewind_frame = 0;
	} else {
	  gmm->rewind_frame = recog->gmmmfcc->f + 1 - recog->jconf->detect.gmm_margin;
	}
#ifdef GMM_VAD_DEBUG
	printf("GMM_VAD: %d: up trigger, start recognition with %d frame rewind\n", recog->gmmmfcc->f, recog->gmmmfcc->f - gmm->rewind_frame);
#endif
	gmm->want_rewind = TRUE;
	gmm->want_rewind_reprocess = TRUE;
	gmm->after_trigger = TRUE;
	return 0;
      } else {
	/* before trigger, noise continues */

	/* if noise goes more than a certain frame, shrink the noise area
	   to avoid unlimited memory usage */
	if (recog->gmmmfcc->f + 1 > GMM_VAD_AUTOSHRINK_LIMIT) {
	  gmm->want_rewind = TRUE;
	  gmm->want_rewind_reprocess = FALSE;
	  gmm->rewind_frame = recog->gmmmfcc->f + 1 - recog->jconf->detect.gmm_margin;
	  if (debug2_flag) {
	    jlog("DEBUG: GMM_VAD: pause exceeded %d, rewind\n", GMM_VAD_AUTOSHRINK_LIMIT);
	  }
	}

	/* skip recognition processing */
	return 0;
      }
    }
#endif /* GMM_VAD */
  }

  for(p = recog->process_list; p; p = p->next) {
    if (!p->live) continue;
    mfcc = p->am->mfcc;
    if (!mfcc->valid) {
      /* このフレームの処理をスキップ */
      /* skip processing the frame */
      continue;
    }

    /* mfcc-f のフレームについて認識処理(フレーム同期ビーム探索)を進める */
    /* proceed beam search for mfcc->f */
    if (mfcc->f == 0) {
      /* 最初のフレーム: 探索処理を初期化 */
      /* initial frame: initialize search process */
      if (get_back_trellis_init(mfcc->param, p) == FALSE) {
	jlog("ERROR: %02d %s: failed to initialize the 1st pass\n", p->config->id, p->config->name);
	return -1;
      }
    }
    if (mfcc->f > 0 || p->am->hmminfo->multipath) {
      /* 1フレーム探索を進める */
      /* proceed search for 1 frame */
      if (get_back_trellis_proceed(mfcc->f, mfcc->param, p, FALSE) == FALSE) {
	mfcc->segmented = TRUE;
	break_decode = TRUE;
      }
      if (p->config->successive.enabled) {
	if (detect_end_of_segment(p, mfcc->f - 1)) {
	  /* セグメント終了検知: 第１パスここで中断 */
	  mfcc->segmented = TRUE;
	  break_decode = TRUE;
	}
      }
    }
  }

  /* セグメントすべきかどうか最終的な判定を行う．
     デコーダベースVADあるいは spsegment の場合，複数インスタンス間で OR
     を取る．また，GMMなど複数基準がある場合は基準間で AND を取る．*/
  /* determine whether to segment at here
     If multiple segmenter exists, take their AND */
  break_flag = FALSE;
  if (break_decode
#ifdef GMM_VAD
      || (recog->gmm != NULL && break_gmm)
#endif
      ) {
    break_flag = TRUE;
  }

  if (break_flag) {
    /* 探索処理の終了が発生したのでここで認識を終える. 
       最初のフレームから [f-1] 番目までが認識されたことになる
    */
    /* the recognition process tells us to stop recognition, so
       recognition should be terminated here.
       the recognized data are [0..f-1] */

    /* 最終フレームを last_time にセット */
    /* set the last frame to last_time */
    for (mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
      mfcc->last_time = mfcc->f - 1;
    }

    if (! recog->jconf->decodeopt.segment) {
      /* ショートポーズ以外で切れた場合，残りのサンプルは認識せずに捨てる */
      /* drop rest inputs if segmented by error */
      for (mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
	mfcc->param->header.samplenum = mfcc->f;
	mfcc->param->samplenum = mfcc->f;
      }
    }

    return 1;
  }

  /* call frame-wise callback for the processing results if any */
#ifdef DETERMINE
  ok_p = FALSE;
  for(p=recog->process_list;p;p=p->next) {
    if (!p->live) continue;
    if (p->have_determine) {
      ok_p = TRUE;
    }
  }
  if (ok_p) callback_exec(CALLBACK_RESULT_PASS1_DETERMINED, recog);
#endif
  ok_p = FALSE;
  for(p=recog->process_list;p;p=p->next) {
    if (!p->live) continue;
    if (p->have_interim) {
      ok_p = TRUE;
    }
  }
  if (ok_p) callback_exec(CALLBACK_RESULT_PASS1_INTERIM, recog);
  
  return 0;
}

#ifdef POWER_REJECT
boolean
power_reject(Recog *recog)
{
  MFCCCalc *mfcc;

  for (mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
    /* skip if not realtime and raw file processing */
    if (mfcc->avg_power == 0.0) continue;
    if (debug2_flag) jlog("STAT: power_reject: MFCC%02d: avg_power = %f\n", mfcc->id, mfcc->avg_power / mfcc->param->samplenum);
    if (mfcc->avg_power / mfcc->param->samplenum < recog->jconf->reject.powerthres) return TRUE;
  }
  return FALSE;
}
#endif

/** 
 * <EN>
 * @brief  End procedure of the first pass (when segmented)
 *
 * This function do things for ending the first pass and prepare for
 * the next recognition, when the input was segmented at the middle of
 * recognition by some reason.
 *
 * First, the best path at each recognition process instance will be parsed
 * and stored.  In case of recognition error or input rejection, the error
 * status will be set.
 *
 * Then, the last pause segment of the processed input will be cut and saved
 * to be processed at first in the recognition of the next or remaining input.
 * 
 * </EN>
 * <JA>
 * @brief  第1パスの終了処理（セグメント時）
 * 
 * 入力が何らかの事由によって途中でセグメントされた時に，第1パスの認識処理を
 * 終了して次回再開するための処理を行う. 
 *
 * まず，各認識処理インスタンスに対して，最尤単語系列を見付け，第1パスの
 * 認識結果として格納する. また，認識失敗・入力棄却の時はエラーステータスをそ
 * れぞれセットする.
 * 
 * そして，次回の認識で，次のセグメントの認識を，検出された末尾雑音
 * 区間から再開するために，その末尾雑音区間を切り出しておく処理を呼ぶ. 
 * 
 * </JA>
 * 
 * @param recog [in] engine instance
 * 
 * @callgraph
 * @callergraph
 */
void
decode_end_segmented(Recog *recog)
{
  boolean ok_p;
  int mseclen;
  RecogProcess *p;
  int last_status;

  /* rejectshort 指定時, 入力が短ければここで第1パス結果を出力しない */
  /* suppress 1st pass output if -rejectshort and input shorter than specified */
  ok_p = TRUE;
  if (recog->jconf->reject.rejectshortlen > 0) {
    mseclen = (float)recog->mfcclist->last_time * (float)recog->jconf->input.period * (float)recog->jconf->input.frameshift / 10000.0;
    if (mseclen < recog->jconf->reject.rejectshortlen) {
      last_status = J_RESULT_STATUS_REJECT_SHORT;
      ok_p = FALSE;
    }
  }
  if (recog->jconf->reject.rejectlonglen >= 0) {
    mseclen = (float)recog->mfcclist->last_time * (float)recog->jconf->input.period * (float)recog->jconf->input.frameshift / 10000.0;
    if (mseclen >= recog->jconf->reject.rejectlonglen) {
      last_status = J_RESULT_STATUS_REJECT_LONG;
      ok_p = FALSE;
    }
  }

#ifdef POWER_REJECT
  if (ok_p) {
    if (power_reject(recog)) {
      last_status = J_RESULT_STATUS_REJECT_POWER;
      ok_p = FALSE;
    }
  }
#endif

  if (ok_p) {
    for(p=recog->process_list;p;p=p->next) {
      if (!p->live) continue;
      finalize_1st_pass(p, p->am->mfcc->last_time);
    }
  } else {
    for(p=recog->process_list;p;p=p->next) {
      if (!p->live) continue;
      p->result.status = last_status;
    }
  }
  if (recog->jconf->decodeopt.segment) {
    finalize_segment(recog);
  }

  if (recog->gmm != NULL) {
    /* GMM 計算の終了 */
    gmm_end(recog);
  }
}

/** 
 * <EN>
 * @brief  End procedure of the first pass
 *
 * This function finish the first pass, when the input was fully
 * processed to the end.
 *
 * The best path at each recognition process instance will be parsed
 * and stored.  In case of recognition error or input rejection, the
 * error status will be set.
 *
 * </EN>
 * <JA>
 * @brief  第1パスの終了処理
 * 
 * 入力が最後まで処理されて終了したときに，第1パスの認識処理を
 * 終了させる. 
 *
 * 各認識処理インスタンスに対して，その時点での第1パスの最尤単語
 * 系列を格納する. また，認識失敗・入力棄却の時はエラーステータスをそ
 * れぞれセットする.
 * 
 * </JA>
 * 
 * @param recog [in] engine instance
 * 
 * @callgraph
 * @callergraph
 */
void
decode_end(Recog *recog)
{
  MFCCCalc *mfcc;
  int mseclen;
  boolean ok_p;
  RecogProcess *p;
  int last_status;

  for (mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
    mfcc->segmented = FALSE;
  }

  if (recog->gmm != NULL) {
    /* GMM 計算の終了 */
    gmm_end(recog);
  }

#ifdef GMM_VAD
  /* もしトリガがかからないまま入力終了に達したのなら，そのままエラー終了 */
  if (recog->jconf->decodeopt.segment) {
    if (recog->gmm) {
      if (recog->gc->after_trigger == FALSE) {
	for(p=recog->process_list;p;p=p->next) {
	  p->result.status = J_RESULT_STATUS_ONLY_SILENCE;	/* reject by decoding */
	}
	/* ショートポーズセグメンテーションの場合,
	   入力パラメータ分割などの最終処理も行なう */
	/* When short-pause segmentation enabled */
	finalize_segment(recog);
	return;
      }
    }
  }
#endif

  /* 第１パスの最後のフレームの認識処理を行う */
  /* finalize 1st pass */
  for(p=recog->process_list;p;p=p->next) {
    if (!p->live) continue;
#ifdef SPSEGMENT_NAIST
    if (recog->jconf->decodeopt.segment) {
      if (p->pass1.after_trigger == FALSE) continue;
    }
#endif
    mfcc = p->am->mfcc;
    if (mfcc->f > 0) {
      get_back_trellis_end(mfcc->param, p);
    }
  }

  /* 終了処理 */
  for(p=recog->process_list;p;p=p->next) {
    if (!p->live) continue;

    ok_p = TRUE;

    /* check rejection by no input */
    if (ok_p) {
      mfcc = p->am->mfcc;
      /* 入力長がデルタの計算に十分でない場合，入力無しとする． */
      /* if input is short for compute all the delta coeff., terminate here */
      if (mfcc->f == 0) {
	jlog("STAT: no input frame\n");
	last_status = J_RESULT_STATUS_FAIL;
	ok_p = FALSE;
      }
    }

    /* check rejection by input length */
    if (ok_p) {
      if (recog->jconf->reject.rejectshortlen > 0) {
	mseclen = (float)mfcc->param->samplenum * (float)recog->jconf->input.period * (float)recog->jconf->input.frameshift / 10000.0;
	if (mseclen < recog->jconf->reject.rejectshortlen) {
	  last_status = J_RESULT_STATUS_REJECT_SHORT;
	  ok_p = FALSE;
	}
      }
      if (recog->jconf->reject.rejectlonglen >= 0) {
	mseclen = (float)mfcc->param->samplenum * (float)recog->jconf->input.period * (float)recog->jconf->input.frameshift / 10000.0;
	if (mseclen >= recog->jconf->reject.rejectlonglen) {
	  last_status = J_RESULT_STATUS_REJECT_LONG;
	  ok_p = FALSE;
	}
      }
    }

#ifdef POWER_REJECT
    /* check rejection by average power */
    if (ok_p) {
      if (power_reject(recog)) {
	last_status = J_RESULT_STATUS_REJECT_POWER;
	ok_p = FALSE;
      }
    }
#endif

#ifdef SPSEGMENT_NAIST
    /* check rejection non-triggered input segment */
    if (ok_p) {
      if (recog->jconf->decodeopt.segment) {
	if (p->pass1.after_trigger == FALSE) {
	  last_status = J_RESULT_STATUS_ONLY_SILENCE;	/* reject by decoding */
	  ok_p = FALSE;
	}
      }
    }
#endif

    if (ok_p) {
      /* valid input segment, finalize it */
      finalize_1st_pass(p, mfcc->param->samplenum);
    } else {
      /* invalid input segment */
      p->result.status = last_status;
    }
  }
  if (recog->jconf->decodeopt.segment) {
    /* ショートポーズセグメンテーションの場合,
       入力パラメータ分割などの最終処理も行なう */
    /* When short-pause segmentation enabled */
    finalize_segment(recog);
  }
}


/** 
 * <JA>
 * @brief  フレーム同期ビーム探索メイン関数（バッチ処理用）
 *
 * 与えられた入力ベクトル列に対して第１パス(フレーム同期ビーム探索)を
 * 行い，その結果を出力する. また全フレームに渡る単語終端を，第２パス
 * のために単語トレリス構造体に格納する. 
 * 
 * この関数は入力ベクトル列があらかじめ得られている場合に用いられる. 
 * 第１パスが入力と並列して実行されるオンライン認識の場合，
 * この関数は用いられず，代わりにこのファイルで定義されている各サブ関数が
 * 直接 realtime-1stpass.c 内から呼ばれる. 
 * 
 * @param recog [in] エンジンインスタンス
 * </JA>
 * <EN>
 * @brief  Frame synchronous beam search: the main (for batch mode)
 *
 * This function perform the 1st recognition pass of frame-synchronous beam
 * search and output the result.  It also stores all the word ends in every
 * input frame to word trellis structure.
 *
 * This function will be called if the whole input vector is already given
 * to the end.  When online recognition, where the 1st pass will be
 * processed in parallel with input, this function will not be used.
 * In that case, functions defined in this file will be directly called
 * from functions in realtime-1stpass.c.
 * 
 * @param recog [in] engine instance
 * </EN>
 * @callgraph
 * @callergraph
 */
boolean
get_back_trellis(Recog *recog)
{
  boolean ok_p;
  MFCCCalc *mfcc;
  int rewind_frame;
  PROCESS_AM *am;
  boolean reprocess;

  /* initialize mfcc instances */
  for(mfcc=recog->mfcclist;mfcc;mfcc=mfcc->next) {
    /* mark all as valid, since all frames are fully prepared beforehand */
    if (mfcc->param->samplenum == 0) mfcc->valid = FALSE;
    else mfcc->valid = TRUE;
    /* set frame pointers to 0 */
    mfcc->f = 0;
  }

  /* callback of process start */
#ifdef BACKEND_VAD
  if (recog->jconf->decodeopt.segment) {
    /* at first time, recognition does not start yet */
    /* reset segmentation flags */
    spsegment_init(recog);
  } else {
    /* execute callback for pass1 begin here */
    callback_exec(CALLBACK_EVENT_RECOGNITION_BEGIN, recog);
    callback_exec(CALLBACK_EVENT_PASS1_BEGIN, recog);
    recog->triggered = TRUE;
  }
#else
  if (recog->jconf->decodeopt.segment) {
    if (!recog->process_segment) {
      callback_exec(CALLBACK_EVENT_RECOGNITION_BEGIN, recog);
    }
    callback_exec(CALLBACK_EVENT_SEGMENT_BEGIN, recog);
  } else {
    callback_exec(CALLBACK_EVENT_RECOGNITION_BEGIN, recog);
  }
  callback_exec(CALLBACK_EVENT_PASS1_BEGIN, recog);
  recog->triggered = TRUE;
#endif

  while(1) {
    ok_p = TRUE;
    for (mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
      if (! mfcc->valid) continue;
      if (mfcc->f < mfcc->param->samplenum) {
	mfcc->valid = TRUE;
	ok_p = FALSE;
      } else {
	mfcc->valid = FALSE;
      }
    }
    if (ok_p) {
      /* すべての MFCC が終わりに達したのでループ終了 */
      /* all MFCC has been processed, end of loop  */
      break;
    }

    switch (decode_proceed(recog)) {
    case -1: /* error */
      return FALSE;
      break;
    case 0:			/* success */
      break;
    case 1:			/* segmented */
      /* 探索中断: 処理された入力は 0 から t-2 まで */
      /* search terminated: processed input = [0..t-2] */
      /* この時点で第1パスを終了する */
      /* end the 1st pass at this point */
      decode_end_segmented(recog);
      /* terminate 1st pass here */
      return TRUE;
    }

#ifdef BACKEND_VAD
    /* check up trigger in case of VAD segmentation */
    if (recog->jconf->decodeopt.segment) {
      if (recog->triggered == FALSE) {
	if (spsegment_trigger_sync(recog)) {
	  if (!recog->process_segment) {
	    callback_exec(CALLBACK_EVENT_RECOGNITION_BEGIN, recog);
	  }
	  callback_exec(CALLBACK_EVENT_SEGMENT_BEGIN, recog);
	  callback_exec(CALLBACK_EVENT_PASS1_BEGIN, recog);
	  recog->triggered = TRUE;
	}
      }
    }
#endif

    if (spsegment_need_restart(recog, &rewind_frame, &reprocess) == TRUE) {
      /* do rewind for all mfcc here */
      spsegment_restart_mfccs(recog, rewind_frame, reprocess);
      /* reset outprob cache for all AM */
      for(am=recog->amlist;am;am=am->next) {
	outprob_prepare(&(am->hmmwrk), am->mfcc->param->samplenum);
      }
    }
    /* call frame-wise callback */
    callback_exec(CALLBACK_EVENT_PASS1_FRAME, recog);

    /* 1フレーム処理が進んだのでポインタを進める */
    /* proceed frame pointer */
    for (mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
      if (!mfcc->valid) continue;
      mfcc->f++;
    }

    if (recog->process_want_terminate) {
      /* termination requested */
      decode_end_segmented(recog);
      return TRUE;
    }
  }

  /* 最終フレーム処理を行い，認識の結果出力と終了処理を行う */
  decode_end(recog);

  return TRUE;
}

/* end of file */
