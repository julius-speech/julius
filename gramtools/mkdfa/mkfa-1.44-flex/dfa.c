/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */
#include "mkfa.h"
#include "dfa.h"
#include "nfa.h"

void r_makeDFA( FA *fa );
ARC *unifyFA( FA *dstFA, ARC *prevarc, ARC *curarc, FA *prevFA );
void usArc2nsArc( FA *fa );
void connectUnifyFA( FA *fa, int inp, FA *nextFA, FLAG reserved,
		    CLASSFLAGS accpt, CLASSFLAGS start );
ARC *unconnectFA( FA *srcFA, ARC *arcPrev, ARC *arc );
void killFA( FA *fa );
void killIsolatedLoop( FA *vanishFA, FA *curFA );
int chkIsolatedLoop( FA *vanishFA, FA *curFA );
UNIFYARC *appendUnifyArc( UNIFYARC *top, int inp, FA *fa, int reserved );
FALIST *appendGroup( FALIST *groupTop, FA *fa );
FALIST *insertFAlist( FALIST *top, FALIST *preAtom, FALIST *nextAtom, FA *fa );
FA *chkGroup( FALIST *group, CLASSFLAGS accptFlag,
	     CLASSFLAGS startFlag, FLAG *newFlag );
int cmpFAlist( FALIST *group1, FALIST *group2 );
FALIST *volatileFA( FALIST *volatileList, FA *fa );
void unvolatileFA( FALIST *volatileList );
void verboseGroup( FALIST *group );

static FALIST *GroupList = NULL; /* 状態融合された新状態のリスト */

static int DFAtravTotal = 0;   /* DFA作成時に立ち寄ったノード数 */
static int DFAtravSuccess = 0; /* そのうち今までに立ち寄っていなかった数 */
static int FAprocessed = 0;    /* 現在のステップにおいて処理されたFAの数 */
extern int FAtotal;            /* FAの総数 */

extern FA *FAlist;             /* FAネットワークにおける開始FAのポインタ */
extern char FAfile[ 1024 ];    /* FAファイル名(DFAorNFA) */
extern int SW_Verbose;
extern int SW_Quiet;
extern int SW_SemiQuiet;
extern int SW_Compati;
extern int NoNewLine;          /* 複数の表示モードで改行問題を解決する */
extern char Clipboard[ 1024 ]; /* sprintf用の一時書き込みバッファ */

void makeDFA( void )
{
    if( !SW_Quiet ){
	fprintf( stderr, "Now making deterministic finite automaton" );
	NoNewLine = 1;
    }
    r_makeDFA( FAlist );
    if( !SW_Quiet ){
	fprintf( stderr, "\rNow making deterministic finite automaton[%d/%d] \n", FAprocessed, FAtotal );
	if( FAtotal != FAprocessed ){
	    fprintf( stderr, "* %d released FA nodes are left on isolated loop\n", FAtotal - FAprocessed );
	}
	NoNewLine = 0;
    }
    /* 何かバグがあったとき恐いが孤立ループのチェックが
       不可能なのでしょうがない */
    FAtotal = FAprocessed;
    if( SW_Verbose ){
	verboseMes( "** traversing efficiency ( success/total )" );
	verboseMes( "r_makeDFA:     %d/%d(%d%%)",
		DFAtravSuccess, DFAtravTotal, 100*DFAtravSuccess/DFAtravTotal);
    }
    newLineAdjust();
    freeFAlist( GroupList );
}

