/**
 * @file   init_phmm.c
 * 
 * <JA>
 * @brief  %HMM 定義ファイルおよびHMMListマッピングファイルのメモリ読み込みと初期化
 * </JA>
 * 
 * <EN>
 * @brief  Load %HMM definition file and HMMList mapping file into memory and set it up.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Tue Feb 15 23:05:33 2005
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
#include <sent/htk_hmm.h>

/** 
 * Allocate memory for a new %HMM definition data.
 * 
 * @return pointer to the newly allocated %HMM definition data.
 */
HTK_HMM_INFO *
hmminfo_new()
{
  HTK_HMM_INFO *new;

  new = (HTK_HMM_INFO *)mymalloc(sizeof(HTK_HMM_INFO));

  new->mroot = NULL;
  new->lroot = NULL;
  new->cdset_root = NULL;
  new->tmp_mixnum = NULL;

  new->opt.stream_info.num = 0;
  new->opt.cov_type = C_DIAG_C;
  new->opt.dur_type = D_NULL;

  new->trstart = NULL;
  new->vrstart = NULL;
  new->swstart = NULL;
  new->ststart = NULL;
  new->dnstart = NULL;
  new->pdfstart = NULL;
  new->start   = NULL;
  new->lgstart = NULL;
  new->physical_root = NULL;
  new->logical_root = NULL;
  new->tr_root = NULL;
  new->vr_root = NULL;
  new->sw_root = NULL;
  new->dn_root = NULL;
  new->pdf_root = NULL;
  new->st_root = NULL;
  new->codebooknum = 0;
  new->codebook_root = NULL;
  new->maxcodebooksize = 0;
  new->totalmixnum = 0;
  new->totalstatenum = 0;
  new->totalhmmnum = 0;
  new->totallogicalnum = 0;
  new->is_triphone = FALSE;
  new->is_tied_mixture = FALSE;
  new->cdset_method = IWCD_NBEST;
  new->cdmax_num = 3;
  new->totalpseudonum = 0;
  new->sp = NULL;
  new->basephone.root = NULL;
  new->cdset_info.cdtree = NULL;
  new->variance_inversed = FALSE;

#ifdef ENABLE_MSD
  new->has_msd = FALSE;
#endif

  return(new);
}

/** 
 * Free memory of an %HMM
 * 
 * @param hmm [i/o] %HMM definition data
 * 
 * @return TRUE on success, or FALSE if failed.
 */
boolean
hmminfo_free(HTK_HMM_INFO *hmm)
{
  if (hmm->cdset_info.binary_malloc) {
    /* cdset are allocated by bmalloc (in case read from binary hmmlist) */
    if (hmm->cdset_root != NULL) mybfree2(&(hmm->cdset_root));
  } else {
    /* cdset does not use bmalloc, so free them separately */
    free_cdset(&(hmm->cdset_info.cdtree), &(hmm->cdset_root));
  }

  /* free all memory that has been allocated by bmalloc2() */
  if (hmm->mroot != NULL) mybfree2(&(hmm->mroot));
  if (hmm->lroot != NULL) mybfree2(&(hmm->lroot));

  /* free whole */
  free(hmm);

  return(TRUE);
}

/** 
 * @brief Load HTK %HMM definition file and HMMList file, and setup phone %HMM information.
 *
 * First try ascii format, then try binary format.
 * 
 * @param hmminfo [out] pointer to store all the %HMM definition data.
 * @param hmmfilename [in] file name of HTK %HMM definition file, NULL if not.
 * @param namemapfile [in] file name of HMMList mapping file.
 * @param para [out] store acoustic analysis condition parameters if exist in hmmfilename.
 */
