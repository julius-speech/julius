/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* nextword --- interactive gramamr checker */

#include <sent/stddefs.h>
#include <sent/vocabulary.h>
#include <sent/dfa.h>
#include <sent/speech.h>
#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif
#include "nextword.h"
#include "common.h"

WORD_INFO *winfo;		/* 単語情報 */
DFA_INFO *dfa;			/* DFA */
char **termname;		/* カテゴリ名 from .term */

/* フラグ達 */
boolean no_term_file;
boolean verbose_flag = FALSE;
boolean term_mode = FALSE;
boolean reverse_mode = FALSE;

/* 入力 */
static WTOKEN *wseq[MAXSEQNUM];	/* 入力単語系列 */
static int nseq;		/* 入力単語数 */

/* パージング結果 */
static STATECHAIN *reach_state;	/* 最大到達状態 */
static boolean can_accept;	/* 受理可能かどうか */
static int nseq_reached;	/* 最大到達単語数 */

/* 到達した状態はスタックにためておく */
static void
push_state(int stateid)
{
  STATECHAIN *new;
  new = (STATECHAIN *)mymalloc(sizeof(STATECHAIN));
  new->state = stateid;
  new->next = reach_state;
  reach_state = new;
}

static void
free_reachstate()
{
  STATECHAIN *st, *tmp;
  
  st = reach_state;
  while (st) {
    tmp = st->next;
    free(st);
    st = tmp;
  }
}

static void
put_state(int s, int l)
{
  int i;
  for (i=0;i<=l;i++) printf("  ");
  printf("[%d]\n",s);
}

/* 状態stateidにてiseq番目の入力が受け付けられるかどうかを返す */
/* 深さ優先探索 */
static void
can_accept_recursive(int stateid, int iseq)
{
  WTOKEN *token;
  int *nterms;
  int *nstate;
  int cnum;
  int i;

  nterms = (int *)mymalloc(sizeof(int)*dfa->term_num * 2);
  nstate = (int *)mymalloc(sizeof(int)*dfa->term_num * 2);

  if (verbose_flag) put_state(stateid, iseq);

  /* end process */
  if (nseq_reached > iseq) nseq_reached = iseq;
  if (iseq < 0) {		/* reaches last */
    /* push current status */
    push_state(stateid);
    if (dfa->st[stateid].status & ACCEPT_S) {
      can_accept = TRUE;
    }
    free(nterms); free(nstate);
    return;
  }

  for (token = wseq[iseq]; token; token = token->next) {
    if (verbose_flag) {
      for(i=0;i<=iseq;i++) printf("  ");
      if (no_term_file) {
	printf("%s(%s)\n",winfo->woutput[token->wid], winfo->wname[token->wid]);
      } else {
	printf("%s(%s:%s)\n",winfo->woutput[token->wid], termname[winfo->wton[token->wid]], winfo->wname[token->wid]);
      }
    }
    cnum = next_terms(stateid, nterms, nstate);
    for (i=0;i<cnum;i++) {
      if (nterms[i] == winfo->wton[token->wid]) { /* found */
	can_accept_recursive(nstate[i], iseq - 1);
      }
    }
  }

  free(nterms); free(nstate);
  return;
}

#if 0
/* 無音モデル silB, silE がなければ挿入 */
static char *
pad_sil(char *s)
{
  char *name;
  char *p;

  name = s;
  
  if (strncmp(name, "silB",4) != 0) {
    p = (char *)mymalloc(strlen(name)+6);
    strcpy(p, "silB ");
    strcat(p, name);
    free(name);
    name = p;
  }
  if (strncmp(&(name[strlen(name)-4]), "silE",4) != 0) {
    p = (char *)mymalloc(strlen(name)+6);
    strcpy(p, name);
    strcat(p, " silE");
    free(name);
    name = p;
  }
  return(name);
}
#endif


static void
put_nextword()
{
  STATECHAIN *tmp;
  int state;
  int *nterms, *nstate;
  int cnum;
  int i,j;
#define NW_PUT_LIMIT 3
  
  nterms = (int *)mymalloc(sizeof(int)*dfa->term_num * 2);
  nstate = (int *)mymalloc(sizeof(int)*dfa->term_num * 2);

  for(tmp=reach_state; tmp; tmp=tmp->next) {
    state = tmp->state;
    cnum = next_terms(state, nterms, nstate);
    cnum = compaction_int(nterms, cnum);
    for (i=0;i<cnum;i++){
      if (no_term_file) {
	printf("\t%-16d (", nterms[i]);
      } else {
	printf("\t%16s (", termname[nterms[i]]);
      }
      if (dfa->term.wnum[nterms[i]] > NW_PUT_LIMIT) {
	for(j=0;j < NW_PUT_LIMIT;j++) {
	  printf("%s ",winfo->woutput[dfa->term.tw[nterms[i]][j]]);
	}
	printf("...)\n");
      } else {
	for(j=0;j<dfa->term.wnum[nterms[i]];j++) {
	  printf("%s ",winfo->woutput[dfa->term.tw[nterms[i]][j]]);
	}
	printf(")\n");
      }
    }
  }
  free(nterms); free(nstate);
}