void r_makeDFA( FA *fa )
{
    ARC *prevarc = NULL;
    ARC *curarc;
    int inp;
    int bundleNum;
    FLAG reserved = 0;
    int i;
    FLAG newFlag;
    FALIST *volatileList = NULL;
    CLASSFLAGS unifyAccptFlag;
    CLASSFLAGS unifyStartFlag;

    verboseMes( "[func]r_makeDFA(FA %08x)", (long)fa );
    DFAtravTotal++;
    if( fa->traversed == 1 ){
	verboseMes( "traversed..." );
	return;
    }
    fa->traversed = 1;
    DFAtravSuccess++;

    FAprocessed++;
    if( !SW_SemiQuiet ){
	fprintf( stderr, "\rNow making deterministic finite automaton[%d/%d] ", FAprocessed, FAtotal );
	NoNewLine = 1;
    }
    curarc = fa->nsList;
    while( curarc != NULL ){
	FA *unifyingDstFA = NULL;
	{
	    ARC *arc = curarc;
	    int inp = arc->inp;
	    FALIST *group = NULL;
	    CLASSFLAGS accptFlag = 0;
	    CLASSFLAGS startFlag = 0;

	    bundleNum = 0;
	    while( 1 ){
		if( arc == NULL || arc->inp != inp ) break;
		group = appendGroup( group, arc->fa );
		accptFlag |= arc->fa->accpt;
		startFlag |= arc->fa->start;
		arc = arc->next;
		bundleNum++;
	    }
	    if( bundleNum > 1 ){
		unifyingDstFA = chkGroup( group, accptFlag,
					 startFlag,&newFlag );
	    } else {
		/* この下4行はブロック外のwhileに対してのもの */
		freeFAlist( group );
		prevarc = curarc;
		curarc = curarc->next;
		continue;
	    }
	}

	inp = curarc->inp;
	unifyAccptFlag = 0;
	unifyStartFlag = 0;
	for( i = 0; i < bundleNum; i++ ){
	    unifyAccptFlag |= curarc->accpt;
	    unifyStartFlag |= curarc->start;
	    if( !newFlag ){
/*		volatileList = volatileFA( volatileList, curarc->ns );*/
		curarc = unconnectFA( fa, prevarc, curarc );
	    } else {
		if( curarc->fa == fa /* self-loop */ ){
		    reserved = 1;
/*		    volatileList = volatileFA( volatileList, fa );*/
		    curarc = unconnectFA( fa, prevarc, curarc );
		} else {
		    curarc = unifyFA( unifyingDstFA, prevarc, curarc, fa );
		}
	    }
	}
	connectUnifyFA( fa, inp, unifyingDstFA, reserved,
		       unifyAccptFlag, unifyStartFlag );
	reserved = 0;
    }
    usArc2nsArc( fa );
/*    unvolatileFA( volatileList );*/

    curarc = fa->nsList;
    while( curarc != NULL ){
	r_makeDFA( curarc->fa );
	curarc = curarc->next;
    }
}

void connectUnifyFA( FA *fa, int inp, FA *nextFA, FLAG reserved,
		    CLASSFLAGS accpt, CLASSFLAGS start )
{
    /* unifyFAへのアークのリストに入力の辞書順で適切位置に挿入
       また同じものがある場合登録しない */
    /* nextFA のpsNumをインクリメントしない */
    UNIFYARC *newArc;
    UNIFYARC *curArc = NULL;
    UNIFYARC *nextArc;
    UNIFYARC *top = fa->usList;

    if( (newArc = calloc( 1, sizeof(UNIFYARC) )) == NULL ){
	errMes( "Can't alloc forward arc buffer of finite automaton." );
    }
    newArc->inp = inp;
    newArc->us = nextFA;
    newArc->reserved = reserved;
    newArc->accpt = accpt;
    newArc->start = start;

    if( (nextArc = top) != NULL ){
	while( 1 ){
	    if( nextArc->inp > inp ) break;
	    if( nextArc->inp == inp && nextArc->us == nextFA ) return;
	    curArc = nextArc;
	    if( (nextArc = nextArc->next) == NULL ) break;
	}
    }
    if( curArc == NULL ){
	newArc->next = top;
	fa->usList = newArc;
    } else {
	newArc->next = nextArc;
	curArc->next = newArc;
    }
}

void usArc2nsArc( FA *fa )
{
    UNIFYARC *uptr;
    UNIFYARC *disused_uptr;
    ARC *nptr;
    ARC *newarc;

    uptr = fa->usList;
    while( uptr != NULL ){
	if( (newarc = calloc( 1, sizeof(ARC) )) == NULL ){
	    errMes( "Can't alloc forward arc buffer of finite automaton." );
	}
	connectFA( fa, uptr->inp, uptr->us, uptr->accpt, uptr->start );
	uptr = uptr->next;
    }
    
    uptr = fa->usList;
    while( uptr != NULL ){
	if( uptr->reserved ){
	    uptr->us->accpt |= fa->accpt;
	    nptr = fa->nsList;
	    while( nptr != NULL ){
		connectFA( uptr->us, nptr->inp, nptr->fa, nptr->accpt, nptr->start );
		nptr = nptr->next;
	    }
	}
	disused_uptr = uptr;
	uptr = uptr->next;
	free( disused_uptr );
    }
}

