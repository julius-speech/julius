/**
 * @file   voca_load_wordlist.c
 * 
 * <JA>
 * @brief  孤立単語認識モード用単語リストの読み込み
 *
 * </JA>
 * 
 * <EN>
 * @brief  Read word list from a file for isolated word recognition mode
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Sun Jul 22 13:29:32 2007
 *
 * $Revision: 1.12 $
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
#include <sent/htk_hmm.h>

/* 
 * dictinary format:
 * 
 * 1 words per line.
 * 
 * fields: OutputString phone1 phone2 ....
 * 
 *     OutputString
 *		   String to output when the word is recognized.
 *
 *     phone1 phone2 ....
 *		   sequence of logical HMM name (normally phoneme)
 *                 to express the pronunciation
 */

#define PHONEMELEN_STEP  30	///< Memory allocation step for phoneme sequence
static char buf[MAXLINELEN];	///< Local work area for input text processing
static char bufbak[MAXLINELEN];	///< Local work area for debug message

/** 
 * Add a triphone name to the missing error list in WORD_INFO.
 * 
 * @param winfo [i/o] word dictionary to add the error phone to error list
 * @param name [in] phone name to be added
 */
static void
add_to_error(WORD_INFO *winfo, char *name)
{
  char *buf;
  char *match;

  buf = (char *)mymalloc(strlen(name) + 1);
  strcpy(buf, name);
  if (winfo->errph_root == NULL) {
    winfo->errph_root = aptree_make_root_node(buf, &(winfo->mroot));
  } else {
    match = aptree_search_data(buf, winfo->errph_root);
    if (match == NULL || !strmatch(match, buf)) {
      aptree_add_entry(buf, buf, match, &(winfo->errph_root), &(winfo->mroot));
    }
  }
}

/** 
 * Traverse callback function to output a error phone.
 * 
 * @param x [in] error phone string of the node
 */
static void
callback_list_error(void *x)
{
  char *name;
  name = x;
  jlog("Error: voca_load_wordlist: %s\n", name);
}
/** 
 * Output all error phones appeared while readin a word dictionary.
 * 
 * @param winfo [in] word dictionary data
 */
static void
list_error(WORD_INFO *winfo)
{
  jlog("Error: voca_load_wordlist: begin missing phones\n");
  aptree_traverse_and_do(winfo->errph_root, callback_list_error);
  jlog("Error: voca_load_wordlist: end missing phones\n");
}

/** 
 * Load a line from buffer and set parameters to the dictionary.
 * 
 * @param buf [in] input buffer containing a word entry
 * @param winfo [i/o] word dictionary to append the entry
 * @param hmminfo [in] phoneme HMM definition
 * @param headphone [in] word head silence model name
 * @param tailphone [in] word tail silence model name
 * @param contextphone [in] silence context name to be used at head and tail
 * 
 * @return TRUE when successfully read, or FALSE on encountered end of
 * dictionary.  When an error occurs, this function will set winfo->ok_flag
 * to FALSE.
 * 
 */
boolean
voca_load_word_line(char *buf, WORD_INFO *winfo, HTK_HMM_INFO *hmminfo, char *headphone, char *tailphone, char *contextphone)
{
  WORD_ID vnum;

  winfo->linenum++;
  vnum = winfo->num;
  if (vnum >= winfo->maxnum) {
    if (winfo_expand(winfo) == FALSE) return FALSE;
  }
  if (voca_load_wordlist_line(buf, &vnum, winfo->linenum, winfo, hmminfo, winfo->do_conv, &(winfo->ok_flag), headphone, tailphone, contextphone) == FALSE) {
    return FALSE;
  }
  winfo->num = vnum;
  return TRUE;
}
/** 
 * Top function to read word list via text
 * 
 * @param fp [in] file pointer
 * @param winfo [out] pointer to word dictionary to store the read data.
 * @param hmminfo [in] HTK %HMM definition data.  if NULL, phonemes are ignored.
 * @param headphone [in] word head silence model name
 * @param tailphone [in] word tail silence model name
 * @param contextphone [in] silence context name to be used at head and tail
 * 
 * @return TRUE on success, FALSE on any error word.
 */
