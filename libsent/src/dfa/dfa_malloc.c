/**
 * @file   dfa_malloc.c
 * 
 * <JA>
 * @brief  文法構造体のメモリ割り付けと開放
 * </JA>
 * 
 * <EN>
 * @brief  Memory allocation of grammar information
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Tue Feb 15 14:16:03 2005
 *
 * $Revision: 1.5 $
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

/** 
 * Allocate a new grammar information data structure and initialize it.
 * 
 * @return pointer to the newly allocated DFA_INFO.
 */
DFA_INFO *
dfa_info_new()
{
  DFA_INFO *new;

  new = (DFA_INFO *)mymalloc(sizeof(DFA_INFO));
  init_dfa_cp(new);
  new->term.tw = NULL;
  new->term.term_num = new->term_num = 0;
  new->maxstatenum = 0;
  new->is_sp = NULL;
  new->sp_id = WORD_INVALID;

  return new;
}

/** 
 * Free all informations in the DFA_INFO.
 * 
 * @param dfa [i/o] grammar information data to be freed.
 */
void
dfa_info_free(DFA_INFO *dfa)
{
  DFA_ARC *arc, *tmparc;
  int i;

  /* free category pair info */
  free_dfa_cp(dfa);
  
  /* free terminal info */
  if (dfa->term_num != 0) {
    free_terminfo(&(dfa->term));
  }
  /* free arcs */
  if (dfa->maxstatenum > 0) {
    for(i=0;i<dfa->state_num;i++) {
      arc=dfa->st[i].arc;
      while(arc != NULL) {
	tmparc = arc->next;
	free(arc);
	arc = tmparc;
      }
    }
    /* free states */
    free(dfa->st);
  }
  if (dfa->is_sp != NULL) free(dfa->is_sp);
  /* free whole */
  free(dfa);
}
