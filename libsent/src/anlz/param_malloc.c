/**
 * @file   param_malloc.c
 * 
 * <JA>
 * @brief  特徴パラメータ構造体のメモリ管理
 * </JA>
 * 
 * <EN>
 * @brief  Memory management of input parameter vector structure.
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Fri Aug  3 14:09:39 2007
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
#include <sent/htk_param.h>

/** 
 * Initialize the content of the parameter data.
 * 
 * @param p [out] parameter data
 */
void
param_init_content(HTK_Param *p)
{
  p->samplenum = 0;
}

/** 
 * Allocate vector area for required length and frames.  Allocate memory if
 * not yet, or expand it if already allocated but not sufficient.  If
 * sufficient amount is already allocated, do nothing.
 * The allocation are updated by HTK_PARAM_INCREMENT_STEP_FRAME step to avoid
 * numerous re-allocation
 * 
 * @param p [i/o] parameter data
 * @param samplenum [in] required number of frames
 * @param veclen [in] required length of vector
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
param_alloc(HTK_Param *p, unsigned int samplenum, short veclen)
{
  unsigned int t;
  VECT **new;
  unsigned int newlen;

  if (p->parvec == NULL) {
    /* at least some length should be allocated */
    if (samplenum < HTK_PARAM_INCREMENT_STEP_FRAME) {
      p->samplenum_alloc = HTK_PARAM_INCREMENT_STEP_FRAME;
    } else {
      p->samplenum_alloc = samplenum;
    }
    /* first time: just allocate veclen x samplenum */
    p->parvec = (VECT **)mybmalloc2(sizeof(VECT *) * p->samplenum_alloc, &(p->mroot));
    for(t=0;t<p->samplenum_alloc;t++) {
      p->parvec[t] = (VECT *)mybmalloc2(sizeof(VECT) * veclen, &(p->mroot));
    }
    p->veclen_alloc = veclen;
  } else {
    /* already allocated */
    /* check required vector length */
    if (veclen > p->veclen_alloc) {
      jlog("Error: param_malloc: longer vector required, re-allocate all\n");
      jlog("Error: param_malloc: allocated = %d, required = %d\n", p->veclen_alloc, veclen);
      return FALSE;
    }
    if (samplenum > p->samplenum_alloc) {
      /* need frame expansion */
      newlen = p->samplenum_alloc;
      while(newlen < samplenum) newlen += HTK_PARAM_INCREMENT_STEP_FRAME;
      //jlog("Debug: param_malloc: parvec extend to %d\n", newlen);
      new = (VECT **)mybmalloc2(sizeof(VECT *) * newlen, &(p->mroot));
      for(t = 0; t < p->samplenum_alloc; t++) {
	new[t] = p->parvec[t];
      }
      for(t = p->samplenum_alloc; t < newlen; t++) {
	new[t] = (VECT *)mybmalloc2(sizeof(VECT) * p->veclen_alloc, &(p->mroot));
      }
      p->parvec = new;
      p->samplenum_alloc = newlen;
    }
  }
  return TRUE;
}

/** 
 * Free and clear the content of the parameter data
 * 
 * @param p [out] parameter data
 */
void
param_free_content(HTK_Param *p)
{
  mybfree2(&(p->mroot));
  p->parvec = NULL;
  p->samplenum_alloc = 0;
  param_init_content(p);
}
    
    
/** 
 * Allocate a new parameter.
 * 
 * @return pointer to the newly allocated area.
 */
HTK_Param *
new_param()
{
  HTK_Param *new;
  new = (HTK_Param *)mymalloc(sizeof(HTK_Param));
  new->mroot = NULL;
  new->parvec = NULL;
  new->samplenum_alloc = 0;
  new->is_outprob = FALSE;
  param_init_content(new);
  return(new);
}

/** 
 * Free the HTK parameter structure.
 * 
 * @param pinfo [in] parameter data to be destroyed.
 */
void
free_param(HTK_Param *pinfo)
{
  param_free_content(pinfo);
  free(pinfo);
}

