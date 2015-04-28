#include "app.h"

#include <stdarg.h>

#define DEFAULT_MODULEPORT 10500

static int module_mode = FALSE;
static int module_port = DEFAULT_MODULEPORT;
int module_sd = -1;
static FILE *module_fp;
static RecogProcess *cur = NULL;

#define MAXBUFLEN 4096 ///< Maximum line length of a message sent from a client
static char mbuf[MAXBUFLEN];	///< Work buffer for message output
static char buf[MAXBUFLEN];	///< Work buffer for exec
static char inbuf[MAXBUFLEN];
#ifdef CHARACTER_CONVERSION
static char outbuf[MAXBUFLEN];
#endif

/** 
 * Generic function to send a formatted message to client module.
 *
 * @param sd [in] socket descriptor
 * @param fmt [in] format string, like printf.
 * @param ... [in] variable length argument like printf.
 * 
 * @return the same as printf, i.e. number of characters printed.
 */
int
module_send(int sd, char *fmt, ...)
{
  va_list ap;
  int ret;
  char *buf;
  
  va_start(ap,fmt);
  ret = vsnprintf(inbuf, MAXBUFLEN, fmt, ap);
  va_end(ap);
  if (ret > 0) {		/* success */
    
#ifdef CHARACTER_CONVERSION
    buf = charconv(inbuf, outbuf, MAXBUFLEN);
#else
    buf = inbuf;
#endif
    if (
#ifdef WINSOCK
	send(sd, buf, strlen(buf), 0)
#else
	write(sd, buf, strlen(buf))
#endif
	< 0) {
      perror("Error: module_send:");
    }
  }
  return(ret);
}

static char *
myfgets(char *buf, int maxlen, FILE *fp)
{
  char *ret;
  int len;
  
  if ((ret = fgets(buf, maxlen, fp)) != NULL) {
    len = strlen(buf);
    if (buf[len-1] == '\n') {
      buf[len-1] = '\0';
      if (len >= 2 && buf[len-2] == '\r') {
	buf[len-2] = '\0';
      }
    }
  }
  return ret;
}

/** 
 * Read grammar (DFA and dictionary) from socket and returns newly allocated
 * grammars.
 * 
 * @param sd [in] socket descpriter
 * @param ret_dfa [out] read DFA
 * @param ret_winfo [out] read dictionary
 * @param hmminfo [in] HMM definition
 * 
 * @return TRUE on success, or FALSE on failure.
 * </EN>
 */
static boolean
read_grammar(FILE *fp, DFA_INFO **ret_dfa, WORD_INFO **ret_winfo, HTK_HMM_INFO *hmminfo, RecogProcess *r)
{
  DFA_INFO *dfa = NULL;
  WORD_INFO *winfo;
  JCONF_LM *lmconf;

  /* load grammar: dfa and dict in turn */
  if (r->lmvar != LM_DFA_WORD) {
    dfa = dfa_info_new();
    if (!rddfa_fp(fp, dfa)) {
      return FALSE;
    }
  }
  winfo = word_info_new();
  if (r->lmvar == LM_DFA_WORD) {
    lmconf = r->lm->config;
    if (!voca_load_wordlist_fp(fp, winfo, hmminfo, lmconf->wordrecog_head_silence_model_name, lmconf->wordrecog_tail_silence_model_name, (lmconf->wordrecog_silence_context_name[0] == '\0') ? NULL : lmconf->wordrecog_silence_context_name)) {
      return FALSE;
    }
  } else {
    if (!voca_load_htkdict_fp(fp, winfo, hmminfo, FALSE)) {
      dfa_info_free(dfa);
      return FALSE;
    }
  }
  *ret_dfa = dfa;
  *ret_winfo = winfo;
  return TRUE;
}


