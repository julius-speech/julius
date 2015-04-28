/*
 * config.h
 */

#define ASCII         0
#define JIS           1
#define EUC           2
#define SJIS          3
#define NEW           4
#define OLD           5
#define NEC           6
#define EUCORSJIS     7
#define NUL           0
#define LF            10
#define FF            12
#define CR            13
#define ESC           27
#define SS2           142
#define TRUE          1
#define FALSE         0

/* 半角カナ(SJIS) を強制的に全角カナに変更する */
#undef NO_HANKAKU_SJIS

/* ひらがな、カタカナの並びから EUC , SJIS を推定する */
#define KANA_NARABI

/* EUC , SJIS を推定を始めの一度しか行なわない */
/* (ファイルを一つしかひらかないなら有効でいい) */
#undef USE_CACHE_KANA_NARABI

/* strdup関数がシステムにあればdefine */
#define HAVE_STRDUP     1

/* デバック用の関数を有効にする */
#undef DEBUG

/* ロケール名(大文字、小文字は区別されない) */
#define SJIS_LOCALE_NAME  "ja_JP.SJIS", "ja_JP.PCK"
#define JIS_LOCALE_NAME   "ja_JP.JIS", "ja_JP.jis7"
#define EUC_LOCALE_NAME   "ja_JP.ujis", "ja_JP.EUC",\
                          "ja_JP.eucJP","japanese", "ja"
