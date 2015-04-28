/**
 * @file   dfa_minimize.c
 * 
 * @brief  Minimize DFA for Julian grammar.
 * 
 * @author Akinobu Lee
 * @date   Wed Oct  4 17:42:16 2006
 *
 * $Revision: 1.5 $
 * 
 */
/*
 * Copyright (c) 2006-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2006-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/dfa.h>

#undef DEBUG			///< Define this to enable debug output

static DFA_INFO *dfa;		///< Input DFA info

static char buf[MAXLINELEN];	///< Local text buffer to read in

/** 
 * Get one line, stripping carriage return and newline.
 * 
 * @param buf [in] text buffer
 * @param maxlen [in] maximum length of @a buf
 * @param fp [in] file pointer
 * 
 * @return pointer to the given buffer, or NULL when failed.
 * </EN>
 */
static char *
mygetl(char *buf, int maxlen, FILE *fp)
{
  int newline;

  while(fgets(buf, maxlen, fp) != NULL) {
    newline = strlen(buf)-1;    /* chop newline */
    if (buf[newline] == '\n') {
      buf[newline] = '\0';
      newline--;
    }
    if (newline >= 0 && buf[newline] == '\r') buf[newline] = '\0';
    if (buf[0] == '\0') continue; /* if blank line, read next */
    return buf;
  }
  return NULL;
}

/** 
 * Read in DFA file, line by line.  Actual parser is in libsent library.
 * 
 * @param fp [in] file pointer
 * @param dinfo [out] DFA info
 * 
 * @return TRUE if succeeded.
 */
static boolean
myrddfa(FILE *fp, DFA_INFO *dinfo)
{
  int state_max, arc_num, terminal_max;

  dfa_state_init(dinfo);
  state_max = 0;
  arc_num = 0;
  terminal_max = 0;
  while (mygetl(buf, MAXLINELEN, fp) != NULL) {
    if (rddfa_line(buf, dinfo, &state_max, &arc_num, &terminal_max) == FALSE) {
      break;
    }
  }
  dinfo->state_num = state_max + 1;
  dinfo->arc_num = arc_num;
  dinfo->term_num = terminal_max + 1;
  return(TRUE);
}

/** 
 * Output usage.
 *
 */
static void
usage()
{
  fprintf(stderr, "usage: dfa_minimize [dfafile] [-o outfile]\n");
}

/************************************************************************/
/** 
 * @brief  Perform minimization.
 *
 * The result will be output in DFA format, to the specified file pointer.
 * 
 * 
 * @param dfa [in] original DFA info
 * @param fpout [in] output file pointer
 */
