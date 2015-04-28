/**
 * @file   chkhmmlist.c
 * 
 * <JA>
 * @brief  HMMList に全ての可能なトライフォンが含まれているかチェックする
 *
 * 与えられた音響モデルにおいて，認識時に登場しうる全てのトライフォンが
 * %HMM定義あるいはHMMListで定義されているかどうかをチェックします．
 *
 * チェック時には辞書の語彙が考慮されます．すなわち，与えられた語彙の単語内
 * および単語間で生じうるトライフォンについてのみチェックされます．
 * </JA>
 * 
 * <EN>
 * @brief  Check existence of all possible triphone in HMMList.
 *
 * These functions check whether all the possible triphones that may appear
 * while recognition process is fully defined or mapped in %HMM definition
 * file and HMMList file.
 *
 * Word dictionary is considered for the test. Only triphones that can
 * appear as word-internal triphones and cross-word triphones on the given
 * dictionary will be considered.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Tue Feb 15 19:17:51 2005
 *
 * $Revision: 1.7 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/htk_hmm.h>
#include <sent/vocabulary.h>

/** 
 * Build a list of base phones by gathering center phones of the defined %HMM.
 * 
 * @param hmminfo [i/o] %HMM definition data
 */
void
make_hmm_basephone_list(HTK_HMM_INFO *hmminfo)
{
  HMM_Logical *lg;
  char p[MAX_HMMNAME_LEN];
  BASEPHONE *match = NULL, *new;
  APATNODE *root;
  int n;

  n = 0;
  root = NULL;
  for(lg=hmminfo->lgstart; lg; lg=lg->next) {
    center_name(lg->name, p);
    if (root != NULL) {
      match = aptree_search_data(p, root);
      if (match != NULL && strmatch(match->name, p)) continue;
    }
    new = (BASEPHONE *)mybmalloc2(sizeof(BASEPHONE), &(hmminfo->mroot));
    new->bgnflag = FALSE;
    new->endflag = FALSE;
    new->name = (char *)mybmalloc2(strlen(p)+1, &(hmminfo->mroot));
    strcpy(new->name, p);
    if (root == NULL) root = aptree_make_root_node(new, &(hmminfo->mroot));
    else aptree_add_entry(new->name, new, match->name, &root, &(hmminfo->mroot));
    n++;
  }
  hmminfo->basephone.num = n;
  hmminfo->basephone.root = root;
}

/** 
 * Traverse function callback to output detailed information of a basephone in text to stdout.
 * 
 * @param x [in] pointer to a basephone data.
 */
static void
print_callback_detail(void *x)
{
  BASEPHONE *b = x;
  printf("\"%s\": bgn=%d, end=%d\n", b->name, b->bgnflag, b->endflag);
}

/** 
 * Traverse function callback to output name of a basephone in text to stdout.
 * 
 * @param x [in] pointer to a basephone data.
 */
static void
print_callback_name(void *x)
{
  BASEPHONE *b = x;
  printf("%s, ", b->name);
}
/** 
 * Output all basephone informations to stdout.
 * 
 * @param base [in] pointer to the top basephone data holder.
 */
void
print_all_basephone_detail(HMM_basephone *base)
{
  aptree_traverse_and_do(base->root, print_callback_detail);
}
/** 
 * Output all basephone names to stdout
 * 
 * @param base [in] pointer to the top basephone data holder.
 */
void
print_all_basephone_name(HMM_basephone *base)
{
  aptree_traverse_and_do(base->root, print_callback_name);
  printf("\n");
}

static int bncnt;		///< Count of basephone that can appear at the beginning of sentence
static int edcnt;		///< Count of basephone that can appear at the end of sentence
/** 
 * Traverse callback function to increment the number of base phones
 * that can appear at beginning of word and end of word.
 * 
 * @param x [in] pointer 
 */
static void
count_callback(void *x)
{
  BASEPHONE *b = x;
  if (b->bgnflag) bncnt++;
  if (b->endflag) edcnt++;
}

/** 
 * Count the total number of base phones that can appear at beginning of
 * word and end of word.
 * 
 * @param base 
 */
static void
count_all_phone(HMM_basephone *base)
{
  bncnt = edcnt = 0;
  aptree_traverse_and_do(base->root, count_callback);
  base->bgnnum = bncnt;
  base->endnum = edcnt;
}

/** 
 * Mark each basephone if it can appear at beginning or end of a word.
 * 
 * @param winfo [in] word dictinary
 * @param base [in] top basephone data holder
 */
