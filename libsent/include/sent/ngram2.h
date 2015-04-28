/**
 * @file   ngram2.h
 *
 * <EN>
 * @brief Definitions for word N-gram
 *
 * This file defines a structure for word N-gram language model.
 * Julius now support N-gram for arbitrary N.
 *
 * Both direction of forward (left-to-right) N-gram and backward
 * (right-to-left) N-gram is supported.  Since the final recognition
 * process is done by right-to-left direction, using backward N-gram
 * is recommended. 
 *
 * A forward 2-gram is necessary for the 1st recognition pass.  If a
 * forward N-gram is specified, Julius simply use its 2-gram part for
 * the 1st pass.  If only backward N-gram is specified, Julius calculate
 * the forward probability from the defined backward N-gram by the
 * equation "P(w_2|w_1) = P(w_1|w_2) * P(w_2) / P(w_1)."  If both
 * forward N-gram and backward N-gram are specified, Julius uses the
 * 2-gram part of the forward n-gram at the 1st pass, and use the
 * backward N-gram at the 2nd pass as the main LM.  Note that the last
 * behavior is the same as previous versions (<=3.5.x)
 *
 * ARPA standard format and Julius binary format is supported.  The
 * binary format can be loaded much faster at startup, so it is
 * recommended to use binary format by converting from ARPA format
 * N-gram beforehand.  All combination of N-gram (forward only,
 * backward only, forward 2-gram + backward N-gram) is supported.
 *
 * @sa mkbingram
 *
 * For memory efficiency of holding the huge word N-gram on memory, Julius
 * merges the two language model into one structure.  So the forward bigram and
 * reverse trigram should meet the following requirements:
 *
 *     - their vocabularies should be the same.
 *     - their unigram probabilities of each word should be the same.
 *     - the same bigram tuple sets are defined.
 *     - the bigram tuples for context word sequences of existing trigram
 *     tuples should exist in both.
 * 
 *  The first three requirements can be fullfilled easily if you train the
 *  forward bigram and reverse trigram on the same training text.
 *  The last condition can be qualified if you set a cut-off value of trigram
 *  which is larger or equal to that of bigram.  These conditions are checked
 *  when Julius or mkbingram reads in the ARPA models, and output error if
 *  not cleared.
 *
 *  From 3.5, tuple ID on 2-gram changed from 32bit to 24bit, and 2-gram
 *  back-off weights will not be saved if the corresponding 3-gram is empty.
 *  They will be performed when reading N-gram to reduce memory size.
 * </EN>
 * <JA>
 * @brief 単語N-gram言語モデルの定義
 *
 * このファイルには単語N-gram言語モデルを格納するための構造体定義が
 * 含まれています．Julius はN-gramにおいて任意の N をサポートしました．
 *
 * 通常の前向き (left-to-right) と後向き (right-to-left) の N-gram が
 * サポートされています．認識の最終パス（第2パス）は後向きに行われるので，
 * 後向き N-gram を使用することを推奨します．
 *
 * 第1パスの実行には前向き2-gramが必要です．前向き N-gram のみが
 * 与えられた場合，Julius はその2-gramの部分を使います．
 * 後向きN-gramのみが与えられた場合，Julius は
 * 式 "P(w_2|w_1) = P(w_1|w_2) * P(w_2) / P(w_1)" にしたがって
 * 前向き2-gramを推定します．前向きと後向きの両方指定された場合は，
 * 前向きN-gramの2-gram部分が第1パスで用いられ，第2パスでは後向きN-gram
 * が使われます．この両方指定したときの挙動は以前のバージョン (<=3.5.3)
 * と同じです．
 * 
 * 入力ファイル形式は
 * ARPA形式とJulius独自のバイナリ形式の２つをサポートしています．
 * 読み込みは後者のほうが高速です．前向き，後向き，両方の
 * 全てのパターンに対応しています．
 *
 * @sa mkbingram
 *
 * NGRAM_INFO ではメモリ量節約のため，これらを一つの構造体で表現しています．
 * このことから，Julius は使用するこれら２つの言語モデルが
 * 以下を満たすことを要求します．
 * 
 *    - 語彙が同一であること
 *    - 各語彙の1-gram確率が同一であること
 *    - 同じ 2-gram tuple 集合が定義されていること
 *    - 3-gram のコンテキストである単語組の2-gramが定義されていること
 *
 * 上記の前提のほとんどは，これらの２つのN-gramを同一のコーパスから
 * 学習することで満たされます．最後の条件については，3-gram のカットオフ
 * 値に 2-gram のカットオフ値と同値かそれ以上の値を指定すればOKです．
 * 与えられたN-gramが上記を満たさない場合，Julius はエラーを出します．
 * </JA>
 * 
 * @author Akinobu LEE
 * @date   Fri Feb 11 15:04:02 2005
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

#ifndef __SENT_NGRAM2_H__
#define __SENT_NGRAM2_H__

#include <sent/stddefs.h>
#include <sent/ptree.h>

typedef unsigned int NNID;	      ///< Type definition for N-gram entry ID (full)
#define NNID_INVALID 0xffffffff  ///< Value to indicate no id (full)
#define NNID_MAX 0xfffffffe	///< Value of maximum value (full)

typedef unsigned char NNID_UPPER; ///< N-gram entry ID (24bit: upper bit)
typedef unsigned short NNID_LOWER; ///< N-gram entry ID (24bit: lower bit)
#define NNID_INVALID_UPPER 255	///< Value to indicate no id at NNID_UPPER (24bit)
#define NNID_MAX_24 16711679        ///< Allowed maximum number of id (255*65536-1) (24bit)

/// Default word string of beginning-of-sentence word
#define BEGIN_WORD_DEFAULT "<s>"
/// Default word string of end-of-sentence word
#define END_WORD_DEFAULT "</s>"
/// Default word string of unknown word for open vocabulary
#define UNK_WORD_DEFAULT "<unk>"
#define UNK_WORD_DEFAULT2 "<UNK>"
/// Maximum length of unknown word string
#define UNK_WORD_MAXLEN 30

/**
 * N-gram entries for a m-gram (1 <= m <= N)
 * 
 */