boolean
voca_load_wordlist(FILE *fp, WORD_INFO *winfo, HTK_HMM_INFO *hmminfo, char *headphone, char *tailphone, char *contextphone)
{
  boolean ret;

  voca_load_start(winfo, hmminfo, FALSE);
  while (getl(buf, sizeof(buf), fp) != NULL) {
    if (voca_load_word_line(buf, winfo, hmminfo, headphone, tailphone, contextphone) == FALSE) break;
  }
  ret = voca_load_end(winfo);

  return(ret);
}


/** 
 * Top function to read word list via file pointer
 * 
 * @param fp [in] file pointer
 * @param winfo [out] pointer to word dictionary to store the read data.
 * @param hmminfo [in] HTK %HMM definition data.  if NULL, phonemes are ignored.
 * @param headphone [in] word head silence model name
 * @param tailphone [in] word tail silence model name
 * @param contextphone [in] silence context name to be used at head and tail
 * 
 * @return TRUE on success, FALSE on any error word.
 */
boolean
voca_load_wordlist_fp(FILE *fp, WORD_INFO *winfo, HTK_HMM_INFO *hmminfo, char *headphone, char *tailphone, char *contextphone)
{
  boolean ret;

  voca_load_start(winfo, hmminfo, FALSE);
  while (getl_fp(buf, sizeof(buf), fp) != NULL) {
    if (voca_load_word_line(buf, winfo, hmminfo, headphone, tailphone, contextphone) == FALSE) break;
  }
  ret = voca_load_end(winfo);

  return(ret);
}

/** 
 * Sub function to Add a dictionary entry line to the word dictionary.
 * 
 * @param buf [i/o] buffer to hold the input string, will be modified in this function
 * @param vnum_p [in] current number of words in @a winfo
 * @param linenum [in] current line number of the input
 * @param winfo [out] pointer to word dictionary to append the data.
 * @param hmminfo [in] HTK %HMM definition data.  if NULL, phonemes are ignored.
 * @param do_conv [in] TRUE if performing triphone conversion
 * @param ok_flag [out] will be set to FALSE if an error occured for this input.
 * @param headphone [in] word head silence model name
 * @param tailphone [in] word tail silence model name
 * @param contextphone [in] silence context name to be used at head and tail
 * 
 * @return FALSE if buf == "DICEND", else TRUE will be returned.
 */
