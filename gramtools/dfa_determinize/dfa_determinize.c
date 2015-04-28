/**
 * @file   dfa_determinize.c
 * 
 * @brief  Determinize DFA for Julian grammar.
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
  fprintf(stderr, "usage: dfa_determinize [dfafile] [-o outfile]\n");
}

/************************************************************************/
/**
 * Structure to hold state set
 * 
 */
typedef struct __stateq__ {
  char *s;			///< State index (if 1, the state is included)
  int len;			///< Buffer length of above.
  int checked;			///< flag to check if the outgoing arcs of this set is already examined
  void *ac;			///< Root pointer to the list of outgoing arcs.
  int start;			///< if 1, this should be a begin node 
  int end;			///< if 1, this should eb an accept node
  int id;			///< assigned ID
  struct __stateq__ *next;	///< Pointer to the next state set.
} STATEQ;

/**
 * Structure to hold outgoing arcs from / to the stateset
 * 
 */
typedef struct __arc__ {
  int label;			///< Input label ID
  STATEQ *to;			///< Destination state set
  struct __arc__ *next;		///< Pointer to the next arc
} STATEQ_ARC;

/** 
 * Output information of a state set to stdout, for debug
 * 
 * @param sq [in] state set
 */
void
sput(STATEQ *sq)
{
  int i;
  STATEQ_ARC *ac;

  for(i=0;i<sq->len;i++) {
    if (sq->s[i] == 1) printf("-%d", i);
  }
  printf("\n");
  printf("checked: %d\n", sq->checked);
  printf("to:\n");
  for(ac=sq->ac;ac;ac=ac->next) {
    printf("\t(%d) ", ac->label);
    for(i=0;i<ac->to->len;i++) {
      if (ac->to->s[i] == 1) printf("-%d", i);
    }
    printf("\n");
  }
}

/** 
 * Create a new state set.
 * 
 * @param num [in] number of possible states
 * 
 * @return pointer to the newly assigned state set.
 */
STATEQ *
snew(int num)
{
  STATEQ *new;
  int i;

  new = (STATEQ *)malloc(sizeof(STATEQ));
  new->s = (char *)malloc(sizeof(char)*num);
  new->len = num;
  new->ac = NULL;
  new->next = NULL;
  for(i=0;i<num;i++) new->s[i] = 0;
  new->checked = 0;
  new->start = 0;
  new->end = 0;
  return new;
}

/** 
 * Free the state set.
 * 
 * @param sq 
 */void
sfree(STATEQ *sq)
{
  STATEQ_ARC *sac, *atmp;
  sac=sq->ac;
  while(sac) {
    atmp = sac->next;
    free(sac);
    sac = atmp;
  }
  free(sq->s);
  free(sq);
}

static STATEQ *root = NULL;	///< root node of current list of state set

/** 
 * @brief  Perform determinization.
 *
 * The result will be output in DFA format, to the specified file pointer.
 * 
 * 
 * @param dfa [in] original DFA info
 * @param fpout [in] output file pointer
 */
