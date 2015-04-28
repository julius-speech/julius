/**
 * @file   gramlist.c
 * 
 * <EN>
 * @brief  Grammar file list management on startup.
 *
 * These functions are for managing list of grammar files to be loaded
 * at startup.  You can also specify (list of) grammars to be included
 * for recognition at startup by calling these functions.  If you want to
 * add, modify or remove grammars while recognition, you should prepare
 * grammar data and call functions in multi-gram.c directly.
 * @sa julius/module.c for the implementation details.
 * 
 * </EN>
 * 
 * <JA>
 * @brief  起動時に読み込む文法ファイルのリスト管理. 
 *
 * これらの関数はエンジン起動時に読み込まれる文法ファイルのリストを管理する
 * 関数です. これらの関数を起動前に呼ぶことで，認識用の文法をアプリケーション
 * 上で明示的に追加することができます. エンジン起動後に動的に文法の
 * 追加や削除，変更を行いたい場合は，文法データを自前で用意して，multi-gram.c
 * 内の関数を直接呼び出す必要があります. その場合は julius/module.c が
 * 実装の参考になるでしょう. (@sa julius/module.c)
 * 
 * </JA>
 * 
 * @author Akinobu Lee
 * @date   Tue Oct 30 12:27:53 2007
 *
 * $Revision: 1.4 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius/julius.h>

/** 
 * <JA>
 * 起動時読み込みリストに文法を追加する. 
 * 
 * @param dfafile [in] DFAファイル
 * @param dictfile [in] 単語辞書
 * @param j [in] LM 設定パラメータ
 * @param lmvar [in] LM 詳細型 id
 * </JA>
 * <EN>
 * Add a grammar to the grammar list to be read at startup.
 * 
 * @param dfafile [in] DFA file
 * @param dictfile [in] dictionary file
 * @param j [in] LM configuration variables
 * @param lmvar [in] LM type variant id
 * </EN>
 *
 * @callgraph
 * @callergraph
 * @ingroup grammar
 */
void
multigram_add_gramlist(char *dfafile, char *dictfile, JCONF_LM *j, int lmvar)
{
  GRAMLIST *new;

  new = (GRAMLIST *)mymalloc(sizeof(GRAMLIST));
  new->dfafile = new->dictfile = NULL;
  if (dfafile) new->dfafile = strcpy((char *)mymalloc(strlen(dfafile)+1), dfafile);
  if (dictfile) new->dictfile = strcpy((char *)mymalloc(strlen(dictfile)+1), dictfile);
  switch(lmvar) {
  case LM_DFA_GRAMMAR:
    new->next = j->gramlist_root;
    j->gramlist_root = new;
    break;
  case LM_DFA_WORD:
    new->next = j->wordlist_root;
    j->wordlist_root = new;
    break;
  }
}

/** 
 * <JA>
 * 起動時読み込みリストを消す. 
 * 
 * @param j [in] LM 設定パラメータ
 * </JA>
 * <EN>
 * Remove the grammar list to be read at startup.
 * 
 * @param j [in] LM configuration variables
 * </EN>
 *
 * @callgraph
 * @callergraph
 * @ingroup grammar
 */
void
multigram_remove_gramlist(JCONF_LM *j)
{
  GRAMLIST *g;
  GRAMLIST *tmp;

  g = j->gramlist_root;
  while (g) {
    tmp = g->next;
    if (g->dfafile) free(g->dfafile);
    if (g->dictfile) free(g->dictfile);
    free(g);
    g = tmp;
  }
  j->gramlist_root = NULL;

  g = j->wordlist_root;
  while (g) {
    tmp = g->next;
    if (g->dfafile) free(g->dfafile);
    if (g->dictfile) free(g->dictfile);
    free(g);
    g = tmp;
  }
  j->wordlist_root = NULL;
}

