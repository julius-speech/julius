/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* generate --- generate random sentences acceptable by given grammar */

#include "common.h"
#include "gen_next.h"
#if defined(_WIN32) && !defined(__CYGWIN32__)
#include "process.h"
#endif

#define MAXHYPO 300

WORD_INFO *winfo;
DFA_INFO *dfa;
char **termname;
boolean verbose_flag = FALSE;
boolean term_mode = FALSE;
boolean no_term_file;

NODE *
new_generate()
{
  NEXTWORD **nw;
  NODE *now;
  int i,j,num,selected;

  /* init */
  nw = nw_malloc();
  now = (NODE *)mymalloc(sizeof(NODE));
  now->endflag = FALSE;
  now->seqnum = 0;

  /* set init hypo */
  if (term_mode) {
    num = dfa_firstterms(nw);
  } else {
    num = dfa_firstwords(nw);
  }

  for (;;) {
    if (verbose_flag) {
      if (no_term_file) {
	for(i=0;i<num;i++)printf("\t-> %s\t%s\n",winfo->wname[nw[i]->id],winfo->woutput[nw[i]->id]);
      } else {
	for(i=0;i<num;i++)printf("\t-> %s\t%s\n",termname[winfo->wton[nw[i]->id]],winfo->woutput[nw[i]->id]);
      }
    }
    /* select random one */
    if (num == 1) {
      selected = 0;
    } else {
      j = abs(rand()) % num;
      for(i=0;i<j;i++) {
	selected = abs(rand()) % num;
      }
    }
    if (selected >= num) selected = num - 1;
    
    now->seq[now->seqnum++] = nw[selected]->id;
    now->state = nw[selected]->next_state;

    if (now->seqnum >= MAXSEQNUM) {
      printf("word num exceeded %d\n", MAXSEQNUM);
      nw_free(nw);
      return(now);
    }

    /* output */
    if (verbose_flag) {
      printf("(%3d) %s\n", now->state, winfo->woutput[now->seq[now->seqnum-1]]);
    }

    /* end check */
    if (dfa_acceptable(now)) break;

    /* get next words */
    if (term_mode) {
      num = dfa_nextterms(now, nw);
    } else {
      num = dfa_nextwords(now, nw);
    }
  }
  
  nw_free(nw);
  return(now);
  
}

static boolean
match_node(NODE *a, NODE *b)
{
  int i;
  
  if (a->seqnum != b->seqnum) return(FALSE);
  for (i=0;i<a->seqnum;i++) {
    if (a->seq[i] != b->seq[i]) return(FALSE);
  }
  return(TRUE);
}

static void
generate_main(int num)
{
  NODE *sent;
  NODE **stock;
  int i,n,c;

  /* avoid generating same sentence */
  stock = (NODE **)mymalloc(sizeof(NODE *)*num);
  n = 0;
  c = 0;
  while (n < num) {
    sent = new_generate();
    for (i=0;i<n;i++) {
      if (match_node(sent, stock[i])) break;
    }
    if (i >= n) {		/* no match, store as new */
      stock[n++] = sent;
      for (i=sent->seqnum-1;i>=0;i--) {
	if (term_mode) {
	  if (no_term_file) {
	    printf(" %s", winfo->wname[sent->seq[i]]);
	  } else {
	    printf(" %s", termname[winfo->wton[sent->seq[i]]]);
	  }
	} else {
	  printf(" %s", winfo->woutput[sent->seq[i]]);
	}
      }
      printf("\n");
      c = 0;
    } else {			/* same, ignored */
      c++;
      if (c >= MAXHYPO) {
	printf("no further sentence in the last %d trial\n", c);
	break;
      }
      free(sent);
    }
  }
  
  for(i=0;i<n;i++) free(stock[i]);
  free(stock);
}


static char *
usage(char *s)
{
  fprintf(stderr, "generate --- sentence random generator\n");
  fprintf(stderr, "usage: %s [-v] [-n] prefix\n",s);
  fprintf(stderr, "  -n num  ... generate N sentences (default: 10)\n");
  fprintf(stderr, "  -t      ... use category symbols instead of words (needs .term)\n");
  fprintf(stderr, "  -s string ... specify short-pause model\n");
  fprintf(stderr, "  -v      ... verbose output\n");
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
  char *prefix = NULL;
  char *dfafile, *dictfile, *termfile;
  int gnum = 10;
  char *spname_default = SPNAME_DEF;
  char *spname = NULL;
#define NEXTARG (++i >= argc) ? (char *)usage(argv[0]) : argv[i]

  /* argument */
  for(i=1;i<argc;i++) {
    if (argv[i][0] == '-') {
      switch(argv[i][1]) {
      case 'v':			/* verbose output */
	verbose_flag = TRUE;
	gnum = 1;
	break;
      case 't':			/* terminal mode */
	term_mode = TRUE;
	break;
      case 'n':
	gnum = atoi(NEXTARG);
	break;
      case 's':
	if (++i >= argc) {
	  usage(argv[0]);
	}
	spname = argv[i];
	break;
      default:
	fprintf(stderr, "no such option: %s\n",argv[i]);
	usage(argv[0]);
      }
    } else {
      prefix = argv[i];
    }
  }
  if (prefix == NULL) usage(argv[0]);

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

  /* random seed */
  srand(getpid());

  /* main loop */
  generate_main(gnum);

  free(dfafile);
  free(dictfile);
  return 0;
}
