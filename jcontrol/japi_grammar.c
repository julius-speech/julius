/**
 * @file   japi_grammar.c
 * 
 * <JA>
 * @brief  文法関連のモジュールコマンド実装
 * </JA>
 * 
 * <EN>
 * @brief  Implementation of grammar relatedd module commands.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Mar 24 07:13:41 2005
 *
 * $Revision: 1.7 $
 * 
 */
/*
 * Copyright (c) 2002-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2002-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include "japi.h"

static char buf[MAXLINELEN];	///< Local work area for string operation

/** 
 * <JA>
 * 与えられたプレフィックスの文法ファイルが存在するかチェックする．
 * 
 * @param prefix [in] 文法ファイルのパスのプレフィックス文字列
 * 
 * @return 存在するとき 0 , 存在しないとき -1 を返す．
 * </JA>
 * <EN>
 * Check if the grammar files exist on the given prefix.
 * 
 * @param prefix [in] prefix string of grammar file path
 * 
 * @return 0 if exist, or -1 if such files not exist.
 * </EN>
 */
static int
check_grammar_path(char *prefix)
{
  int i;

  i = strlen(prefix) - 1;
  while(prefix[i] != '.' && i >= 0) i--;
  if (i < 0 || strcmp(&(prefix[i]), ".dict") != 0) {
    snprintf(buf, MAXLINELEN, "%s.dfa", prefix);
    if (access(buf, R_OK) < 0) {
      fprintf(stderr, "Error: \"%s.dfa\" not exist\n", prefix);
      return -1;
    }
    snprintf(buf, MAXLINELEN, "%s.dict", prefix);
    if (access(buf, R_OK) < 0) {
      fprintf(stderr, "Error: \"%s.dict\" not exist\n", prefix);
      return -1;
    }
  } else {
    if (access(prefix, R_OK) < 0) {
      fprintf(stderr, "Error: \"%s\" not exist\n", prefix);
    }
  }
  return 0;
}

/** 
 * <JA>
 * 文法ファイル(.dfaおよび.dict)を文法としてサーバに送信する．
 * 
 * @param sd [in] 送信ソケット
 * @param prefix [in] 文法プレフィックスパス
 * 
 * @return 成功時 0, 失敗時 -1 を返す．
 * </JA>
 * <EN>
 * Send a grammar (.dfa and .dict file) to server.
 * 
 * @param sd [in] socket to send data
 * @param prefix [in] grammar prefix path
 * 
 * @return 0 on success, or -1 on failure.
 * </EN>
 */
static int
send_grammar(int sd, char *prefix)
{
  FILE *fp;
  int i;

  i = strlen(prefix) - 1;
  while(prefix[i] != '.' && i >= 0) i--;
  if (i < 0 || strcmp(&(prefix[i]), ".dict") != 0) {
    snprintf(buf, MAXLINELEN, "%s.dfa", prefix);
    if ((fp = fopen(buf, "r")) == NULL) {
      perror("japi_change_grammar"); return -1;
    }
    while(fgets(buf, MAXLINELEN, fp) != NULL) {
      do_send(sd, buf);
    }
    do_send(sd, "DFAEND\n");
    fclose(fp);
    snprintf(buf, MAXLINELEN, "%s.dict", prefix);
  } else {
    snprintf(buf, MAXLINELEN, "%s", prefix);
  }
  if ((fp = fopen(buf, "r")) == NULL) {
    perror("japi_change_grammar"); return -1;
  }
  while(fgets(buf, MAXLINELEN, fp) != NULL) {
    do_send(sd, buf);
  }
  do_send(sd, "DICEND\n");
  fclose(fp);
  return 0;
}

/** 
 * <JA>
 * "1,3,5" の形式で与えられた文法IDのリストを送信する．
 * 
 * @param sd [in] 送信ソケット
 * @param idstr [in] コンマ区切りの文法ID番号リストを格納した文字列
 * 
 * @return 0 を常に返す．
 * </JA>
 * <EN>
 * Send comma-separated grammar ID list (ex. "1,3,5").
 * 
 * 
 * @param sd [in] socket to send data
 * @param idstr [in] string of comma separated grammar ID list
 * 
 * @return always 0.
 * </EN>
 */
static int
send_idlist(int sd, char *idstr)
{
  char *p;

  /* convert comma to space */
  strcpy(buf, idstr);
  p = buf;
  while(*p != '\0') {
    if (*p == ',') *p = ' ';
    p++;
  }
  /* send them */
  do_sendf(sd, "%s\n", buf);
  return 0;
}


/** 
 * <JA>
 * コマンド CHANGEGRAM: サーバへ指定された文法を送信し，文法を入れ替える．
 * 
 * @param sd [in] 送信ソケット
 * @param prefixpath [in] 文法ファイルパスのプレフィックス
 * </JA>
 * <EN>
 * Command "CHANGEGRAM": send specified grammar to server and swap grammar.
 * 
 * @param sd [in] socket to send data
 * @param prefixpath [in] prefix string of grammar file path
 * </EN>
 */