static boolean
mark_word_edge(WORD_INFO *winfo, HMM_basephone *base)
{
  WORD_ID w;
  char p[MAX_HMMNAME_LEN];
  char *key;
  BASEPHONE *match;
  boolean ok_p = TRUE;

  /* mark what is at beginning of word (can be right context) */
  for(w=0;w<winfo->num;w++) {
    if (w == winfo->head_silwid) continue;
    key = center_name(winfo->wseq[w][0]->name, p);
    match = aptree_search_data(key, base->root);
    if (match != NULL && strmatch(match->name, key)) {
      match->bgnflag = TRUE;
    } else {
      /* not found!!! */
      jlog("Error: chkhmmlist: basephone \"%s\" used in dictionary not exist in HMM definition\n", key);
      ok_p = FALSE;
    }
  }
  /* mark what is at end of word (can be left context) */
  for(w=0;w<winfo->num;w++) {
    if (w == winfo->tail_silwid) continue;
    key = center_name(winfo->wseq[w][winfo->wlen[w]-1]->name, p);
    match = aptree_search_data(key, base->root);
    if (match != NULL && strmatch(match->name, key)) {
      match->endflag = TRUE;
    } else {
      /* not found!!! */
      jlog("Error: chkhmmlist: basephone \"%s\" used in dictionary not exist in HMM definition\n", key);
      ok_p = FALSE;
    }
  }

  return ok_p;
}


/* check if all possible triphones are exist in logical HMM */
/* temporal storage for aptree() callback */
static HTK_HMM_INFO *local_hmminfo; ///< Local work area to hold HTK %HMM data
static WORD_INFO *local_winfo;	///< Local work area to hold word dictionary
static APATNODE *local_root;	///< Local work area to hold basephone index root
static WORD_ID current_w;	///< Local work area to hold current word ID
static char gbuf[MAX_HMMNAME_LEN];		///< Local work area for phone name handling

static APATNODE *error_root;	///< Error phone list
static int error_num;		///< Number of encountered error phone

/** 
 * Add unknown (error) triphone to error list.
 * 
 * @param lostname [in] name of error triphone.
 * @param hmminfo [i/o] %HMM definition data
 */
static void
add_to_error(char *lostname, HTK_HMM_INFO *hmminfo)
{
  char *match = NULL, *new;
  if (error_root != NULL) {
    match = aptree_search_data(lostname, error_root);
    if (match != NULL && strmatch(match, lostname)) return;
  }
  new = (char *)mybmalloc2(strlen(lostname)+1, &(hmminfo->mroot));
  strcpy(new, lostname);
  if (error_root == NULL) error_root = aptree_make_root_node(new, &(hmminfo->mroot));
  else aptree_add_entry(new, new, match, &error_root, &(hmminfo->mroot));

  error_num++;
}

/** 
 * Traverse callback function to output error phone name.
 * 
 * @param x [in] pointer to error phone name
 */
static void
print_error_callback(void *x)
{
  char *p = x;
  printf("%s\n", p);
}

/** 
 * Traverse callback function to check if the cross-word triphones
 * "basephone x - word[current_w]" and "word[current_w] + basephone x" exist,
 * according to the basephone mark.
 * 
 * @param x [in] a basephone
 */
static void
triphone_callback_normal(void *x)
{
  BASEPHONE *b = x;
  WORD_ID w = current_w;
  HMM_Logical *lg, *found;

  if (b->endflag) {		/* x can appear as end of word */
    lg = local_winfo->wseq[w][0];
    strcpy(gbuf, lg->name);
    add_left_context(gbuf, b->name);
    /* printf("checking \"%s\" - \"%s\"\n", b->name, lg->name); */
    if ((found = htk_hmmdata_lookup_logical(local_hmminfo, gbuf)) == NULL) {
      if (lg->is_pseudo) {
	printf("Error: chkhmmlist: \"%s\" not found, fallback to pseudo {%s}\n", gbuf, lg->name);
	add_to_error(gbuf, local_hmminfo);
      }
    }
  }
  if (b->bgnflag) {		/* x can appear as beginning of word */
    lg = local_winfo->wseq[w][local_winfo->wlen[w]-1];
    strcpy(gbuf, lg->name);
    add_right_context(gbuf, b->name);
    /* printf("checking \"%s\" - \"%s\"\n", lg->name, b->name); */
    if ((found = htk_hmmdata_lookup_logical(local_hmminfo, gbuf)) == NULL) {
      if (lg->is_pseudo) {
	printf("Error: chkhmmlist: \"%s\" not found, fallback to pseudo {%s}\n", gbuf, lg->name);
	add_to_error(gbuf, local_hmminfo);
      }
    }
  }
}

/* for words with only one phone, all combination of "x - current_w + x"
   should be checked */
/** 
 * Traverse callback function to check if the cross-word triphone
 * "basephone x - word[current_w] + basephone x" exist, for
 * words with only one phone: right part.
 * 
 * @param x [in] a basephone
 */