static void
send_process_stat(RecogProcess *r)
{
  module_send(module_sd, "<SR ID=\"%d\" NAME=\"%s\"", r->config->id, r->config->name);
  switch(r->lmtype) {
  case LM_PROB: module_send(module_sd, " LMTYPE=\"PROB\""); break;
  case LM_DFA: module_send(module_sd, " LMTYPE=\"DFA\""); break;
  }
  switch(r->lmvar) {
  case LM_NGRAM: module_send(module_sd, " LMVAR=\"NGRAM\""); break;
  case LM_DFA_GRAMMAR: module_send(module_sd, " LMVAR=\"GRAMMAR\""); break;
  case LM_DFA_WORD: module_send(module_sd, " LMVAR=\"WORD\""); break;
  case LM_NGRAM_USER: module_send(module_sd, " LMVAR=\"USER\""); break;
  }
  if (r->live) {
    module_send(module_sd, " LIVE=\"ACTIVE\"");
  } else {
    module_send(module_sd, " LIVE=\"INACTIVE\"");
  }
  module_send(module_sd, "/>\n.\n");
}

static void
send_current_process(RecogProcess *r)
{
  module_send(module_sd, "<RECOGPROCESS INFO=\"CURRENT\">\n");
  send_process_stat(r);
  module_send(module_sd, "</RECOGPROCESS>\n.\n");
}

/** 
 * <JA>
 * @brief  モジュールコマンドを処理する. 
 *
 * クライアントより与えられたコマンドを処理する. この関数はクライアントから
 * コマンドが送られてくるたびに音声認識処理に割り込んで呼ばれる. 
 * ステータス等についてはここですぐに応答メッセージを送るが，
 * 文法の追加や削除などは，ここでは受信のみ行い，実際の変更処理
 * （各文法からのグローバル文法の再構築など）は認識処理の合間に実行される. 
 * この文法再構築処理を実際に行うのは multigram_update() である. 
 * 
 * @param command [in] コマンド文字列
 * </JA>
 * <EN>
 * @brief  Process a module command.
 *
 * This function processes command string received from module client.
 * This will be called whenever a command arrives from a client, interrupting
 * the main recognition process.  The status responses will be performed
 * at this function immediately.  On the whole, grammar modification
 * (add/delete/(de)activation) will not be performed here.  The received
 * data are just stored in this function, and they will be processed later
 * by calling multigram_update() between the recognition process.
 * 
 * @param command [in] command string
 * </EN>
 */