boolean
init_hmminfo(HTK_HMM_INFO *hmminfo, char *hmmfilename, char *namemapfile, Value *para)
{
  FILE *fp;
  boolean ok_p;
  boolean binary;

  ok_p = FALSE;

  /* read hmmdef file */
  jlog("Stat: init_phmm: Reading in HMM definition\n");
  /* first, try ascii format */
  if ((fp = fopen_readfile(hmmfilename)) == NULL) {
    jlog("Error: init_phmm: failed to open %s\n",hmmfilename);
    return FALSE;
  }
  if (rdhmmdef(fp, hmminfo) == TRUE) {
    ok_p = TRUE;
  }
  if (fclose_readfile(fp) < 0) {
    jlog("Error: init_phmm: failed to close %s\n", hmmfilename);
    return FALSE;
  }
  if (ok_p == FALSE) {
    /* second, try binary format */
    if ((fp = fopen_readfile(hmmfilename)) == NULL) {
      jlog("Error: init_phmm: failed to open %s\n",hmmfilename);
      return FALSE;
    }
    if (read_binhmm(fp, hmminfo, TRUE, para) == TRUE) {
      ok_p = TRUE;
    }
    if (fclose_readfile(fp) < 0) {
      jlog("Error: init_phmm: failed to close %s\n", hmmfilename);
      return FALSE;
    }
  }
  if (ok_p == FALSE) {
    jlog("Error: init_phmm: failed to read %s\n", hmmfilename);
    return FALSE;
  }

  jlog("Stat: init_phmm: defined HMMs: %5d\n", hmminfo->totalhmmnum);

  /* make mapping from logically named HMM to real defined HMM name */
  if (namemapfile != NULL) {
    /* use HMMList map file */
    if ((fp = fopen_readfile(namemapfile)) == NULL) {
      jlog("Error: init_phmm: failed to open %s\n",namemapfile);
      return FALSE;
    }
    /* detect binary / ascii by the first 4 bytes */
    {
      int x;
      if (myfread(&x, sizeof(int), 1, fp) < 1) {
	jlog("Error: init_phmm: failed to read %s\n", namemapfile);
	return FALSE;
      }
      if (x == 0) {
	binary = TRUE;
      } else {
	binary = FALSE;
	myfrewind(fp);
      }
    }
    if (binary) {
      /* binary format */
      jlog("Stat: init_phmm: loading binary hmmlist\n");
      if (load_hmmlist_bin(fp, hmminfo) == FALSE) {
	jlog("Error: init_phmm: HMMList \"%s\" read error\n",namemapfile);
	return FALSE;
      }
    } else {
      /* ascii format */
      jlog("Stat: init_phmm: loading ascii hmmlist\n");
      if (rdhmmlist(fp, hmminfo) == FALSE) {
	jlog("Error: init_phmm: HMMList \"%s\" read error\n",namemapfile);
	return FALSE;
      }
    }
    if (fclose_readfile(fp) < 0) {
      jlog("Error: init_phmm: failed to close %s\n", namemapfile);
      return FALSE;
    }
    jlog("Stat: init_phmm: logical names: %5d in HMMList\n", hmminfo->totallogicalnum);

  } else {
    /* add all names of physical HMMs as logical names */
    hmm_add_physical_to_logical(hmminfo);
    jlog("Stat: init_phmm: logical names: %5d\n", hmminfo->totallogicalnum);
  }

  /* extract basephone */
  make_hmm_basephone_list(hmminfo);
  jlog("Stat: init_phmm: base phones: %5d used in logical\n", hmminfo->basephone.num);

  /* Guess we need to handle context dependency */
  /* (word-internal CD is done statically, cross-word CD dynamically */
  if (guess_if_cd_hmm(hmminfo)) {
    hmminfo->is_triphone = TRUE;
  } else {
    hmminfo->is_triphone = FALSE;
  }

  jlog("Stat: init_phmm: finished reading HMM definitions\n");

  return TRUE;
}

/** 
 * Set up pause model.
 * 
 * @param hmminfo [i/o] %HMM definition data
 * @param spmodel_name [in] name string of short pause model
 */
void
htk_hmm_set_pause_model(HTK_HMM_INFO *hmminfo, char *spmodel_name)
{
  HMM_Logical *l;

  l = htk_hmmdata_lookup_logical(hmminfo, spmodel_name);
  if (l == NULL) {
    jlog("Warning: init_phmm: no model named as \"%s\", no short pause model assigned\n", spmodel_name);
  }
  hmminfo->sp = l;
}
