/**
 * @file   confout.c
 * 
 * <JA>
 * @brief  コンパイル時の設定をテキスト出力する
 * </JA>
 * 
 * <EN>
 * @brief  Output compilation time configurations strings.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Feb 17 15:34:39 2005
 *
 * $Revision: 1.13 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/util.h>
#include <sent/ngram2.h>
#include <sent/speech.h>
#include <sent/adin.h>

/** 
 * Output version of this libsent library.
 * 
 * @param strm [in] file pointer to output
 */
void
confout_version(FILE *strm)
{
  fprintf(strm, "version %s\n", LIBSENT_VERSION);
}

/** 
 * Output audio configuration of this libsent library.
 * 
 * @param strm [in] file pointer to output
 */
void
confout_audio(FILE *strm)
{
  fprintf(strm, " - Audio input\n");
#ifdef USE_MIC
  fprintf(strm, "    primary A/D-in driver   : %s (%s)\n", AUDIO_API_NAME, AUDIO_API_DESC);
#else
  fprintf(strm, "    primary A/D-in driver   : N/A\n");
#endif
  fprintf(strm, "    available drivers       :");
#ifdef HAS_ALSA
  fprintf(strm, " alsa");
#endif
#ifdef HAS_OSS
  fprintf(strm, " oss");
#endif
#ifdef HAS_ESD
  fprintf(strm, " esd");
#endif
#ifdef HAS_PULSEAUDIO
  fprintf(strm, " pulseaudio");
#endif
#ifdef USE_NETAUDIO
  fprintf(strm, " DATLink/NetAudio");
#endif
  fprintf(strm, "\n");
  fprintf(strm, "    wavefile formats        : %s\n", AUDIO_FORMAT_DESC);
  fprintf(strm, "    max. length of an input : %d samples, %d words\n", MAXSPEECHLEN, MAXSEQNUM);
}

/** 
 * Output language model configuration of this libsent library.
 * 
 * @param strm [in] file pointer to output
 */
void
confout_lm(FILE *strm)
{
  fprintf(strm, " - Language Model\n");
#ifdef CLASS_NGRAM
  fprintf(strm, "    class N-gram support    : yes\n");
#else
  fprintf(strm, "    class N-gram support    : no\n");
#endif
#ifdef USE_MBR
  fprintf(strm, "    MBR weight support      : yes\n");
#else
  fprintf(strm, "    MBR weight support      : no\n");
#endif
#ifdef WORDS_INT
  fprintf(strm, "    word id unit            : integer (%lu bytes)\n", sizeof(WORD_ID));
#else
  fprintf(strm, "    word id unit            : short (%lu bytes)\n", sizeof(WORD_ID));
#endif
}

/** 
 * Output acoustic model configuration of this libsent library.
 * 
 * @param strm [in] file pointer to output
 */
void
confout_am(FILE *strm)
{
  fprintf(strm, " - Acoustic Model\n");
/* 
 * #ifdef MULTIPATH_VERSION
 *   fprintf(strm, "	multi-path HMM handling : yes\n");
 * #else
 *   fprintf(strm, "	multi-path HMM handling : no\n");
 * #endif
 */
  fprintf(strm, "    multi-path treatment    : autodetect\n");
#ifdef ENABLE_MSD
  fprintf(strm, "    MSD-HMM support         : yes\n");
#endif
}

/** 
 * Output about linked libraries of this libsent library.
 * 
 * @param strm [in] file pointer to output
 */
void
confout_lib(FILE *strm)
{
  fprintf(strm, " - External library\n");
  fprintf(strm, "    file decompression by   : %s\n", GZIP_READING_DESC);
}

/** 
 * Output about process handling of this libsent library.
 * 
 * @param strm [in] file pointer to output
 */
void
confout_process(FILE *strm)
{
  fprintf(strm, " - Process hangling\n");
#ifdef FORK_ADINNET
  fprintf(strm, "    fork on adinnet input   : yes\n");
#else
  fprintf(strm, "    fork on adinnet input   : no\n");
#endif
}

/** 
 * Output all information of this libsent library.
 * 
 * @param strm [in] file pointer to output
 */
void
confout(FILE *strm)
{
  confout_version(strm);
  confout_audio(strm);
  confout_lm(strm);
  confout_am(strm);
  confout_lib(strm);
  confout_process(strm);
}