static void
nextword_main()
{
  int i;
  char *buf;
  /* get word sequence */
  do {
    if (term_mode) buf = rl_gets("cate > ");
    else buf = rl_gets("wseq > ");
  } while (buf == NULL || new_get_wtoken(buf, wseq, &nseq) == FALSE);
  put_wtoken(wseq, nseq);

  reach_state = NULL;
  nseq_reached = nseq;
  can_accept = FALSE;
  for (i=0;i<dfa->state_num;i++) {
    if ((dfa->st[i].status & INITIAL_S) != 0) { /* 初期状態から */
      can_accept_recursive(i, nseq-1);
    }
  }
  /* results stored in can_accept and reach_state */
  if (reach_state == NULL) {	/* rejected */
    printf("REJECTED at %d\n", nseq_reached + 1);
  } else {
    if (can_accept) printf("ACCEPTABLE\n");
    printf("PREDICTED CATEGORIES/WORDS:\n");
    put_nextword();
  }

  free_wtoken(wseq, nseq);
  free_reachstate();
}


static char *
usage(char *s)
{
  fprintf(stderr, "nextword --- tty-based interactive grammar checker\n");
  fprintf(stderr, "usage: %s prefix\n",s);
  fprintf(stderr, "  -t      ... use category symbols instead of words (needs .term)\n");
  fprintf(stderr, "  -s string ... specify short-pause model\n");
  fprintf(stderr, "  -r      ... reverse order input\n");
  fprintf(stderr, "  -v      ... verbose output\n");
#ifndef HAVE_READLINE
  fprintf(stderr, "(READLINE feature disabled)\n");
#endif
  exit(1);
}

static void
put_dfainfo()
{
  printf("%d categories, %d words\n",dfa->term_num,winfo->num);
  printf("DFA has %d nodes and %d arcs\n", dfa->state_num, dfa->arc_num);
}

int main(int argc, char *argv[])
{
  int i, len;
  char *prefix;
  char *dfafile, *dictfile, *termfile;
  char *spname_default = SPNAME_DEF;
  char *spname = NULL;
#define NEXTARG (++i >= argc) ? (char *)usage(argv[0]) : argv[i]

  /* argument */
  if (argc == 1) usage(argv[0]);
  for(i=1;i<argc;i++) {
    if (argv[i][0] == '-') {
      switch(argv[i][1]) {
      case 'v':			/* verbose output */
	verbose_flag = TRUE;
	break;
      case 't':
	term_mode = TRUE;
	break;
      case 'r':
	reverse_mode = TRUE;
	break;
      case 's':
	if (++i >= argc) {
	  usage(argv[0]);
	}
	spname = argv[i];
      default:
	fprintf(stderr, "no such option: %s\n",argv[i]);
	usage(argv[0]);
      }
    } else {
      prefix = argv[i];
    }
  }
  if (spname == NULL) spname = spname_default;
  
  len = strlen(prefix) + 10;
  dfafile = (char *)mymalloc(len);
  dictfile = (char *)mymalloc(len);
  termfile = (char *)mymalloc(len);
  strcpy(dfafile, prefix);
  strcat(dfafile, ".dfa");
  strcpy(dictfile, prefix);
  strcat(dictfile, ".dict");
  strcpy(termfile, prefix);
  strcat(termfile, ".term");

  /* start init */
  winfo = word_info_new();
  init_voca(winfo, dictfile, NULL, TRUE, FALSE);
  dfa = dfa_info_new();
  init_dfa(dfa, dfafile);
  make_dfa_voca_ref(dfa, winfo);
  termname = (char **)mymalloc(sizeof(char *) * dfa->term_num);
  init_term(termfile, termname);
  if (termname[0] == NULL) {	/* no .term file */
    no_term_file = TRUE;
  } else {
    no_term_file = FALSE;
  }

  /* output info */
  put_dfainfo();

  /* set dfa->sp_id and dfa->is_sp[cid] from name "sp" */
  {
    int t, i;
    WORD_ID w;

    dfa->sp_id = WORD_INVALID;
    dfa->is_sp = (boolean *)mymalloc(sizeof(boolean) * dfa->term_num);
    for(t=0;t<dfa->term_num;t++) {
      dfa->is_sp[t] = FALSE;
      for(i=0;i<dfa->term.wnum[t]; i++) {
	w = dfa->term.tw[t][i];
	if (strcmp(winfo->woutput[w], spname) == 0) {
	  if (dfa->sp_id == WORD_INVALID) dfa->sp_id = w;
	  dfa->is_sp[t] = TRUE;
	  break;
	}
      }
    }
  }
  if (verbose_flag) {
    if (dfa->sp_id != WORD_INVALID) {
      printf("skippable word for NOISE: %s\t%s\n", winfo->wname[dfa->sp_id], winfo->woutput[dfa->sp_id]);
    }
  }
  printf("----- \n");

#ifdef HAVE_READLINE
  printf("command completion is built-in\n----- \n");
#else
  printf("command completion is disabled\n----- \n");
#endif

  /* initialize readline */
#ifdef HAVE_READLINE
  /* rl_bind_key(PAGE, rl_menu_complete); */
  if (term_mode && !no_term_file) {
#ifdef HAVE_READLINE_4_1_OLDER
    rl_completion_entry_function = (Function *)dfaterm_generator;
#else
    rl_completion_entry_function = (rl_compentry_func_t *)dfaterm_generator;
#endif
  } else {
#ifdef HAVE_READLINE_4_1_OLDER
    rl_completion_entry_function = (Function *)dfaword_generator;
#else
    rl_completion_entry_function = (rl_compentry_func_t *)dfaword_generator;
#endif
  }
#endif
  /* main loop */
  for (;;) {
    nextword_main();
  }

  free(dfafile);
  free(dictfile);
  return 0;
}