boolean
determinize(DFA_INFO *dfa, FILE *fpout)
{
  STATEQ *src, *stmp, *stest;
  STATEQ_ARC *sac;
  int i, t, tnum;
  DFA_ARC *ac;
  int *tlist;
  int modified;
  int arcnum, nodenum;
  STATEQ **slist;

  /* allocate work area */
  tlist = (int *)malloc(sizeof(int) * dfa->state_num);
  
  /* set initial node (a state set with single initial state) */
  src = NULL;
  for(i=0;i<dfa->state_num;i++) {
    if (dfa->st[i].status & INITIAL_S) {
      if (src == NULL) {
	src = snew(dfa->state_num);
	src->s[i] = 1;
	src->start = 1;
	root = src;
      } else {
	printf("Error: more than one initial node??\n");
	return FALSE;
      }
    }
  }

  /* loop until no more state set is generated */
  do {
#ifdef DEBUG
    printf("---\n");
#endif
    modified = 0;
    for(src=root;src;src=src->next) {
      if (src->checked == 1) continue;
#ifdef DEBUG
      printf("===checking===\n");
      sput(src);
      printf("==============\n");
#endif

      for(t=0;t<dfa->term_num;t++) {
	
	/* examining an input label "t" on state set "src" */

	/* get list of outgoing states from this state set by the input
	   label "t", and set to tlist[0..tnum-1] */
	tnum = 0;
	for(i=0;i<src->len;i++) {
	  if (src->s[i] == 1) {
	    for(ac=dfa->st[i].arc;ac;ac=ac->next) {
	      if (ac->label == t) {
		tlist[tnum] = ac->to_state;
		tnum++;
	      }
	    }
	  }
	}

	/* if no output with this label, skip it */
	if (tnum == 0) continue;

	/* build the destination state set */
	stest = snew(dfa->state_num);
	for(i=0;i<tnum;i++) {
	  stest->s[tlist[i]] = 1;
	}

#ifdef DEBUG
	printf("\tinput (%d) -> states: ", t);
	for(i=0;i<stest->len;i++) {
	  if (stest->s[i] == 1) printf("-%d", i);
	}
	printf("\n");
#endif

	/* find if the destination state set is already generated */
	for(stmp=root;stmp;stmp=stmp->next) {
	  if (memcmp(stmp->s, stest->s, sizeof(char) * stest->len) ==0) {
	    break;
	  }
	}
	if (stmp == NULL) {
	  /* not yet generated, register it as new */
#ifdef DEBUG
	  printf("\tNEW\n");
#endif
	  stest->next = root;
	  root = stest;
	  stmp = stest;
	} else {
	  /* already generated, just point to it */
#ifdef DEBUG
	  printf("\tFOUND\n");
#endif
	  sfree(stest);
	}

	/* add arc to the destination state set to "src" */
	sac = (STATEQ_ARC *)malloc(sizeof(STATEQ_ARC));
	sac->label = t;
	sac->to = stmp;
	sac->next = src->ac;
	src->ac = sac;
      }
      src->checked = 1;
      modified = 1;
#ifdef DEBUG
      printf("====result====\n");
      sput(src);
      printf("==============\n");
#endif
    }
  } while (modified == 1);

  /* annotate ID and count number of nodes */
  /* Also, force the state number of initial nodes to 0 by Julian requirement */
  nodenum = 1;
  for(src=root;src;src=src->next) {
    if (src->start == 1) {
      src->id = 0;
    } else {
      src->id = nodenum++;
    }
    for(i=0;i<src->len;i++) {
      if (src->s[i] == 1) {
	if (dfa->st[i].status & ACCEPT_S) {
	  src->end = 1;
	}
      }
    }
  }

  /* output the result in DFA form */
  slist = (STATEQ **)malloc(sizeof(STATEQ *) * nodenum);
  for(src=root;src;src=src->next) slist[src->id] = src;
  arcnum = 0;
  
  for(i=0;i<nodenum;i++) {
    src = slist[i];
    t = 0;
    if (src->end == 1) t = 1;
    for(sac=src->ac;sac;sac=sac->next) {
      if (t == 1) {
	fprintf(fpout, "%d %d %d 1 0\n", src->id, sac->label, sac->to->id);
	t = 0;
      } else {
	fprintf(fpout, "%d %d %d 0 0\n", src->id, sac->label, sac->to->id);
      }
      arcnum++;
    }
    if (t == 1) {
      fprintf(fpout, "%d -1 -1 1 0\n", src->id);
    }
  }
  free(slist);

  /* output status to stderr */
  fprintf(stderr, "-> determinized: %d nodes, %d arcs\n", nodenum, arcnum);

  /* free work area */
  src = root;
  while(src) {
    stmp = src->next;
    sfree(src);
    src = stmp;
  }      
  free(tlist);

  return TRUE;
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

  /* do determinization */
  if (determinize(dfa, fpout) == FALSE) {
    fprintf(stderr, "Error in determinization\n");
    return -1;
  }

  if (fpout != stdout) {
    fclose(fpout);
  }

  return 0;
}
