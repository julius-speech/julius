/**
 * @file   cdset.c
 * 
 * <JA>
 * @brief  Pseudo %HMM セットの作成と管理
 *
 * "Pseudo %HMM" は，与えられた音響モデルや%HMMリストで定義されていない
 * バイフォンやモノフォンについて，それを共通コンテキストとする
 * トライフォン集合で代替するもので，主に第１パスの単語間トライフォン
 * 計算に用いられます．
 *
 * Julius は %HMM 定義ファイルおよび%HMMリストを読み込んだあと，
 * まず全ての許され得るモノフォンおよびバイフォンのリストを生成します．
 * そしてれぞれについて，それを共通のコンテキストとする
 * トライフォンのリストを作成し，そのリスト中のトライフォンの各状態を
 * マージしたもの (CD_State_Set) を要素とする構造体 CD_Set を
 * HTKの%HMM定義とは別に新たに生成します．
 * 
 * 例えば，"a-k" という名前の pseudo %HMM は，"a-k+e", "a-k+b" などの
 * トライフォン状態の集合体となります．また "k" というモノフォンの pseudo %HMM
 * は，ベース音素が "k" である全てのトライフォンの状態の集合となります．
 * この生成された pseudo %HMM は全て HTK_HMM_INFO 内の @a cdset_info に
 * 保存されます．
 *
 * さらに，%HMM論理名から実体を探すインデックス木 (@a logical_root) に，
 * この pseudo %HMM のリストが追加されます．これにより，%HMM定義ファイル
 * および%HMMリストファイルのどちらにも定義されていないバイフォンや
 * モノフォンについては，この pseudo %HMM が代用されるようになります．
 * バイフォンやモノフォンが %HMM 定義ファイルや %HMMリストファイルのどちらかで
 * 明示的に指定されていれば，そちらが優先されます．
 * </JA>
 * 
 * <EN>
 * @brief  Generate and manage the pseudo %HMM set
 *
 * "Pseudo %HMM" is mainly for a substitution for unknown context-dependent
 * biphone and monophone %HMM that has not been defined in HTK %HMM
 * definition and HMMList mapping file.  They are used mainly in the
 * cross-word triphone computation on the 1st pass.
 *
 * Julius first generates a list of possible biphone and monophone after
 * reading HTK %HMM definition file and HMMList logical name mapping file.
 * It then generate CD_Set structure for each possible biphone and
 * monophones by parsing all the %HMM definition to find the same context as
 * each phones.
 *
 * For example, the triphones like "a-k+e", "a-k+b", "a-k+a" will be grouped
 * as pseudo phone set "a-k".  A pseudo phone "k" will contain all triphone
 * variants of the same base phone "k".  This generated pseudo %HMM sets are
 * stored in @a cdset_info in HTK_HMM_INFO.
 *
 * Then, the pseudo phones, whose names (biphone or monophone) do not appear
 * in both of the HTK %HMM definitions and HMMList mapping file, will be added
 * as aliases to unspecified phones in the %HMM index tree.  If biphones or
 * monophones are explicitly defined in %HMM definition or HMMList file,
 * they will be used instead of this pseudo phone.
 * 
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Tue Feb 15 17:58:54 2005
 *
 * $Revision: 1.8 $
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

//@{
/// @ingroup cdset

#define CD_STATE_SET_STEP 10	///< CD_State_Set memory allocation step

/** 
 * Initialize total pseudo %HMM information in the given %HMM definition data.
 * 
 * @param hmminfo [i/o] HTK %HMM definitions
 */
static void
cdset_init(HTK_HMM_INFO *hmminfo)
{
  hmminfo->cdset_info.binary_malloc = FALSE;
  hmminfo->cdset_info.cdtree = NULL;
}

/** 
 * Allocate a CD_Set data for a new pseudo phone set.
 * 
 * @return pointer to newly allocated CD_Set.
 */
static CD_Set *
cdset_new()
{
  return((CD_Set *)mymalloc(sizeof(CD_Set)));
}

/** 
 * Look up for a pseudo phone with the name, and return the content.
 * 
 * @param hmminfo [in] %HMM information to search for.
 * @param cdstr [in] string of pseudo phone name to search.
 * 
 * @return pointer to the pseudo phone if found, or NULL if not found.
 */
CD_Set *
cdset_lookup(HTK_HMM_INFO *hmminfo, char *cdstr)
{
  CD_Set *cd;
  cd = aptree_search_data(cdstr, hmminfo->cdset_info.cdtree);
  if (cd != NULL && strmatch(cdstr, cd->name)) {
    return cd;
  } else {
    return NULL;
  }
}

/** 
 * Look up for a pseudo phone by the "left - center" name of the given phone name.
 * 
 * @param hmminfo [in] %HMM information to search for.
 * @param hmmname [in] string of the phone name.
 * 
 * @return pointer to the pseudo phone if found, or NULL if not found.
 */