static void
msock_exec_command(char *command, Recog *recog)
{
  DFA_INFO *new_dfa;
  WORD_INFO *new_winfo;
  static char *p, *q;
  int gid;
  int ret;
  RecogProcess *r;

  /* prompt the received command string */
  printf("[[%s]]\n",command);

  if (cur == NULL) {
    cur = recog->process_list;
  }

  if (strmatch(command, "STATUS")) {
    /* return status */
    if (recog->process_active) {
      module_send(module_sd, "<SYSINFO PROCESS=\"ACTIVE\"/>\n.\n");
    } else {
      module_send(module_sd, "<SYSINFO PROCESS=\"SLEEP\"/>\n.\n");
    }
  } else if (strmatch(command, "DIE")) {
    /* disconnect */
    close_socket(module_sd);
    module_sd = -1;
#if defined(_WIN32) && !defined(__CYGWIN32__)
    /* this is single process and has not forked, so
       we just disconnect the connection here.  */
#else
    /* this is a forked process, so exit here. */

#endif
  } else if (strmatch(command, "VERSION")) {
    /* return version */
    module_send(module_sd, "<ENGINEINFO TYPE=\"%s\" VERSION=\"%s\" CONF=\"%s\"/>\n.\n",
		JULIUS_PRODUCTNAME, JULIUS_VERSION, JULIUS_SETUP);
  } else if (strmatch(command, "PAUSE")) {
    /* pause recognition: will stop when the current input ends */
    j_request_pause(recog);
  } else if (strmatch(command, "TERMINATE")) {
    j_request_terminate(recog);
  } else if (strmatch(command, "RESUME")) {
    j_request_resume(recog);
  } else if (strmatch(command, "INPUTONCHANGE")) {
    /* change grammar switching timing policy */
    if (
	myfgets(buf, MAXBUFLEN, module_fp)
	== NULL) {
      fprintf(stderr, "Error: msock(INPUTONCHANGE): no argument\n");
      return;
    }
    if (strmatch(buf, "TERMINATE")) {
      recog->gram_switch_input_method = SM_TERMINATE;
    } else if (strmatch(buf, "PAUSE")) {
      recog->gram_switch_input_method = SM_PAUSE;
    } else if (strmatch(buf, "WAIT")) {
      recog->gram_switch_input_method = SM_WAIT;
    } else {
      fprintf(stderr, "Error: msock(INPUTONCHANGE): unknown method [%s]\n", buf); exit(-1);
    }
  } else if (strnmatch(command, "GRAMINFO", strlen("GRAMINFO"))) {
    send_gram_info(cur);
  } else if (strnmatch(command, "CHANGEGRAM", strlen("CHANGEGRAM"))) {
    /* receive grammar (DFA + DICT) from the socket, and swap the whole grammar  */
    /* read grammar name if any */
    p = &(command[strlen("CHANGEGRAM")]);
    while (*p == ' ' && *p != '\r' && *p != '\n' && *p != '\0') p++;
    if (*p != '\r' && *p != '\n' && *p != '\0') {
      q = buf;
      while (*p != ' ' && *p != '\r' && *p != '\n' && *p != '\0') *q++ = *p++;
      *q = '\0';
      p = buf;
    } else {
      p = NULL;
    }
    /* read a new grammar via socket */
    if (read_grammar(module_fp, &new_dfa, &new_winfo, cur->am->hmminfo, cur) == FALSE) {
      module_send(module_sd, "<GRAMMAR STATUS=\"ERROR\" REASON=\"WRONG DATA\"/>\n.\n");
    } else {
      if (cur->lmtype == LM_DFA) {
	/* delete all existing grammars */
	multigram_delete_all(cur->lm);
	/* register the new grammar to multi-gram tree */
	multigram_add(new_dfa, new_winfo, p, cur->lm);
	/* need to rebuild the global lexicon */
	/* tell engine to update at requested timing */
	schedule_grammar_update(recog);
	/* make sure this process will be activated */
	cur->active = 1;
	/* tell module client  */
	module_send(module_sd, "<GRAMMAR STATUS=\"RECEIVED\"/>\n.\n");
	send_gram_info(cur);
      } else {
	module_send(module_sd, "<GRAMMAR STATUS=\"ERROR\" REASON=\"NOT A GRAMMAR-BASED LM\"/>\n.\n");
      }
    }
  } else if (strnmatch(command, "ADDGRAM", strlen("ADDGRAM"))) {
    /* receive grammar and add it to the current grammars */
    /* read grammar name if any */
    p = &(command[strlen("ADDGRAM")]);
    while (*p == ' ' && *p != '\r' && *p != '\n' && *p != '\0') p++;
    if (*p != '\r' && *p != '\n' && *p != '\0') {
      q = buf;
      while (*p != ' ' && *p != '\r' && *p != '\n' && *p != '\0') *q++ = *p++;
      *q = '\0';
      p = buf;
    } else {
      p = NULL;
    }
    /* read a new grammar via socket */
    if (read_grammar(module_fp, &new_dfa, &new_winfo, cur->am->hmminfo, cur) == FALSE) {
      module_send(module_sd, "<GRAMMAR STATUS=\"ERROR\" REASON=\"WRONG DATA\"/>\n.\n");
    } else {
      if (cur->lmtype == LM_DFA) {
	/* add it to multi-gram tree */
	multigram_add(new_dfa, new_winfo, p, cur->lm);
	/* need to rebuild the global lexicon */
	/* make sure this process will be activated */
	cur->active = 1;
	/* tell engine to update at requested timing */
	schedule_grammar_update(recog);
	/* tell module client  */
	module_send(module_sd, "<GRAMMAR STATUS=\"RECEIVED\"/>\n.\n");
	send_gram_info(cur);
      } else {
	module_send(module_sd, "<GRAMMAR STATUS=\"ERROR\" REASON=\"NOT A GRAMMAR-BASED LM\"/>\n.\n");
      }
    }
  } else if (strmatch(command, "DELGRAM")) {
    /* remove the grammar specified by ID or name */
    /* read a list of grammar IDs to be deleted */
    if (
	myfgets(buf, MAXBUFLEN, module_fp)
	== NULL) {
      fprintf(stderr, "Error: msock(DELGRAM): no argument\n");
      return;
    }
    /* extract IDs and mark them as delete
       (actual deletion will be performed on the next 
    */
    if (cur->lmtype == LM_DFA) {
      for(p=strtok(buf," ");p;p=strtok(NULL," ")) {
	q = p;
	while(*q != '\0' && *q != '\r' && *q != '\n') {
	  if (*q < '0' || *q > '9') break;
	  q++;
	}
	if (*q == '\0' || *q == '\r' || *q == '\n') { /* numeric */
	  gid = atoi(p);
	} else {		/* string */
	  gid = multigram_get_id_by_name(cur->lm, p);
	  if (gid == -1) continue;
	}
	if (multigram_delete(gid, cur->lm) == FALSE) { /* deletion marking failed */
	  fprintf(stderr, "Warning: msock(DELGRAM): gram #%d failed to delete, ignored\n", gid);
	  /* tell module */
	  module_send(module_sd, "<GRAMMAR STATUS=\"ERROR\" REASON=\"Gram #%d not found\"/>\n.\n", gid);
	}
      }
      /* need to rebuild the global lexicon */
      /* tell engine to update at requested timing */
      schedule_grammar_update(recog);
    } else {
      module_send(module_sd, "<GRAMMAR STATUS=\"ERROR\" REASON=\"NOT A GRAMMAR-BASED LM\"/>\n.\n");
    }
  } else if (strmatch(command, "ACTIVATEGRAM")) {
    /* activate grammar in this engine */
    /* read a list of grammar IDs or names to be activated */
    if (
	myfgets(buf, MAXBUFLEN, module_fp)
	== NULL) {
      fprintf(stderr, "Error: msock(ACTIVATEGRAM): no argument\n");
      return;
    }
    /* mark them as active */
    if (cur->lmtype == LM_DFA) {
      for(p=strtok(buf," ");p;p=strtok(NULL," ")) {
	q = p;
	while(*q != '\0' && *q != '\r' && *q != '\n') {
	  if (*q < '0' || *q > '9') break;
	  q++;
	}
	if (*q == '\0' || *q == '\r' || *q == '\n') { /* numeric */
	  gid = atoi(p);
	} else {		/* string */
	  gid = multigram_get_id_by_name(cur->lm, p);
	  if (gid == -1) continue;
	}
	ret = multigram_activate(gid, cur->lm);
	if (ret == 1) {
	  /* already active */
	  module_send(module_sd, "<WARN MESSAGE=\"Gram #%d already active\"/>\n.\n", gid);
	} else if (ret == -1) {
	  /* not found */
	  module_send(module_sd, "<WARN MESSAGE=\"Gram #%d not found\"/>\n.\n", gid);
	}	/* else success */
      }
      /* tell engine to update at requested timing */
      schedule_grammar_update(recog);
    } else {
      module_send(module_sd, "<GRAMMAR STATUS=\"ERROR\" REASON=\"NOT A GRAMMAR-BASED LM\"/>\n.\n");
    }
  } else if (strmatch(command, "DEACTIVATEGRAM")) {
    /* deactivate grammar in this engine */
    /* read a list of grammar IDs or names to be de-activated */
    if (
	myfgets(buf, MAXBUFLEN, module_fp)
	== NULL) {
      fprintf(stderr, "Error: msock(DEACTIVATEGRAM): no argument\n");
      return;
    }
    if (cur->lmtype == LM_DFA) {
      /* mark them as not active */
      for(p=strtok(buf," ");p;p=strtok(NULL," ")) {
	q = p;
	while(*q != '\0' && *q != '\r' && *q != '\n') {
	  if (*q < '0' || *q > '9') break;
	  q++;
	}
	if (*q == '\0' || *q == '\r' || *q == '\n') { /* numeric */
	  gid = atoi(p);
	} else {		/* string */
	  gid = multigram_get_id_by_name(cur->lm, p);
	  if (gid == -1) continue;
	}
	ret = multigram_deactivate(gid, cur->lm);
	if (ret == 1) {
	  /* already inactive */
	  module_send(module_sd, "<WARN MESSAGE=\"Gram #%d already inactive\"/>\n.\n", gid);
	} else if (ret == -1) {
	  /* not found */
	  module_send(module_sd, "<WARN MESSAGE=\"Gram #%d not found\"/>\n.\n", gid);
	}	/* else success */
      }
      schedule_grammar_update(recog);
    } else {
      module_send(module_sd, "<GRAMMAR STATUS=\"ERROR\" REASON=\"NOT A GRAMMAR-BASED LM\"/>\n.\n");
    }
  } else if (strmatch(command, "SYNCGRAM")) {
    /* update grammar if necessary */
    if (cur->lmtype == LM_DFA) {
      multigram_update(cur->lm);  /* some modification occured if return TRUE */
      for(r=recog->process_list;r;r=r->next) {
	if (r->lmtype == LM_DFA && r->lm->global_modified) {
	  multigram_build(r);
	}
      }
      cur->lm->global_modified = FALSE;
      module_send(module_sd, "<GRAMMAR STATUS=\"READY\"/>\n.\n");
    } else {
      module_send(module_sd, "<GRAMMAR STATUS=\"ERROR\" REASON=\"NOT A GRAMMAR-BASED LM\"/>\n.\n");
    }
  } else if (strmatch(command, "CURRENTPROCESS")) {
    JCONF_SEARCH *sconf;
    RecogProcess *r;
    if (
	myfgets(buf, MAXBUFLEN, module_fp)
	== NULL) {
      /* when no argument, just return current process */
      send_current_process(cur);
      return;
    }
    if (buf[0] == '\0') {
      /* when no argument, just return current process */
      send_current_process(cur);
      return;
    }
    sconf = j_get_searchconf_by_name(recog->jconf, buf);
    if (sconf == NULL) {
      fprintf(stderr, "Error: msock(CURRENTPROCESS): no such process \"%s\"\n", buf);
      module_send(module_sd, "<RECOGPROCESS STATUS=\"ERROR\" REASON=\"NO SUCH PROCESS\"/>\n.\n");
      return;
    }
    for(r=recog->process_list;r;r=r->next) {
      if (r->config == sconf) {
	cur = r;
	break;
      }
    }
    if (!r) {
      fprintf(stderr, "Error: msock(CURRENTPROCESS): no process assigned to searchconf \"%s\"??\n", buf);
      module_send(module_sd, "<RECOGPROCESS STATUS=\"ERROR\" REASON=\"NO SUCH PROCESS\"/>\n.\n");
      return;
    }
    send_current_process(cur);
  }

  else if (strmatch(command, "SHIFTPROCESS")) {
    cur = cur->next;
    if (cur == NULL) {
      fprintf(stderr, "SHIFTPROCESS: reached end, rotated to first\n");
      cur = recog->process_list;
    }
    send_process_stat(cur);
  }

  else if (strmatch(command, "ADDPROCESS")) {
    Jconf *jconf;
    JCONF_LM *lmconf;
    JCONF_AM *amconf;
    JCONF_SEARCH *sconf;
    RecogProcess *r;

    if (
	myfgets(buf, MAXBUFLEN, module_fp)
	== NULL) {
      fprintf(stderr, "Error: msock(ADDPROCESS): no argument\n");
      module_send(module_sd, "<RECOGPROCESS STATUS=\"ERROR\" REASON=\"NO ARGUMENT\"/>\n.\n");
      return;
    }
    /* load specified jconf file and use its last LM conf as new */
    jconf = j_jconf_new();
    j_config_load_file(jconf, buf);
    lmconf = jconf->lmnow;

    /* create a search instance */
    sconf = j_jconf_search_new();
    /* all the parameters are defaults */

    /* create process instance with new LM and SR */
    if (j_process_add_lm(recog, lmconf, sconf, buf) == FALSE) {
      fprintf(stderr, "Error: failed to regist new process \"%s\"\n", buf);
      module_send(module_sd, "<RECOGPROCESS STATUS=\"ERROR\" REASON=\"FAILED TO REGISTER\"/>\n.\n");
      j_jconf_search_free(sconf);
      return;
    }
    printf("added process: SR%02d %s\n", sconf->id, sconf->name);
    module_send(module_sd, "<RECOGPROCESS INFO=\"ADDED\">\n");
    for(r=recog->process_list;r;r=r->next) {
      if (r->config == sconf) {
	send_process_stat(r);
      }
    }
    module_send(module_sd, "</RECOGPROCESS>\n.\n");
  }

  else if (strmatch(command, "DELPROCESS")) {
    JCONF_SEARCH *sconf;
    JCONF_LM *lmconf;
    RecogProcess *r;

    if (
	myfgets(buf, MAXBUFLEN, module_fp)
	== NULL) {
      fprintf(stderr, "Error: msock(DELPROCESS): no argument\n");
      module_send(module_sd, "<RECOGPROCESS STATUS=\"ERROR\" REASON=\"NO ARGUMENT\"/>\n.\n");
      return;
    }

    sconf =  j_get_searchconf_by_name(recog->jconf, buf);
    if (sconf == NULL) {
      fprintf(stderr, "Error: msock(DELPROCESS): no searchconf named %s\n", buf);
      module_send(module_sd, "<RECOGPROCESS STATUS=\"ERROR\" REASON=\"NO RECOGPROCESS OF THE NAME\"/>\n.\n");
      return;
    }

    lmconf = sconf->lmconf;
    printf("remove process: SR%02d %s, LM%02d %s\n", sconf->id, sconf->name, lmconf->id, lmconf->name);
    module_send(module_sd, "<RECOGPROCESS INFO=\"DELETE\">\n");
    for(r=recog->process_list;r;r=r->next) {
      if (r->config == sconf) send_process_stat(r);
    }
    module_send(module_sd, "</RECOGPROCESS>\n.\n");
    j_process_remove(recog, sconf);
    j_process_lm_remove(recog, lmconf);
    /* change current */
    for(r=recog->process_list;r;r=r->next) {
      if (r == cur) break;
    }
    if (!r) {
      cur = recog->process_list;
      printf("now current moved to SR%02d %s\n", cur->config->id, cur->config->name);
      send_current_process(cur);
    }
    
  }

  else if (strmatch(command, "LISTPROCESS")) {
    RecogProcess *r;
    
    module_send(module_sd, "<RECOGPROCESS INFO=\"STATUS\">\n");
    for(r=recog->process_list;r;r=r->next) {
      send_process_stat(r);
    }
    module_send(module_sd, "</RECOGPROCESS>\n.\n");
  }

  else if (strmatch(command, "ACTIVATEPROCESS")) {
    if (
	myfgets(buf, MAXBUFLEN, module_fp)
	== NULL) {
      fprintf(stderr, "Error: msock(ACTIVATEPROCESS): no argument\n");
      module_send(module_sd, "<RECOGPROCESS STATUS=\"ERROR\" REASON=\"NO ARGUMENT\"/>\n.\n");
      return;
    }
    if (j_process_activate(recog, buf) == FALSE) {
      module_send(module_sd, "<RECOGPROCESS STATUS=\"ERROR\" REASON=\"ACTIVATION FAILED\"/>\n.\n");
    } else {
      module_send(module_sd, "<RECOGPROCESS INFO=\"ACTIVATED\" NAME=\"%s\"/>\n.\n", buf);
    }
  }
  else if (strmatch(command, "DEACTIVATEPROCESS")) {
    if (
	myfgets(buf, MAXBUFLEN, module_fp)
	== NULL) {
      fprintf(stderr, "Error: msock(DEACTIVATEPROCESS): no argument\n");
      module_send(module_sd, "<RECOGPROCESS STATUS=\"ERROR\" REASON=\"NO ARGUMENT\"/>\n.\n");
      return;
    }
    if (j_process_deactivate(recog, buf) == FALSE) {
      module_send(module_sd, "<RECOGPROCESS STATUS=\"ERROR\" REASON=\"DEACTIVATION FAILED\"/>\n.\n");
    } else {
      module_send(module_sd, "<RECOGPROCESS INFO=\"DEACTIVATED\" NAME=\"%s\"/>\n.\n", buf);
    }
    module_send(module_sd, ".\n");
  }
  else if (strmatch(command, "ADDWORD")) {
    WORD_INFO *words;
    boolean ret;
    int id;

    /* get gramamr ID to add */
    if (
	myfgets(buf, MAXBUFLEN, module_fp)
	== NULL) {
      fprintf(stderr, "Error: msock(DEACTIVATEPROCESS): no argument\n");
      module_send(module_sd, "<RECOGPROCESS STATUS=\"ERROR\" REASON=\"NO ARGUMENT\"/>\n.\n");
      return;
    }
    id = atoi(buf);

    /* read list of word entries, will stop by "DICEND" */
    words = word_info_new();
    voca_load_start(words, cur->am->hmminfo, FALSE);
    while (
	myfgets(buf, MAXBUFLEN, module_fp)
	!= NULL) {
      if (cur->lmvar == LM_DFA_WORD) {
	ret = voca_load_word_line(buf, words, cur->am->hmminfo, 
				  cur->lm->config->wordrecog_head_silence_model_name,
				  cur->lm->config->wordrecog_tail_silence_model_name,
				  (cur->lm->config->wordrecog_silence_context_name[0] == '\0') ? NULL : cur->lm->config->wordrecog_silence_context_name);
      } else {
	ret = voca_load_line(buf, words, cur->am->hmminfo);
      }
      if (ret == FALSE) break;
    }
    ret = voca_load_end(words);
    if (ret == FALSE) {
      fprintf(stderr, "Error: msock(ADDWORD): error in reading word entries\n");
      module_send(module_sd, "<RECOGPROCESS STATUS=\"ERROR\" REASON=\"ERROR IN READING WORD ENTRIES\"/>\n.\n");
      word_info_free(words);
      return;
    }
    if (words->num == 0) {
      fprintf(stderr, "Error: msock(ADDWORD): no word specified\n");
      module_send(module_sd, "<RECOGPROCESS STATUS=\"ERROR\" REASON=\"NO WORD SPECIFIED\"/>\n.\n");
      word_info_free(words);
      return;
    }
    printf("%d words read\n", words->num);
    /* add the words to the grammar */
    if (multigram_add_words_to_grammar_by_id(cur->lm, id, words) == FALSE) {
      fprintf(stderr, "Error: msock(ADDWORD): failed to add words to grammar #%d\n", id);
      module_send(module_sd, "<RECOGPROCESS STATUS=\"ERROR\" REASON=\"FAILED\"/>\n.\n");
      word_info_free(words);
      return;
    }
    /* book for update */
    schedule_grammar_update(recog);
    module_send(module_sd, "%d words added to grammar #%d\n.\n", words->num, id);
    module_send(module_sd, "<RECOGPROCESS INFO=\"ADDEDWORD\" GRAMMARID=\"%d\" NUM=\"%d\"/>\n.\n", id, words->num);

    word_info_free(words);
  }
}