typedef struct {
  NNID totalnum;		///< Number of defined tuples
  boolean is24bit;		///< TRUE if this m-gram uses 24bit index for tuples instead of 32bit
  NNID bgnlistlen;		///< Length of bgn and num, should be the same as @a context_num of (m-1)-gram
  NNID_UPPER *bgn_upper;	///< Beginning ID of a tuple set whose context is the (m-1) tuple for 24bit mode (upper 8bit)
  NNID_LOWER *bgn_lower;	///< Beginning ID of a tuple set whose context is the (m-1) tuple for 24bit mode (lower 16bit)
  NNID *bgn;			///< Beginning ID of a tuple set whose context is the (m-1) tuple for 32bit mode
  WORD_ID *num;		///< Size of a tuple set whose context is the (m-1) tuple

  WORD_ID *nnid2wid;		///< List of Word IDs of edge word of the tuple
  LOGPROB *prob;		///< Log probabilities of edge word of the tuple

  NNID context_num;		///< Number of tuples to be a context of (m+1)-gram (= number of defined back-off weights)
  LOGPROB *bo_wt;		///< Back-off weights for (m+1)-gram, the length is @a context_num if @a ct_compaction is TRUE, or @a totalnum if FALSE.
  boolean ct_compaction;	///< TRUE if use compacted index for back-off contexts
  NNID_UPPER *nnid2ctid_upper;	///< Index to map tuple ID of this m-gram to valid context id (upper 8bit)
  NNID_LOWER *nnid2ctid_lower;	///< Index to map tuple ID of this m-gram to valid context id (upper 16bit)

} NGRAM_TUPLE_INFO;

/**
 * @brief Main N-gram structure
 *
 * bigrams and trigrams are stored in the form of sequential lists.
 * They are grouped by the same context, and referred from the
 * context ((N-1)-gram) data by the beginning ID and its number.
 * 
 */
typedef struct __ngram_info__ {
  int n;			///< N-gram order (ex. 3 for 3-gram)
  int dir;			///< direction (either DIR_LR or DIR_RL)
  boolean from_bin;		///< TRUE if source was bingram, otherwise ARPA
  boolean bigram_index_reversed;		///< TRUE if read from old (<=3.5.3) bingram, in which case the 2-gram tuple index is reversed (DIR_LR) against the RL 3-gram.
  boolean bos_eos_swap;		///< TRUE if swap BOS and SOS on backward N-gram
  WORD_ID max_word_num;		///< N-gram vocabulary size
  char **wname;			///< List of word strings.
  PATNODE *root;		///< Root of index tree to search n-gram word ID from its name
  WORD_ID unk_id;		///< Word ID of unknown word.
  int unk_num;			///< Number of dictionary words that are not in this N-gram vocabulary
  LOGPROB unk_num_log;		///< Log10 value of @a unk_num, used for calculating probability of unknown words
  boolean isopen;		///< TRUE if dictionary has unknown words, which does not appear in this N-gram

  NGRAM_TUPLE_INFO *d;	///< Main body of N-gram info

  /* for pass1 */
  LOGPROB *bo_wt_1;		///< back-off weights for 2-gram on 1st pass
  LOGPROB *p_2;			///< 2-gram prob for the 1st pass
  LOGPROB (*bigram_prob)(struct __ngram_info__ *, WORD_ID, WORD_ID); ///< Pointer of a function to compite bigram probability on the 1st pass.  See bi_prob_func_set() for details

  BMALLOC_BASE *mroot;		///< Pointer for block memory allocation for lookup index

} NGRAM_INFO;


