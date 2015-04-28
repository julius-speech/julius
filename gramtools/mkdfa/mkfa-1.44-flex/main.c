/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */
#include "mkfa.h"
#include "nfa.h"
#include "dfa.h"
#include "triplet.h"

void getSwitch( int argc, char *argv[] );
int setSwitch( char *sw );
int setFilename( char *fname, int kind );
void usage( void );

char VerNo[] = VER_NO;

CLASS *ClassList = NULL;       /* クラスの線形リスト */
CLASS *ClassListTail = NULL;   /* クラスの線形リストの最後尾のノード */
CLASS *StartSymbol = NULL;     /* 開始記号のクラス */

char GramFile[ 1024 ];         /* 文法ファイル名 */
char VocaFile[ 1024 ];         /* 語彙ファイル名 */
char FAfile[ 1024 ];           /* FAファイル名(DFAorNFA) */
char HeaderFile[ 1024 ];       /* ヘッダファイル名 */
int NoNewLine = 0;             /* 複数の表示モードで改行問題を解決する */
FA *FAlist = NULL;             /* FAネットワークにおける開始FAのポインタ */
char Clipboard[ 1024 ];        /* sprintf用の一時書き込みバッファ */

static int optF = 0;           /* -f が指定された(-dfaとの問題解決に) */

int SW_SentList = 0;
int SW_NoWarning = 0;
int SW_Compati = 0;
int SW_Quiet = 0;
int SW_SemiQuiet = 0;
int SW_Debug = 0;
int SW_NFAoutput = 0;
int SW_Verbose = 0;
int SW_EdgeStart;
int SW_EdgeAccpt;

int main( int argc, char *argv[] )
{
#ifdef YYDEBUG
    extern int yydebug;
    yydebug = 1;
#endif
    getSwitch( argc, argv );
    if( SW_EdgeAccpt ){
	errMes( "I'm sorry. AcceptFlag on edge is under construction." );
    }
    setGram();
    setVoca();
    makeNFA();
    if( !SW_NFAoutput ) makeDFA();
    makeTriplet();
    return( 0 );
}

void getSwitch( int argc, char *argv[] )
{
    int i;
    int filemode = 0;
    int filefinish = 0;

    for( i = 1; i < argc; i++ ){
	if( filemode == 0 ){
	    if( argv[ i ][ 0 ] == '-' ){
		filemode = setSwitch( &argv[ i ][ 1 ] );
	    } else {
		usage();
	    }
	} else {
	    filefinish = setFilename( argv[ i ], filemode );
	    filemode = 0;
	}
    }
    if( !filefinish ) usage();
}

int setSwitch( char *sw )
{
    char *sname[] = {
	"l", "nw", "c", "db", "dfa", "nfa",
	"fg", "fv", "fo", "fh", "f", "v",
	"c", "e", "e0", "e1", "q0", "q",
	"q1", NULL
    };

    int swNo;

    for( swNo = 0; ; swNo++ ){
	if( sname[ swNo ] == NULL ) break;
	if( strcmp( sw, sname[ swNo ] ) == 0 ) break;
    }
    switch( swNo ){
      case 0:
	SW_SentList = 1;
	break;
      case 1:
	SW_NoWarning = 1;
	break;
      case 2:
	SW_Compati = 1;
	break;
      case 3:
	SW_Debug = 1;
	break;
      case 4:
	if( optF ) usage();
	SW_NFAoutput = 0;
	break;
      case 5:
	if( optF ) usage();
	SW_NFAoutput = 1;
	break;
      case 6:
	return( 1 );
      case 7:
	return( 2 );
      case 8:
	return( 3 );
      case 9:
	return( 4 );
      case 10:
	return( 5 );
      case 11:
	SW_Verbose = 1;
	break;
      case 12:
	SW_Compati = 1;
	break;
      case 13:
	SW_EdgeAccpt = 1;
	SW_EdgeStart = 1;
	break;
      case 14:
	SW_EdgeAccpt = 1;
	break;
      case 15:
	SW_EdgeStart = 1;
	break;
      case 16:
	SW_Quiet = 1;
      case 17:
      case 18:
	SW_SemiQuiet = 1;
	break;
      default:
	usage();
    }
    return( 0 );
}

