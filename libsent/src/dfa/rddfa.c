/**
 * @file   rddfa.c
 * 
 * <JA>
 * @brief  DFA文法の読み込み
 * </JA>
 * 
 * <EN>
 * @brief  Read DFA grammar from a file
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Tue Feb 15 14:54:40 2005
 *
 * $Revision: 1.6 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/dfa.h>

static char buf[MAXLINELEN];	///< Local text buffer

/** 
 * Initialize and allocate DFA state information list in the grammar.
 * 
 * @param dinfo [i/o] DFA grammar
 */
void
dfa_state_init(DFA_INFO *dinfo)
{
  int i;
  dinfo->maxstatenum = DFA_STATESTEP;
  dinfo->st = (DFA_STATE *)mymalloc(sizeof(DFA_STATE) * dinfo->maxstatenum);
  for (i=0;i<dinfo->maxstatenum;i++) {
    dinfo->st[i].number = i;
    dinfo->st[i].status = 0;
    dinfo->st[i].arc = NULL;
  }
  dinfo->state_num = dinfo->arc_num = dinfo->term_num = 0;
  dinfo->sp_id = WORD_INVALID;
}

/** 
 * Expand the state information list to the required length.
 * 
 * @param dinfo [i/o] DFA grammar
 * @param needed [in] required new length
 */
void
dfa_state_expand(DFA_INFO *dinfo, int needed)
{
  int oldnum, i;
  oldnum = dinfo->maxstatenum;
  dinfo->maxstatenum += DFA_STATESTEP;
  if (dinfo->maxstatenum < needed) dinfo->maxstatenum = needed;
  dinfo->st = (DFA_STATE *)myrealloc(dinfo->st, sizeof(DFA_STATE) * dinfo->maxstatenum);
  for (i=oldnum;i<dinfo->maxstatenum;i++) {
    dinfo->st[i].number = i;
    dinfo->st[i].status = 0;
    dinfo->st[i].arc = NULL;
  }
}

/** 
 * Top loop function to read DFA grammar via file pointer (gzip enabled)
 * 
 * @param fp [in] file pointer that points to the DFA grammar data
 * @param dinfo [out] the read data will be stored in this DFA grammar structure
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
rddfa(FILE *fp, DFA_INFO *dinfo)
{
  int state_max, arc_num, terminal_max;

  /* initialize */
  dfa_state_init(dinfo);
  state_max = 0;
  arc_num = 0;
  terminal_max = 0;

  while (getl(buf, MAXLINELEN, fp) != NULL) {
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
 * Top loop function to read DFA grammar via file descriptor
 * 
 * @param fp [in] file pointer that points to the DFA grammar data
 * @param dinfo [out] the read data will be stored in this DFA grammar structure
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
rddfa_fp(FILE *fp, DFA_INFO *dinfo)
{
  int state_max, arc_num, terminal_max;

  /* initialize */
  dfa_state_init(dinfo);
  state_max = 0;
  arc_num = 0;
  terminal_max = 0;

  while(getl_fp(buf, MAXLINELEN, fp) != NULL) {
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
 * Parse the input line and set grammar information, one by line.
 * 
 * @param line [in] text buffer that holds a line of DFA file
 * @param dinfo [i/o] the read data will be appended to this DFA data
 * @param state_max [i/o] maximum number of state id appeared, will be updated
 * @param arc_num [i/o] number of read arcs, will be updated
 * @param terminal_max [i/o] maximum number of state id appended, will be updated
 * 
 * @return TRUE if the line was successfully parsed, FALSE if failed.
 */
boolean
rddfa_line(char *line, DFA_INFO *dinfo, int *state_max, int *arc_num, int *terminal_max)
{
  DFA_ARC *newarc;
  int state, terminal, next_state;
  unsigned int status;
  char *p;

  if (strmatch(buf, "DFAEND")) return(FALSE);
  /* format: state terminalID nextstate statuscode_of_state */
  if ((p = strtok(line, DELM)) == NULL) {
    jlog("Error: rddfa: failed to parse, corrupted or invalid data?\n");
    return FALSE;
  }
  state = atoi(p);
  if ((p = strtok(NULL, DELM)) == NULL) {
    jlog("Error: rddfa: failed to parse, corrupted or invalid data?\n");
    return FALSE;
  }
  terminal = atoi(p);
  if ((p = strtok(NULL, DELM)) == NULL) {
    jlog("Error: rddfa: failed to parse, corrupted or invalid data?\n");
    return FALSE;
  }
  next_state = atoi(p);
  if ((p = strtok(NULL, DELM)) == NULL) {
    jlog("Error: rddfa: failed to parse, corrupted or invalid data?\n");
    return FALSE;
  }
  sscanf(p, "%x", &status);

  if (state >= dinfo->maxstatenum) {      /* expand */
    dfa_state_expand(dinfo, state+1);
  }
  if (next_state >= dinfo->maxstatenum) { /* expand */
    dfa_state_expand(dinfo, next_state+1);
  }

  /* set state status (accept / initial) */
  if (status & ACCEPT_S) {
    dinfo->st[state].status |= ACCEPT_S;
  }
  /* the state #0 is an initial state */
  if (state == 0) {
    dinfo->st[state].status |= INITIAL_S;
  }
  
  /* skip line with negative terminalID/nextstate */
  if (terminal > 0 || next_state > 0) {
    /* add new arc to the state */
    newarc = (DFA_ARC *)mymalloc(sizeof(DFA_ARC));
    newarc->label    = terminal;
    newarc->to_state = next_state;
    newarc->next     = dinfo->st[state].arc;
    dinfo->st[state].arc = newarc;
    (*arc_num)++;
  }

  if (*state_max < state) *state_max = state;
  if (*terminal_max < terminal) *terminal_max = terminal;

  return(TRUE);
}

/* append dfa info to other */
/* soffset: state offset  coffset: category(terminal) offset */

/** 
 * Append the DFA state information to other
 * 
 * @param dst [i/o] DFA grammar
 * @param src [i/o] DFA grammar to be appended to @a dst
 * @param soffset [in] offset state number in @a dst where the new state should be stored
 * @param coffset [in] category id offset in @a dst where the new data should be stored
 */
void
dfa_append(DFA_INFO *dst, DFA_INFO *src, int soffset, int coffset)
{
  DFA_ARC *arc, *newarc;
  int s, state, terminal, next_state;
  unsigned int status;

  for (s = 0; s < src->state_num; s++) {
    state = s + soffset;
    status = src->st[s].status;
    if (state >= dst->maxstatenum) {      /* expand */
      dfa_state_expand(dst, state+1);
    }
    /* set state status (accept / initial) */
    if (status & ACCEPT_S) {
      dst->st[state].status |= ACCEPT_S;
    }
    /* the state #0 is an initial state */
    if (s == 0) {
      dst->st[state].status |= INITIAL_S;
    }
    for (arc = src->st[s].arc; arc; arc = arc->next) {
      terminal = arc->label + coffset;
      next_state = arc->to_state + soffset;

      if (next_state >= dst->maxstatenum) { /* expand */
	dfa_state_expand(dst, next_state+1);
      }
      /* add new arc to the state */
      newarc = (DFA_ARC *)mymalloc(sizeof(DFA_ARC));
      newarc->label    = terminal;
      newarc->to_state = next_state;
      newarc->next     = dst->st[state].arc;
      dst->st[state].arc = newarc;

      dst->arc_num++;
      if (dst->term_num < terminal + 1) dst->term_num = terminal + 1;
    }
    if (dst->state_num < state + 1) dst->state_num = state + 1;
  }
}
