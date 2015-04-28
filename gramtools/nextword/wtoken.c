/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/vocabulary.h>
#include <sent/dfa.h>
#include <sent/speech.h>
#include "nextword.h"

extern WORD_INFO *winfo;
extern DFA_INFO *dfa;
extern char **termname;

/* フラグ達 */
extern boolean no_term_file;
extern boolean verbose_flag;
extern boolean term_mode;
extern boolean reverse_mode;

/* 全wtokenを出力 */
void
put_wtoken(WTOKEN **wseq, int nseq)
{
  int i;
  WTOKEN *tok;
  
  printf("[wseq:");
  for (i=0;i<nseq;i++) {
    printf(" %s", winfo->woutput[wseq[i]->wid]);
  }
  printf("]\n");
  printf("[cate:");
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
  printf("]\n");
}

/* buf から wseq を生成 */
boolean
new_get_wtoken(char *buf, WTOKEN **wseq, int *nseq_ret)
{
  char *p;
  int i,it;
  WTOKEN *new, *prev;
  int nseq;
  
  /* decode string -> wid */
  nseq = 0;
  for(p = strtok(buf, " "); p; p = strtok(NULL, " ")) {
    it = 0;
    prev = NULL;
    if (term_mode) {
      if (no_term_file) {
	if (atoi(p) >= 0 && atoi(p) < dfa->term_num) {
	  new = (WTOKEN *)mymalloc(sizeof(WTOKEN));
	  if (dfa->term.wnum[atoi(p)] == 0) {
	    printf("word %d: category \"%s\" has no word\n", nseq+1, p);
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
	      printf("word %d: category \"%s\" has no word\n", nseq+1, p);
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
	printf("word %d: category \"%s\" not exist\n", nseq+1, p);
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
	printf("word %d: word \"%s\" not in voca\n", nseq+1, p);
	return(FALSE);
      }
    }
    wseq[nseq++] = new;
  }
  
  if (reverse_mode) {
    for (i=0;i < nseq / 2;i++) {
      new = wseq[i];
      wseq[i] = wseq[nseq - 1 - i];
      wseq[nseq - 1 - i] = new;
    }
  }

  *nseq_ret = nseq;

  return(TRUE);
}

/* メモリ解放 */
void
free_wtoken(WTOKEN **wseq, int nseq)
{
  int i;
  WTOKEN *tok, *tmp;

  for (i=0;i<nseq;i++) {
    tok = wseq[i];
    while (tok) {
      tmp = tok->next;
      free(tok);
      tok = tmp;
    }
  }
}

