/**
 * @file   htk_hmm.h
 *
 * <EN>
 * @brief Data structures for handling HTK %HMM definition
 *
 * This file defines data structures for %HMM definition file in HTK format.
 * </EN>
 * <JA>
 * @brief HTK形式の%HMMを扱うデータ構造の定義
 *
 * このファイルには, HTK形式の%HMM定義ファイルを読み込むための構造体が
 * 定義されています．
 * </JA>
 *
 * @author Akinobu LEE
 * @date   Thu Feb 10 19:36:47 2005
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

#ifndef __SENT_HTK_HMM_2_H__
#define __SENT_HTK_HMM_2_H__

#include <sent/stddefs.h>
#include <sent/htk_defs.h>
#include <sent/ptree.h>
#include <sent/mfcc.h>

/// Macro to check whether the next token is "A"
#define currentis(A)  (!strcasecmp(A, rdhmmdef_token))
/// Macro to jump to error if no token left
#define NoTokErr(S)      if (!rdhmmdef_token) rderr(S)

/// Delimiter string for parsing %HMM definition file
#define HMMDEF_DELM " \t\r\n<>"

/**
 * @defgroup hmminfo HTK HMM definition
 * <EN>
 * @brief Data structures for HTK %HMM definition
 *
 * The data is defined in each levels from model, state to Gaussian
 * components (mean and variance).  Each level unit almost corresponds
 * to the macro
 * definition in the HTK definition language.  Each data has links to
 * data of lower level, and also has a linked list to the data in the
 * same level. 
 * </EN>
 * <JA>
 * @brief HTKの%HMMを格納するためのデータ構造定義
 *
 * データ構造はモデル・状態からガウス分布の平均・分散まで各レベルごとに
 * 定義されています．各レベルはおおよそ HTK のマクロ定義に対応しています．
 * 各データは, 下位のデータ構造へのポインタ
 * および同レベルの構造体同士のリンクリストを保持しています．
 * </JA>
 *
 */
//@{
/// @ingroup hmminfo

/// Possible maximum value of state ID (in unsigned short)
#define MAX_STATE_NUM 2147483647

/// Delimiter strings/characters to generate logical triphone names
#define HMM_RC_DLIM "+"		///< Right context delimiter in string
#define HMM_LC_DLIM "-"		///< Left context delimiter in string
#define HMM_RC_DLIM_C '+'	///< Right context delimiter in character
#define HMM_LC_DLIM_C '-'	///< Left context delimiter in character

/// Default logical name of short pause model
#define SPMODEL_NAME_DEFAULT "sp"

/// Length limit of HMM name (including ones generated in Julius)
#define MAX_HMMNAME_LEN 256

/// Specify method of calculating approximated acoustic score at inter-word context pseudo phones on word edge
enum iwcd_type {
  IWCD_UNDEF,			///< not specified explicitly
  IWCD_MAX,			///< Use maximum score among context variants
  IWCD_AVG,			///< Use average score among context variants
  IWCD_NBEST			///< Use average of N-best scores among context variants
};

/* options info */

/// Stream information (although current Julius supports only single stream)
typedef struct {
  short num;			///< Number of stream
  short vsize[MAXSTREAMNUM];	///< Vector size for each stream
} HTK_HMM_StreamInfo;

/// %HMM Option
typedef struct {
  HTK_HMM_StreamInfo stream_info; ///< Stream information of this %HMM
  short vec_size;		///< Size of parameter vector in number of dimension
  short cov_type;		///< Type of covariance matrix , see also htk_defs.h
  short dur_type;		///< Type of duration , see also htk_defs.h
  short param_type;		///< Type of parameter , see also htk_defs.h
} HTK_HMM_Options;

/// %HMM transition table
typedef struct _HTK_HMM_trans {
  char *name;			///< Name (NULL if not defined as Macro)
  short statenum;		///< Number of state
  PROB **a;			///< Matrix of transition probabilities
  int id; 			///< Uniq transition id starting from 0
  struct _HTK_HMM_trans *next;  ///< Pointer to next data, NULL if last
} HTK_HMM_Trans;

