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
#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif
#include "nextword.h"

extern WORD_INFO *winfo;
extern DFA_INFO *dfa;
extern char **termname;

/* フラグ達 */
extern boolean no_term_file;
extern boolean verbose_flag;
extern boolean term_mode;

static char *line_read = (char *)NULL; /* 読み込んだ文字列 */

#ifdef HAVE_READLINE
/* readline関係 */
/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
char *
rl_gets (char *prompt)
{
  /* If the buffer has already been allocated, return the memory
     to the free pool. */
  if (line_read) {
    free (line_read);
    line_read = (char *)NULL;
  }
  /* Get a line from the user. */
  line_read = readline (prompt);
  /* If the line has any text in it, save it on the history. */
  if (line_read && *line_read) {
    /*line_read = pad_sil(line_read);
    printf("%s\n",line_read);*/
    add_history (line_read);
  }
  return (line_read);
}

char *
dfaterm_generator(char *text, int state)
{
  static int list_index, len;
  char *name;

  if (!state) {
    list_index = 0;
    len = strlen(text);
  }
  
  while (list_index < dfa->term_num) {
    name = termname[list_index++];
    if (strncmp(name, text, len) == 0) {
      return(strdup(name));
    }
  }
  return((char *)NULL);
}
char *
dfaword_generator(char *text, int state)
{
  static int list_index, len;
  char *name;

  if (!state) {
    list_index = 0;
    len = strlen(text);
  }
  
  while (list_index < winfo->num) {
    name = winfo->woutput[list_index++];
    if (strncmp(name, text, len) == 0) {
      return(strdup(name));
    }
  }
  return((char *)NULL);
}

#else  /* ~HAVE_READLINE */

/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
char *
rl_gets (char *prompt)
{
  char *p;
  /* allocate buffer if not yet */
  if (! line_read) {
    if ((line_read = malloc(MAXLINELEN)) == NULL) {
      fprintf(stderr, "memory exceeded\n");
      exit(1);
    }
  }
  /* Get a line from the user. */
  fprintf(stderr, "%s", prompt);
  if (fgets(line_read, MAXLINELEN, stdin) == NULL) { /* input error */
    return NULL;
  }
  /* strip last newline */
  p = line_read + strlen(line_read) - 1;
  while (p >= line_read && *p == '\n') {
    *p = '\0';
    p--;
  }
  if (*line_read == '\0') {	/* no input */
    return NULL;
  }
  return (line_read);
}

#endif /* HAVE_READLINE */
