%{
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */
#include "mkfa.h"
#define YYSTYPE char *
#define CLASS_NUM 100

void appendNonTerm( char *name, int modeAssign );
BODY *setNonTerm( void );
CLASS *entryNonTerm( char *name, BODY *body, int modeAccpt, int start, int member, int tmp );
void pushBody( CLASS *class, BODY *newbody );
int unifyBody( char *name, BODY *body, BODY *newbody );
char *getNewClassName( char *keyname );
void outputHeader( char *name );
char *chkNoInstantClass( void );

extern CLASS *ClassList;
extern CLASS *ClassListTail;
extern CLASS *StartSymbol;
extern int NoNewLine;
extern char GramFile[ 1024 ];
extern char HeaderFile[ 1024 ];
extern int SW_Compati;
extern int SW_Quiet;
extern int SW_SemiQuiet;
extern char VerNo[];
static char HeadName[ SYMBOL_LEN ];
static char BodyName[ CLASS_NUM ][ SYMBOL_LEN ];
static int BodyNo = 0;
static int ClassNo = 0;
static int ModeAssignAccptFlag = 1;
static int BlockReverseSw;
static int ModeBlock = 0;
static int CurClassNo = 0;
static int StartFlag = 0;
static FILE *FPheader;
static int ErrParse = 0;
static int GramModifyNum = 0;
%}

%token CTRL_ASSIGN
%token CTRL_IGNORE
%token OPEN
%token CLOSE
%token REVERSE
%token STARTCLASS
%token LET
%token TAG
%token SYMBOL
%token REMARK
%token NL

%%
src : statement | statement src;

statement : block | single | contol | remark
| error NL
{
    yyerrok;
};

block : tag OPEN remark members CLOSE remark;

tag : TAG
{
    BlockReverseSw = 0;
    if( ModeAssignAccptFlag ) outputHeader( $1 );
}
| REVERSE TAG
{
    BlockReverseSw = 1;
    if( !ModeAssignAccptFlag ) outputHeader( $2 );
};

members : member | member members;

member : define
{
    appendNonTerm( HeadName, ModeAssignAccptFlag ^ BlockReverseSw );
}
| head remark
{
    entryNonTerm( HeadName, NULL, ModeAssignAccptFlag ^ BlockReverseSw, 0, 1, 0 ); /*空登録*/
}
| remark;

single : define
{
    appendNonTerm( HeadName, ModeAssignAccptFlag );
}
| REVERSE define
{
    appendNonTerm( HeadName, !ModeAssignAccptFlag );
};

define : head LET bodies remark;

bodies : body | body bodies;

head : SYMBOL
{
    strcpy( HeadName, $1 );
}
| STARTCLASS SYMBOL
{
    StartFlag = 1;
    strcpy( HeadName, $2 );
};

body : SYMBOL
{
    strcpy( BodyName[ BodyNo++ ], $1 );
};

contol : CTRL_ASSIGN remark
{
    ModeAssignAccptFlag = 1;
}
| CTRL_IGNORE
{
    ModeAssignAccptFlag = 0;
};

remark : REMARK | NL;

%%
#include "lex.yy.c"
void appendNonTerm( char *name, int modeAssign )
{
    BODY *body;

    body = setNonTerm();
    entryNonTerm( name, body, modeAssign, StartFlag, ModeBlock, 0 );
    BodyNo = 0;
}

BODY *setNonTerm( void )
{
    int i;
    BODY *body;
    BODY *top = NULL, *prev = NULL;

    for( i = 0; i < BodyNo; i++ ){
	if( (body = malloc( sizeof(BODY) )) == NULL ){
	    errMes( "Can't alloc nonterminal list buffer" );
	}
	strcpy( body->name, BodyName[ i ] );
	body->abort = 0;
	if( prev != NULL ){
	    prev->next = body;
	} else {
	    top = body;
	}
	prev = body;   
    }
    body->next = NULL;
    return( top );
}

CLASS *entryNonTerm( char *name, BODY *body, int modeAccpt, int start, int member, int tmp )
{
    CLASS *class;

    class = getClass( name );
    if( class != NULL ){
	if( member ){
	    errMes("Accepted flag of class \"%s\" is re-assigned", HeadName );
	    ErrParse++;
	}
    } else {
	if( (class = malloc( sizeof(CLASS) )) == NULL ){
	    errMes( "Can't alloc memory for Class Finite Automaton." );
	}
	strcpy( class->name, name );
	if( modeAccpt ){
	    if( member ){
		class->no = CurClassNo;
	    } else {
		if( !tmp ){
		    outputHeader( name );
		    class->no = CurClassNo;
		}
	    }
	} else {
	    class->no = -1;
	}
	class->branch = 0;
	class->usedFA = 0;
	class->used = 1;	/* non-terminal does not appear in voca */
	class->bodyList = NULL;
	class->tmp = tmp;
	class->next = NULL;
	if( ClassListTail == NULL ){
	    ClassList = class;
	} else {
	    ClassListTail->next = class;
	}
	ClassListTail = class;
    }
    if( body != NULL ) pushBody( class, body );
    if( start ){
	StartFlag = 0;
	if( StartSymbol == NULL ){
	    StartSymbol = class;
	} else {
	    errMes("Start symbol is redifined as \"%s\"", class->name );
	    ErrParse++;
	}
    }
    return( class );
}