/// %HMM variance data
typedef struct _HTK_HMM_variance {
  char *name;			///< Name (NULL if not defined as Macro)
  VECT *vec;			///< Covariance vector (diagonal)
  short len;			///< Length of above
  struct _HTK_HMM_variance *next; ///< Pointer to next data, NULL if last
} HTK_HMM_Var;

/// %HMM Gaussian density (or mixture) data
typedef struct _HTK_HMM_dens {
  char *name;			///< Name (NULL if not defined as Macro)
  VECT *mean;			///< Mean vector
  short meanlen;		///< Length of above
  HTK_HMM_Var *var;		///< Link to assigned variance vector
  /**
   * Constant value in log scale for calculating Gaussiann output probability.
   * @sa libsent/sec/hmminfo/rdhmmdef_dens.c
   */
  LOGPROB gconst;
  struct _HTK_HMM_dens *next;	///< Pointer to next data, NULL if last
} HTK_HMM_Dens;

/// %HMM stream weight definition
typedef struct _HTK_HMM_stream_weight {
  char *name;			///< Name (NULL for in-line definition)
  VECT *weight;			///< Weight of each stream in log scale
  short len;			///< Length of above
  struct _HTK_HMM_stream_weight *next; ///< Pointer to next data, NULL on last
} HTK_HMM_StreamWeight;

/**
 * @brief %HMM mixture PDF for a stream
 * 
 * @note
 * In a tied-mixture model, @a b points to a codebook defined as GCODEBOOK
 * intead of the array of densities.
 * 
 */
typedef struct _HTK_HMM_PDF {
  char *name;			///< Name (NULL for in-line definition)
  boolean tmix;			///< TRUE if this is assigned to tied-mixture codebook
  short stream_id;		///< Stream ID to which this pdf is assigned, begins from 0
  short mix_num;		///< Number of densities (mixtures) assigned.
  HTK_HMM_Dens **b;		///< Link array to assigned densities, or pointer to GCODEBOOK in tied-mixture model
  PROB *bweight;		///< Weights corresponding to above
  struct _HTK_HMM_PDF *next;	///< Pointer to next data, or NULL at last
} HTK_HMM_PDF;

/**
 * @brief %HMM state data
 *
 */
typedef struct _HTK_HMM_state {
  char *name;			///< Name (NULL if not defined as Macro)
  short nstream;		///< Num of stream
  HTK_HMM_StreamWeight *w;	///< Pointer to stream weight data, or NULL is not specified
  HTK_HMM_PDF **pdf;	        ///< Array of mixture PDFs for each stream
  int id; 			///< Uniq state id starting from 0 for caching of output probability
  struct _HTK_HMM_state *next;  ///< Pointer to next data, NULL if last
} HTK_HMM_State;

/// Top %HMM model, corresponds to "~h" macro in hmmdefs
typedef struct _HTK_HMM_data {
  char *name;			///< Name (NULL if not defined as Macro)
  short state_num;		///< Number of states in this model
  HTK_HMM_State **s;		///< Array of states in this model
  HTK_HMM_Trans *tr;		///< Link to assigned transition matrix
  struct _HTK_HMM_data *next;   ///< Pointer to next data, NULL if last
} HTK_HMM_Data;

/// Gaussian mixture codebook in tied-mixture model
typedef struct {
  char *name;			///< Codebook name (NULL if not defined as Macro)
  int num;			///< Number of mixtures in this codebook
  HTK_HMM_Dens **d;		///< Array of links to mixture instances
  unsigned short id;            ///< Uniq id for caching of output probability
} GCODEBOOK;
//@}

/// Set of %HMM states for Gaussian Mixture Selection
typedef struct {
  HTK_HMM_State *state;		///< Pointer to %HMM states defined for GMS
  /* GCODEBOOK *book;*/		/* pointer to the corresponding codebook in hmminfo */
} GS_SET;