/* Definitions for binary N-gram */

/// Header string to identify version of bingram (v3: <= rev.3.4.2)
#define BINGRAM_IDSTR "julius_bingram_v3"
/// Header string to identify version of bingram (v4: <= rev.3.5.3)
#define BINGRAM_IDSTR_V4 "julius_bingram_v4"
/// Header string to identify version of bingram (v5: >= rev.4.0)
#define BINGRAM_IDSTR_V5 "julius_bingram_v5"
/// Bingram header size in bytes
#define BINGRAM_HDSIZE 512
/// Bingram header info string to identify the unit byte (head)
#define BINGRAM_SIZESTR_HEAD "word="
/// Bingram header string that indicates 4 bytes unit
#define BINGRAM_SIZESTR_BODY_4BYTE "4byte(int)"
/// Bingram header string that indicates 2 bytes unit
#define BINGRAM_SIZESTR_BODY_2BYTE "2byte(unsigned short)"
#ifdef WORDS_INT
#define BINGRAM_SIZESTR_BODY BINGRAM_SIZESTR_BODY_4BYTE
#else
#define BINGRAM_SIZESTR_BODY BINGRAM_SIZESTR_BODY_2BYTE
#endif
/// Bingram header info string to identify the byte order (head) (v4)
#define BINGRAM_BYTEORDER_HEAD "byteorder="
/// Bingram header info string to identify the byte order (body) (v4)
#ifdef WORDS_BIGENDIAN
#define BINGRAM_NATURAL_BYTEORDER "BE"
#else
#define BINGRAM_NATURAL_BYTEORDER "LE"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* function declaration */
NNID search_ngram(NGRAM_INFO *ndata, int n, WORD_ID *w);
LOGPROB ngram_prob(NGRAM_INFO *ndata, int n, WORD_ID *w);
LOGPROB uni_prob(NGRAM_INFO *ndata, WORD_ID w);
LOGPROB bi_prob(NGRAM_INFO *ndata, WORD_ID w1, WORD_ID w2);
void bi_prob_func_set(NGRAM_INFO *ndata);

boolean ngram_read_arpa(FILE *fp, NGRAM_INFO *ndata, boolean addition);
boolean ngram_read_bin(FILE *fp, NGRAM_INFO *ndata);
boolean ngram_write_bin(FILE *fp, NGRAM_INFO *ndata, char *header_str);

boolean ngram_compact_context(NGRAM_INFO *ndata, int n);

void ngram_make_lookup_tree(NGRAM_INFO *ndata);
WORD_ID ngram_lookup_word(NGRAM_INFO *ndata, char *wordstr);
WORD_ID make_ngram_ref(NGRAM_INFO *, char *);

NGRAM_INFO *ngram_info_new();
void ngram_info_free(NGRAM_INFO *ngram);
boolean init_ngram_bin(NGRAM_INFO *ndata, char *ngram_file);
boolean init_ngram_arpa(NGRAM_INFO *ndata, char *ngram_file, int dir);
boolean init_ngram_arpa_additional(NGRAM_INFO *ndata, char *bigram_file);
void set_default_unknown_id(NGRAM_INFO *ndata);
void set_unknown_id(NGRAM_INFO *ndata, char *str);

void print_ngram_info(FILE *fp, NGRAM_INFO *ndata);

#include <sent/vocabulary.h>
boolean make_voca_ref(NGRAM_INFO *ndata, WORD_INFO *winfo);
void fix_uniprob_srilm(NGRAM_INFO *ndata, WORD_INFO *winfo);

#ifdef __cplusplus
}
#endif

#endif /* __SENT_NGRAM2_H__ */
