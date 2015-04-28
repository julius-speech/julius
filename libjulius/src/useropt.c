/**
 * @file   useropt.c
 * 
 * <JA>
 * @brief  ユーザ定義オプション
 * </JA>
 * 
 * <EN>
 * @brief  User-defined option handling
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Sun Sep 02 19:44:37 2007
 *
 * $Revision: 1.6 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius/juliuslib.h>

/**
 * List of user option data
 * 
 */
static USEROPT *useropt_root = NULL;

/** 
 * <EN>
 * Generate a new option data.
 * </EN>
 * <JA>
 * 新たなユーザオプションデータを生成. 
 * </JA>
 * 
 * @return a new user option data.
 * 
 */
static USEROPT *
useropt_new()
{
  USEROPT *new;

  new = (USEROPT *)mymalloc(sizeof(USEROPT));
  new->optstr = NULL;
  new->desc = NULL;
  new->argnum = 0;
  new->reqargnum = 0;
  new->next = NULL;

  return new;
}

/** 
 * <EN>
 * ユーザオプションデータを解放. 
 * </EN>
 * <JA>
 * Release a user option data.
 * </JA>
 * 
 * @param x [in] a user option data to release
 * 
 */
static void
useropt_free(USEROPT *x)
{
  if (x->optstr) free(x->optstr);
  if (x->desc) free(x->desc);
  free(x);
}

/** 
 * <EN>
 * Release all user option data.
 * </EN>
 * <JA>
 * 全てのユーザオプションデータを解放する. 
 * </JA>
 *
 * @callgraph
 * @callergraph
 */
void
useropt_free_all()
{
  USEROPT *x, *tmp;

  x = useropt_root;
  while(x) {
    tmp = x->next;
    useropt_free(x);
    x = tmp;
  }
  useropt_root = NULL;
}
  
/** 
 * <EN>
 * Add a user-defined option to Julius.
 * When reqargnum is lower than argnum, the first (reqargnum) arguments
 * are required and the rest (argnum - reqargnum) options are optional.
 * </EN>
 * <JA>
 * Julius にユーザ定義オプションを追加する. 
 * argnum には引数の最大数，reqargnum はそのうち必須である引数の数を
 * 指定する. argnum > reqargnum の場合，先頭から reqargnum 個が必須で，
 * それ以降が optional として扱われる. 
 * </JA>
 * 
 * @param fmt [in] option string (should begin with '-')
 * @param argnum [in] total number of argument for this option (including optional)
 * @param reqargnum [in] number of required argument
 * @param desc [in] description string for help 
 * @param func [in] option handling function
 * 
 * @return TRUE on success, FALSE on failure
 * 
 * @callgraph
 * @callergraph
 * @ingroup engine
 */
boolean
j_add_option(char *fmt, int argnum, int reqargnum, char *desc, boolean (*func)(Jconf *jconf, char *arg[], int argnum))
{
  USEROPT *new;

  if (fmt[0] != '-') {
    jlog("ERROR: j_add_option: option string must start with \'-\': %s\n", fmt);
    return FALSE;
  }
  if (argnum < reqargnum) {	/* error */
    jlog("ERROR: j_add_option: number of required argument (%d) larger than total (%d)\n", reqargnum, argnum);
    return FALSE;
  }

  /* if this is first time, register free function at app exit */
  if (useropt_root == NULL) atexit(useropt_free_all);

  /* allocate new */
  new = useropt_new();
  /* set option string */
  new->optstr = strcpy((char *)mymalloc(strlen(fmt)+1), fmt);
  /* set number of arguments */
  new->argnum = argnum;
  /* set number of required arguments.
     If less than argnum, the latter options should be optional */
  new->reqargnum = reqargnum;
  /* set description string */
  new->desc = strcpy((char*)mymalloc(strlen(desc)+1),desc);

  /* set user-given function to process this option */
  new->func = func;

  /* add to list */
  new->next = useropt_root;
  useropt_root = new;

  return TRUE;
}


/** 
 * <EN>
 * Inspect for the user-specified options at option parsing
 * </EN>
 * <JA>
 * オプション設定においてユーザ定義オプション処理を行う. 
 * 
 * </JA>
 * 
 * @param jconf [in] global configuration variables
 * @param argv [in] argument array
 * @param argc [in] number of arguments in argv
 * @param n [i/o] current position in argv
 * 
 * @return 1 when the current argument was processed successfully
 * by one of the user options, 0 when no user option matched for the
 * current argument, or -1 on error.
 * 
 * @callgraph
 * @callergraph
 */
int
useropt_exec(Jconf *jconf, char *argv[], int argc, int *n)
{
  USEROPT *x;
  int narg, i;

  for(x=useropt_root;x;x=x->next) {
    if (strmatch(argv[*n], x->optstr)) {
      i = *n + 1;
      while(i < argc && (argv[i][0] != '-' || (argv[i][1] >= '0' && argv[i][1] <= '9'))) i++;
      
      narg = i - *n - 1;
      if (narg > x->argnum || narg < x->reqargnum) {
	if (x->reqargnum != x->argnum) {
	  jlog("ERROR: useropt_exec: \"%s\" should have at least %d argument(s)\n", x->optstr, x->reqargnum);
	} else {
	  jlog("ERROR: useropt_exec: \"%s\" should have %d argument(s)\n", x->optstr, x->argnum);
	}
	return -1;		/* error */
      }

      if ((*(x->func))(jconf, &(argv[(*n)+1]), narg) == FALSE) {
	jlog("ERROR: useropt_exec: \"%s\" function returns FALSE\n", x->optstr);
	return -1;		/* error */
      }
      *n += narg;
      return 1;			/* processed */
    }
  }

  return 0;			/* nothing processed */
}

/** 
 * <EN>
 * Output description of all the registered user options.
 * </EN>
 * <JA>
 * 登録されている全てのユーザ定義オプションの説明を出力する. 
 * </JA>
 * 
 * @param fp [in] file pointer to output for
 * 
 * @callgraph
 * @callergraph
 */
void
useropt_show_desc(FILE *fp)
{
  USEROPT *x;
  int i;

  if (! useropt_root) return;
  fprintf(fp, "\n Additional options for application:\n");
  for(x=useropt_root;x;x=x->next) {
    fprintf(fp, "    [%s", x->optstr);
    for(i=0;i<x->reqargnum;i++) fprintf(fp, " arg");
    for(i=x->reqargnum;i<x->argnum;i++) fprintf(fp, " (arg)");
    fprintf(fp, "]\t%s\n", x->desc);
  }
}

/* end of file */