/** 
 * <JA>
 * @brief  プレフィックスから複数の文法を起動時読み込みリストに追加する. 
 *
 * プレフィックスは "foo", あるいは "foo,bar" のようにコンマ区切りで
 * 複数与えることができます. 各文字列の後ろに ".dfa", ".dict" をつけた
 * ファイルを，それぞれ文法ファイル・辞書ファイルとして順次読み込みます. 
 * 読み込まれた文法は順次，文法リストに追加されます. 
 * 
 * @param prefix_list [in]  プレフィックスのリスト
 * @param cwd [in] カレントディレクトリの文字列
 * @param j [in] LM 設定パラメータ
 * @param lmvar [in] LM 詳細型 id
 * </JA>
 * <EN>
 * @brief  Add multiple grammars given by their prefixs to the grammar list.
 *
 * This function read in several grammars, given a prefix string that
 * contains a list of file prefixes separated by comma: "foo" or "foo,bar".
 * For each prefix, string ".dfa" and ".dict" will be appended to read
 * dfa file and dict file.  The read grammars will be added to the grammar
 * list.
 * 
 * @param prefix_list [in] string that contains comma-separated list of grammar path prefixes
 * @param cwd [in] string of current working directory
 * @param j [in] LM configuration variables
 * @param lmvar [in] LM type variant id
 * </EN>
 * 
 * @callgraph
 * @callergraph
 * @ingroup grammar
 */
boolean
multigram_add_prefix_list(char *prefix_list, char *cwd, JCONF_LM *j, int lmvar)
{
  char buf[MAXGRAMNAMELEN], *p, *q;
  char buf2_d[MAXGRAMNAMELEN], *buf_d;
  char buf2_v[MAXGRAMNAMELEN], *buf_v;
  boolean ok_p, ok_p_total;

  if (prefix_list == NULL) return TRUE;
  
  p = &(prefix_list[0]);

  ok_p_total = TRUE;
  
  while(*p != '\0') {
    /* extract one prefix to buf[] */
    q = p;
    while(*p != '\0' && *p != ',') {
      buf[p-q] = *p;
      p++;
    }
    buf[p-q] = '\0';

    switch(lmvar) {
    case LM_DFA_GRAMMAR:
      /* register the new grammar to the grammar list to be read later */
      /* making file names from the prefix */
      ok_p = TRUE;
      strcpy(buf2_d, buf);
      strcat(buf2_d, ".dfa");
      buf_d = filepath(buf2_d, cwd);
      if (!checkpath(buf_d)) {
	jlog("ERROR: gramlist: cannot read dfa file \"%s\"\n", buf_d);
	ok_p = FALSE;
      }
      strcpy(buf2_v, buf);
      strcat(buf2_v, ".dict");
      buf_v = filepath(buf2_v, cwd);
      if (!checkpath(buf_v)) {
	jlog("ERROR: gramlist: cannot read dict file \"%s\"\n", buf_v);
	ok_p = FALSE;
      }
      if (ok_p == TRUE) {
	multigram_add_gramlist(buf_d, buf_v, j, lmvar);
      } else {
	ok_p_total = FALSE;
      }
      break;
    case LM_DFA_WORD:
      /* register the new word list to the list */
      /* treat the file name as a full file path (not prefix) */
      buf_v = filepath(buf, cwd);
      if (!checkpath(buf_v)) {
	jlog("ERROR: gramlist: cannot read wordlist file \"%s\"\n", buf_v);
	ok_p_total = FALSE;
      } else {
	multigram_add_gramlist(NULL, buf_v, j, lmvar);
      }
      break;
    } 

    /* move to next */
    if (*p == ',') p++;
  }

  return ok_p_total;
}

