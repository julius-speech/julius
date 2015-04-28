/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */
#include "mkfa.h"
#include "nfa.h"

typedef struct _HIS{
    CLASS *class;                    /* クラスへのポインタ */
    FA *fa;                          /* 生成動作時のFA(再帰に用いる) */
    struct _HIS *prev;               /* 親の経歴へのポインタ */
    ARC *nsList;                     /* クラス開始ノードからの遷移可能アーク */
    FA *cloneFA;                     /* 再帰時の戻り先のFA */
} HIS;

typedef struct _TOKEN{               /* namingがちと苦しいが */
    CLASS *class;                    /* クラスへのpointer */
    FLAG abort;                      /* 途中脱出可能フラグ */
} TOKEN;

FA *r_makeNFA( CLASS *class, FA *fa, FA *exitFA, FALIST *orgExtraFAs, HIS *his );
int getNextToken( TOKEN *token, BODYLIST **pBodyList, BODY **pBody );
void connectFAforNFA( FA *fa, int input, FA *nextFA, HIS *his );
FA *appendFA( FA *fa, int input, HIS *his );
ARC *appendArc( ARC *top, FA *dst, int inp, CLASSFLAGS accpt, CLASSFLAGS start );
void appendHisArc( HIS *his, FA *fa, int inp, FA *nextFA, CLASSFLAGS accpt, CLASSFLAGS start );
void chkClassInfo( CLASS *class );
CLASS *getClass( char *name );
FA *getRecursion( CLASS *class, HIS *his );
void chkLeftRecursion( CLASS *class, FA *fa, HIS *his );
char *strAncestors( HIS *me, HIS *ancestor );

extern CLASS *ClassList;       /* クラスの線形リスト */
extern CLASS *ClassListTail;   /* クラスの線形リストの最後尾のノード */
extern CLASS *StartSymbol;     /* 開始記号のクラス */
extern FA *FAlist;             /* FAネットワークにおける開始FAのポインタ */
static int FAprocessed = 0;    /* 現在のステップにおいて処理されたFAの数 */
int FAtotal = 0;               /* FAの総数 */

extern int SW_Quiet;
extern int SW_SemiQuiet;
extern int NoNewLine;          /* 複数の表示モードで改行問題を解決する */

void makeNFA( void )
{
    if( StartSymbol == NULL ){
	errMes( "No definition of grammar" );
    }
    FAprocessed = 0;
    FAlist = makeNewFA();
    FAlist->psNum++; /* 開始ノードの削除を防ぐため */
    if( !SW_Quiet ){
	fprintf( stderr, "\rNow making nondeterministic finite automaton" );
	NoNewLine = 1;
    }
    r_makeNFA( StartSymbol, FAlist, NULL, NULL, NULL );
    if( !SW_Quiet ){
	fprintf( stderr, "\rNow making nondeterministic finite automaton[%d/%d]\n", FAtotal, FAtotal );
	NoNewLine = 0;
    }
    chkClassInfo( ClassList );
}