void
minimize(DFA_INFO *dfa, FILE *fpout)
{
  int *group;			/* group ID assigned for states */
  int gnum;			/* number of groups currently assigned */
  int *gmark;			/* mark states of different transition pattern in a group */
  int **l;			/* transition pattern for states */
  int **pt;			/* uniq transition pattern within a group */
  int i, n, s, g;
  DFA_ARC *ac;
  boolean modified;
  int starting;
  int *glist;
  int groupnum, arcnum;
  int *is_start;
  
  /* allocate work area */
  group = (int *)malloc(sizeof(int) * dfa->state_num);
  gmark = (int *)malloc(sizeof(int) * dfa->state_num);
  l = (int **)malloc(sizeof(int *) * dfa->state_num);
  for(i=0;i<dfa->state_num;i++) {
    l[i] = (int *)malloc(sizeof(int) * dfa->term_num);
  }
  pt = (int **)malloc(sizeof(int *) * dfa->state_num);
  
  /* set initial group */
  /* acceptable = 0, others = 1 */
  for(i=0;i<dfa->state_num;i++) {
    if ((dfa->st[i].status & ACCEPT_S) != 0) {
      group[i] = 0;
    } else {
      group[i] = 1;
    }
  }
  gnum = 2;
  
  /* loop until nothing has been modified in the last loop */
  do {
    modified = FALSE;
#ifdef DEBUG
    printf("-----\n");
#endif
    
    /* list transition pattern of each state */
    for(s=0;s<dfa->state_num;s++) {
      for(i=0;i<dfa->term_num;i++) l[s][i] = -1;
      for(ac=dfa->st[s].arc;ac;ac=ac->next) {
	l[s][ac->label] = group[ac->to_state];
      }
    }
    
#ifdef DEBUG
    for(s=0;s<dfa->state_num;s++) {
      printf("state %d: group=%d\n  ", s, group[s]);
      for(i=0;i<dfa->term_num;i++) {
	if (l[s][i] == -1) {
	  printf(" *");
	} else {
	  printf(" %d", l[s][i]);
	}
      }
      printf("\n");
    }
#endif

    /* check for each group if they has different trans. pattern in it */
    for(g=0;g<gnum;g++) {
      n = 0;
      for(s=0;s<dfa->state_num;s++) {
	if (group[s] != g) continue;
	/* compare the trans. pattern with the already detected ones */
	for(i=0;i<n;i++) {
	  if (memcmp(l[s], pt[i], sizeof(int) * dfa->term_num) == 0) break;
	}
	if (i < n) {
	  /* same pattern has already detected */
	  /* mark the state with the pattern ID */
	  gmark[s] = i;
	} else {
	  /* same pattern was not found */
	  /* it's a new pattern, so store it to the pattern list and
	     annotate a new pattern ID */
	  pt[n] = l[s];
	  gmark[s] = n;
	  n++;
	}
      }
      /* "n" now holds the number of trans. patterns */
      if (n > 1) {
	/* several diffrent pattern has been found in a group */
	/* divide this group into a new subgroup, by assigning
	   new group ID for each different patterns */
	for(s=0;s<dfa->state_num;s++) {
	  if (group[s] != g) continue;
	  group[s] = gnum + gmark[s];
	}
	/* now the number of groups has increased */
	gnum += n;
	modified = TRUE;
      }
    }
  } while (modified);		/* loop until no group has been divided */
  
  /* rebuild the final transition pattern */
  for(s=0;s<dfa->state_num;s++) {
    for(i=0;i<dfa->term_num;i++) l[s][i] = -1;
    for(ac=dfa->st[s].arc;ac;ac=ac->next) {
      l[s][ac->label] = group[ac->to_state];
    }
  }
  
  /* relocate group ID for output */
  /* glist[gnum] -> new ID */
  glist = (int *)malloc(sizeof(int) * gnum);
  for(g=0;g<gnum;g++) glist[g] = -1;
  n = 0;
  for(s=0;s<dfa->state_num;s++) {
    if (glist[group[s]] == -1) {
      glist[group[s]] = n;
      n++;
    }
  }
  groupnum = n;
  
  /* Julian DFA assumes the state 0 to be the starting state.
     So we should find starting states
     and swap the ID with the one with group ID=0. */
  is_start = (int *)malloc(sizeof(int) * groupnum);
  for(g=0;g<groupnum;g++) is_start[g] = 0;
  for(s=0;s<dfa->state_num;s++) {
    if (dfa->st[s].status & INITIAL_S) {
      is_start[glist[group[s]]] = 1;
    }
  }
  starting = -1;
  for(g=0;g<groupnum;g++) {
    if (is_start[g] == 1) {
      if (starting == -1) {
	starting = g;
      } else {
	printf("Error: more than one initial node??\n");
	for(g=0;g<groupnum;g++) {
	  if (is_start[g] == 1) printf(" %d", g);
	}
	printf("\n");
	printf("Warning: resulting DFA may not be used in Julian by multiple initial nodes!!\n");
	starting = 0;
	break;
      }
    }
  }
#ifdef DEBUG
  printf("starting=%d\n", starting);
#endif
  free(is_start);
  for(g=0;g<gnum;g++) {
    if (glist[g] == 0) {
      glist[g] = starting;
    } else if (glist[g] == starting) {
      glist[g] = 0;
    }
  }

#ifdef DEBUG
  printf("renumber:\n");
  for(g=0;g<gnum;g++) {
    printf("  %d %d\n", g, glist[g]);
  }
#endif

  /* output the result in DFA form */
  {
    int *is_accept;

    is_accept = (int *)malloc(sizeof(int) * groupnum);
    for(g=0;g<groupnum;g++) is_accept[g] = 0;
    for(s=0;s<dfa->state_num;s++) {
      if (dfa->st[s].status & ACCEPT_S) {
	is_accept[glist[group[s]]] = 1;
      }
    }

    arcnum = 0;
    for(g=0;g<groupnum;g++) {
      for(s=0;s<dfa->state_num;s++) {
	if (glist[group[s]] != g) continue;
	for(i=0;i<dfa->term_num;i++) {
	  if (l[s][i] == -1) continue;
	  if (is_accept[g] == 1) {
	    fprintf(fpout, "%d %d %d 1 0\n", g, i, glist[l[s][i]]);
	    is_accept[g] = 0;
	  } else {
	    fprintf(fpout, "%d %d %d 0 0\n", g, i, glist[l[s][i]]);
	  }
	  arcnum++;
	}
	break;
      }
      if (is_accept[g] == 1) {
	fprintf(fpout, "%d -1 -1 1 0\n", g);
      }
    }

    free(is_accept);
  }
  
  fprintf(stderr, "-> minimized: %d nodes, %d arcs\n", groupnum, arcnum);

  /* free work area */
  free(glist);
  free(pt);
  for(i=0;i<dfa->state_num;i++) free(l[i]);
  free(l);
  free(gmark);
  free(group);
}

/************************************************************************/
/** 
 * Main function.
 * 
 * @param argc [in] number of command argument
 * @param argv [in] array of command arguments
 * 
 * @return -1 on failure, 0 on success
 */
int
main(int argc, char *argv[])
{
  FILE *fp, *fpout;
  char *infile, *outfile;
  int i;
  
  /* option parsing */
  infile = NULL; outfile = NULL;
  for(i=1;i<argc;i++) {
    if (argv[i][0] == '-') {
      switch(argv[i][1]) {
      case 'h':
	usage(); return -1;
	break;
      case 'o':
	if (++i >= argc) {
	  usage(); return -1;
	}
	outfile = argv[i];
	break;
      default:
	fprintf(stderr, "invalid option: %s\n", argv[i]);
	usage(); return -1;
      }
    } else {
      infile = argv[i];
    }
  }
  
  /* open files */
  if (infile != NULL) {
    if ((fp = fopen(infile, "r")) == NULL) {
      fprintf(stderr, "Error: cannot open \"%s\"\n", infile);
      return -1;
    }
  } else {
    fp = stdin;
  }
  if (outfile != NULL) {
    if ((fpout = fopen(outfile, "w")) == NULL) {
      fprintf(stderr, "Error: cannot open \"%s\" for writing\n", outfile);
      return -1;
    }
  } else {
    fpout = stdout;
  }

  /* read in a DFA file */
  dfa = dfa_info_new();
  if (!myrddfa(fp, dfa)) {
    fprintf(stderr, "Failed to read DFA from ");
    if (infile) printf("\"%s\"\n", infile);
    else printf("stdin\n");
  }
  if (fp != stdin) fclose(fp);

  fprintf(stderr, "%d categories, %d nodes, %d arcs\n", dfa->term_num, dfa->state_num, dfa->arc_num);

  /* execute minimization */
  minimize(dfa, fpout);

  if (fpout != stdout) {
    fclose(fpout);
  }

  return 0;
}
