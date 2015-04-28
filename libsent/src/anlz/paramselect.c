/**
 * @file   paramselect.c
 *
 * <JA>
 * @brief  パラメータベクトルの型のチェックと調整
 *
 * %HMMと入力特徴パラメータの型をチェックします．タイプが一致しない場合，
 * 特徴パラメータの一部を削除することで一致するよう調整できるかどうか
 * を試みます．（例：特徴量ファイルが MFCC_E_D_Z (26次元) で与えられた
 * とき，音響モデルが MFCC_E_D_N_Z (25次元) である場合，絶対値パワー項を
 * 差し引くことで調整できます．）
 *
 * 調整アルゴリズムは以下のとおりです．
 *    -# 入力の各ベクトル要素に対応するマークを 0 に初期化
 *    -# %HMMで要求されている型に対応しないベクトル要素に 1 をマークする
 *    -# 新たにパラメータ領域を確保し，必要な要素（マークされていない要素）
 *       のみをコピーする．
 * 
 * </JA>
 * <EN>
 * @brief  Check and adjust parameter vector types
 *
 * This file is to check if %HMM parameter and input parameter are the same.
 * If they are not the same, it then tries to modify the input to match the
 * required format in %HMM.  Available parameter modification is only to
 * delete some part of the parameter (ex. MFCC_E_D_Z (26 dim.) can be
 * modified to MFCC_E_D_N_Z (25 dim.) by just deleting the absolute power).
 * Note that no parameter generation or conversion is implemented currently.
 *
 * The adjustment algorithm is as follows:
 *    -# Initialize mark to 0 for each input vector element.
 *    -# Compare parameter type and mark unnecessary element as EXCLUDE(=1).
 *    -# Allocate a new parameter area and copy needed (=NOT marked) element.
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Sun Feb 13 20:46:39 2005
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
#include <sent/htk_param.h>
#include <sent/htk_hmm.h>


/** 
 * Put exlusion marks for vector for @a len elements from @a loc -th dimension.
 * 
 * @param loc [in] beginning dimension to mark
 * @param len [in] number of dimension to mark from @a loc
 * @param vmark [in] mark buffer
 * @param vlen [in] length of vmark
 */
static void
mark_exclude_vector(int loc, int len, int *vmark, int vlen)
{
  int i;
#ifdef DEBUG
  printf("delmark: %d-%d\n",loc, loc+len-1);
#endif
  for (i=0;i<len;i++) {
#ifdef DEBUG
    if (loc + i >= vlen) {
      printf("delmark buffer exceeded!!\n");
      exit(0);
    }
#endif
    vmark[loc+i] = 1;
  }
#ifdef DEBUG
  printf("now :");
  for (i=0;i<vlen;i++) {
    if (vmark[i] == 1) {
      printf("-");
    } else {
      printf("O");
    }
  }
  printf("\n");
#endif
}

/**
 * @brief  Execute exclusion for a parameter data according to the
 * exclusion marks.
 * 
 * Execute vector element exclusion will be done inline.
 * 
 * @param p [i/o] parameter
 * @param vmark [in] mark buffer
 */
static void
exec_exclude_vectors(HTK_Param *p, int *vmark)
{
  int src, dst;
  unsigned int t;

  /* shrink */
  for(t = 0; t < p->samplenum; t++) {
    dst = 0;
    for (src = 0; src < p->veclen; src++) {
      if (vmark[src] == 0) {
	if (dst != src) p->parvec[t][dst] = p->parvec[t][src];
	dst++;
      }
    }
  }
  p->veclen = dst;
#ifdef DEBUG
  printf("new length = %d\n", p->veclen);
#endif
}


/** 
 * Guess the length of the base coefficient according to the total vector
 * length and parameter type.
 * 
 * @param p [in] parameter data
 * @param qualtype [in] parameter type
 * 
 * @return the guessed size of the base coefficient.
 */
int
guess_basenum(HTK_Param *p, short qualtype)
{
  int size;
  int compnum;
  
  compnum = 1 + ((qualtype & F_DELTA) ? 1 : 0) + ((qualtype & F_ACCL) ? 1 : 0);
  
  size = p->veclen;
  if (p->header.samptype & F_ENERGY_SUP) size += 1;
  if ((size % compnum) != 0) {
    jlog("Error: paramselect: illegal vector length (should not happen)\n");
    return -1;
  }
  size /= compnum;
  if (p->header.samptype & F_ENERGY) size -= 1;
  if (p->header.samptype & F_ZEROTH) size -= 1;

  return(size);
}

/* can add: _N */
/* can sub: _E_D_A_0 */