/**
 * @defgroup cdset Context-Dependent HMM set
 * <EN>
 * @brief Set of %HMM states with the same base phone and state location
 *
 * This structure will be used to handle cross-word triphone on the 1st pass.
 * At a triphone %HMM at the edge of a word in the tree lexicon,
 * the state nodes should have a set of %HMM states with the same base phone of
 * all triphones at the same location instead of a single state information.
 * This context-dependent %HMM set for cross-word triphone is also
 * called as "pseudo" phone in Julius.
 * 
 * When computing the 1st pass, the maximum (or average or N-best average)
 * value from the likelihoods of state set will be taken as the output
 * probability of the states instead of the actual cross-word triphone.
 *
 *
 * This approximated value will be fixed by re-computation on the 2nd pass.
 * </EN>
 * <JA>
 * @brief 同じベース音素の同じ位置にある%HMM状態の集合
 *
 * この構造体は第１パスで単語間トライフォンを扱うのに用いられます．
 * 木構造化辞書上で，単語の末端のトライフォン%HMMにおける各状態は，
 * 通常の%HMMとは異なりその終端音素と同じベース音素を持つトライフォンの
 * 同じ位置の状態のリストを持ちます．このリスト化されたコンテキスト依存
 * %HMMの集合は，"pseudo" phone とも呼ばれます．
 *
 * 第１パス計算時には，その状態の音響尤度は，真の単語間トライフォンの
 * 近似値として，リスト中の各状態の音響尤度の最大値（あるいは平均値，
 * あるいはNbestの状態の平均値）が用いられる．
 *
 * この近似値は第２パスで再計算される．
 * </JA>
 *
 * @sa htk_hmm.h
 * @sa libsent/src/hmminfo/cdhmm.c
 * @sa libsent/src/hmminfo/cdset.c
 * @sa libsent/src/hmminfo/guess_cdHMM.c
 *
 */
//@{
/// @ingroup cdset

/// Context-dependent state set, equivalent to HTK_HMM_State, part of pseudo phone
typedef struct {
  HTK_HMM_State **s;		///< Link Array to belonging states
  unsigned short num;		///< Number of states
  unsigned short maxnum;	///< Allocated number of above
} CD_State_Set;
/**
 * @brief Context-dependent %HMM set (called "pseudo") for a logical context
 * 
 * Context-dependent %HMM set for a logical context
 * (e.g. "a-k", "e+b", "e", each corresponds to triphone list of
 * "a-k+*", "*-e+b", "*-e+*").
 */
typedef struct _cd_set{
  char *name;			///< Logical name of this %HMM set ("a-k", "e+b", "e", etc.)
  CD_State_Set *stateset;	///< Array of state set for each state location
  unsigned short state_num;	///< Number of state set
  HTK_HMM_Trans *tr;		///< Transition matrix
  struct _cd_set *next;         ///< Pointer to next data, NULL if last
} CD_Set;
/// Top structure to hold all the %HMM sets
typedef struct {
  boolean binary_malloc;	///< TRUE if read from binary
  APATNODE *cdtree;		///< Root of index tree for name lookup
} HMM_CDSET_INFO;
//@}

/**
 * @ingroup cdset
 *
 * @brief Logical %HMM to map logical names to physical/pseudo %HMM
 *
 * This data maps logical %HMM name to physical (defined) %HMM or pseudo %HMM.
 * The logical %HMM names are basically loaded from %HMMList mapping file.
 * Biphone/monophone %HMM names, not listed in the %HMMList file,
 * are mapped to pseudo phones, which represents the context-dependent %HMM
 * set.
 *
 * For example, if logical biphone %HMM name "e-k" is defined in %HMM definition
 * file or its mapping is specified in the HMMList file, the Logical %HMM name
 * "e-k" will be mapped to the corresponding defined %HMM.
 * If "e-k" does not exist in
 * both %HMM definition file and HMMList file, triphones whose name matches
 * "e-k+*" will be gathered to phone context-dependent %HMM set "e-k", and
 * the logical %HMM name "e-k" will be mapped to this %HMM set.
 *
 * The context-dependent %HMM is also called a "pseudo" phone in Julius.
 *
 */
typedef struct _HMM_logical {
  char *name;			///< Name string of this logical %HMM
  boolean is_pseudo;		///< TRUE if this is mapped to pseudo %HMM
  /// Actual body of state definition
  union {
    HTK_HMM_Data *defined;	///< pointer to the mapped physical %HMM
    CD_Set *pseudo;		///< pointer to the mapped pseudo %HMM
  } body;
  struct _HMM_logical *next;   ///< Pointer to next data, NULL if last
} HMM_Logical;

/**
 * @ingroup hmminfo
 *
 * @brief Basephone information extracted from hmminfo
 */