FA *r_makeNFA( CLASS *class, FA *fa, FA *exitFA, FALIST *orgExtraFAs, HIS *his )
{
    FA *baseFA = fa;             /* 当該クラスの入口ノード */
    FA *loopFA = NULL;           /* 再帰ループのノード */
    HIS curhis;                  /* 解析木の経歴における現在のノード */
    TOKEN curToken;              /* 現在注目のトークン */
    TOKEN nextToken;             /* 先読みトークン */
    FLAG exitFlag = 0;           /* 脱出できるトポロジーか */
    BODYLIST *bodyList = class->bodyList;
    BODY *body = NULL;
    FALIST *extraFAs = NULL;
    CLASSFLAGS initStartFlag;

    if( !SW_SemiQuiet ){
	fprintf( stderr, "\rNow making nondeterministic finite automaton[%d/%d]", FAtotal, FAtotal );
	NoNewLine = 1;
    }

    class->usedFA= 1;
    if( class->no >= 0 ){
	fa->start |= (1 << class->no);
	fa->aStart |= (1 << class->no);
    }
    initStartFlag = fa->aStart;

    if( exitFA == NULL ) exitFA = makeNewFA();
    chkLeftRecursion( class, fa, his );
    curhis.class = class;
    curhis.fa = fa;
    curhis.prev = his;
    curhis.nsList = NULL;
    curhis.cloneFA = NULL;

    while( 1 ){ /* 定義列間のループ */
	getNextToken( &nextToken, &bodyList, &body );
	extraFAs = cpyFAlist( extraFAs, orgExtraFAs ); /*C++なら代入一発…*/
	while( 1 ){ /* 定義列内のループ */
	    curToken = nextToken;
	    if( getNextToken( &nextToken, &bodyList, &body ) /*後続があるか?*/ ){
		if( curToken.class->branch > 0 /* 後続があり非終端 */ ){
		    if( (loopFA = getRecursion( nextToken.class, &curhis )) != NULL ){
			if( curToken.abort ){
			    /* この定義列は終了なので親から引き継いだ副出口も渡す*/
			    /* !curToken.abortなら終了でないのでNULLのまま */
			    extraFAs = appendFAlist( extraFAs, exitFA );
			    exitFlag = 1;
			}
			/* nextTokenがloopなのでそこへつなぐ */
			fa = r_makeNFA( curToken.class, fa, loopFA, extraFAs, &curhis );
			/* ただしその次に後続があるならエラー */
			if( getNextToken( &curToken, &bodyList, &body ) ){
			    errMes( "Symbols following recursion exist in class \"%s\"", class->name );
			}
			break;
		    } else {
			FALIST *anExtraFAs = NULL;
			if( curToken.abort ){
			    /* この定義列はこの場所で脱出する */
			    anExtraFAs = cpyFAlist( anExtraFAs, extraFAs );
			    anExtraFAs = appendFAlist( anExtraFAs, exitFA );
			    exitFlag = 1;
			}
			fa = r_makeNFA( curToken.class, fa, NULL, anExtraFAs, &curhis );
			freeFAlist( anExtraFAs );
			continue;
		    }
		} else { /* 後続があり終端 */
		    if( curToken.abort ){
			FA *extraFA;
			FALIST *pExtraFAs = extraFAs;
			connectFAforNFA( fa, curToken.class->no, exitFA, &curhis );
			while( pExtraFAs != NULL ){
			    extraFA = pExtraFAs->fa;
			    connectFAforNFA( fa, curToken.class->no, extraFA, &curhis );
			    pExtraFAs = pExtraFAs->next;
			}
			exitFlag = 1;
		    }
		    if( (loopFA = getRecursion( nextToken.class,&curhis )) != NULL ){
			connectFAforNFA( fa, curToken.class->no, loopFA, &curhis );
			if( getNextToken( &curToken, &bodyList, &body ) ){
			    errMes( "Symbols following recursion exist in class \"%s\"", class->name );
			}
			break;
		    } else {
			fa = appendFA( fa, curToken.class->no, &curhis );
			continue;
		    }
		}
	    } else { /* 後続がない */
		exitFlag = 1;
		if( curToken.class->branch > 0 ){
		    exitFA = r_makeNFA( curToken.class, fa, exitFA, extraFAs, &curhis );
		} else {
		    FA *extraFA;
		    FALIST *pExtraFAs = extraFAs;
		    while( pExtraFAs != NULL ){
			extraFA = pExtraFAs->fa;
			connectFAforNFA( fa, curToken.class->no, extraFA, &curhis );
			pExtraFAs = pExtraFAs->next;
		    }
		    connectFAforNFA( fa, curToken.class->no, exitFA, &curhis );
		}
		break;
	    }
	} /* 定義列内のループの終了 */

	if( class->no >= 0 ){
	    FALIST *extraFA = extraFAs;
	    while( extraFA != NULL ){
		extraFA->fa->accpt |= (1 << class->no);
		extraFA = extraFA->next;
	    }
	}
	if( bodyList == NULL ) break;
	fa = baseFA;
	fa->aStart = initStartFlag;
    } /* 定義列間のループの終了 */

    if( !exitFlag ){
	errMes( "Infinite definition is formed %s", strAncestors( curhis.prev, NULL ) );
    }
    if( class->no >= 0 ){
	exitFA->accpt |= (1 << class->no);
	if( curhis.cloneFA != NULL ){
	    curhis.cloneFA->accpt |= (1 << class->no);
	}
    }
    extraFAs = freeFAlist( extraFAs ); /*C++ならdestructorがやるのに…*/

    if( curhis.cloneFA == NULL ){
	ARC *curArc, *tmpArc;
	for( curArc = curhis.nsList; curArc != NULL; ){
	    curArc->fa->psNum--;
	    /* cloneFAが指す(予定だった)FAはもとの遷移のコピーなので
	       psNumが0の判定(そしてFAの消去)は省略できる(0にはならない)*/
	    tmpArc = curArc->next;
	    free( curArc );
	    curArc = tmpArc;
	}
    }
    return( exitFA );
}

