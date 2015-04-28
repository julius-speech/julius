/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */
#include "mkfa.h"

typedef struct _TFA{                 /* 3つ組リスト用 */
    int stat;
    int inp;
    int ns;
    unsigned int accpt;
    struct _TFA *next;
} TFA;

void r_makeTriplet( FA *fa, FILE *fp );
int getNewStatNo( FA *fa );
FA *processTripletQueue( FA *fa );

static int FAprocessed = 0;    /* 現在のステップにおいて処理されたFAの数 */
extern int FAtotal;            /* FAの総数 */
static int TFAtravTotal = 0;   /* 3つ組作成時に立ち寄ったノード数 */
static int TFAtravSuccess = 0; /* そのうち今までに立ち寄っていなかった数 */

extern char FAfile[ 1024 ];    /* FAファイル名(DFAorNFA) */
extern FA *FAlist;             /* FAネットワークにおける開始FAのポインタ */
extern int NoNewLine;          /* 複数の表示モードで改行問題を解決する */
extern char Clipboard[ 1024 ]; /* sprintf用の一時書き込みバッファ */

extern int SW_Verbose;
extern int SW_Quiet;
extern int SW_SemiQuiet;
extern int SW_Compati;
extern int SW_EdgeStart;
extern int SW_EdgeAccpt;

void makeTriplet( void )
{
    FILE *fp_fa;
    FA *fa;

    FAprocessed = 0;
    if( (fp_fa = fopen( FAfile, "w" )) == NULL ){
	errMes( "Can't open dfa File for writting\"%s\"", FAfile );
    }
    getNewStatNo( FAlist );
    if( !SW_Quiet ){
	fprintf( stderr, "Now making triplet list" );
	NoNewLine = 1;
    }
    while( 1 ){
	if( (fa = processTripletQueue( NULL )) == NULL ) break;
	r_makeTriplet( fa, fp_fa );
    }
    fclose( fp_fa );
    if( !SW_Quiet ){
	fprintf( stderr, "\rNow making triplet list[%d/%d]\n", FAprocessed, FAtotal );
	NoNewLine = 0;
    }
    if( SW_Verbose ){
	verboseMes( "r_makeTriplet: %d/%d(%d%%)",
		TFAtravSuccess, TFAtravTotal, 100*TFAtravSuccess/TFAtravTotal);
    }
    newLineAdjust();
}

void r_makeTriplet( FA *fa, FILE *fp_fa )
{
    ARC *arc;
    CLASSFLAGS accpt;
    CLASSFLAGS start;

    TFAtravTotal++;
    if( fa->traversed == 2 ){
	return;
    }
    fa->traversed = 2;
    TFAtravSuccess++;

    FAprocessed++;
    if( !SW_SemiQuiet ){
	fprintf( stderr, "\rNow making triplet list[%d/%d]", FAprocessed, FAtotal );
	NoNewLine = 1;
    }

    if( (arc = fa->nsList) == NULL ){
	if( SW_EdgeAccpt && SW_EdgeStart ) return;
	if( !SW_EdgeAccpt ){
	    accpt = fa->accpt;
	} else {
	    accpt = 0;
	}
	if( !SW_EdgeStart ){
	    start = fa->start;
	} else {
	    start = 0;
	}
	if( SW_Compati ){
	    fprintf( fp_fa, "%d -1 -1 %x\n", fa->stat, accpt & 1 );
	} else {
	    fprintf( fp_fa, "%d -1 -1 %x %x\n", fa->stat, accpt, start );
	}
	return;
    }
    while( arc != NULL ){
	if( !SW_EdgeAccpt ){
	    accpt = fa->accpt;
	} else {
	    accpt = arc->accpt;
	}
	if( !SW_EdgeStart ){
	    start = fa->start;
	} else {
	    start = arc->start;
	}
	if( SW_Compati ){
	    accpt &= 1;
	    fprintf( fp_fa, "%d %d %d %x\n",
		    fa->stat, arc->inp, getNewStatNo(arc->fa), accpt );
	} else {
	    fprintf( fp_fa, "%d %d %d %x %x\n",
		    fa->stat, arc->inp, getNewStatNo(arc->fa), accpt, start );
	}
	arc = arc->next;
    }
}

int getNewStatNo( FA *fa )
{
    static int FAstat = 0;

    if( fa->stat >= 0 ) return( fa->stat );
    fa->stat = FAstat;
    processTripletQueue( fa );
    return( FAstat++ );
}

FA *processTripletQueue( FA *fa )
{
    /* NULL:pop, !NULL:push */

    typedef struct _FAQ{
	FA *fa;
	struct _FAQ *next;
    } FAQ;

    static FAQ *queueTop = NULL;
    static FAQ *queuqTail = NULL;
    FAQ *newFAQ;

    if( fa != NULL ){
	if( (newFAQ = malloc( sizeof(FAQ) )) == NULL ){
	    errMes( "Can't malloc queue for breadth-first search of triplet list" );
	}
	newFAQ->fa = fa;
	newFAQ->next = NULL;

	if( queueTop == NULL ){
	    queueTop = queuqTail = newFAQ;
	    return( NULL );
	} else {
	    queuqTail->next = newFAQ;
	    queuqTail = newFAQ;
	    return( NULL );
	}
    } else {
	if( queueTop != NULL ){
	    FAQ *popedFAQ = queueTop;
	    FA *popedFA = queueTop->fa;
	    queueTop = queueTop->next;
	    free( popedFAQ );
	    return( popedFA );
	} else {
	    return( NULL );
	}
    }
}
