/**
 * @file   voca_lookup.c
 * 
 * <JA>
 * @brief  単語辞書上の単語の検索
 *
 * 単語を，「言語エントリ名」あるいは「言語エントリ名[出力文字列]」
 * ，あるいは「#単語番号」から検索します．
 * </JA>
 * 
 * <EN>
 * @brief  Look up a word on dictionary by string
 *
 * String can be "langentry" or "langentry[outputstring]", or
 * "#number".
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Fri Feb 18 21:24:01 2005
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
#include <sent/vocabulary.h>

/** 
 * Look up a word on dictionary by string.
 * 
 * @param keyword [in] keyword to search
 * @param winfo [in] word dictionary
 * 
 * @return the word id if found, or WORD_INVALID if not found.
 */
WORD_ID
voca_lookup_wid(char *keyword, WORD_INFO *winfo)
{
  WORD_ID i, found;
  int plen,totallen;
  boolean numflag = TRUE;
  int wid;
  char *c;

  if (keyword == NULL) return WORD_INVALID;
  
  if (keyword[0] == '#') {
    
    for(i=1;i<strlen(keyword);i++) {
      if (keyword[i] < '0' || keyword[i] > '9') {
	numflag = FALSE;
	break;
      }
    }
    if (numflag) {
      wid = atoi(&(keyword[1]));
      if (wid < 0 || wid >= winfo->num) {
	return(WORD_INVALID);
      } else {
	return(wid);
      }
    } else {
      return(WORD_INVALID);
    }
  }
      
  found = WORD_INVALID;
  totallen = strlen(keyword);
  if ((c = strchr(keyword, '[')) != NULL) {
    plen = c - keyword;
    for (i=0;i<winfo->num;i++) {
      if (strnmatch(keyword,winfo->wname[i], plen)
	  && strnmatch(c+1, winfo->woutput[i], totallen-plen-2)) {
	if (found == WORD_INVALID) {
	  found = i;
	} else {
	  jlog("Warning: voca_lookup: several \"%s\" found in dictionary, use the first one..\n");
	  break;
	}
      }
    }
  } else {
    for (i=0;i<winfo->num;i++) {
      if (strmatch(keyword,winfo->wname[i])) {
	if (found == WORD_INVALID) {
	  found = i;
	} else {
	  jlog("Warning: voca_lookup: several \"%s\" found in dictionary, use the first one..\n");
	  break;
	}
      }
    }
  }
  return found;
}

/* convert space-separated words string -> array of wid */
/* return malloced array */
#define WSSTEP 10 ///< Allocation step 

/** 
 * Convert string of space-separated word strings to array of word ids.
 * 
 * @param winfo [in] word dictionary
 * @param s [in] string of space-separated word strings
 * @param len_return [out] number of found words
 * 
 * @return pointer to a newly allocated word list.
 */
WORD_ID *
new_str2wordseq(WORD_INFO *winfo, char *s, int *len_return)
{
  char *p;
  int num;
  int maxnum;
  WORD_ID *wseq;

  maxnum = WSSTEP;
  wseq = (WORD_ID *)mymalloc(sizeof(WORD_ID)*maxnum);
  num = 0;
  for (p = strtok(s, " "); p != NULL; p = strtok(NULL, " ")) {
    if (num >= maxnum) {
      maxnum += WSSTEP;
      wseq = (WORD_ID *)myrealloc(wseq, sizeof(WORD_ID) * maxnum);
    }
    if ((wseq[num] = voca_lookup_wid(p, winfo)) == WORD_INVALID) {
      /* not found */
      jlog("Error: voca_lookup: word \"%s\" not found in dict\n", p);
      free(wseq);
      return NULL;
    }
    num++;
  }

  *len_return = num;
  return(wseq);
}
