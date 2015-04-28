/**
 * @file   rdhmmdef.c
 * 
 * <JA>
 * @brief  HTK %HMM 定義ファイルの読み込み：メイン
 *
 * ここには HTK 形式の %HMM 定義ファイルを読み込むための関数群を呼び出す
 * メイン関数が収められています．
 *
 * このファイルはまた，読み込み関数群で共通して用いられるトークン単位の
 * ファイル読み込み関数を提供します．
 * %HMM 定義ファイルは read_token() によってトークン単位で順次読み込まれ，
 * グローバル変数 rdhmmdef_token に格納されます．各関数群はこの
 * rdhmmdef_token を参照して現在のトークンを得ます．
 * </JA>
 * 
 * <EN>
 * @brief  Read HTK %HMM definition file: the main
 *
 * This file includes the main routine to read the %HMM definition file in
 * HTK format.
 *
 * This file also contains functions and global variables for per-token
 * reading tailored for reading HTK %HMM definition file.  The read_token()
 * will read the file per token, and the read token is stored in a global
 * variable rdhmmdef_token.  The other reading function will refer to this
 * variable to read the current token.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 00:17:18 2005
 *
 * $Revision: 1.9 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/htk_param.h>
#include <sent/htk_hmm.h>
#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

#define MAXBUFLEN  4096		///< Maximum length of a line in the input

char *rdhmmdef_token;		///< Current token string (GLOBAL)
static boolean last_line_full = FALSE;
static char buf[MAXBUFLEN];	///< Local work area for token reading
static int line;		///< Input Line count

/* global functions for rdhmmdef_*.c */

/** 
 * Output error message, with current reading status, and terminate
 * 
 * @param str [in] error string
 */
void
rderr(char *str)
{
  if (rdhmmdef_token == NULL) {	/* end of file */
    jlog("Error: rdhmmdef: %s on end of file\n", str);
  } else {
    jlog("Error: rdhmmdef: read error at line %d: %s\n", line, (str) ? str : "parse error");
  }
  jlog_flush();
  exit(1);
}

/** 
 * Read next token and ste it to rdhmmdef_token.
 * 
 * @param fp [in] file pointer
 * 
 * @return the pointer to the read token, or NULL on end of file or error.
 */
char *
read_token(FILE *fp)
{
  int len;
  int bp = 0;
  int maxlen = MAXBUFLEN;
  static char delims[] = HMMDEF_DELM;

  if ((rdhmmdef_token = mystrtok_quote(NULL, HMMDEF_DELM)) != NULL) {
    /* has token */
    if (mystrtok_movetonext(NULL, HMMDEF_DELM) != NULL || last_line_full == FALSE) {
      /* return the current token, if this is not a last token, or
	 last is newline terminated */
      return rdhmmdef_token;
    } else {
      /* concatinate the last token with next line */
      len = strlen(rdhmmdef_token);
      memmove(buf, rdhmmdef_token, len);
      bp = len;
      maxlen -= len;
    }
  }

  /* read new 1 line a*/
  while(
#ifdef HAVE_ZLIB
	gzgets((gzFile)fp, &(buf[bp]), maxlen) != Z_NULL
#else
	fgets(&(buf[bp]), maxlen, fp) != NULL
#endif
	) {
    /* chop delimiters at end of line (incl. newline) */
    /* if no delimiter at end of line, last_line_full is TRUE */
    last_line_full = TRUE;
    len = strlen(buf)-1;
    while (len >= 0 && strchr(delims, buf[len])) {
      last_line_full = FALSE;
      buf[len--] = '\0';
    }
    if (buf[0] != '\0') {
      /* start getting next token */
      rdhmmdef_token = mystrtok_quote(buf, HMMDEF_DELM);
      /* increment line */
      line++;
      return rdhmmdef_token;
    }
  }
  /* when reading error, return NULL */
  rdhmmdef_token = NULL;
  return rdhmmdef_token;
}