CD_Set *
lcdset_lookup_by_hmmname(HTK_HMM_INFO *hmminfo, char *hmmname)
{
  char buf[MAX_HMMNAME_LEN];

  return(cdset_lookup(hmminfo, leftcenter_name(hmmname, buf)));
}

/** 
 * Look up for a pseudo phone by the "center + right" name of the given phone name.
 * 
 * @param hmminfo [in] %HMM information to search for.
 * @param hmmname [in] string of the phone name.
 * 
 * @return pointer to the pseudo phone if found, or NULL if not found.
 */
CD_Set *
rcdset_lookup_by_hmmname(HTK_HMM_INFO *hmminfo, char *hmmname)
{
  char buf[MAX_HMMNAME_LEN];

  return(cdset_lookup(hmminfo, rightcenter_name(hmmname, buf)));
}


/** 
 * Output text information of a pseudo phone to stdout.
 * 
 * @param ptr [in] pointer to a pseudo phone set.
 */
static void
put_cdset(void *ptr)
{
  int i;
  CD_Set *a;
  int j;

  a = ptr;
  printf("name: %s\n", a->name);
  /* printf("state_num: %d\n", a->state_num); */
  for(i=0;i<a->state_num;i++) {
    if (a->stateset[i].num == 0) {
      printf("\t[state %d]  not exist\n", i);
    } else {
      printf("\t[state %d]  %d variants\n", i, a->stateset[i].num);
    }
    for(j=0;j<a->stateset[i].num;j++) {
      if (a->stateset[i].s[j]->name) {
	printf("\t\t%s %d\n", a->stateset[i].s[j]->name, a->stateset[i].s[j]->id);
      } else {
	printf("\t\t(NULL) %d\n", a->stateset[i].s[j]->id);
      }
    }
  }
}

/** 
 * Output all pseudo phone set information to stdout
 * 
 * @param hmminfo [in] %HMM definition data that holds pseudo phone data.
 */
void
put_all_cdinfo(HTK_HMM_INFO *hmminfo)
{
  aptree_traverse_and_do(hmminfo->cdset_info.cdtree, put_cdset);
}


/** 
 * Register a physical %HMM as a member of a pseudo phone set.
 * 
 * @param root [i/o] root node of %HMM search index node.
 * @param d [in] a physical defined %HMM to be added.
 * @param cdname [in] name of the pseudo phone set.
 * 
 * @return TRUE if newly registered, FALSE if the specified physical %HMM already exists in the pseudo phone.
 */
boolean
regist_cdset(APATNODE **root, HTK_HMM_Data *d, char *cdname, BMALLOC_BASE **mroot)
{
  boolean need_new;
  CD_State_Set *tmp;
  CD_Set *lset = NULL, *lmatch = NULL;
  int j,n;
  boolean changed = FALSE;

  if (strlen(cdname) >= MAX_HMMNAME_LEN) {
    jlog("Error: cdset: HMM name exceeds limit (%d): %s!\n", MAX_HMMNAME_LEN, cdname);
    jlog("Error: cdset: Please increase the value of MAX_HMMNAME_LEN (current = %d)\n", MAX_HMMNAME_LEN);
    exit(1);
  }
  
  /* check if the cdset already exist */
  need_new = TRUE;
  if (*root != NULL) {
    lmatch = aptree_search_data(cdname, *root);
    if (lmatch != NULL && strmatch(lmatch->name, cdname)) {
      /* exist, add to it later */
      lset = lmatch;
      need_new = FALSE;
      /* if the state num is larger than allocated, expand the lset */
      if (d->state_num > lset->state_num) {
	lset->stateset = (CD_State_Set *)myrealloc(lset->stateset, sizeof(CD_State_Set) * d->state_num);
	/* 0 1 ... (lset->state_num-1) */
	/* N A ... N                   */
	/* 0 1 ...                     ... (d->state_num-1) */
	/* N A ... A ..................... N                */
	/* malloc new area to expanded state (N to A above) */
	for(j = lset->state_num - 1; j < d->state_num - 1; j++) {
	  lset->stateset[j].maxnum = CD_STATE_SET_STEP;
	  lset->stateset[j].s = (HTK_HMM_State **)mymalloc(sizeof(HTK_HMM_State *) * lset->stateset[j].maxnum);
	  lset->stateset[j].num = 0;
	}
	lset->stateset[d->state_num-1].s = NULL;
	lset->stateset[d->state_num-1].num = 0;
	lset->stateset[d->state_num-1].maxnum = 0;
	
	lset->state_num = d->state_num;

	/* update transition table */
	lset->tr = d->tr;

	changed = TRUE;
      }
    }
  }

  if (need_new) {
    /* allocate as new with blank data */
    lset = cdset_new();
    lset->name = strdup(cdname);
    lset->state_num = d->state_num;
    lset->stateset = (CD_State_Set *)mymalloc(sizeof(CD_State_Set) * lset->state_num);
    /* assume first and last state has no outprob */
    lset->stateset[0].s = lset->stateset[lset->state_num-1].s = NULL;
    lset->stateset[0].num = lset->stateset[lset->state_num-1].num = 0;
    lset->stateset[0].maxnum = lset->stateset[lset->state_num-1].maxnum = 0;
    for(j=1;j<lset->state_num-1; j++) {
      /* pre-allocate only the first step */
      lset->stateset[j].maxnum = CD_STATE_SET_STEP;
      lset->stateset[j].s = (HTK_HMM_State **)mymalloc(sizeof(HTK_HMM_State *) * lset->stateset[j].maxnum);
      lset->stateset[j].num = 0;
    }
    /* assign transition table of first found %HMM (ad-hoc?) */
    lset->tr = d->tr;
    /* add to search index tree */
    if (*root == NULL) {
      *root = aptree_make_root_node(lset, mroot);
    } else {
      aptree_add_entry(lset->name, lset, lmatch->name, root, mroot);
    }

    changed = TRUE;
  }
    
  /* register each HMM states to the lcdset */
  for (j=1;j<d->state_num-1;j++) {
    tmp = &(lset->stateset[j]);
    /* check if the state has already registered */
    for(n = 0; n < tmp->num ; n++) {
      if (tmp->s[n] == d->s[j]) { /* compare by pointer */
	/*jlog("\tstate %d has same\n", n);*/
	break;
      }
    }
    if (n < tmp->num ) continue;	/* same state found, cancel regist. */
    
    /* expand storage area if necessary */
    if (tmp->num >= tmp->maxnum) {
      tmp->maxnum += CD_STATE_SET_STEP;
      tmp->s = (HTK_HMM_State **)myrealloc(tmp->s, sizeof(HTK_HMM_State *) * tmp->maxnum);
    }
    
    tmp->s[tmp->num] = d->s[j];
    tmp->num++;

    changed = TRUE;
  }

  return(changed);
}

