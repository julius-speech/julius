%%
"@"[a-zA-Z0-9_]+ {
		yylval = yytext + 1;
		return( TAG );
}

[a-zA-Z0-9_]+ {
		yylval = yytext;
		return( SYMBOL );
}

"{" {
		ModeBlock = 1;
		return( OPEN );
}

"}" {
		ModeBlock = 0;
		return( CLOSE );
}

"%ASSIGN"	return( CTRL_ASSIGN );
"%IGNORE"	return( CTRL_IGNORE );
"!"		return( REVERSE );
"*"		return( STARTCLASS );
":"		return( LET );
\n		return( NL );
"#".*\n		return( REMARK );
[ \t]		{};

. {
	errMes("Lexical mistake \"%s\"", yytext );
	exit( 1 );
}