typedef struct {
  char *name;			///< Base phone name
  boolean bgnflag;		///< TRUE if it can appear on word beginning determined by word dictionary
  boolean endflag;		///< TRUE if it can appear on word end determined by word dictionary
} BASEPHONE;
/**
 * @ingroup hmminfo
 *
 * @brief List of all basephone in hmminfo
 */
typedef struct {
  int num;			///< Total number of base phone
  int bgnnum;			///< Number of phones that can appear on word beginning
  int endnum;			///< Number of phones that can appear on word end
  APATNODE *root;		///< Root of index tree for name lookup
} HMM_basephone;

/**
 * @ingroup hmminfo
 * 
 * @brief Top %HMM structure that holds all the HTK %HMM definition
 */
typedef struct {
  /**
   * @name %HMM definitions from hmmdefs
   */
  //@{
  HTK_HMM_Options opt;		///< Global option
  HTK_HMM_Trans *trstart;	///< Root pointer to the list of transition matrixes
  HTK_HMM_Var *vrstart;		///< Root pointer to the list of variance data
  HTK_HMM_Dens *dnstart;	///< Root pointer to the list of density (mixture) data
  HTK_HMM_PDF *pdfstart;	///< Root pointer to the list of mixture pdf data
  HTK_HMM_StreamWeight *swstart; ///< Root pointer to the list of stream weight data
  HTK_HMM_State *ststart;	///< Root pointer to the list of state data
  HTK_HMM_Data *start;		///< Root pointer to the list of models
  //@}

  /**
   * @name logical %HMM
   */
  //@{
  HMM_Logical *lgstart;		///< Root pointer to the list of Logical %HMMs
  //@}
  
  /**
   * @name Root nodes of index tree for name lookup of %HMM instances
   */
  //@{
  APATNODE *tr_root;		///< Root index node for transition matrixes
  APATNODE *vr_root;		///< Root index node for variance data
  APATNODE *sw_root;		///< Root index node for stream weight data
  APATNODE *dn_root;		///< Root index node for density data
  APATNODE *pdf_root;		///< Root index node for mixture PDF
  APATNODE *st_root;		///< Root index node for state data
  APATNODE *physical_root;	///< Root index node for defined %HMM name
  APATNODE *logical_root;	///< Root index node for logical %HMM name
  APATNODE *codebook_root;	///< Root index node for Gaussian codebook of tied mixture %HMM
  //@}

  /**
   * @name Information extracted from %HMM instances
   */
  //@{
  HMM_basephone basephone;	///< Base phone names extracted from logical %HMM
  HMM_CDSET_INFO cdset_info;	///< Context-dependent pseudo phone set
  //@}
  
  /**
   * @name Misc. model information
   */
  //@{
  boolean need_multipath; ///< TRUE if this model needs multipath handling
  boolean multipath;		///< TRUE if this model is treated in multipath mode
  boolean is_triphone;		///< TRUE if this is triphone model
  boolean is_tied_mixture;	///< TRUE if this is tied-mixture model
  short cdset_method;		///< Selected method of computing pseudo phones in iwcd_type
  short cdmax_num;		///< Number of N-best states when IWCD_NBEST
  HMM_Logical *sp;		///< Link to short pause model
  LOGPROB iwsp_penalty;		///< Extra ransition penalty for interword skippable short pause insertion for multi-path mode
  boolean variance_inversed;	///< TRUE if variances are inversed
  
  int totaltransnum;		///< Total number of transitions
  int totalmixnum;		///< Total number of defined mixtures
  int totalstatenum;		///< Total number of states
  int totalhmmnum;		///< Total number of physical %HMM
  int totallogicalnum;		///< Total number of logical %HMM
  int totalpseudonum;		///< Total number of pseudo %HMM
  int totalpdfnum;		///< Total number of mixture PDF
  int codebooknum;		///< Total number of codebook on tied-mixture model
  int maxcodebooksize;		///< Maximum size of codebook on tied-mixture model
  int maxmixturenum;		///< Maximum number of Gaussian per mixture
  int maxstatenum;		///< Maximum number of state per model

  BMALLOC_BASE *mroot;		///< Pointer for block memory allocation
  BMALLOC_BASE *lroot;		///< Pointer for block memory allocation for logical HMM
  BMALLOC_BASE *cdset_root;		///< Pointer for block memory allocation for logical HMM

  int *tmp_mixnum;		///< Work area for state reading

#ifdef ENABLE_MSD
  boolean has_msd;		///< TRUE if this model contains MSD part
#endif

  void *hook;			///< General purpose hook

  //@}
} HTK_HMM_INFO;


