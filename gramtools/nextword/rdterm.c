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
#include "common.h"

void
init_term(char *filename, char **termname)
{
  FILE *fd;
  int n;
  static char buf[512];
  char *p;
  
  fprintf(stderr, "Reading in term file (optional)...");
  
  if ((fd = fopen_readfile(filename)) == NULL) {
    termname[0] = NULL;
    fprintf(stderr, "not found\n");
    return;
  }

  while (getl(buf, sizeof(buf), fd) != NULL) {
    if ((p = strtok(buf, DELM)) == NULL) {
      fprintf(stderr, "Error: term file failed to parse, corrupted or invalid data?\n");
      return;
    }
    n = atoi(p);
    if ((p = strtok(NULL, DELM)) == NULL) {
      fprintf(stderr, "Error: term file failed to parse, corrupted or invalid data?\n");
      return;
    }
    termname[n] = strdup(p);
  }
  if (fclose_readfile(fd) == -1) {
    fprintf(stderr, "close error\n");
    exit(1);
  }

  fprintf(stderr, "done\n");
}