/** 
 * Convert all the transition probabilities to log10 scale.
 * 
 * @param hmm [i/o] %HMM definition data to modify.
 */
static void
conv_log_arc(HTK_HMM_INFO *hmm)
{
  HTK_HMM_Trans *tr;
  int i,j;
  LOGPROB l;

  for (tr = hmm->trstart; tr; tr = tr->next) {
    for(i=0;i<tr->statenum;i++) {
      for(j=0;j<tr->statenum;j++) {
	l = tr->a[i][j];
	tr->a[i][j] = (l != 0.0) ? (float)log10(l) : LOG_ZERO;
      }
    }
  }
}
/** 
 * Invert all the variance values.
 * 
 * @param hmm [i/o] %HMM definition data to modify.
 */
void
htk_hmm_inverse_variances(HTK_HMM_INFO *hmm)
{
  HTK_HMM_Var *v;
  int i;

  for (v = hmm->vrstart; v; v = v->next) {
    for(i=0;i<v->len;i++) {
      v->vec[i] = 1.0 / v->vec[i];
    }
  }
}

#ifdef ENABLE_MSD
/** 
 * Check if this HMM contains MSD-HMM.  The status will be set to hmm->has_msd.
 * 
 * @param hmm [i/o] %HMM definition data to check.
 */
void
htk_hmm_check_msd(HTK_HMM_INFO *hmm)
{
  HTK_HMM_PDF *m;
  int vlen;
  int i;

  hmm->has_msd = FALSE;
  for (m = hmm->pdfstart; m; m = m->next) {
    /* skip tied-mixture pdf */
    if (m->tmix) continue;
    /* check if vector length are the same */
    vlen = hmm->opt.stream_info.vsize[m->stream_id];
    for(i=0;i<m->mix_num;i++) {
      if (m->b[i]->meanlen != vlen) {
	jlog("Stat: rdhmmdef: assume MSD-HMM since Gaussian dimension are not consistent\n");
	hmm->has_msd = TRUE;
	return;
      }
    }
  }
}
#endif

boolean
htk_hmm_check_sid(HTK_HMM_INFO *hmm)
{
  HTK_HMM_State *stmp;
  boolean *check;
  int i;
  boolean ok_p;
  
  /* check if each state is assigned a valid sid */
  check = (boolean *)mymalloc(sizeof(boolean) * hmm->totalstatenum);
  for(i = 0; i < hmm->totalstatenum; i++) check[i] = FALSE;
  for (stmp = hmm->ststart; stmp; stmp = stmp->next) {
    if (stmp->id == -1) {
      jlog("Error: rdhmmdef: no SID on some states\n");
      free(check);
      return FALSE;
    }
    if (stmp->id < 0) {
      jlog("Error: rdhmmdef: invalid SID value: %d\n", stmp->id);
      free(check);
      return FALSE;
    }
    if (stmp->id >= hmm->totalstatenum) {
      jlog("Error: rdhmmdef: SID value exceeds the number of states? (%d > %d)\n", stmp->id, hmm->totalstatenum);
      free(check);
      return FALSE;
    }
    if (check[stmp->id] == TRUE) {
      jlog("Error: rdhmmdef: duplicate definition to the same SID: %d\n", stmp->id);
      free(check);
      return FALSE;
    }
    check[stmp->id] = TRUE;
  }
  ok_p = TRUE;
  for(i = 0; i < hmm->totalstatenum; i++) {
    if (check[i] == FALSE) {
      jlog("Error: rdhmmdef: missing SID: %d\n", i);
      ok_p = FALSE;
    }
  }
  free(check);

  return ok_p;
}

