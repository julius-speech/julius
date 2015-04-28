/**
 * @file   hmm_calc.h
 *
 * <EN>
 * @brief  Work area and outprob cache for acoustic computation.
 *
 * </EN>
 * <JA>
 * @brief  音響尤度計算用ワークエリアとキャッシュ
 *
 * </JA>
 *
 * @author Akinobu LEE
 * @date   Thu Feb 10 14:54:06 2005
 *
 * $Revision: 1.11 $
 *
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __SENT_HMM_CALC_H__
#define __SENT_HMM_CALC_H__

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>

/**
 * @brief Symbols to specify which Gaussian pruning algorithm to use.
 *
 *   - GPRUNE_SEL_UNDEF: unspecified by user
 *   - GPRUNE_SEL_NONE: no pruning
 *   - GPRUNE_SEL_SAFE: safe pruning
 *   - GPRUNE_SEL_HEURISTIC: heuristic pruning
 *   - GPRUNE_SEL_BEAM: beam pruning
 *   - GPRUNE_SEL_USER: user-defined function
 * 
 */
enum{GPRUNE_SEL_UNDEF, GPRUNE_SEL_NONE, GPRUNE_SEL_SAFE, GPRUNE_SEL_HEURISTIC, GPRUNE_SEL_BEAM, GPRUNE_SEL_USER};

/**
 * @brief Score beam offset for GPRUNE_SEL_BEAM.
 *
 * Larger value may ease pruning error, but processing may become slower.
 * Smaller value can speed up the acoustic computation, but may cause error.
 * 
 */
#define TMBEAMWIDTH 5.0

/// A component of per-codebook probability cache while search
typedef struct {
  LOGPROB score;		///< Cached probability of below
  int id;		///< ID of the cached Gaussian in the codebook
} MIXCACHE;

/**
 * Work area and cache for %HMM computation
 * 
 */
typedef struct __hmmwork__{

  /* pointer to functions, selected by model configuration */

  /// Function to compute output probability with/without code book level cache
  LOGPROB (*calc_outprob)(struct __hmmwork__ *);

  /// Function to compute state output with/without GMS support
  LOGPROB (*calc_outprob_state)(struct __hmmwork__ *);

  /// Pruning function to compute likelihood of a mixture component
  void (*compute_gaussset)(struct __hmmwork__ *, HTK_HMM_Dens **g, int num, int *last_id, int lnum);

  /// Initialization function that corresponds to compute_gaussset.
  boolean (*compute_gaussset_init)(struct __hmmwork__ *);

  /// Function to Free above
  void (*compute_gaussset_free)(struct __hmmwork__ *);

  /* local storage of pointers to the HMM */
  HTK_HMM_INFO *OP_hmminfo; ///< Current %HMM definition data
  HTK_HMM_INFO *OP_gshmm; ///< Current GMS %HMM data

  /* local storage of input parameters */
  HTK_Param *OP_param;	///< Current parameter
  int OP_gprune_num; ///< Current number of computed mixtures for pruning
  int OP_time;		///< Current time
  int OP_last_time;	///< last time

  /* current computing state */
  HTK_HMM_State *OP_state;	///< Current state
  int OP_state_id;		///< Current state ID

  /* for multi-stream input */
  short OP_nstream;		///< Number of input stream (copy from header)
  VECT *OP_vec_stream[MAXSTREAMNUM]; ///< input vector for each stream at current frame
  short OP_veclen_stream[MAXSTREAMNUM]; ///< vector length for each stream

  /* temporal buffers to hold result of mixture computation at each stream */
  VECT *OP_vec;		///< Current input vector to be computed
  short OP_veclen;		///< Current vector length to be computed
  int OP_calced_maxnum; ///< Allocated length of below
  LOGPROB *OP_calced_score; ///< Scores of computed mixtures
  int *OP_calced_id; ///< IDs of computed mixtures
  int OP_calced_num; ///< Number of computed mixtures

  /* state level cache */
  int statenum;		///< Local work area that holds total number of HMM states in the %HMM definition data
  LOGPROB **outprob_cache; ///< State-level cache [t][stateid]
  int outprob_allocframenum;	///< Allocated frames of the cache
  BMALLOC_BASE *croot;	///< Root alloc pointer to state outprob cache
  LOGPROB *last_cache;	///< Local work are to hold cache list of current time

  /* mixture level cache for tied-mixture model */
  MIXCACHE ***mixture_cache; ///< Codebook cache: [time][book_id][0..computed_mixture_num]
  short **mixture_cache_num; ///< Num of mixtures to be calculated and stored in mixture_cache
  BMALLOC_BASE *mroot;	///< Root alloc pointer to state outprob cache

  /* work area for tied-mixture computation */
  int *tmix_last_id;		///< List of computed mixture id on the previous input frame
  int tmix_allocframenum;	///< Allocated frame length of codebook cache

  /* work area for gaussian pruning (common) */
  boolean *mixcalced;	///< Mark which Gaussian has been computed
  /* for beam gaussian pruning */
  LOGPROB *dimthres;	///< Threshold for each dimension (inversed)
  int dimthres_num;	///< Length of above
  /* for heuristic gaussian pruning */
  LOGPROB *backmax;	///< Backward sum of max for each dimension (inversed)
  int backmax_num;		///< Length of above

  /* work area for outprob_cd_nbest */
  LOGPROB *cd_nbest_maxprobs;	///< Work area that holds N-best state scores for pseudo state set
  int cd_nbest_maxn;		///< Allocated length of above

  /* work area for GMS */
  /* GMS variables */
  int my_nbest;		///< Number of states to be selected
  int gms_allocframenum;	///< Allocated number of frame for storing fallback scores per frame
  /* GMS info */
  GS_SET *gsset;		///< Set of GS states
  int gsset_num;		///< Num of above
  int *state2gs; ///< Mapping from triphone state id to gs id
  /* GMS results */
  boolean *gms_is_selected;	///< TRUE if the frame is already selected
  LOGPROB **fallback_score; ///< [t][gssetid], LOG_ZERO if selected
  /* GMS calculation */
  int *gsindex;		///< Index buffer
  LOGPROB *t_fs;		///< Current fallback_score
  /* GMS gprune local cache */
  int **gms_last_max_id_list;	///< maximum mixture id of last call for each states

  boolean batch_computation;

} HMMWork;  