FALIST *appendFAlist( FALIST *faList, FA *fa )
{
    FALIST *atom;

    if( (atom = calloc( 1, sizeof(FALIST) )) == NULL ){
	errMes( "Can't alloc FA list buffer." );
    }

    atom->fa = fa;
    atom->next = faList;
    return( atom );
}

FALIST *cpyFAlist( FALIST *dst, FALIST *src )
{
    if( dst != NULL ) dst = freeFAlist( dst );
    while( src != NULL ){
	dst = appendFAlist( dst, src->fa );
	src = src->next;
    }
    return( dst );
}

FALIST *freeFAlist( FALIST *faList )
{
    while( faList != NULL ){
	FALIST *atom = faList;
	faList = faList->next;
	free( atom );
    }
    return( NULL );
}

int getNextToken( TOKEN *token, BODYLIST **pBodyList, BODY **pBody )
{
    BODYLIST *bodyList = *pBodyList;
    BODY *body = *pBody;

    if( body == NULL ){
	body = bodyList->body;
    } else {
	body = body->next;
	if( body == NULL ){
	    bodyList = bodyList->next;
	    *pBodyList = bodyList;
	    *pBody = body;
	    return( 0 );
	}
    }
    if( (token->class = getClass( body->name )) == NULL ){
	errMes( "undefined class \"%s\"", body->name );
    }
    token->abort = body->abort;
    *pBodyList = bodyList;
    *pBody = body;
    return( 1 );
}

FA *makeNewFA( void )
{
    FA *newFA;
    if( (newFA = calloc( 1, sizeof(FA) )) == NULL ){
	errMes( "Can't alloc Finite Automaton buffer" );
    }
    newFA->stat = -1; /* まだ番号が振られていないの意味 */
    FAtotal++;
    return( newFA );
}

FA *appendFA( FA *fa, int input, HIS *his )
{
    FA *newFA;

    newFA = makeNewFA();
    connectFAforNFA( fa, input, newFA, his );
    return( newFA );
}

void connectFAforNFA( FA *fa, int inp, FA *nextFA, HIS *his )
{
    CLASSFLAGS startOnArc = fa->aStart;

    fa->aStart = 0;
    connectFA( fa, inp, nextFA, 0, startOnArc );
    appendHisArc( his, fa, inp, nextFA, 0, startOnArc );
}

void connectFA( FA *fa, int inp, FA *nextFA, CLASSFLAGS accpt, CLASSFLAGS start )
{
    /* 注: nextFAのpsNumをincrementする */

    fa->nsList = appendArc( fa->nsList, nextFA, inp, accpt, start );
    nextFA->psNum++;
}