#ifdef __cplusplus
extern "C" {
#endif

/* init_phmm.c */
void htk_hmm_set_pause_model(HTK_HMM_INFO *hmminfo, char *spmodel_name);
/* rdhmmdef.c */
void rderr(char *str);
char *read_token(FILE *fp);
boolean rdhmmdef(FILE *, HTK_HMM_INFO *);
void htk_hmm_inverse_variances(HTK_HMM_INFO *hmm);
#ifdef ENABLE_MSD
void htk_hmm_check_msd(HTK_HMM_INFO *hmm);
#endif
boolean htk_hmm_check_sid(HTK_HMM_INFO *hmm);
/* rdhmmdef_options.c */
boolean set_global_opt(FILE *fp, HTK_HMM_INFO *hmm);
char *get_cov_str(short covtype);
char *get_dur_str(short durtype);
/* rdhmmdef_trans.c */
void trans_add(HTK_HMM_INFO *hmm, HTK_HMM_Trans *newParam);
HTK_HMM_Trans *get_trans_data(FILE *, HTK_HMM_INFO *);
void def_trans_macro(char *, FILE *, HTK_HMM_INFO *);
/* rdhmmdef_state.c */
HTK_HMM_State *get_state_data(FILE *, HTK_HMM_INFO *);
void def_state_macro(char *, FILE *, HTK_HMM_INFO *);
HTK_HMM_State *state_lookup(HTK_HMM_INFO *hmm, char *keyname);
void state_add(HTK_HMM_INFO *hmm, HTK_HMM_State *newParam);
/* rdhmmdef_mpdf.c */
void mpdf_add(HTK_HMM_INFO *hmm, HTK_HMM_PDF *newParam);
HTK_HMM_PDF *mpdf_lookup(HTK_HMM_INFO *hmm, char *keyname);
HTK_HMM_PDF *get_mpdf_data(FILE *fp, HTK_HMM_INFO *hmm, int mix_num, short stream_id);
void def_mpdf_macro(char *name, FILE *fp, HTK_HMM_INFO *hmm);
/* rdhmmdef_dens.c */
HTK_HMM_Dens *get_dens_data(FILE *, HTK_HMM_INFO *);
void def_dens_macro(char *, FILE *, HTK_HMM_INFO *);
HTK_HMM_Dens *dens_lookup(HTK_HMM_INFO *hmm, char *keyname);
void dens_add(HTK_HMM_INFO *hmm, HTK_HMM_Dens *newParam);
/* rdhmmdef_var.c */
HTK_HMM_Var *get_var_data(FILE *, HTK_HMM_INFO *);
void def_var_macro(char *, FILE *, HTK_HMM_INFO *);
void var_add(HTK_HMM_INFO *hmm, HTK_HMM_Var *newParam);
/* rdhmmdef_streamweight.c */
HTK_HMM_StreamWeight *get_streamweight_data(FILE *fp, HTK_HMM_INFO *hmm);
void def_streamweight_macro(char *, FILE *, HTK_HMM_INFO *);
void sw_add(HTK_HMM_INFO *hmm, HTK_HMM_StreamWeight *newParam);
/* rdhmmdef_data.c */
void def_HMM(char *, FILE *, HTK_HMM_INFO *);
HTK_HMM_Data *htk_hmmdata_new(HTK_HMM_INFO *);
void htk_hmmdata_add(HTK_HMM_INFO *hmm, HTK_HMM_Data *newParam);
/* rdhmmdef_tiedmix.c */
void tmix_read(FILE *fp, HTK_HMM_PDF *mpdf, HTK_HMM_INFO *hmm);
void codebook_add(HTK_HMM_INFO *hmm, GCODEBOOK *newParam);
/* rdhmmdef_regtree.c */
void def_regtree_macro(char *name, FILE *fp, HTK_HMM_INFO *hmm);
/* rdhmmdef_hmmlist.c */
boolean rdhmmlist(FILE *fp, HTK_HMM_INFO *hmminfo);
boolean save_hmmlist_bin(FILE *fp, HTK_HMM_INFO *hmminfo);
boolean load_hmmlist_bin(FILE *fp, HTK_HMM_INFO *hmminfo);

/* put_htkdata_info.c */
void put_htk_trans(FILE *fp, HTK_HMM_Trans *t);
void put_htk_var(FILE *fp, HTK_HMM_Var *v);
void put_htk_dens(FILE *fp, HTK_HMM_Dens *d);
void put_htk_mpdf(FILE *fp, HTK_HMM_PDF *m);
void put_htk_state(FILE *fp, HTK_HMM_State *s);
void put_htk_hmm(FILE *fp, HTK_HMM_Data *h);
void put_logical_hmm(FILE *fp, HMM_Logical *l);
void print_hmmdef_info(FILE *fp, HTK_HMM_INFO *);

HTK_HMM_INFO *hmminfo_new();
boolean hmminfo_free(HTK_HMM_INFO *);
boolean init_hmminfo(HTK_HMM_INFO *hmminfo, char *filename, char *mapfile, Value *para);
HTK_HMM_Data *htk_hmmdata_lookup_physical(HTK_HMM_INFO *, char *);
HMM_Logical *htk_hmmdata_lookup_logical(HTK_HMM_INFO *, char *);
void hmm_add_physical_to_logical(HTK_HMM_INFO *);
void hmm_add_pseudo_phones(HTK_HMM_INFO *hmminfo);
/* chkhmmlist.c */
void make_hmm_basephone_list(HTK_HMM_INFO *hmminfo);

/* HMM type check functions */
boolean htk_hmm_has_several_arc_on_edge(HTK_HMM_INFO *hmminfo);
boolean check_hmm_limit(HTK_HMM_Data *dt);
boolean check_all_hmm_limit(HTK_HMM_INFO *hmm);
boolean check_hmm_options(HTK_HMM_INFO *hmm);
boolean is_skippable_model(HTK_HMM_Data *d);

/* CCD related */
boolean guess_if_cd_hmm(HTK_HMM_INFO *hmm);
HMM_Logical *get_right_context_HMM(HMM_Logical *base, char *rc_name, HTK_HMM_INFO *hmminfo);
HMM_Logical *get_left_context_HMM(HMM_Logical *base, char *lc_name, HTK_HMM_INFO *hmminfo);
void add_right_context(char name[], char *rc);
void add_left_context(char name[], char *lc);
char *center_name(char *hmmname, char *buf);
char *leftcenter_name(char *hmmname, char *buf);
char *rightcenter_name(char *hmmname, char *buf);

/* CD_SET related */
boolean regist_cdset(APATNODE **root, HTK_HMM_Data *d, char *cdname, BMALLOC_BASE **mroot);
boolean make_cdset(HTK_HMM_INFO *hmminfo);
void put_all_cdinfo(HTK_HMM_INFO *hmminfo);
void free_cdset(APATNODE **root, BMALLOC_BASE **mroot);
CD_Set *cdset_lookup(HTK_HMM_INFO *hmminfo, char *cdstr);
CD_Set *lcdset_lookup_by_hmmname(HTK_HMM_INFO *hmminfo, char *hmmname);
CD_Set *rcdset_lookup_by_hmmname(HTK_HMM_INFO *hmminfo, char *hmmname);
int hmm_logical_state_num(HMM_Logical *lg);
HTK_HMM_Trans *hmm_logical_trans(HMM_Logical *lg);

#include <sent/htk_param.h>
boolean check_param_coherence(HTK_HMM_INFO *hmm, HTK_Param *pinfo);
boolean check_param_basetype(HTK_HMM_INFO *hmm, HTK_Param *pinfo);
int param_check_and_adjust(HTK_HMM_INFO *hmm, HTK_Param *pinfo, boolean vflag);


/* binary format */
boolean write_binhmm(FILE *fp, HTK_HMM_INFO *hmm, Value *para);
boolean read_binhmm(FILE *fp, HTK_HMM_INFO *hmm, boolean gzfile_p, Value *para);

#ifdef __cplusplus
}
#endif


#endif /* __SENT_HTK_HMM_2_H__ */