FALIST *volatileFA( FALIST *volatileList, FA *fa )
{
    FALIST *atom;

    if( (atom = malloc( sizeof(FALIST) )) == NULL ){
	errMes( "Can't alloc FA list buffer." );
    }
    fa->volatiled = 1;

    atom->fa = fa;
    atom->next = volatileList;
    return( atom );
}

void unvolatileFA( FALIST *volatileList )
{
    FALIST *atom;
    FA *fa;

    while( volatileList != NULL ){
	atom = volatileList;
	fa = atom->fa;
	fa->volatiled = 0;
/*	if( chkIsolatedLoop( fa, fa ) ){
	    killIsolatedLoop( fa, fa );
	}*/
	volatileList = volatileList->next;
	free( atom );
    }
}

ARC *unifyFA( FA *dstFA, ARC *prevarc, ARC *curarc, FA *prevFA )
{
    FA *srcFA = curarc->fa;
    ARC *arc = srcFA->nsList;

    dstFA->accpt |= srcFA->accpt;
    while( arc != NULL ){
	connectFA( dstFA, arc->inp, arc->fa, arc->accpt, arc->start );
	arc = arc->next;
    }
    return( unconnectFA( prevFA, prevarc, curarc ) );
}

ARC *unconnectFA( FA *srcFA, ARC *arcPrev, ARC *arc )
/* 切ったアークの次のアークを返す */
{
    /* 指定の前ノードとの接続を切り、消滅すべきなら次ノードすべてとの接続を
       切って消滅させる。*/

    ARC *arcNext = arc->next;
    FA *vanishFA;

    if( arcPrev == NULL ){
	srcFA->nsList = arcNext;
    } else {
	arcPrev->next = arcNext;
    }
    vanishFA = arc->fa;
    free( arc );

    if( --vanishFA->psNum == 0 ){
	killFA( vanishFA );
    }/* else if( chkIsolatedLoop( vanishFA, vanishFA ) ){
	killIsolatedLoop( vanishFA, vanishFA );
    }*/
    return( arcNext );
}

void killFA( FA *fa )
{
    ARC *arc = fa->nsList;
    verboseMes( "a FA node is vanished" );
    while( arc != NULL ){
	arc = unconnectFA( fa, NULL, arc );
    }
    free( fa );
    FAtotal--;
}

int chkIsolatedLoop( FA *vanishFA, FA *curFA )
/* もし自分が消滅すると仮定したら自分へのアークが無くなるかをチェック
   すなわちループによる生き残りを駆除する */
{
    ARC *arc;
    int result;

    if( curFA->volatiled ) return( 0 );
    if( curFA->psNum > 1 ) return( 0 );
    arc = curFA->nsList;

    while( arc != NULL ){
	FA *nextFA = arc->fa;
	if( nextFA == vanishFA ) return( 1 );
	result = chkIsolatedLoop( vanishFA, nextFA );
	if( result ) return( 1 );
	arc = arc->next;
    }
    return( 0 );
}

void killIsolatedLoop( FA *vanishFA, FA *curFA )
/* もし自分が消滅すると仮定したら自分へのアークが無くなるかをチェック
   すなわちループによる生き残りを駆除する */
{
    ARC *arc;
    ARC *prevarc = NULL;

    if( curFA->volatiled ) return;
    if( curFA->psNum > 1 ) return;

    arc = curFA->nsList;
    while( arc != NULL ){
	FA *nextFA = arc->fa;
	if( nextFA != vanishFA ){
	    unconnectFA( curFA, prevarc, arc );
	}
	prevarc = arc;
	arc = arc->next;
    }
    free( curFA );
    FAtotal--;
}