#ifdef __cplusplus
extern "C" {
#endif

/* addlog.c */
void make_log_tbl();
LOGPROB addlog(LOGPROB x, LOGPROB y);
LOGPROB addlog_array(LOGPROB *x, int n);

/* outprob_init.c */
boolean
outprob_init(HMMWork *wrk, HTK_HMM_INFO *hmminfo,
	     HTK_HMM_INFO *gshmm, int gms_num,
	     int gprune_method, int gprune_mixnum
	     );
boolean outprob_prepare(HMMWork *wrk, int framenum);
void outprob_free(HMMWork *wrk);
void outprob_set_batch_computation(HMMWork *wrk, boolean flag);
/* outprob.c */
boolean outprob_cache_init(HMMWork *wrk);
boolean outprob_cache_prepare(HMMWork *wrk);
void outprob_cache_free(HMMWork *wrk);
LOGPROB outprob_state(HMMWork *wrk, int t, HTK_HMM_State *stateinfo, HTK_Param *param);
void outprob_cd_nbest_init(HMMWork *wrk, int num);
void outprob_cd_nbest_free(HMMWork *wrk);
LOGPROB outprob_cd(HMMWork *wrk, int t, CD_State_Set *lset, HTK_Param *param);
boolean outprob_cache_output(FILE *fp, HMMWork *wrk, int framenum);

/* gms.c */
boolean gms_init(HMMWork *wrk);
boolean gms_prepare(HMMWork *wrk, int framelen);
void gms_free(HMMWork *wrk);
LOGPROB gms_state(HMMWork *wrk);
/* gms_gprune.c */
void gms_gprune_init(HMMWork *wrk);
void gms_gprune_prepare(HMMWork *wrk);
void gms_gprune_free(HMMWork *wrk);
void compute_gs_scores(HMMWork *wrk);

/* calc_mix.c */
LOGPROB calc_mix(HMMWork *wrk);
/* calc_tied_mix.c */
boolean calc_tied_mix_init(HMMWork *wrk);
boolean calc_tied_mix_prepare(HMMWork *wrk, int framenum);
void calc_tied_mix_free(HMMWork *wrk);
LOGPROB calc_tied_mix(HMMWork *wrk);
LOGPROB calc_compound_mix(HMMWork *wrk);

/* gprune_common.c */
int cache_push(HMMWork *wrk, int id, LOGPROB score, int len);
/* gprune_none.c */
LOGPROB compute_g_base(HMMWork *wrk, HTK_HMM_Dens *binfo);
boolean gprune_none_init(HMMWork *wrk);
void gprune_none_free(HMMWork *wrk);
void gprune_none(HMMWork *wrk, HTK_HMM_Dens **g, int num, int *last_id, int lnum);
/* gprune_safe.c */
LOGPROB compute_g_safe(HMMWork *wrk, HTK_HMM_Dens *binfo, LOGPROB thres);
boolean gprune_safe_init(HMMWork *wrk);
void gprune_safe_free(HMMWork *wrk);
void gprune_safe(HMMWork *wrk, HTK_HMM_Dens **g, int gnum, int *last_id, int lnum);
/* gprune_heu.c */
boolean gprune_heu_init(HMMWork *wrk);
void gprune_heu_free(HMMWork *wrk);
void gprune_heu(HMMWork *wrk, HTK_HMM_Dens **g, int gnum, int *last_id, int lnum);
/* gprune_beam.c */
boolean gprune_beam_init(HMMWork *wrk);
void gprune_beam_free(HMMWork *wrk);
void gprune_beam(HMMWork *wrk, HTK_HMM_Dens **g, int gnum, int *last_id, int lnum);


#ifdef __cplusplus
}
#endif

#endif /* __SENT_HMM_CALC_H__ */