static void
triphone_callback_right(void *x)
{
  BASEPHONE *b = x;
  WORD_ID w = current_w;
  HMM_Logical *lg, *found;
  static char buf[MAX_HMMNAME_LEN];

  if (b->bgnflag) {
    lg = local_winfo->wseq[w][0];
    strcpy(buf, gbuf);
    add_right_context(buf, b->name);
    /* printf("	   checking \"%s\" - \"%s\"\n", gbuf, b->name); */
    if ((found = htk_hmmdata_lookup_logical(local_hmminfo, buf)) == NULL) {
      if (lg->is_pseudo) {
	printf("Error: chkhmmlist: \"%s\" not found, fallback to pseudo {%s}\n", buf, lg->name);
	add_to_error(buf, local_hmminfo);
      }
    }
  }
}

/** 
 * Traverse callback function to check if the cross-word triphone
 * "basephone x - word[current_w] + basephone x" exist, for
 * words with only one phone: left part.
 * 
 * @param x [in] a basephone
 */
static void
triphone_callback_left(void *x)
{
  BASEPHONE *b = x;
  WORD_ID w = current_w;
  HMM_Logical *lg;

  if (b->endflag) {
    lg = local_winfo->wseq[w][0];
    strcpy(gbuf, lg->name);
    add_left_context(gbuf, b->name);
    aptree_traverse_and_do(local_root, triphone_callback_right);
  }
}

/** 
 * Top function to check if all the possible triphones on given word
 * dictionary actually exist in the logical %HMM.
 * 
 * @param hmminfo [in] %HMM definition information, with basephone list.
 * @param winfo [in] word dictionary information
 */
void
test_interword_triphone(HTK_HMM_INFO *hmminfo, WORD_INFO *winfo)
{
  WORD_ID w;
  local_hmminfo = hmminfo;
  local_winfo = winfo;
  local_root = hmminfo->basephone.root;
  error_root = NULL;
  error_num = 0;

  printf("Inter-word triphone existence test...\n");
  for(w=0;w<winfo->num;w++) {
    current_w = w;
    if (winfo->wlen[w] > 1) {
      /* check beginning phone and ending phone of this word */
      aptree_traverse_and_do(hmminfo->basephone.root, triphone_callback_normal);
    } else {
      /* for word of only 1 phoneme, check both */
      aptree_traverse_and_do(hmminfo->basephone.root, triphone_callback_left);
    }
  }
  if (error_root == NULL) {
    printf("passed\n");
  } else {
    printf("following triphones are missing in HMMList:\n");
    aptree_traverse_and_do(error_root, print_error_callback);
    printf("total %d missing inter-word triphones\n", error_num);
  }
}



/** 
 * @brief  Build basephone information
 *
 * Extract base phones from %HMM definition, mark them whether they appear
 * on word head or word tail, and count the number.
 * 
 * @param hmminfo [i/o] %HMM definition information, basephone list will be added.
 * @param winfo [in] word dictionary information
 */
boolean
make_base_phone(HTK_HMM_INFO *hmminfo, WORD_INFO *winfo)
{
  /* gather base phones and word-{head,tail} phones */
  jlog("Stat: chkhmmlist: Exploring HMM database and lexicon tree:\n");
  if (mark_word_edge(winfo, &(hmminfo->basephone)) == FALSE) {
    return FALSE;
  }
  count_all_phone(&(hmminfo->basephone));
  return TRUE;
}

/** 
 * Output general information concerning phone mapping in %HMM definition.
 * 
 * @param fp [in] file descriptor
 * @param hmminfo [in] %HMM definition data.
 */
void
print_phone_info(FILE *fp, HTK_HMM_INFO *hmminfo)
{
  /* output information */
  fprintf(fp, "%5d physical HMMs defined in hmmdefs\n", hmminfo->totalhmmnum);
  if (hmminfo->totalhmmnum == hmminfo->totallogicalnum - hmminfo->totalpseudonum) {
    fprintf(fp, "   no HMMList, physical HMM names are redirected to logicalHMM\n");
  } else {
    if (hmminfo->is_triphone) {
      fprintf(fp, "%5d triphones listed in hmmlist\n", hmminfo->totallogicalnum - hmminfo->totalpseudonum);
    } else {
      fprintf(fp, "%5d phones in hmmlist\n", hmminfo->totallogicalnum - hmminfo->totalpseudonum);
    }
  }
  if (hmminfo->totalpseudonum != 0) {
    fprintf(fp, "%5d pseudo HMM generated for missing mono/bi-phones\n",hmminfo->totalpseudonum);
  }
  fprintf(fp, "%5d TOTAL logical HMMs\n", hmminfo->totallogicalnum);
  fprintf(fp, "%5d base phones in logical HMM\n", hmminfo->basephone.num);
  fprintf(fp, "%5d phones appear on word head, %d phones on word tail\n", hmminfo->basephone.bgnnum, hmminfo->basephone.endnum);
}
