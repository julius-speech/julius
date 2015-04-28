/**
 * @file   multi-gram.h
 * 
 * <JA>
 * @brief  複数の文法を同時に扱うための定義. 
 * </JA>
 * 
 * <EN>
 * @brief  Definitions for managing multiple grammars.
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Fri Jul  8 14:47:05 2005
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

#ifndef __J_MULTI_GRAM_H__
#define __J_MULTI_GRAM_H__

/// Maximum length of grammar name
#define MAXGRAMNAMELEN 512

/// Grammar holder
typedef struct __multi_gram__ {
  char name[MAXGRAMNAMELEN];	///< Unique name given by user
  unsigned short id;		///< Unique ID 
  DFA_INFO *dfa;		///< DFA describing syntax of this grammar
  WORD_INFO *winfo;		///< Dictionary of this grammar
  int hook;			///< Work area to store command hook
  boolean newbie;		///< TRUE if just read and not yet configured
  boolean active;		///< TRUE if active for recognition
  ///< below vars holds the location of this grammar within the global grammar */
  int state_begin;		///< Location of DFA states in the global grammar
  int cate_begin;		///< Location of category entries in the global grammar
  int word_begin;		///< Location of words in the dictionary of global grammar
  struct __multi_gram__ *next;	///< Link to the next grammar entry
} MULTIGRAM;

/// List of grammars to be read at startup
typedef struct __gram_list__ {
  char *dfafile;		///< DFA file path
  char *dictfile;		///< Dict file path
  struct __gram_list__ *next;   ///< Link to next entry
} GRAMLIST;

/* for command hook */
#define MULTIGRAM_DEFAULT    0	///< Grammar hook value of no operation
#define MULTIGRAM_DELETE     1  ///< Grammar hook bit specifying that this grammar is to be deleted
#define MULTIGRAM_ACTIVATE   2  ///< Grammar hook bit specifying that this grammar is to be activated
#define MULTIGRAM_DEACTIVATE 4  ///< Grammar hook bit specifying that this grammar is to be deactivated
#define MULTIGRAM_MODIFIED   8 /// < Grammar hook bit indicating modification  and needs rebuilding the whole lexicon


#endif /* __J_MULTI_GRAM_H__ */