/** 
 * <JA>
 * 現在クライアントモジュールからの命令がバッファにあるか調べ，
 * もしあれば処理する. なければそのまま終了する. 
 * 
 * </JA>
 * <EN>
 * Process one commands from client module.  If no command is in the buffer,
 * it will return without blocking.
 * 
 * </EN>
 */
static void
msock_check_and_process_command(Recog *recog, void *dummy)
{
  fd_set rfds;
  int ret;
  struct timeval tv;

  /* check if some commands are waiting in queue */
  FD_ZERO(&rfds);
  FD_SET(module_sd, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;	      /* 0 msec timeout: return immediately */
  ret = select(module_sd+1, &rfds, NULL, NULL, &tv);
  if (ret < 0) {
    perror("msock_check_and_process_command: cannot poll\n");
  }
  if (ret > 0) {
    /* there is data to read */
    /* process command and change status if necessaty */
    while(select(module_sd+1, &rfds, NULL, NULL, &tv) > 0 &&
	  myfgets(mbuf, MAXBUFLEN, module_fp)
	  != NULL) {
      msock_exec_command(mbuf, recog);
    }
  }
}

/** 
 * <JA>
 * クライアントモジュールからの命令を読み込んで処理する. 
 * 命令が無い場合，次のコマンドが来るまで待つ. 
 * msock_exec_command() 内で j_request_resume() が呼ばれて
 * recog->process_active が TRUE になるまで繰り返す. 
 * この関数が終わったときプロセスは resume |!する. |
 * 
 * </JA>
 * <EN>
 * Process one commands from client module.  If no command is in the buffer,
 * it will block until next command comes.
 * 
 * </EN>
 */
static void
msock_process_command(Recog *recog, void *dummy)
{

  while(!recog->process_active) {
    if (
	myfgets(mbuf, MAXBUFLEN, module_fp)
	!= NULL) {
      msock_exec_command(mbuf, recog);
    }
  }
}

static void
module_regist_callback(Recog *recog, void *data)
{
  callback_add(recog, CALLBACK_POLL, msock_check_and_process_command, data);
  callback_add(recog, CALLBACK_PAUSE_FUNCTION, msock_process_command, data);
}

/************************************************************************/
static boolean
opt_module(Jconf *jconf, char *arg[], int argnum)
{
  module_mode = TRUE;
  if (argnum > 0) {
    module_port = atoi(arg[0]);
  }
  return TRUE;
}

static boolean
opt_outcode(Jconf *jconf, char *arg[], int argnum)
{
  decode_output_selection(arg[0]);
  return TRUE;
}

void
module_add_option()
{
  j_add_option("-module", 1, 0, "run as a server module", opt_module);
  j_add_option("-outcode", 1, 1, "select info to output to the module: WLPSCwlps", opt_outcode);
}

boolean
is_module_mode()
{
  return module_mode;
}

void
module_setup(Recog *recog, void *data)
{
  /* register result output callback functions */
  module_regist_callback(recog, data);
  setup_output_msock(recog, data);
}
  
void
module_server()
{
  int listen_sd;	///< Socket to listen to a client
#if defined(_WIN32) && !defined(__CYGWIN32__)
  int sd;
#endif
  
  /* prepare socket to listen */
  if ((listen_sd = ready_as_server(module_port)) < 0) {
    fprintf(stderr, "Error: failed to bind socket\n");
    return;
  }
  
  printf  ("///////////////////////////////\n");
  printf  ("///  Module mode ready\n");
  printf  ("///  waiting client at %5d\n", module_port);
  printf  ("///////////////////////////////\n");
  printf  ("///  ");
  
  /* no fork, just wait for one connection and proceed */
  if ((module_sd = accept_from(listen_sd)) < 0) {
    fprintf(stderr, "Error: failed to accept connection\n");
    return;
  }
#if defined(_WIN32) && !defined(__CYGWIN32__)
  /* call winsock function to make the socket capable of reading/writing */
  if ((sd = _open_osfhandle(module_sd, O_RDWR|O_BINARY)) < 0) {
    fprintf(stderr, "Error: failed to open_osfhandle\n");
    return;
  }
  if ((module_fp = fdopen(sd, "rb+")) == NULL) {
    fprintf(stderr, "Error: failed to fdopen socket\n");
    return;
  }
#else
  if ((module_fp = fdopen(module_sd, "r+")) == NULL) {
    fprintf(stderr, "Error; failed to fdopen socket\n");
    return;
  }
#endif
}

void
module_disconnect()
{
  /* disconnect control module */
  if (module_sd >= 0) { /* connected now */
    module_send(module_sd, "<SYSINFO PROCESS=\"ERREXIT\"/>\n.\n");
    close_socket(module_sd);
    module_sd = -1;
  }
}