/** 
 * @brief  Main top routine to read in HTK %HMM definition file.
 *
 * A HTK %HMM definition file will be read from @a fp.  After reading,
 * the parameter type is checked and calculate some statistics.
 * 
 * @param fp [in] file pointer
 * @param hmm [out] pointer to a %HMM definition structure to store data.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
rdhmmdef(FILE *fp, HTK_HMM_INFO *hmm)
{
  char macrosw;
  char *name;

  /* variances in htkdefs are not inversed yet */
  hmm->variance_inversed = FALSE;

  /* read the first token */
  /* read new 1 line */
  line = 1;
  if (getl(buf, MAXBUFLEN, fp) == NULL) {
    rdhmmdef_token = NULL;
  } else {
    rdhmmdef_token = mystrtok_quote(buf, HMMDEF_DELM);
  }
  
  /* the toplevel loop */
  while (rdhmmdef_token != NULL) {/* break on EOF */
    if (rdhmmdef_token[0] != '~') { /* toplevel commands are always macro */
      return FALSE;
    }
    macrosw = rdhmmdef_token[1];
    read_token(fp);		/* read next token after the "~.."  */
    switch(macrosw) {
    case 'o':			/* global option */
      if (set_global_opt(fp,hmm) == FALSE) {
	return FALSE;
      }
      break;
    case 't':			/* transition macro */
      name = mybstrdup2(rdhmmdef_token, &(hmm->mroot));
      if (strlen(name) >= MAX_HMMNAME_LEN) rderr("Macro name too long");
      read_token(fp);
      def_trans_macro(name, fp, hmm);
      break;
    case 's':			/* state macro */
      name = mybstrdup2(rdhmmdef_token, &(hmm->mroot));
      if (strlen(name) >= MAX_HMMNAME_LEN) rderr("Macro name too long");
      read_token(fp);
      def_state_macro(name, fp, hmm);
      break;
    case 'm':			/* density (mixture) macro */
      name = mybstrdup2(rdhmmdef_token, &(hmm->mroot));
      if (strlen(name) >= MAX_HMMNAME_LEN) rderr("Macro name too long");
      read_token(fp);
      def_dens_macro(name, fp, hmm);
      break;
    case 'h':			/* HMM define */
      name = mybstrdup2(rdhmmdef_token, &(hmm->mroot));
      if (strlen(name) >= MAX_HMMNAME_LEN) rderr("Macro name too long");
      read_token(fp);
      def_HMM(name, fp, hmm);
      break;
    case 'v':			/* Variance macro */
      name = mybstrdup2(rdhmmdef_token, &(hmm->mroot));
      if (strlen(name) >= MAX_HMMNAME_LEN) rderr("Macro name too long");
      read_token(fp);
      def_var_macro(name, fp, hmm);
      break;
    case 'w':			/* Stream weight macro */
      name = mybstrdup2(rdhmmdef_token, &(hmm->mroot));
      if (strlen(name) >= MAX_HMMNAME_LEN) rderr("Macro name too long");
      read_token(fp);
      def_streamweight_macro(name, fp, hmm);
      break;
    case 'r':			/* Regression class macro (ignore) */
      name = mybstrdup2(rdhmmdef_token, &(hmm->mroot));
      if (strlen(name) >= MAX_HMMNAME_LEN) rderr("Macro name too long");
      read_token(fp);
      def_regtree_macro(name, fp, hmm);
      break;
    case 'p':			/* Mixture pdf macro (extension of HTS) */
      name = mybstrdup2(rdhmmdef_token, &(hmm->mroot));
      if (strlen(name) >= MAX_HMMNAME_LEN) rderr("Macro name too long");
      read_token(fp);
      def_mpdf_macro(name, fp, hmm);
      break;
    }
  }

  /* convert transition prob to log scale */
  conv_log_arc(hmm);

  jlog("Stat: rdhmmdef: ascii format HMM definition\n");
  
  /* check limitation */
  if (check_all_hmm_limit(hmm)) {
    jlog("Stat: rdhmmdef: limit check passed\n");
  } else {
    jlog("Error: rdhmmdef: cannot handle this HMM due to system limitation\n");
    return FALSE;
  }

  /* determine whether this model needs multi-path handling */
  hmm->need_multipath = htk_hmm_has_several_arc_on_edge(hmm);
  if (hmm->need_multipath) {
    jlog("Stat: rdhmmdef: this HMM requires multipath handling at decoding\n");
  } else {
    jlog("Stat: rdhmmdef: this HMM does not need multipath handling\n");
  }
  
  /* inverse all variance values for faster computation */
  if (! hmm->variance_inversed) {
    htk_hmm_inverse_variances(hmm);
    hmm->variance_inversed = TRUE;
  }

  /* check HMM parameter option type */
  if (!check_hmm_options(hmm)) {
    jlog("Error: rdhmmdef: hmm options check failed\n");
    return FALSE;
  }

  /* add ID number for all HTK_HMM_State if not assigned */
  {
    HTK_HMM_State *stmp;
    int n;
    boolean has_sid;

    /* caclculate total num and check if has sid */
    has_sid = FALSE;
    n = 0;
    for (stmp = hmm->ststart; stmp; stmp = stmp->next) {
      n++;
      if (n >= MAX_STATE_NUM) {
	jlog("Error: rdhmmdef: too much states in a model > %d\n", MAX_STATE_NUM);
	return FALSE;
      }
      if (stmp->id != -1) {
	has_sid = TRUE;
      }
    }
    hmm->totalstatenum = n;
    if (has_sid) {
      jlog("Stat: rdhmmdef: <SID> found in the definition\n");
      /* check if each state is assigned a valid sid */
      if (htk_hmm_check_sid(hmm) == FALSE) {
	jlog("Error: rdhmmdef: error in SID\n");
	return FALSE;
      }
    } else {
      /* assign internal sid (will not be saved) */
      jlog("Stat: rdhmmdef: no <SID> embedded\n");
      jlog("Stat: rdhmmdef: assign SID by the order of appearance\n");
      n = hmm->totalstatenum;
      for (stmp = hmm->ststart; stmp; stmp = stmp->next) {
	stmp->id = --n;
      }
    }
  }
  /* calculate the maximum number of mixture */
  {
    HTK_HMM_State *stmp;
    int max, s, mix;
    max = 0;
    for (stmp = hmm->ststart; stmp; stmp = stmp->next) {
      for(s=0;s<stmp->nstream;s++) {
	mix = stmp->pdf[s]->mix_num;
	if (max < mix) max = mix;
      }
    }
    hmm->maxmixturenum = max;
  }
  /* compute total number of HMM models and maximum length */
  {
    HTK_HMM_Data *dtmp;
    int n, maxlen;
    n = 0;
    maxlen = 0;
    for (dtmp = hmm->start; dtmp; dtmp = dtmp->next) {
      if (maxlen < dtmp->state_num) maxlen = dtmp->state_num;
      n++;
    }
    hmm->maxstatenum = maxlen;
    hmm->totalhmmnum = n;
  }
  /* compute total number of Gaussians */
  {
    HTK_HMM_Dens *dtmp;
    int n = 0;
    for (dtmp = hmm->dnstart; dtmp; dtmp = dtmp->next) {
      n++;
    }
    hmm->totalmixnum = n;
  }
  /* check of HMM name length exceed the maximum */
  {
    HTK_HMM_Dens *dtmp;
    int n = 0;
    for (dtmp = hmm->dnstart; dtmp; dtmp = dtmp->next) {
      n++;
    }
    hmm->totalmixnum = n;
  }
  /* compute total number of mixture PDFs */
  {
    HTK_HMM_PDF *p;
    int n = 0;
    for (p = hmm->pdfstart; p; p = p->next) {
      n++;
    }
    hmm->totalpdfnum = n;
  }
  /* assign ID number for all HTK_HMM_Trans */
  {
    HTK_HMM_Trans *ttmp;
    int n = 0;
    for (ttmp = hmm->trstart; ttmp; ttmp = ttmp->next) {
      ttmp->id = n++;
    }
    hmm->totaltransnum = n;
  }
#ifdef ENABLE_MSD
  /* check if MSD-HMM */
  htk_hmm_check_msd(hmm);
#endif

  return(TRUE);			/* success */
}