void
japi_change_grammar(int sd, char *prefixpath)
{
  /* file existing check */
  if (check_grammar_path(prefixpath) < 0) return;
  /* send data */
  /* do_send(sd, "CHANGEGRAM\n"); */
  /* send data with its name */
  snprintf(buf, MAXLINELEN, "CHANGEGRAM %s\n", prefixpath);
  do_send(sd, buf);
  send_grammar(sd, prefixpath);
}

/** 
 * <JA>
 * コマンド ADDGRAM: サーバへ指定された文法を送信して追加する．
 * 
 * @param sd [in] 送信ソケット
 * @param prefixpath [in] 文法ファイルパスのプレフィックス
 * </JA>
 * <EN>
 * Command "ADDGRAM": send specified grammar to server and add it to the current
 * grammar list.
 * 
 * @param sd [in] socket to send data
 * @param prefixpath [in] prefix string of grammar file path
 * </EN>
 */
void
japi_add_grammar(int sd, char *prefixpath)
{
  /* file existing check */
  if (check_grammar_path(prefixpath) < 0) return;
  /* send data */
  /* do_send(sd, "ADDGRAM\n"); */
  /* send data with its name */
  snprintf(buf, MAXLINELEN, "ADDGRAM %s\n", prefixpath);
  do_send(sd, buf);
  send_grammar(sd, prefixpath);
}

/** 
 * <JA>
 * コマンド DELGRAM: 指定した番号の文法をサーバ上から削除する．
 * 
 * @param sd [in] 送信ソケット
 * @param idlist [in] コンマ区切りの文法ID番号リストを格納した文字列
 * </JA>
 * <EN>
 * Command "DELGRAM": delete grammars on the server specified by the ID.
 * 
 * @param sd [in] socket to send data
 * @param idlist [in] string of comma separated grammar ID list
 * </EN>
 */
void
japi_delete_grammar(int sd, char *idlist)
{
  do_send(sd, "DELGRAM\n");
  send_idlist(sd, idlist);
}

/** 
 * <JA>
 * コマンド ACTIVATEGRAM: 指定した番号の文法をサーバ上で有効にする．
 * 
 * @param sd [in] 送信ソケット
 * @param idlist [in] コンマ区切りの文法ID番号リストを格納した文字列
 * </JA>
 * <EN>
 * Command "ACTIVATEGRAM": activate grammars on the server specified by the ID.
 * 
 * @param sd [in] socket to send data
 * @param idlist [in] string of comma separated grammar ID list
 * </EN>
 */
void
japi_activate_grammar(int sd, char *idlist)
{
  do_send(sd, "ACTIVATEGRAM\n");
  send_idlist(sd, idlist);
}

/** 
 * <JA>
 * コマンド DEACTIVATEGRAM: 指定した番号の文法をサーバ上で一時無効にする．
 * 
 * @param sd [in] 送信ソケット
 * @param idlist [in] コンマ区切りの文法ID番号リストを格納した文字列
 * </JA>
 * <EN>
 * Command "DEACTIVATEGRAM": temporary de-activate grammars on the server
 * specified by the ID.  The deactivated ones can be activated again
 * by ACTIVATEGRAM command.
 * 
 * @param sd [in] socket to send data
 * @param idlist [in] string of comma separated grammar ID list
 * </EN>
 */
void
japi_deactivate_grammar(int sd, char *idlist)
{
  do_send(sd, "DEACTIVATEGRAM\n");
  send_idlist(sd, idlist);
}

/** 
 * <JA>
 * コマンド SYNCGRAM: 文法の更新を行う．
 *
 * 文法の更新は通常認識開始直前(resume後)に行われるが，文法が大きいと
 * 認識開始までにディレイが生じる可能性がある．このコマンドを使うと
 * 即時に文法の更新を促せる．resume後すぐに認識を行いたいときに有効．
 * 
 * @param sd [in] 送信ソケット
 * </JA>
 * <EN>
 * Command "SYNCGRAM": tell Julian to update the grammar to ready for
 * recognition.  When updaing grammars while paused, calling this just
 * before "resume" may improve the delay caused by update process of
 * grammar at the beginning of recognition restart.
 * 
 * @param sd [in] socket to send data
 * </EN>
 */
void
japi_sync_grammar(int sd)
{
  do_send(sd, "SYNCGRAM\n");
}

void
japi_get_graminfo(int sd)
{
  do_send(sd, "GRAMINFO\n");
}

void
japi_add_words(int sd, char *idstr, char *dictfile)
{
  FILE *fp;

  if ((fp = fopen(dictfile, "r")) == NULL) {
    perror("japi_add_words");
    return;
  }
  do_send(sd, "ADDWORD\n");
  do_sendf(sd, "%s\n", idstr);
  while(fgets(buf, MAXLINELEN, fp) != NULL) {
    do_send(sd, buf);
  }
  do_send(sd, "DICEND\n");
  fclose(fp);
}