ARC *appendArc( ARC *top, FA *dst, int inp, CLASSFLAGS accpt, CLASSFLAGS start )
{
    /* リストに入力の辞書順で適切位置に挿入
       また同じものがある場合アーク上のフラグをorする */
    ARC *newArc;
    ARC *curArc = NULL;
    ARC *nextArc;

    if( (newArc = calloc( 1, sizeof(ARC) )) == NULL ){
	errMes( "Can't alloc forward arc buffer of finite automaton." );
    }
    newArc->inp = inp;
    newArc->fa = dst;
    newArc->start = start;
    newArc->accpt = accpt;

    if( (nextArc = top) != NULL ){
	while( 1 ){
	    if( nextArc->inp > inp ) break;
	    if( nextArc->inp == inp && nextArc->fa == dst ){
		nextArc->start |= newArc->start;
		nextArc->accpt |= newArc->accpt;
		return( top );
	    }
	    curArc = nextArc;
	    if( (nextArc = nextArc->next) == NULL ) break;
	}
    }
    if( curArc == NULL ){
	newArc->next = top;
	return( newArc );
    } else {
	newArc->next = nextArc;
	curArc->next = newArc;
	return( top );
    }
}
    
void appendHisArc( HIS *his, FA *fa, int inp, FA *nextFA, CLASSFLAGS accpt, CLASSFLAGS start )
{
    /* 着目クラスの開始FAで遷移可能なら履歴バッファへ登録
       さらに親たちも遷移可能か調べる */
    while( his != NULL && his->fa == fa /* クラスの開始FAでない */ ){
	his->nsList = appendArc( his->nsList, nextFA, inp, accpt, start );
	if( his->cloneFA != NULL ) his->cloneFA->nsList = his->nsList;
	his = his->prev;
	nextFA->psNum++;
    }
}

void chkClassInfo( CLASS *class )
{
    CLASS *freeClass;
    int wrong = 0;

     while( 1 ){
	if( class == NULL ) break;
	if( class->branch > 0 && !class->usedFA && !class->tmp ){
	    warnMes( "Class \"%s\" isn't used", class->name );
	}
	if (! class->used) {
	  warnMes( "\"%s\" in voca not referred by grammar", class->name);
	  wrong = 1;
	}
	freeClass = class;
	class = class->next;
	free( freeClass );
    }
     if (wrong) {
       errMes( "Some vocabulary not referred in grammar, compilation terminated");
     }
}

FA *getRecursion( CLASS *class, HIS *his )
{
    while( his != NULL ){
	if( his->class == class ){
	    if( his->cloneFA == NULL ){
		his->cloneFA = makeNewFA();
		his->cloneFA->nsList = his->nsList;
	    }
	    return( his->cloneFA );
	}
	his = his->prev;
    }
    return( NULL );
}

void chkLeftRecursion( CLASS *class, FA *fa, HIS *his )
{
    HIS *hisPtr = his;

    while( hisPtr != NULL && hisPtr->fa == fa ){
	if( hisPtr->class == class ){
	    errMes( "Left recusion is formed %s", strAncestors( his, hisPtr ) );
	}
	hisPtr = hisPtr->prev;
    }
}

char *strAncestors( HIS *me, HIS *ancestor )
{
    static char ancestorsList[ 1024 ];
    if( me == NULL /* infinite errの発見の都合上NULLが来る場合がある */ ){
	sprintf( ancestorsList, "in class,\"%s\"", StartSymbol->name );
    } else if( me == ancestor ){
	sprintf( ancestorsList, "in class,\"%s\"", me->class->name );
    } else {
	static char className[ 1024 ];
	strcpy( ancestorsList, "between classes" );
	while( 1 ){
	    sprintf( className, ",\"%s\"", me->class->name );
	    strcat( ancestorsList, className );
	    if( me == ancestor ) break;
	    if( (me = me->prev) == NULL ) break;
	}
    }
    return( ancestorsList );
}
    
CLASS *getClass( char *name )
{
    CLASS *class = ClassList;
    if( class == NULL ) return( NULL );
    while( 1 ){
	if( strcmp( class->name, name ) == 0 ){
	  class->used = 1;
	    return( class );
	}
	class = class->next;
	if( class == NULL ) return( NULL );
    }
}