/** 
 * Compare source parameter type and required type in HTK %HMM, and set mark.
 * 
 * @param src [in] input parameter
 * @param dst_type_arg [in] required parameter type
 * @param vmark [in] mark buffer
 * @param vlen [in] length of vmark
 * @param new_type [out] return the new type
 * 
 * @return TRUE on success, FALSE on failure.
 */
static boolean
select_param_vmark(HTK_Param *src, short dst_type_arg, int *vmark, int vlen, short *new_type)
{
  short dst_type;
  short del_type, add_type;
  int basenum, pb[3],pe[3],p0[3]; /* location */
  int i, len;
  char srcstr[80], dststr[80], buf[80];
  short src_type;

  src_type = src->header.samptype & ~(F_COMPRESS | F_CHECKSUM);
  src_type &= ~(F_BASEMASK);	/* only qualifier code needed */
  srcstr[0] = '\0';
  param_qualcode2str(srcstr, src_type, FALSE);
  dst_type = dst_type_arg & ~(F_COMPRESS | F_CHECKSUM);
  dst_type &= ~(F_BASEMASK);	/* only qualifier code needed */
  dststr[0] = '\0';
  param_qualcode2str(dststr, dst_type, FALSE);

#ifdef DEBUG
  printf("try to select qualifiers: %s -> %s\n", srcstr, dststr);
#endif

  if (dst_type == F_ERR_INVALID) {
    jlog("Error: paramselect: unknown parameter kind for selection: %s\n", dststr);
    return(FALSE);
  }
  
  /* guess base coefficient num */
  basenum = guess_basenum(src, src_type);
  if (basenum < 0) {		/* error */
    return(FALSE);
  }
#ifdef DEBUG
  printf("base num = %d\n", basenum);
#endif

  /* determine which component to use */
  del_type = src_type & (~(dst_type));
  add_type = (~(src_type)) & dst_type;

  /* vector layout for exclusion*/
  pb[0] = 0;
  if ((src_type & F_ENERGY) && (src_type & F_ZEROTH)){
    p0[0] = basenum;
    pe[0] = basenum + 1;
    len = basenum + 2;
  } else if ((src_type & F_ENERGY) || (src_type & F_ZEROTH)){
    p0[0] = pe[0] = basenum;
    len = basenum + 1;
  } else {
    p0[0] = pe[0] = 0;
    len = basenum;
  }
  for (i=1;i<3;i++) {
    pb[i] = pb[i-1] + len;
    pe[i] = pe[i-1] + len;
    p0[i] = p0[i-1] + len;
  }
  if (src_type & F_ENERGY_SUP) {
    pe[0] = 0;
    for (i=1;i<3;i++) {
      pb[i]--;
      pe[i]--;
      p0[i]--;
    }
  }
  
  /* modification begin */
  /* qualifier addition: "_N" */
#ifdef DEBUG
  buf[0] = '\0';
  printf("try to add: %s\n", param_qualcode2str(buf, add_type, FALSE));
#endif
  
  if (add_type & F_ENERGY_SUP) {
    if (src_type & F_ENERGY) {
      mark_exclude_vector(pe[0], 1, vmark, vlen);
      src_type = src_type | F_ENERGY_SUP;
    } else if (src_type & F_ZEROTH) {
      mark_exclude_vector(p0[0], 1, vmark, vlen);
      src_type = src_type | F_ENERGY_SUP;
    } else {
      jlog("Warning: paramselect: \"_N\" needs \"_E\" or \"_0\". ignored\n");
    }
    add_type = add_type & (~(F_ENERGY_SUP)); /* set to 0 */
  }
  if (add_type != 0) {		/* others left */
    buf[0] = '\0';
    jlog("Warning: paramselect: can do only parameter exclusion. qualifiers %s ignored\n", param_qualcode2str(buf, add_type, FALSE));
  }
  
  /* qualifier excludeion: "_D","_A","_0","_E" */
#ifdef DEBUG
  buf[0] = '\0';
  printf("try to del: %s\n", param_qualcode2str(buf, del_type, FALSE));
#endif

  if (del_type & F_DELTA) del_type |= F_ACCL;
  /* mark delete vector */
  if (del_type & F_ACCL) {
    mark_exclude_vector(pb[2], len, vmark, vlen);
    src_type &= ~(F_ACCL);
    del_type &= ~(F_ACCL);
  }
  if (del_type & F_DELTA) {
    mark_exclude_vector(pb[1], len, vmark, vlen);
    src_type &= ~(F_DELTA);
    del_type &= ~(F_DELTA);
  }
  
  if (del_type & F_ENERGY) {
    mark_exclude_vector(pe[2], 1, vmark, vlen);
    mark_exclude_vector(pe[1], 1, vmark, vlen);
    if (!(src_type & F_ENERGY_SUP)) {
      mark_exclude_vector(pe[0], 1, vmark, vlen);
    }
    src_type &= ~(F_ENERGY | F_ENERGY_SUP);
    del_type &= ~(F_ENERGY | F_ENERGY_SUP);
  }
  if (del_type & F_ZEROTH) {
    mark_exclude_vector(p0[2], 1, vmark, vlen);
    mark_exclude_vector(p0[1], 1, vmark, vlen);
    if (!(src_type & F_ENERGY_SUP)) {
      mark_exclude_vector(p0[0], 1, vmark, vlen);
    }
    src_type &= ~(F_ZEROTH | F_ENERGY_SUP);
    del_type &= ~(F_ZEROTH | F_ENERGY_SUP);
  }
  
  if (del_type != 0) {		/* left */
    buf[0] = '\0';
    jlog("Warning: paramselect: cannot exclude qualifiers %s. selection ignored\n", param_qualcode2str(buf, del_type, FALSE));
  }


  *new_type = src_type;

  return(TRUE);
}