void pushBody( CLASS *class, BODY *newbody )
{
    BODYLIST *bodyList = class->bodyList;
    BODYLIST *preBodyList = NULL;
    BODYLIST *newBodyList;
    BODY *body;
    int cmp;
    int defineNo = 1;

    while( bodyList != NULL ){
	body = bodyList->body;
	cmp = strcmp( body->name, newbody->name );
	if( cmp > 0 ) break;
	if( cmp == 0 ){
	    if( unifyBody( class->name, body, newbody ) ){
		warnMes( "Class \"%s\" is defined as \"%s..\" again.", class->name, body->name );
	    }
	    return;
	}
	preBodyList = bodyList;
	bodyList = bodyList->next;
	defineNo++;
    }
    if( (newBodyList = malloc( sizeof(BODYLIST) )) == NULL ){
	errMes( "Can't alloc class body buffer." );
    }
    newBodyList->body = newbody;

    if( preBodyList != NULL ){
	preBodyList->next = newBodyList;
    } else {
	class->bodyList = newBodyList;
    }
    newBodyList->next = bodyList;
    class->branch++;
}

int unifyBody( char *className, BODY *body, BODY *newbody )
{
    BODY *bodyNext, *newbodyNext;
    char *newClassName;
    BODY *newBody;
    CLASS *class;

    bodyNext = body->next;
    newbodyNext = newbody->next;
    while( 1 ){
	if( bodyNext == NULL && newbodyNext == NULL ){
	    return( -1 );
	}
	if( newbodyNext == NULL ){
	    if( body->abort ){
		return( -1 );
	    } else {
		body->abort = 1;
		return( 0 );
	    }
	}
	if( bodyNext == NULL ){
	    body->abort = 1;
	    body->next = newbodyNext;
	    return( 0 );
	}
	if( strcmp( bodyNext->name, newbodyNext->name ) ) break;
	body = bodyNext;
	newbody = newbodyNext;
	bodyNext = body->next;
	newbodyNext = newbody->next;
    }
    class = getClass( body->name );
    if( class != NULL && class->tmp ){
	entryNonTerm( body->name, newbodyNext, 0, 0, 0, 1 );
    } else {
	newClassName = getNewClassName( className );
	entryNonTerm( newClassName, bodyNext, 0, 0, 0, 1 );
	entryNonTerm( newClassName, newbodyNext, 0, 0, 0, 1 );
	if( (newBody = malloc( sizeof(BODY) )) == NULL ){
	    errMes( "Can't alloc body buffer of tmp class, \"%s\".", newClassName );
	}
	strcpy( newBody->name, newClassName );
	newBody->abort = 0;
	newBody->next = NULL;
	body->next = newBody;
	newbody->next = newBody;
    }
    return( 0 );
}

char *getNewClassName( char *keyname )
{
    static char classname[ SYMBOL_LEN ];
    static int tmpClassNo = 0;

    sprintf( classname, "%s#%d", keyname , tmpClassNo++ );
    if( !SW_SemiQuiet ){
	fprintf( stderr, "\rNow modifying grammar to minimize states[%d]", GramModifyNum );
	NoNewLine = 1;
    }
    GramModifyNum++;
    return( classname );
}

void setGram( void )
{
    char *name;

    if( (yyin = fopen( GramFile, "r" )) == NULL ){
	errMes( "Can't open grammar file \"%s\"", GramFile );
    }
    if( SW_Compati ){
	strcpy( HeaderFile, "/dev/null" );
    }
    if( (FPheader = fopen( HeaderFile, "w" )) == NULL ){
	errMes( "Can't open Header File for writting\"%s\"", HeaderFile );
    }
    fprintf( FPheader,
	    "/* Header of class reduction flag for finite automaton parser\n"
	    "                    made with mkfa %s\n\n"
	    "        Do logicalAND between label and FA's field #4,#5.\n"
	    "*/\n\n", VerNo
	    );
    if( !SW_Quiet ) fputs( "Now parsing grammar file\n", stderr );
    yyparse();
    if( !SW_Quiet ){
	fprintf( stderr, "\rNow modifying grammar to minimize states[%d]\n", GramModifyNum - 1 );
	NoNewLine = 0;
    }
    if( StartSymbol == NULL ) StartSymbol = ClassList;
    fprintf( FPheader, "/* Start Symbol: %s */\n", StartSymbol->name );
    fclose( FPheader );
    if( (name = chkNoInstantClass()) != NULL ){
	errMes( "Prototype-declared Class \"%s\" has no instant definitions", name );
    }
    if( ErrParse ) errMes( "%d fatal errors exist", ErrParse );
}

void outputHeader( char *name )
{
    if( ClassNo >= CLASSFLAG_MAX ){
	if( !SW_Compati ){
	    warnMes( "Class accepted flag overflow.\"%s\"", name );
	    CurClassNo = -1;
	}
    } else {
	if( !SW_Compati ){
	    fprintf( FPheader, "#define ACCEPT_%s 0x%08x\n",
		    name, 1 << ClassNo );
	}
	CurClassNo = ClassNo++;
    }
}

char *chkNoInstantClass( void )
{
    CLASS *class = ClassList;

    while( class != NULL ){
	if( !class->branch ) return( class->name );
	class = class->next;
    }
    return( NULL );
}

int yyerror( char *mes )
{
    errMes(mes );
    ErrParse++;
    return( 0 );
}