FALIST *appendGroup( FALIST *groupTop, FA *fa )
{
    /* faが融合状態でないならFAのポインタをソートしてグループリストへ
       融合状態ならその構成リストとグループリストを合わせてソートする */

    FALIST *preAtom = NULL;
    FALIST *curAtom = groupTop;
    FALIST *srcCurAtom = NULL;
    long cmp;

    if( fa->group == NULL ){
	while( curAtom != NULL ){
	    cmp = (long)fa - (long)curAtom->fa;
	    if( cmp == 0 ) return( groupTop );
	    if( cmp < 0 ) break;
	    preAtom = curAtom;
	    curAtom = curAtom->next;
	}
	return( insertFAlist( groupTop, preAtom, curAtom, fa ) );
    } else {
	/* srcCurAtomがソートされていることを利用すればもっと処理が速くなるが
	   そうするとなぜか状態数が多少増えてしまうので必ずしも保証されていないかも
	   "for"の注釈を取って、preAtom = NULL; curAtom = groupTop;(2個所) を殺す */
	for( srcCurAtom = fa->group; srcCurAtom != NULL;
	    srcCurAtom = srcCurAtom->next ){
	    if( curAtom == NULL ){
		groupTop = insertFAlist( groupTop, preAtom, curAtom, srcCurAtom->fa );
		preAtom = NULL;
		curAtom = groupTop;
	    }
/*		for( ; srcCurAtom != NULL; srcCurAtom = srcCurAtom->next ){
		    groupTop = insertFAlist( groupTop, preAtom, NULL, srcCurAtom->fa );
		    if( preAtom == NULL ){
			preAtom = groupTop->next;
		    } else {
			preAtom = preAtom->next;
		    }
		}
		break;
	    }*/
	    cmp = (long)srcCurAtom->fa - (long)curAtom->fa;
	    if( cmp == 0 ) continue;
	    if( cmp < 0 ){
		groupTop = insertFAlist( groupTop, preAtom, curAtom, srcCurAtom->fa );
		preAtom = NULL;
		curAtom = groupTop;
	    } else {
		preAtom = curAtom;
		curAtom = curAtom->next;
	    }
	}
	return( groupTop );
    }
}

FALIST *insertFAlist( FALIST *top, FALIST *preAtom, FALIST *nextAtom, FA *fa )
{
    FALIST *atom;

    if( (atom = malloc( sizeof(FALIST) )) == NULL ){
	errMes( "Can't alloc group buffer for unifying FA" );
    }
    atom->fa = fa;
    if( preAtom == NULL ){
	atom->next = nextAtom;
	return( atom );
    } else {
	preAtom->next = atom;
	atom->next = nextAtom;
	return( top );
    }
}

FA *chkGroup( FALIST *group, CLASSFLAGS accptFlag ,
	     CLASSFLAGS startFlag, FLAG *newFlag )
{
    FALIST *curGroupList = GroupList;
    FALIST *preGroupList = NULL;
    int cmp;
    FA *fa;

    while( curGroupList != NULL ){
	cmp = cmpFAlist( curGroupList->fa->group, group );
	if( cmp == 0 ){
	    if( SW_Compati || (accptFlag == curGroupList->fa->accpt
	       || startFlag == curGroupList->fa->start) ){
		freeFAlist( group );
		*newFlag = 0;
		return( curGroupList->fa );
	    }
	}
	if( cmp < 0 ) break;
	preGroupList = curGroupList;
	curGroupList = curGroupList->next;
    }
    if( SW_Verbose ){ 
	verboseGroup( group );
    }
    fa = makeNewFA();
    GroupList = insertFAlist( GroupList, preGroupList, curGroupList, fa );
    fa->group = group;
    fa->accpt = accptFlag;
    fa->start = startFlag;
    *newFlag = 1;
    return( fa );
}

void verboseGroup( FALIST *group )
{
    verboseMes( "Created New Group" );
    while( group != NULL ){
	verboseMes( "  FAadr: %08x", (long)group->fa );
	group = group->next;
    }
}

int cmpFAlist( FALIST *group1, FALIST *group2 )
{
    long cmp;

    while( 1 ){
	if( group1 == NULL && group2 == NULL ) return( 0 );
	if( group1 == NULL ) return( -1 );
	if( group2 == NULL ) return( 1 );
	cmp = (long)group1->fa - (long)group2->fa;
	if( cmp != 0 ) return( cmp );
	group1 = group1->next;
	group2 = group2->next;
    }
}