/** 
 * Extracts needed parameter vector specified in dst_type_arg from src,
 * and returns newly allocated parameter structure.
 * 
 * @param src [in] input parameter
 * @param dst_type_arg [in] required parameter type
 * 
 * @return newly allocated adjusted parameter, NULL on failure.
 */
static boolean
select_param_kind(HTK_Param *p, short dst_type_arg)
{
  int *vmark;
  int vlen;
  int i;
  short new_type;

  /* prepare work area */
  vmark = (int *)mymalloc(sizeof(int) * p->veclen);
  vlen = p->veclen;
  for (i=0;i<vlen;i++) {
    vmark[i] = 0;
  }

  /* mark to determine operation */
  if (select_param_vmark(p, dst_type_arg, vmark, vlen, &new_type) == FALSE) return(FALSE);
  /* execute deletion (copy needed to new param)*/
  exec_exclude_vectors(p, vmark);
  
  /* copy & set header info */
  p->header.sampsize = p->veclen * sizeof(VECT);
  p->header.samptype = new_type | (p->header.samptype & F_BASEMASK);
  
#ifdef DEBUG
 {
   char pbuf[80];
   printf("new param made: %s\n", param_code2str(pbuf, p->header.samptype, FALSE));
 }
#endif
  
  /* free work area */
  free(vmark);

  return(TRUE);
}

/** 
 * @brief  Top function to adjust parameter.
 *
 * It compares the types for the given parameter @a param and
 * %HMM definition @a hmminfo.  If type is not the same, adjustment will be
 * tried.
 *
 * @param hmminfo [in] HTK %HMM definition
 * @param param [i/o] input parameter, will be freed if adjustment was
 * performed in this function
 * @param vflag [in] if TRUE, output verbose messages
 * 
 * @return 1 on success, 0 if no adjustment needed, or -1 on failure (in case
 * parameter type does not match even by the adjustment).
 */
int
param_check_and_adjust(HTK_HMM_INFO *hmminfo, HTK_Param *param, boolean vflag)
{
  char pbuf[80],hbuf[80];
  
  param_code2str(pbuf, (short)(param->header.samptype & ~(F_COMPRESS | F_CHECKSUM)), FALSE);
  param_code2str(hbuf, hmminfo->opt.param_type, FALSE);  
  if (!check_param_basetype(hmminfo, param)) {
    /* error if base type not match */
    jlog("Error: paramselect: incompatible parameter type\n");
    jlog("Error: paramselect:  HMM   trained   by  %s(%d)\n", hbuf, hmminfo->opt.vec_size);
    jlog("Error: paramselect:  input parameter is  %s(%d)\n", pbuf, param->veclen);
    return -1;
  }
  if (!check_param_coherence(hmminfo, param)) {
    /* try to select needed parameter vector */
    if (vflag) jlog("Stat: paramselect: attaching %s\n", pbuf);
    if (select_param_kind(param, hmminfo->opt.param_type) == FALSE) {
      if (vflag) jlog("Error: paramselect: failed to attach to %s\n", hbuf);

      jlog("Error: paramselect: incompatible parameter type\n");
      jlog("Error: paramselect:  HMM   trained   by  %s(%d)\n", hbuf, hmminfo->opt.vec_size);
      jlog("Error: paramselect:  input parameter is  %s(%d)\n", pbuf, param->veclen);
      return -1;
    }
    param_code2str(pbuf, param->header.samptype, FALSE);
    if (vflag) jlog("Stat: paramselect: attached to %s\n", pbuf);
    return(1);
  }
  return(0);
}
