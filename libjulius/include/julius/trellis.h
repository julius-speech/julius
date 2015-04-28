/**
 * @file   trellis.h
 * 
 * <JA>
 * @brief  単語トレリスの構造体定義
 *
 * </JA>
 * 
 * <EN>
 * @brief  Structure definitions of word trellis.
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Fri Aug 17 18:30:17 2007
 *
 * $Revision: 1.2 $
 * 
 */

#ifndef __J_TRELLIS_H__
#define __J_TRELLIS_H__

/**
 * Word trellis element that holds survived word ends at each frame
 * on the 1st pass.
 * 
 */
typedef struct __trellis_atom__ {
  LOGPROB backscore;		///< Accumulated score from start
  LOGPROB lscore;		///< LM score of this word
  WORD_ID wid;			///< Word ID
  short begintime;		///< Beginning frame
  short endtime;		///< End frame
#ifdef WORD_GRAPH
  boolean within_wordgraph;	///< TRUE if within word graph
  boolean within_context;	///< TRUE if any of its following word was once survived in beam while search
#endif
  struct __trellis_atom__ *last_tre; ///< Pointer to previous context trellis word
  struct __trellis_atom__ *next; ///< Temporary link to store generated trellis word on 1st pass
} TRELLIS_ATOM;

/**
 * Whole word trellis (aka backtrellis) generated as a result of 1st pass.
 * 
 */
typedef struct __backtrellis__ {
  int framelen;			///< Frame length
  int *num;			///< Number of trellis words at frame [t]
  TRELLIS_ATOM ***rw;		///< List to trellis words at frame [t]: rw[t][0..num[t]]
  TRELLIS_ATOM *list;		///< Temporary storage point used in 1st pass
  BMALLOC_BASE *root;		///< memory allocation base for mybmalloc2()
} BACKTRELLIS;

#endif /*  __J_TRELLIS_H__ */
