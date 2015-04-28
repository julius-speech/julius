/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* accept_check --- sentence accept checker for DFA grammar */

#include <sent/stddefs.h>
#include <sent/vocabulary.h>
#include <sent/dfa.h>
#include <sent/speech.h>
#include "common.h"

#define SPNAME_DEF "sp"

WORD_INFO *winfo;
DFA_INFO *dfa;
char **termname;
boolean no_term_file;

boolean verbose_flag = FALSE;
boolean term_mode = FALSE;

#define MAXBUFLEN 4096
typedef struct __wtoken__ {
  WORD_ID wid;
  struct __wtoken__ *next;
} WTOKEN;

static char buf[MAXBUFLEN];
static WTOKEN *wseq[MAXSEQNUM];
static int nseq;
static int nseq_reached;

static void
put_wtoken()
{
  int i;
  WTOKEN *tok;
  
  printf("wseq:");
  for (i=0;i<nseq;i++) {
    printf(" %s", winfo->woutput[wseq[i]->wid]);
  }
  printf("\n");
  printf("cate:");
  for (i=0;i<nseq;i++) {
    if (wseq[i]->next != NULL) { /* more than one */
      printf(" (");
    } else {
      printf(" ");
    }
    if (no_term_file) {
      printf("%s", winfo->wname[wseq[i]->wid]);
    } else {
      printf("%s", termname[winfo->wton[wseq[i]->wid]]);
    }
    if (wseq[i]->next != NULL) { /* more than one */
      for(tok = wseq[i]->next; tok; tok = tok->next) {
	if (no_term_file) {
	  printf("|%s", winfo->wname[tok->wid]);
	} else {
	  printf("|%s", termname[winfo->wton[tok->wid]]);
	}
      }
      printf(")");
    }
  }
  printf("\n");
}

static boolean
get_wtoken()
{
  char *p;
  int i,it;
  WTOKEN *new, *prev;
  
  /* get word sequence from stdin */
  if (term_mode) {
    fprintf(stderr, "please input category sequence>");
  } else {
    fprintf(stderr, "please input word sequence>");
  }
  if (fgets(buf, MAXBUFLEN, stdin) == NULL) {
    /* if input error, terminate program */
    exit(0);
  }

  /* decode string -> wid */
  nseq = 0;
  for(p = strtok(buf, " \n"); p; p = strtok(NULL, " \n")) {
    it = 0;
    prev = NULL;
    if (term_mode) {
      if (no_term_file) {
	if (atoi(p) >= 0 && atoi(p) < dfa->term_num) {
	  new = (WTOKEN *)mymalloc(sizeof(WTOKEN));
	  if (dfa->term.wnum[atoi(p)] == 0) {
	    printf("rejected at %d: category \"%s\" has no word\n", nseq+1, p);
	    return(FALSE);
	  }
	  new->wid = dfa->term.tw[atoi(p)][0];
	  new->next = prev;
	  prev = new;
	  it++;
	}
      } else {			/* termname exist */
	for (i=0;i<dfa->term_num;i++) {
	  if (strmatch(p, termname[i])) {
	    if (dfa->term.wnum[i] == 0) {
	      printf("rejected at %d: category \"%s\" has no word\n", nseq+1, p);
	      return(FALSE);
	    }
	    new = (WTOKEN *)mymalloc(sizeof(WTOKEN));
	    new->wid = dfa->term.tw[i][0];
	    new->next = prev;
	    prev = new;
	    it++;
	  }
	}
      }
      if (prev == NULL) {		/* not found */
	printf("rejected at %d: category \"%s\" not exist\n", nseq+1, p);
	return(FALSE);
      }
    } else {			/* normal word mode */
      for (i=0;i<winfo->num;i++) {
	if (strmatch(p, winfo->woutput[i])) {
	  new = (WTOKEN *)mymalloc(sizeof(WTOKEN));
	  new->wid = i;
	  new->next = prev;
	  prev = new;
	  it++;
	}
      }
      if (prev == NULL) {		/* not found */
	printf("rejected at %d: word \"%s\" not in voca\n", nseq+1, p);
	return(FALSE);
      }
    }
    wseq[nseq++] = new;
  }

  /* output */
  put_wtoken();
  return(TRUE);
}

/* 状態stateidにてiseq番目の入力が受け付けられるかどうかを返す */
/* 深さ優先探索 */
static void
put_state(int s, int l)
{
  int i;
  for (i=0;i<=l;i++) printf("  ");
  printf("[%d]\n",s);
}

static boolean
can_accept_recursive(int stateid, int iseq)
{
  WTOKEN *token;
  DFA_ARC *arc, *arc2;
  int cate, cate2, ns, ns2;
  int i;

  if (verbose_flag) put_state(stateid, iseq);

  if (nseq_reached > iseq) nseq_reached = iseq;
  if (iseq < 0) {		/* reaches last */
    /* previous call is last word */
    if (dfa->st[stateid].status | ACCEPT_S) {
      return TRUE;
    } else {
      return FALSE;
    }
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
    for (arc = dfa->st[stateid].arc; arc; arc = arc->next) {
      cate = arc->label;
      ns = arc->to_state;
      if (dfa->is_sp[cate]) {
	for (arc2 = dfa->st[ns].arc; arc2; arc2 = arc2->next) {
	  cate2 = arc2->label;
	  ns2 = arc2->to_state;
	  if (cate2 == winfo->wton[token->wid]) { /* found */
	    if (can_accept_recursive(ns2, iseq - 1)) {
	      return TRUE;
	    } else {
	      /* examine next */
	      if (verbose_flag) put_state(stateid, iseq);
	    }
	  }
	}
      } else {			/* not noise */
	if (cate == winfo->wton[token->wid]) { /* found */
	  if (can_accept_recursive(ns, iseq - 1)) {
	    return TRUE;
	  } else {
	    /* examine next */
	      if (verbose_flag) put_state(stateid, iseq);
	  }
	}
      }
    }
  }

  /* not allowed under this node */
  return FALSE;
}

static void
accept_main()
{
  int i;
  
  if (!get_wtoken()) return; /* failed */
  if (nseq == 0) return;

  nseq_reached = nseq;

  for (i=0;i<dfa->state_num;i++) {
    if ((dfa->st[i].status & INITIAL_S) != 0) { /* 初期状態から */
      if (can_accept_recursive(i, nseq-1)) {
	printf("accepted\n");
	return;
      }
    }
  }
  printf("rejected at %d by DFA\n", nseq_reached + 1);
}


static char *
usage(char *s)
{
  fprintf(stderr, "accept_check --- determine acception/rejection of transcription from stdin\n");
  fprintf(stderr, "usage: %s [-t] [-v] prefix\n",s);
  fprintf(stderr, "  -t  ... use category symbols instead of words (needs .term)\n");
  fprintf(stderr, "  -s string ... specify short-pause model\n");
  fprintf(stderr, "  -v  ... verbose output\n");
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

  /* main loop */
  for (;;) {
    accept_main();
  }

  free(dfafile);
  free(dictfile);
  return 0;
}