boolean
voca_load_wordlist_line(char *buf, WORD_ID *vnum_p, int linenum, WORD_INFO *winfo, HTK_HMM_INFO *hmminfo, boolean do_conv, boolean *ok_flag, char *headphone, char *tailphone, char *contextphone)
{
  char *ptmp, *lp = NULL, *p;
  static char cbuf[MAX_HMMNAME_LEN];
  static HMM_Logical **tmpwseq = NULL;
  static int tmpmaxlen;
  int i, len;
  HMM_Logical *tmplg;
  boolean pok, first;
  int vnum;

  vnum = *vnum_p;

  if (strmatch(buf, "DICEND")) return FALSE;

  /* allocate temporal work area for the first call */
  if (tmpwseq == NULL) {
    tmpmaxlen = PHONEMELEN_STEP;
    tmpwseq = (HMM_Logical **)mymalloc(sizeof(HMM_Logical *) * tmpmaxlen);
  }

  /* backup whole line for debug output */
  strcpy(bufbak, buf);
  
  /* Output string */
  if ((ptmp = mystrtok_quote(buf, " \t\n")) == NULL) {
    jlog("Error: voca_load_wordlist: line %d: corrupted data:\n> %s\n", linenum, bufbak);
    winfo->errnum++;
    *ok_flag = FALSE;
    return TRUE;
  }
  winfo->wname[vnum] = strcpy((char *)mybmalloc2(strlen(ptmp)+1, &(winfo->mroot)), ptmp);

  /* reset transparent flag */
  winfo->is_transparent[vnum] = FALSE;

  /* just move pointer to next token */
  if ((ptmp = mystrtok_movetonext(NULL, " \t\n")) == NULL) {
    jlog("Error: voca_load_wordlist: line %d: corrupted data:\n> %s\n", linenum, bufbak);
    winfo->errnum++;
    *ok_flag = FALSE;
    return TRUE;
  }
#ifdef CLASS_NGRAM
  winfo->cprob[vnum] = 0.0;	/* prob = 1.0, logprob = 0.0 */
#endif
  
  if (ptmp[0] == '@') {		/* class N-gram prob */
#ifdef CLASS_NGRAM
    /* word probability within the class (for class N-gram) */
    /* format: classname @classprob wordname [output] phoneseq */
    /* classname equals to wname, and wordname will be omitted */
    /* format: @%f (log scale) */
    /* if "@" not found or "@0", it means class == word */
    if ((ptmp = mystrtok(NULL, " \t\n")) == NULL) {
      jlog("Error: voca_load_wordlist: line %d: corrupted data:\n> %s\n", linenum, bufbak);
      winfo->errnum++;
      *ok_flag = FALSE;
      return TRUE;
    }
    if (ptmp[1] == '\0') {	/* space between '@' and figures */
      jlog("Error: voca_load_wordlist: line %d: value after '@' missing, maybe wrong space?\n> %s\n", linenum, bufbak);
      winfo->errnum++;
      *ok_flag = FALSE;
      return TRUE;
    }
    winfo->cprob[vnum] = atof(&(ptmp[1]));
    if (winfo->cprob[vnum] != 0.0) winfo->cwnum++;
    /* read next word entry (just skip them) */
    if ((ptmp = mystrtok(NULL, " \t\n")) == NULL) {
      jlog("Error: voca_load_wordlist: line %d: corrupted data:\n> %s\n", linenum,bufbak);
      winfo->errnum++;
      *ok_flag = FALSE;
      return TRUE;
    }
    /* move to the next word entry */
    if ((ptmp = mystrtok_movetonext(NULL, " \t\n")) == NULL) {
      jlog("Error: voca_load_wordlist: line %d: corrupted data:\n> %s\n", linenum, bufbak);
      winfo->errnum++;
      *ok_flag = FALSE;
      return TRUE;
    }
#else  /* ~CLASS_NGRAM */
    jlog("Error: voca_load_wordlist: line %d: cannot handle in-class word probability\n> %s\n", linenum, ptmp, bufbak);
    winfo->errnum++;
    *ok_flag = FALSE;
    return TRUE;
#endif /* CLASS_NGRAM */
  }

  /* OutputString */
  switch(ptmp[0]) {
  case '[':			/* ignore transparency */
    ptmp = mystrtok_quotation(NULL, " \t\n", '[', ']', 0);
    break;
  case '{':			/* ignore transparency */
    ptmp = mystrtok_quotation(NULL, " \t\n", '{', '}', 0);
    break;
  default:
    /* ALLOW no entry for output */
    /* same as wname is used */
    ptmp = winfo->wname[vnum];
  }
  if (ptmp == NULL) {
    jlog("Error: voca_load_wordlist: line %d: corrupted data:\n> %s\n", linenum, bufbak);
    winfo->errnum++;
    *ok_flag = FALSE;
    return TRUE;
  }
  winfo->woutput[vnum] = strcpy((char *)mybmalloc2(strlen(ptmp)+1, &(winfo->mroot)), ptmp);

#ifdef USE_MBR
  /* just move pointer to next token */
  if ((ptmp = mystrtok_movetonext(NULL, " \t\n")) == NULL) {
    jlog("Error: voca_load_wordlist: line %d: corrupted data:\n> %s\n", linenum, bufbak);
    winfo->errnum++;
    *ok_flag = FALSE;
    return TRUE;
  }

  if (ptmp[0] == ':') {        /* Word weight (use minimization WWER on MBR) */

    /* Word weight (use minimization WWER on MBR) */
    /* format: (classname @classprob) wordname [output] :weight phoneseq */
    /* format: :%f (linear scale) */
    /* if ":" not found, it means weight == 1.0 (same minimization WER) */

    if ((ptmp = mystrtok(NULL, " \t\n")) == NULL) {
      jlog("Error: voca_load_wordlist: line %d: corrupted data:\n> %s\n", linenum, bufbak);
      winfo->errnum++;
      *ok_flag = FALSE;
      return TRUE;
    }
    if ((ptmp[1] < '0' || ptmp[1] > '9') && ptmp[1] != '.') {     /* not figure after ':' */
      jlog("Error: voca_load_wordlist: line %d: value after ':' missing, maybe wrong space?\n> %s\n", linenum, bufbak);
      winfo->errnum++;
      *ok_flag = FALSE;
      return TRUE;
    }

    /* allocate if not yet */
    if (winfo->weight == NULL) {
      winfo->weight = (LOGPROB *)mymalloc(sizeof(LOGPROB) * winfo->maxnum);
      for (i = 0; i < vnum; i++) {
	winfo->weight[i] = 1.0;
      }
    }

    winfo->weight[vnum] = atof(&(ptmp[1]));
  }
  else{
    if (winfo->weight) 
      winfo->weight[vnum] = 1.0; /* default, same minimization WER */
  }
#endif

    
  /* phoneme sequence */
  if (hmminfo == NULL) {
    /* don't read */
    winfo->wseq[vnum] = NULL;
    winfo->wlen[vnum] = 0;
  } else {

    len = 0;
    first = TRUE;
    pok = TRUE;

    for (;;) {
      if (do_conv) {
	if (first) {
	  /* init phone cycler */
	  cycle_triphone(NULL);
	  /* insert head phone at beginning of word */
	  if (contextphone) {
	    if (strlen(contextphone) >= MAX_HMMNAME_LEN) {
	      jlog("Error: voca_load_wordlist: line %d: too long phone name: %s\n", linenum, contextphone);
	      winfo->errnum++;
	      *ok_flag = FALSE;
	      return TRUE;
	    }
	    cycle_triphone(contextphone);
	  } else {
	    cycle_triphone("NULL_C");
	  }
	  if ((lp = mystrtok(NULL, " \t\n")) == NULL) {
	    jlog("Error: voca_load_wordlist: line %d: word %s has no phoneme:\n> %s\n", linenum, winfo->wname[vnum], bufbak);
	    winfo->errnum++;
	    *ok_flag = FALSE;
	    return TRUE;
	  }
	  if (strlen(lp) >= MAX_HMMNAME_LEN) {
	    jlog("Error: voca_load_wordlist: line %d: too long phone name: %s\n", linenum, lp);
	    winfo->errnum++;
	    *ok_flag = FALSE;
	    return TRUE;
	  }
	  p = cycle_triphone(lp);
	  first = FALSE;
	} else {		/* do_conv, not first */
	  if (lp != NULL) {	/* some token processed at last loop */
	    lp = mystrtok(NULL, " \t\n");
	    if (lp != NULL) {
	      /* token exist */
	      if (strlen(lp) >= MAX_HMMNAME_LEN) {
		jlog("Error: voca_load_wordlist: line %d: too long phone name: %s\n", linenum, lp);
		winfo->errnum++;
		*ok_flag = FALSE;
		return TRUE;
	      }
	      p = cycle_triphone(lp);
	    } else {
	      /* no more token, insert tail phone at end of word */
	      if (contextphone) {
		if (strlen(contextphone) >= MAX_HMMNAME_LEN) {
		  jlog("Error: voca_load_wordlist: line %d: too long phone name: %s\n", linenum, contextphone);
		  winfo->errnum++;
		  *ok_flag = FALSE;
		  return TRUE;
		}
		p = cycle_triphone(contextphone);
	      } else {
		p = cycle_triphone("NULL_C");
	      }
	    }
	  } else {		/* no more token at last input  */
	    /* flush tone cycler */
	    p = cycle_triphone_flush();
	  }
	}
      } else {			/* not do_conv */
	if (first) {
	  p = lp = headphone;
	  first = FALSE;
	} else {
	  if (lp != NULL) {	/* some token processed at last loop */
	    p = lp = mystrtok(NULL, " \t\n");
	    /* if no more token, use tailphone */
	    if (lp == NULL) p = tailphone;
	  } else {
	    /* no more token at last input, exit loop */
	    p = NULL;
	  }
	}
      }
      if (p == NULL) break;
      /* for headphone and tailphone, their context should not be handled */
      /* and when they appear as context they should be replaced by contextphone */
      if (do_conv) {
	center_name(p, cbuf);
	if (contextphone) {
	  if (strmatch(cbuf, contextphone)) {
	    if (len == 0) {
	      p = headphone;
	    } else if (lp == NULL) {
	      p = tailphone;
	    }
	  }
	} else {
	  if (strmatch(cbuf, "NULL_C")) {
	    if (len == 0) {
	      p = headphone;
	    } else if (lp == NULL) {
	      p = tailphone;
	    }
	  } else {
	    if (strnmatch(p, "NULL_C", 6)) {
	      if (strnmatch(&(p[strlen(p)-6]), "NULL_C", 6)) {
		p = cbuf;
	      } else {
		p = rightcenter_name(p, cbuf);
	      }
	    } else if (strnmatch(&(p[strlen(p)-6]), "NULL_C", 6)) {
	      p = leftcenter_name(p, cbuf);
	    }
	  }
	}
      }
      //printf("[[%s]]\n", p);

      /* both defined/pseudo phone is allowed */
      tmplg = htk_hmmdata_lookup_logical(hmminfo, p);
      if (tmplg == NULL) {
	/* not found */
	if (do_conv) {
	  /* logical phone was not found */
	  jlog("Error: voca_load_wordlist: line %d: logical phone \"%s\" not found\n", linenum, p);
	  snprintf(cbuf,MAX_HMMNAME_LEN,"%s", p);
	} else {
	  jlog("Error: voca_load_wordlist: line %d: phone \"%s\" not found\n", linenum, p);
	  snprintf(cbuf, MAX_HMMNAME_LEN, "%s", p);
	}
	add_to_error(winfo, cbuf);
	pok = FALSE;
      } else {
	/* found */
	if (len >= tmpmaxlen) {
	  /* expand wseq area by PHONEMELEN_STEP */
	  tmpmaxlen += PHONEMELEN_STEP;
	  tmpwseq = (HMM_Logical **)myrealloc(tmpwseq, sizeof(HMM_Logical *) * tmpmaxlen);
	}
	/* store to temporal buffer */
	tmpwseq[len] = tmplg;
      }
      len++;
    }
    if (!pok) {			/* error in phoneme */
      jlog("Error: voca_load_wordlist: the line content was: %s\n", bufbak);
      winfo->errnum++;
      *ok_flag = FALSE;
      return TRUE;
    }
    if (len == 0) {
      jlog("Error: voca_load_wordlist: line %d: no phone specified:\n> %s\n", linenum, bufbak);
      winfo->errnum++;
      *ok_flag = FALSE;
      return TRUE;
    }
    /* store to winfo */
    winfo->wseq[vnum] = (HMM_Logical **)mybmalloc2(sizeof(HMM_Logical *) * len, &(winfo->mroot));
    memcpy(winfo->wseq[vnum], tmpwseq, sizeof(HMM_Logical *) * len);
    winfo->wlen[vnum] = len;
    winfo->wton[vnum] = 0;
  }

  vnum++;
  *vnum_p = vnum;
  
  return(TRUE);
}