int setFilename( char *fname, int kind )
{
    static int f_gram = 0;
    static int f_voca = 0;
    static int f_out = 0;
    static int f_header = 0;
    switch( kind ){
      case 1:
	strcpy( GramFile, fname );
	f_gram = 1;
	break;
      case 2:
	strcpy( VocaFile, fname );
	f_voca = 1;
	break;
      case 3:
	strcpy( FAfile, fname );
	f_out = 1;
	break;
      case 4:
	strcpy( HeaderFile, fname );
	f_header = 1;
	break;
      case 5:
	sprintf( GramFile, "%s.grammar", fname );
	sprintf( VocaFile, "%s.voca", fname );
	if( SW_NFAoutput ){
	    sprintf( FAfile, "%s.nfa", fname );
	} else {
	    sprintf( FAfile, "%s.dfa", fname );
	}
	optF = 1;
	sprintf( HeaderFile, "%s.h", fname );
	f_gram = f_voca = f_out = f_header = 1;
	return( 1 );
    }
    if( f_gram && f_voca && f_out && f_header ){
	return( 1 );
    } else {
	return( 0 );
    }
}

void errMes( char *fmt, ... )
{
    va_list argp;
    if( NoNewLine ) putc( '\n', stderr );
    va_start( argp, fmt );
    vsprintf( Clipboard, fmt, argp );
    va_end( argp );
    fprintf( stderr, "Error:       %s\n", Clipboard );
    exit( 1 );
}

void warnMes( char *fmt, ... )
{
    va_list argp;
    if( SW_NoWarning ) return;
    if( NoNewLine ) putc( '\n', stderr );
    va_start( argp, fmt );
    vsprintf( Clipboard, fmt, argp );
    va_end( argp );
    fprintf( stderr, "Warning:     %s\n", Clipboard );
    NoNewLine = 0;
}
void verboseMes( char *fmt, ... )
{
    va_list argp;
    if( !SW_Verbose ) return;
    if( NoNewLine ) putc( '\n', stderr );
    va_start( argp, fmt );
    vsprintf( Clipboard, fmt, argp );
    va_end( argp );
    fprintf( stderr, "[verbose]    %s\n", Clipboard );
    NoNewLine = 0;
}

void usage( void )
{
    fprintf( stderr,
	    "finite automaton generator, mkfa %s programmed by 1995-1996 S.Hamada\n"
	    "function:  grammar & vocabulary -> FA & header for parsing\n"
	    "usage:     mkfa <option>.. <file-spec1>..; or mkfa <option>.. <file-spec2>\n"
	    "option:    -dfa    DFA output(default)\n"
	    "           -nfa    NFA output\n"
	    "           -c      compatible FA output with g2fa\n"
	    "           -e[0|1] putting class reduction flag on edge(default: on vertex)\n"
            "                   (0:accept 1:start omitted:both)\n"
	    "           -nw     no warning messages\n"
	    "           -q[0|1] contol of processing report\n"
	    "                   (0:no report 1:semi-quiet omitted:semi-quiet)\n"
	    "           -v      verbose mode(to stderr)\n"
	    "filespec1: -fg     grammar filename\n"
	    "           -fv     vocabulary filename\n"
	    "           -fo     output filename(DFA or NFA file)\n"
	    "           -fh     header filename of class reduction flag for parser\n"
	    "filespec2: -f      basename of above I/O files\n"
	    "                   (respectively appended .grammar, .voca, .dfa(.nfa), .h)\n"
	    "NOTES:     * Regular expression with left recursion can't be processed.\n"
	    "           * Option -dfa and -nfa must not follow option -f.\n"
            "           * State#1 isn't always final state even if compiled with -c.\n", VerNo );
    exit( 1 );
}
