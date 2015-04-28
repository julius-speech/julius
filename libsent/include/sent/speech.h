/**
 * @file   speech.h
 *
 * <EN>
 * @brief Miscellaneous definitions for speech input processing
 *
 * This file contains miscellaneous definitions for speech input processing.
 * Several limitation for input speech length is also defined here.
 *
 * Please refer to adin.h for speech capturing, mfcc.h for MFCC parameter
 * extraction, htk_param.h for storing the parameter vectors.
 * </EN>
 * <JA>
 * @brief  音声入出力処理に関する定義
 *
 * このファイルには，音声の入出力に関する雑多な定義が収められています．
 * 一発話あたりの入力長に関する制限などが定義されています．
 * 
 * 入力ソースに関する定義は adin.h，MFCC 特徴量抽出に関する定義は mfcc.h,
 * 特徴量パラメータについては htk_param.h を参照して下さい．
 * </JA>
 *
 * @author Akinobu LEE
 * @date   Sat Feb 12 11:16:41 2005
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

/* speech input limitation */
#ifndef __SENT_SPEECH__
#define __SENT_SPEECH__

#include <sent/adin.h>

/**
 * @brief Maximum number of words in an input.
 *
 * This value defines limitation of word length in one utterance input.
 * If the number of words exceeds this value, Julius produces error.
 * So you have to set large value enough.
 * 
 */
#define MAXSEQNUM     150

/**
 * @brief Maximum length of an input in samples
 *
 * This value defines limitation of speech input length in one utterance input.
 * If the length of an input exceeds this value, Julius stop the input
 * at that point and recognize it, disgarding the rest until the end of speech
 * (long silence) comes.
 *
 * The default value is 320000, which means you can give Julius an input of
 * at most 20 secons in 16kHz sampling.  Setting smaller value saves
 * memory usage.
 * 
 */
#define MAXSPEECHLEN  320000

/**
 * @brief Maximum length of input delay in seconds
 *
 * This value defines maximum delay on live speech recognition with slow
 * machines. If an input delays over this sample, the overflowed samples
 * will be dropped.  This value is used on callback-based ad-in,
 * namely on portaudio interface.
 *
 * The default value is 8 seconds.  Setting smaller value saves
 * memory usage but risk of overflow grows on slow machines
 * 
 */
#define INPUT_DELAY_SEC  8

/**
 * @brief Expansion period in frames for output probability cache
 *
 * When recognition, the 1st recognition pass stores all the output
 * probabilities of %HMM states for every incoming input frame, to speed up the
 * re-computation of acoustic likelihoods in the 2nd pass.
 * In live input mode, this output probability cache will be
 * re-allocated dynamically as the input becomes longer.
 *
 * This value specifies the re-allocation period in frames.  The probability
 * cache are will be expanded as the input proceeds this frame.
 * 
 * Smaller value may improve memory efficiency, but Too small value may
 * result in the overhead of memory re-allocation and slow down the
 * recognition.
 * 
 */
#define OUTPROB_CACHE_PERIOD 100


#ifdef __cplusplus
extern "C" {
#endif

/// Macro to convert smpPeriod (100nsec unit) to frequency (Hz)
#define period2freq(A)  (10000000.0 / (float)(A))
/// Macro to convert sampling frequency (Hz) to smpPeriod (100nsec unit)
#define freq2period(A)  (10000000.0 / (float)(A))

/* for anlz/wrsamp.c */
int wrsamp(int fd, SP16 *buf, int len);

/* for anlz/wrwav.c */
FILE *wrwav_open(char *filename, int sfreq);
boolean wrwav_data(FILE *fp, SP16 *buf, int len);
boolean wrwav_close(FILE *fp);

/* for an;z/strip.c */
int strip_zero(SP16 a[], int len);

#ifdef __cplusplus
}
#endif

#endif /* __SENT_SPEECH__ */
