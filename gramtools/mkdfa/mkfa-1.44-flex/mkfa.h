/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <stdlib.h>

#define VER_NO "ver.1.44-flex-p1"
#define SYMBOL_LEN 256
typedef short FLAG;
typedef unsigned int CLASSFLAGS;
#define CLASSFLAG_MAX sizeof(CLASSFLAGS)*8

typedef struct _BODY{
    char name[ SYMBOL_LEN ];         /* 構成するクラスの名前 */
    FLAG abort;                      /* BODYの途中終了フラグ */
    struct _BODY *next;
} BODY;

typedef struct _BODYLIST{
    BODY *body;
    struct _BODYLIST *next;
} BODYLIST;

typedef struct _CLASS{
    short no;                        /* クラス番号 非終端と終端で独立割当
					非終端は#31までの登録、その他は#-1 */
    char name[ SYMBOL_LEN ];         /* クラスの名前 */
    struct _CLASS *next;             /* 次のクラスへのポインタ */
    BODYLIST *bodyList;              /* クラスのボディのリストへのポインタ */
    int branch;                      /* 定義数 */
    FLAG usedFA;                     /* クラスが使用されたか */
    FLAG used;
    FLAG tmp;                        /* 最小化のため一時的にできたクラスか */
} CLASS;

/*
   bodyList, branch は終端と非終端で意味が全く違う
   non-terminal:
    bodyList: 構成するクラス名のリストのリストへのポインタ
    branch:   配列の数(定義の数だけ存在する)
   terminal:
    bodyList: その終端記号に該当する実際の単語
    branch:   単語の種類に-1をかけたもの
   終端と非終端は正負で判断する
*/

typedef struct _ARC{
    int inp;                         /* 入力 */
    struct _FA *fa;                  /* 遷移先のFA */
    CLASSFLAGS start;                /* クラス開始フラグ */
    CLASSFLAGS accpt;                /* クラス受理フラグ */
    struct _ARC *next;               /* リストの次項目 */
} ARC;

typedef struct _UNIFYARC{
    int inp;                         /* 入力 */
    struct _FA *us;                  /* FA次状態 */
    CLASSFLAGS start;                /* クラス開始フラグ */
    CLASSFLAGS accpt;                /* クラス受理フラグ */
    struct _UNIFYARC *next;          /* リストの次項目 */
    FLAG reserved;                   /* 自己ループの枝を融合する時は、変換
					処理が完全に終了してから行なう。そ
					の未処理フラグ */
} UNIFYARC;

typedef struct _FALIST{
    struct _FA *fa;
    struct _FALIST *next;
} FALIST;

typedef struct _FA{
    /* common */
    int stat;                        /* 状態番号(3つ組作成時に振られる) */
    ARC *nsList;                     /* 入力と次状態リスト */
    CLASSFLAGS start;                /* クラス開始フラグ(全てのアークのor) */
    CLASSFLAGS accpt;                /* クラス受理フラグ(全てのアークのor) */
    CLASSFLAGS aStart;               /* 着目中のアークのクラス開始フラグ */
    FLAG traversed;                  /* 立寄 1:NFA->DFA 2:3つ組作成時 */

    /* for DFA */
    int psNum;                       /* ARCで指されているアーク数 */
                   /* connectUnifyFAではincrementされないことに注意。 */
    UNIFYARC *usList;                /* NFA->DFAで融合された次状態 */
    FALIST *group;                   /* 融合したときの構成する状態 */
    FLAG volatiled;    /* アーク変更中のため孤立判定を取りやめろの意 */
} FA;

void errMes( char *fmt, ... );
void warnMes( char *fmt, ... );
void verboseMes( char *fmt, ... );
void setGramFile( char *fname );
void setVoca( void );
CLASS *getClass( char *name );
void setGram( void );

#define newLineAdjust()\
{\
    if( NoNewLine ){\
	putc( '\n', stderr );\
	NoNewLine = 0;\
    }\
}