/** 
 * <JA>
 * @brief リストファイルを読み込み複数文法を起動時読み込みリストに追加する. 
 *
 * ファイル内に1行に１つずつ記述された文法のプレフィックスから,
 * 対応する文法ファイルを順次読み込みます. 
 * 
 * 各行の文字列の後ろに ".dfa", ".dict" をつけたファイルを，
 * それぞれ文法ファイル・辞書ファイルとして順次読み込みます. 
 * 読み込まれた文法は順次，文法リストに追加されます. 
 * 
 * @param listfile [in] プレフィックスリストのファイル名
 * @param j [in] LM 設定パラメータ
 * @param lmvar [in] LM 詳細型 id
 * </JA>
 * <EN>
 * @brief  Add multiple grammars from prefix list file to the grammar list.
 *
 * This function read in multiple grammars at once, given a file that
 * contains a list of grammar prefixes, each per line.
 *
 * For each prefix, string ".dfa" and ".dict" will be appended to read the
 * corresponding dfa and dict file.  The read grammars will be added to the
 * grammar list.
 * 
 * @param listfile [in] path of the prefix list file
 * @param j [in] LM configuration variables
 * @param lmvar [in] LM type variant id
 * </EN>
 * 
 * @callgraph
 * @callergraph
 * @ingroup grammar
 */
boolean
multigram_add_prefix_filelist(char *listfile, JCONF_LM *j, int lmvar)
{
  FILE *fp;
  char buf[MAXGRAMNAMELEN], *p, *src_bgn, *src_end, *dst;
  char *cdir;
  char buf2_d[MAXGRAMNAMELEN], *buf_d;
  char buf2_v[MAXGRAMNAMELEN], *buf_v;
  boolean ok_p, ok_p_total;

  if (listfile == NULL) return FALSE;
  if ((fp = fopen(listfile, "r")) == NULL) {
    jlog("ERROR: gramlist: failed to open grammar list file %s\n", listfile);
    return FALSE;
  }

  /* convert relative paths as relative to this list file */
  cdir = strcpy((char *)mymalloc(strlen(listfile)+1), listfile);
  get_dirname(cdir);

  ok_p_total = TRUE;

  while(getl_fp(buf, MAXGRAMNAMELEN, fp) != NULL) {
    /* remove comment */
    p = &(buf[0]);
    while(*p != '\0') {
      if (*p == '#') {
	*p = '\0';
	break;
      }
      p++;
    }
    if (buf[0] == '\0') continue;
    
    /* trim head/tail blanks */
    p = (&buf[0]);
    while(*p == ' ' || *p == '\t' || *p == '\r') p++;
    if (*p == '\0') continue;
    src_bgn = p;
    p = (&buf[strlen(buf) - 1]);
    while((*p == ' ' || *p == '\t' || *p == '\r') && p > src_bgn) p--;
    src_end = p;
    dst = (&buf[0]);
    p = src_bgn;
    while(p <= src_end) *dst++ = *p++;
    *dst = '\0';
    if (buf[0] == '\0') continue;


    switch(lmvar) {
    case LM_DFA_GRAMMAR:
      /* register the new grammar to the grammar list to be read later */
      ok_p = TRUE;
      strcpy(buf2_d, buf);
      strcat(buf2_d, ".dfa");
      buf_d = filepath(buf2_d, cdir);
      if (!checkpath(buf_d)) {
	jlog("ERROR: gramlist: cannot read dfa file \"%s\"\n", buf_d);
	ok_p = FALSE;
      }
      strcpy(buf2_v, buf);
      strcat(buf2_v, ".dict");
      buf_v = filepath(buf2_v, cdir);
      if (!checkpath(buf_v)) {
	jlog("ERROR: gramlist: cannot read dict file \"%s\"\n", buf_v);
	ok_p = FALSE;
      }
      if (ok_p == TRUE) {
	multigram_add_gramlist(buf_d, buf_v, j, lmvar);
      } else {
	ok_p_total = FALSE;
      }
      break;
    case LM_DFA_WORD:
      /* register the new word list to the list */
      /* treat the file name as a full file path (not prefix) */
      buf_v = filepath(buf, cdir);
      if (!checkpath(buf_v)) {
	jlog("ERROR: gramlist: cannot read wordlist file \"%s\"\n", buf_v);
	ok_p_total = FALSE;
      } else {
	multigram_add_gramlist(NULL, buf_v, j, lmvar);
      }
      break;
    }

  }

  free(cdir);
  
  fclose(fp);

  return ok_p_total;
}

/* end of file */