/** 
 * Construct the whole pseudo %HMM information, and also add them to the logical Triphone tree.
 * 
 * @param hmminfo [i/o] %HMM definition data.  The generated data will also
 * be stored within this.
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
make_cdset(HTK_HMM_INFO *hmminfo)
{
  HMM_Logical *lg;
  char buf[MAX_HMMNAME_LEN];

  cdset_init(hmminfo);
  /* make cdset name from logical HMM name */
  /* left-context set: "a-k" for /a-k+i/, /a-k+o/, ...
     for 1st pass (word end) */
  for(lg = hmminfo->lgstart; lg; lg = lg->next) {
    if (lg->is_pseudo) continue;
    regist_cdset(&(hmminfo->cdset_info.cdtree), lg->body.defined, leftcenter_name(lg->name, buf), &(hmminfo->cdset_root));
  }
  /* right-context set: "a+o" for /b-a+o/, /t-a+o/, ...
     for 2nd pass (word beginning) */
  for(lg = hmminfo->lgstart; lg; lg = lg->next) {
    if (lg->is_pseudo) continue;
    regist_cdset(&(hmminfo->cdset_info.cdtree), lg->body.defined, rightcenter_name(lg->name, buf), &(hmminfo->cdset_root));
  }
  /* both-context set: "a" for all triphone with same base phone "a"
     for 1st pass (1 phoneme word, with no previous word hypo.) */
  for(lg = hmminfo->lgstart; lg; lg = lg->next) {
    if (lg->is_pseudo) continue;
    regist_cdset(&(hmminfo->cdset_info.cdtree), lg->body.defined, center_name(lg->name, buf), &(hmminfo->cdset_root));
  }

  /* now that cdset is completely built */
  hmminfo->cdset_info.binary_malloc = FALSE;
  
  return(TRUE);
}

/** 
 * callback for aptree function to free the content of pseudo phone set.
 * 
 * @param arg [in] pointer to the pseudo phone set to be free
 */
static void
callback_free_lcdset_content(void *arg)
{
  CD_Set *d;
  int j;

  d = arg;
  for(j=0;j<d->state_num;j++) {
    if (d->stateset[j].s != NULL) free(d->stateset[j].s);
  }
  free(d->stateset);
  free(d->name);
  free(d);
}

/** 
 * Remove all the registered category-indexed pseudo state sets.
 * This function will be called when a grammar is changed to re-build the
 * state sets.
 * 
 * @param root [i/o] pointer to hold the root index pointer
 */
void
free_cdset(APATNODE **root, BMALLOC_BASE **mroot)
{
  if (*root != NULL) {
    aptree_traverse_and_do(*root, callback_free_lcdset_content);
    mybfree2(mroot);
    *root = NULL;
  }
}

//@}
