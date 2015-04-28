/**
 * @file   jconf.h
 * 
 * <JA>
 * @brief  Jconf 構造体の定義
 *
 * </JA>
 * 
 * <EN>
 * @brief  Jconf structure
 *
 * </EN>
 * 
 * <pre>
 * JCONF
 *   +- JCONF_AM[] (linked list)
 *   +- JCONF_LM[] (linked list)
 *   +- JCONF_SEARCH[] (linked list) -> each has pointer to *JCONF_AM, *JCONF_LM
 *   +- JCONF_AM for GMM
 *   +- (engine configurations)
 * </pre>
 * 
 * @author Akinobu Lee
 * @date   Fri Feb 16 13:42:28 2007
 *
 * $Revision: 1.20 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/*
*/

#ifndef __J_JCONF_H__
#define __J_JCONF_H__

#include <sent/stddefs.h>
#include <sent/hmm.h>
#include <sent/vocabulary.h>
#include <sent/ngram2.h>
#include <sent/dfa.h>
#include <sent/mfcc.h>
#include <julius/wchmm.h>
#include <julius/search.h>
#include <julius/callback.h>

/**
 * Maximum length of module name string
 * 
 */
#define JCONF_MODULENAME_MAXLEN 64

/**
 * Default module name if not specified (in case of old jconf)
 * 
 */
#define JCONF_MODULENAME_DEFAULT "_default"

/**
 * Configuration parameters (AM)
 * 
 */
typedef struct __jconf_am__ {

  /**
   * Unique ID
   * 
   */
  short id;

  /**
   * Unique name
   * 
   */
  char name[JCONF_MODULENAME_MAXLEN];

  /**
   * HMM definition file (-h)
   */
  char *hmmfilename;
  /**
   * HMMList file to map logical (tri)phones to physical models (-hlist)
   */
  char *mapfilename;
  /**
   * Gaussian pruning method (-gprune)
   * Default: use value from compile-time engine configuration default.
   */
  int gprune_method;
  /**
   * Number of Gaussian to compute per mixture on Gaussian pruning (-tmix)
     */
  int mixnum_thres;   
  /**
   * Logical HMM name of short pause model (-spmodel)
   * Default: "sp"
   */
  char *spmodel_name;
  /**
   * GMS: HMM definition file for GMS (-gshmm)
   */
  char *hmm_gs_filename;
  /**
   * GMS: number of mixture PDF to select (-gsnum)
   */
  int gs_statenum;    

  /**
   * Calculation method for outprob score of a lcdset on cross-word
   * triphone (-iwcd1) 
   */
  short iwcdmethod;
  
  /**
   * N-best states to be calculated on IWCD_NBEST (-iwcd1 best N)
   */
  short iwcdmaxn;

  /**
   * Transition penalty of inter-word short pause (-iwsppenalty)
   * for multi-path mode
   */
  LOGPROB iwsp_penalty; 

  /**
   * force multipath mode
   * 
   */
  boolean force_multipath;

  /**
   * Acoustic Analysis Conditions.  Parameter setting priority is:
   * user-specified > specified HTK Config > model-embedded > Julius default.
   * 
   */
  struct {
    /**
     * All MFCC computation parameters, actually used for recognition.
     */
    Value para;         
    /**
     * default parameters of Julius
     */
    Value para_default;
    /**
     * parameters from binhmm header
     */
    Value para_hmm;             
    /**
     * parameters from HTK Config (-htkconf)
     */
    Value para_htk;     
    /**
     * CMN: load initial cepstral mean from file at startup (-cmnload)
     */
    char *cmnload_filename;
    /**
     * CMN: update cepstral mean while recognition
     * (-cmnnoupdate to unset)
     */
    boolean cmn_update;
    /**
     * CMN: save cepstral mean to file at end of every recognition (-cmnsave)
     */
    char *cmnsave_filename;     
    /**
     * CMN: MAP weight for initial cepstral mean on (-cmnmapweight)
     */
    float cmn_map_weight;

  } analysis;


  /**
   * Frontend processing parameter for this AM
   * 
   */
  struct {
    /**
     * Alpha coefficient for spectral subtraction
     * 
     */
    float ss_alpha;

    /**
     * Flooring coefficient for spectral subtraction
     * 
     */
    float ss_floor;

    /**
     * SS: compute noise spectrum from head silence on file input (-sscalc)
     */
    boolean sscalc;

    /**
     * With "-sscalc", specify noise length at input head in msec (-sscalclen)
     */
    int sscalc_len;

    /**
     * Load noise spectrum data from file (-ssload), that was made by "mkss".
     */
    char *ssload_filename;
  } frontend;

  /**
   * Plugin source ID when using plugin (gprune_method is GPRUNE_SEL_USER)
   */
  int gprune_plugin_source;

  /* pointer to next instance */
  struct __jconf_am__ *next;

} JCONF_AM;

/**
 * Name lister for language model configurations
 * 
 */
typedef struct __jconf_lm_namelist__ {
  /**
   * Entry name
   */
  char *name;
  /**
   * Pointer to next object
   */
  struct __jconf_lm_namelist__ *next;

} JCONF_LM_NAMELIST;

/**
 * Language models (N-gram / DFA), dictionary, and related parameters.
 * 
 */
typedef struct __jconf_lm__ {

  /**
   * Unique ID
   * 
   */
  short id;

  /**
   * Unique name
   * 
   */
  char name[JCONF_MODULENAME_MAXLEN];

  /**
   * Language model type: one of LM_UNDEF, LM_NGRAM, LM_DFA
   * 
   */
  int lmtype;

  /**
   * Variation type of language model: one of LM_NGRAM, LM_DFA_GRAMMAR,
   * LM_DFA_WORD
   * 
   */
  int lmvar;

  /**
   * Word dictionary file (-v)
   */
  char *dictfilename;
  
  /**
   * Silence word to be placed at beginning of speech (-silhead) for N-gram
   */
  char *head_silname;
  /**
   * Silence word to be placed at end of search (-siltail) for N-gram
   */
  char *tail_silname;
  
  /**
   * Skip error words in dictionary and continue (-forcedict)
   */
  boolean forcedict_flag;
  
  /**
   * N-gram in binary format (-d)
   */
  char *ngram_filename;
  /**
   * LR 2-gram in ARPA format (-nlr)
   */
  char *ngram_filename_lr_arpa;
  /**
   * RL 3-gram in ARPA format (-nrl)
   */
  char *ngram_filename_rl_arpa;
  
  /**
   * DFA grammar file (-dfa, for single use)
   */
  char *dfa_filename;
  
  /**
   * List of grammars to be read at startup (-gram) (-gramlist)
   */
  GRAMLIST *gramlist_root;
  
  /**
   * List of word lists to be read at startup (-w) (-wlist)
   */
  GRAMLIST *wordlist_root;
  
  /**
   * Enable inter-word short pause handling on multi-path version (-iwsp)
   * for multi-path mode
   */
  boolean enable_iwsp; 
  
  /**
   * Enable automatic addition of "short pause word" to the dictionary
   * (-iwspword) for N-gram
   */
  boolean enable_iwspword;
  /**
   * Dictionary entry to be added on "-iwspword" (-iwspentry) for N-gram
   */
  char *iwspentry;
  
#ifdef SEPARATE_BY_UNIGRAM
  /**
   * Number of best frequency words to be separated (linearized)
   * from lexicon tree (-sepnum)
   */
  int separate_wnum;
#endif

  /**
   * For isolated word recognition mode: name of head silence model
   */
  char wordrecog_head_silence_model_name[MAX_HMMNAME_LEN];
  /**
   * For isolated word recognition mode: name of tail silence model
   */
  char wordrecog_tail_silence_model_name[MAX_HMMNAME_LEN];
  /**
   * For isolated word recognition mode: name of silence as phone context
   */
  char wordrecog_silence_context_name[MAX_HMMNAME_LEN];

  /**
   * Name string of Unknown word for N-gram
   */
  char unknown_name[UNK_WORD_MAXLEN];

  /**
   * List of additional dictionary files
   */
  JCONF_LM_NAMELIST *additional_dict_files;

  /**
   * List of additional dictionary entries
   */
  JCONF_LM_NAMELIST *additional_dict_entries;

  /**
   * Pointer to next instance
   * 
   */
  struct __jconf_lm__ *next;
  
} JCONF_LM;

/**
 * Search parameters
 * 
 */
typedef struct __jconf_search__ {

  /**
   * Unique ID
   * 
   */
  short id;

  /**
   * Unique name
   * 
   */
  char name[JCONF_MODULENAME_MAXLEN];

  /**
   * Which AM configuration to refer
   * 
   */
  JCONF_AM *amconf;

  /**
   * Which LM configuration to refer
   * 
   */
  JCONF_LM *lmconf;

  /**
   * Compute only 1pass (-1pass)
   */
  boolean compute_only_1pass;
    
  /**
   * context handling
   */
  boolean ccd_handling;
    
  /**
   * force context-dependent handling
   */
  boolean force_ccd_handling;

  /**
   * LM weights
   * 
   */
  struct {
    /**
     * N-gram Language model weight (-lmp)
     */
    LOGPROB lm_weight;  
    /**
     * N-gram Word insertion penalty (-lmp)
     */
    LOGPROB lm_penalty; 
    /**
   * N-gram Language model weight for 2nd pass (-lmp2)
   */
    LOGPROB lm_weight2; 
    /**
     * N-gram Word insertion penalty for 2nd pass (-lmp2)
     */
    LOGPROB lm_penalty2;        
    /**
     * N-gram Additional insertion penalty for transparent words (-transp)
     */
    LOGPROB lm_penalty_trans;
    
    /**
     * Word insertion penalty for DFA grammar on 1st pass (-penalty1)
     */
    LOGPROB penalty1;
    /**
     * Word insertion penalty for DFA grammar on 2nd pass (-penalty2)
     */
    LOGPROB penalty2;
    
    /**
     * INTERNAL: TRUE if -lmp2 specified
     */
    boolean lmp2_specified;
    
    /**
     * INTERNAL: TRUE if -lmp specified
     */
    boolean lmp_specified;
  } lmp;
    
  /**
   * First pass parameters
   * 
   */
  struct {
    /**
     * Beam width of rank pruning for the 1st pass. If value is -1
     * (not specified), system will guess the value from dictionary
     * size.  If 0, a possible maximum value will be assigned to do
     * full search.
     */
    int specified_trellis_beam_width;

#ifdef SCORE_PRUNING
    /**
     * Another beam width for score pruning at the 1st pass. If value
     * is -1, or not specified, score pruning will be disabled.
     */
#endif
    LOGPROB score_pruning_width;
    
#if defined(WPAIR) && defined(WPAIR_KEEP_NLIMIT)
    /**
     * Keeps only N token on word-pair approximation (-nlimit)
     */
    int wpair_keep_nlimit;
#endif

#ifdef HASH_CACHE_IW
    /**
     * Inter-word LM cache size rate (-iwcache)
     */
    int iw_cache_rate;
#endif

    /**
     * (DEBUG) use old build_wchmm() instead of build_wchmm2() for lexicon
     * construction (-oldtree)
     */
    boolean old_tree_function_flag;

#ifdef DETERMINE
    /**
     * (EXPERIMENTAL) score threshold between maximum node score and
     * maximum word end score for early word determination
     * 
     */
    LOGPROB determine_score_thres;

    /**
     * (EXPERIMENTAL) frame duration threshold for early word determination
     * 
     */
    int determine_duration_thres;

#endif /* DETERMINE */


  } pass1;

  /**
   * Second pass parameters
   * 
   */
  struct {
    /**
     * Search until N-best sentences are found (-n). Also see "-output".
     */
    int nbest;                
    /**
     * Word beam width of 2nd pass. -1 means no beaming (-b2)
     */
    int enveloped_bestfirst_width;
#ifdef SCAN_BEAM
    /**
     * Score beam threshold of 2nd pass (-sb)
     */
    LOGPROB scan_beam_thres;
#endif
    /**
     * Hypothesis overflow threshold at 2nd pass (-m)
     */
    int hypo_overflow;
    /**
     * Hypothesis stack size of 2nd pass (-s)
     */
    int stack_size;
    /**
     * Get next words from word trellis with a range of this frames
     * on 2nd pass (-lookuprange)
     */
    int lookup_range;
    
    /**
     * Limit expansion words for trellis words on neighbor frames
     * at 2nd pass of DFA for speedup (-looktrellis)
     */
    boolean looktrellis_flag;
    
  } pass2;

  /**
   * Word graph output
   * 
   */
  struct {

    /**
     * GraphOut: if enabled, graph search is enabled.
     * 
     */
    boolean enabled;

    /**
     * GraphOut: if enabled, output word graph
     * 
     */
    boolean lattice;

    /**
     * GraphOut: if enabled, generate confusion network
     * 
     */
    boolean confnet;

    /**
     * GraphOut: allowed margin for post-merging on word graph generation
     * (-graphrange) if set to -1, same word with different phone context
     * will be separated.
     */
    int graph_merge_neighbor_range;

#ifdef   GRAPHOUT_DEPTHCUT
    /**
     * GraphOut: density threshold to cut word graph at post-processing.
     * (-graphcut)  Setting larger value is safe for all condition.
     */
    int graphout_cut_depth;
#endif

#ifdef   GRAPHOUT_LIMIT_BOUNDARY_LOOP
    /**
     * GraphOut: limitation of iteration loop for word boundary adjustment
     * (-graphboundloop)
     */
    int graphout_limit_boundary_loop_num;
#endif

#ifdef GRAPHOUT_SEARCH_DELAY_TERMINATION
    /**
     * GraphOut: delay the termination of search on graph merging until
     * at least one sentence candidate is found
     * (-graphsearchdelay / -nographsearchdelay)
     */
    boolean graphout_search_delay;
#endif

  } graph;
  
  /**
   * Successive decoding (--enable-sp-segment)
   * 
   */
  struct {

    /**
     * TRUE if short-pause segmentation enabled for this instance
     * 
     */
    boolean enabled;

    /**
     * Length threshold to detect short-pause segment in frames
     */
    int sp_frame_duration;

    /**
     * name string of pause model
     * 
     */
    char *pausemodelname;

#ifdef SPSEGMENT_NAIST
    /**
     * Backstep margin when speech trigger detected by NAIST short-pause
     * detection system
     * 
     */
    int sp_margin;

    /**
     * Delay frame of speech trigger detection in NAIST short-pause
     * detection system
     * 
     */
    int sp_delay;
#endif

  } successive;


  /**
   * Annotation to the output
   * 
   */
  struct {

#ifdef CONFIDENCE_MEASURE
    /**
     * Scaling factor for confidence scoring (-cmalpha)
     */
    LOGPROB cm_alpha;

#ifdef   CM_MULTIPLE_ALPHA
    /**
     * Begin value of alpha
     */
    LOGPROB cm_alpha_bgn;
    /**
     * End value of alpha
     */
    LOGPROB cm_alpha_end;
    /**
     * Number of test values (will be set from above values)
     */
    int cm_alpha_num;
    /**
     * Step value of alpha
     */
    LOGPROB cm_alpha_step;
#endif

#ifdef   CM_SEARCH_LIMIT
    /**
     * Cut-off threshold for generated hypo. for confidence decoding (-cmthres)
     */
    LOGPROB cm_cut_thres;
#endif

#ifdef   CM_SEARCH_LIMIT_POPO
    /**
     * Cut-off threshold for popped hypo. for confidence decoding (-cmthres2)
     */
    LOGPROB cm_cut_thres_pop;
#endif

#endif /* CONFIDENCE_MEASURE */


    /**
     * Forced alignment: per word (-walign)
     */
    boolean align_result_word_flag;
    /**
     * Forced alignment: per phoneme (-palign)
     */
    boolean align_result_phoneme_flag;
    /**
     * Forced alignment: per state (-salign)
     */
    boolean align_result_state_flag;

  } annotate;

  /**
   * Output configurations
   * 
   */
  struct {
    /**
     * Result: number of sentence to output (-output) , also see @a nbest (-n).
     */
    int output_hypo_maxnum;
    /**
     * Result: output partial recognition result on the 1st pass (-progout)
     */
    boolean progout_flag;
    /**
     * Result: Progressive output interval on 1st pass in msec (-proginterval)
     */
    int progout_interval;
    /**
     * Result: INTERNAL: interval in number of frames
     */
    int progout_interval_frame;

    /**
     * Get results for all grammars independently on 2nd pass on DFA
     * (-multigramout / -nomultigramout)
     */
    boolean multigramout_flag;

  } output;

  /**
   * Misc. switches
   * 
   */
  struct {
    /**
     * Enter trellis interactive check routine after boot (-check trellis)
     */
    boolean trellis_check_flag;
    /**
     * Enter triphone existence check routine after boot (-check triphone)
     */
    boolean triphone_check_flag;
    /**
     * Enter lexicon structure consulting mode after boot (-check wchmm)
     */
    boolean wchmm_check_flag;
    /**
     * should be set to TRUE at startup when this process should start
     * with inactive status
     * 
     */
    boolean start_inactive;
    /**
     * In case the 2nd pass search fails, this option specifies Julius
     * to use the result of the previous 1st pass as final result.
     * When this is TRUE, no RECOGFAIL occur.
     * 
     */
    boolean fallback_pass1_flag;
    
  } sw;

#ifdef USE_MBR
  struct {

    /* Rescoring sentence on MBR (-mbr) */
    boolean use_mbr;

    /* Use word weight on MBR (-mbr_wwer) */
    boolean use_word_weight;

    /* Likelihood weight */
    float score_weight;

    /* Loss function weight */
    float loss_weight;

  } mbr;
#endif

  /* pointer to next instance */
  struct __jconf_search__ *next;

} JCONF_SEARCH;

/**
 * Configuration parameters (global)
 * 
 */
typedef struct __Jconf__ {

  /**
   * Input source information, gathered from all AM conf.
   */
  struct {

    /**
     * Input source type. (waveform / mfc)
     * 
     */
    int type;

    /**
     * Input source.
     * 
     */
    int speech_input;

    /**
     * Input device.
     * 
     */
    int device;

    /**
     * id of the selected plug-in if using plugin
     * 
     */
    int plugin_source;

    /**
     * Sampling frequency
     * 
     */
    int sfreq;
    /**
     * Sampling period in 100ns units
     * 
     */
    int period;
    /**
     * Window size in samples, similar to WINDOWSIZE in HTK (unit is different)
     * 
     */
    int framesize;
    /**
     * Frame shift length in samples
     * 
     */
    int frameshift;

    /**
     * Use 48kHz input and perform down sampling to 16kHz (-48)
     */
    boolean use_ds48to16;
    /**
     * List of input files for rawfile / mfcfile input (-filelist) 
     */
    char *inputlist_filename;
    /**
     * Port number for adinnet input (-adport)
     */
    int adinnet_port;
#ifdef USE_NETAUDIO
    /**
     * Host/unit name for NetAudio/DatLink input (-NA)
     */
    char *netaudio_devname;
#endif
    /**
     * Check input parameter type with header of the hmmdefs
     * for parameter file input.  FALSE avoids the check.
     */
    boolean paramtype_check_flag;

  } input;

  /**
   * Configurations for Voice activity detection
   * 
   */
  struct {
    /**
     * Input level threshold from 0 to 32767 (-lv)
     */
    int level_thres;
    /**
     * Head margin in msec (-headmargin)
     */
    int head_margin_msec;
    /**
     * Tail margin in msec (-tailmargin)
     */
    int tail_margin_msec;
    /**
     * Zero cross number threshold per a second (-zc)
     */
    int zero_cross_num;
    /**
     * Silence detection and cutting: 0=off, 1=on, 2=accept device default
     * (-cutsilence / -nocutsilence)
     */
    int silence_cut;
    /**
     * Chunk size in samples, i.e. processing unit for audio input
     * detection.  Segmentation will be done by this unit.
     * 
     */
    int chunk_size;
#ifdef GMM_VAD
    /**
     * (GMM_VAD) Backstep margin when speech trigger is detected.
     * 
     */
    int gmm_margin;
    /**
     * (GMM_VAD) Up trigger threshold of GMM likelihood, where GMM
     * likelihood is defined as \[ \max_{m \in M_v} p(x|m) - \max_{m
     * \in M_n} p(x|m) \] where $M_v$ is a set of voice GMM, and $M_n$
     * is a set of noise GMM whose names are specified by
     * "-gmmreject".  Julius calculate this value for each input
     * frame, and average it for the last gmm_margin frames, and when
     * the value gets higher than this value, Julius will start recognition.
     */
    float gmm_uptrigger_thres;
    /**
     * (GMM_VAD) Down trigger threshold of GMM likelihood, where GMM
     * likelihood is defined as \[ \max_{m \in M_v} p(x|m) - \max_{m
     * \in M_n} p(x|m) \] where $M_v$ is a set of voice GMM, and $M_n$
     * is a set of noise GMM whose names are specified by
     * "-gmmreject".  Julius calculate this value for each input
     * frame, and average it for the last gmm_margin frames, and when
     * the value gets lower than this value, Julius will stop recognition.
     */
    float gmm_downtrigger_thres;
#endif
  } detect;

  /**
   * Pre-processing parameters before frontends
   * 
   */
  struct {

    /**
     * Strip off zero samples (-nostrip to unset)
     */
    boolean strip_zero_sample;

    /**
     * Remove DC offset by zero mean (-zmean / -nozmean)
     */
    boolean use_zmean;

    /**
     * Input level scaling factor (-lvscale)
     */
    float level_coef;

  } preprocess;

  /**
   * Models and parameters for input rejection
   * 
   */
  struct {
    /**
     * GMM definition file (-gmm)
     */
    char *gmm_filename;
    /**
     * Number of Gaussians to be computed on GMM calculation (-gmmnum)
     */
    int gmm_gprune_num;
    /**
     * Comma-separated list of GMM model name to be rejected (-gmmreject)
     */
    char *gmm_reject_cmn_string;
    /**
     * Length threshold to reject input (-rejectshort)
     */
    int rejectshortlen;
    /**
     * Length threshold to reject input (-rejectlong)
     */
    int rejectlonglen;
#ifdef POWER_REJECT
    /**
     * Rejection power threshold
     * 
     */
    float powerthres;
#endif
  } reject;

  /**
   * decoding parameters to control recognition process (global)
   * 
   */
  struct {
    /**
     * INTERNAL: do on-the-fly decoding if TRUE (value depends on
     * device default and forced_realtime.
     */
    boolean realtime_flag;    
    
    /**
     * INTERNAL: TRUE if either of "-realtime" or "-norealtime" is
     * explicitly specified by user.  When TRUE, the user-specified value
     * in forced_realtime will be applied to realtime_flag.
     */
    boolean force_realtime_flag;
    
    /**
     * Force on-the-fly decoding on 1st pass with audio input and
     * MAP-CMN (-realtime / -norealtime)
     */
    boolean forced_realtime;
    
    /**
     * TRUE if a kind of speech segmentation is enabled
     * 
     */
    boolean segment;

  } decodeopt;

  /**
   * Configurations for acoustic models (HMM, HMMList) and am-specific
   * parameters
   * 
   */
  JCONF_AM *am_root;

  /**
   * Language models (N-gram / DFA), dictionary, and related parameters.
   * 
   */
  JCONF_LM *lm_root;

  /**
   * Search parameters (LM/AM independent), annotation,
   * and output parameters
   * 
   */
  JCONF_SEARCH *search_root;

  /**
   * Current JCONF_AM for reading options
   * 
   */
  JCONF_LM *lmnow;
  /**
   * Current JCONF_AM for reading options
   * 
   */
  JCONF_AM *amnow;
  /**
   * Current JCONF_AM for reading options
   * 
   */
  JCONF_SEARCH *searchnow;

  /**
   * Config parameters for GMM computation.
   * (only gmmconf->analysis.* is used)
   * 
   */
  JCONF_AM *gmm;

  /**
   * Current option declaration mode while loading options
   * 
   */
  short optsection;

  /**
   * Whether option sectioning ristriction should be applied or not
   * 
   */
  boolean optsectioning;

  /*
   * Filename to save state probability output
   *
   */
  char *outprob_outfile;

} Jconf;

enum {
  JCONF_OPT_DEFAULT,
  JCONF_OPT_GLOBAL,
  JCONF_OPT_AM,
  JCONF_OPT_LM,
  JCONF_OPT_SR,
  SIZEOF_JCONF_OPT
};

#endif /* __J_JCONF_H__ */

/*

=======================================================
  An OLD variable name mapping from old global.h to common.h

  These data are bogus, left here only for reference
=======================================================

result_reorder_flag -> DELETED
adinnet_port ->jconf.input.adinnet_port
align_result_phoneme_flag ->jconf.annotate.align_result_phoneme_flag
align_result_state_flag ->jconf.annotate.align_result_state_flag
align_result_word_flag ->jconf.annotate.align_result_word_flag
backmax ->recog.backmax
backtrellis ->recog.backtrellis
ccd_flag ->jconf.am.ccd_flag
ccd_flag_force ->jconf.am.ccd_flag_force
cm_alpha ->jconf.annotate.cm_alpha
cm_alpha_bgn ->jconf.annotate.cm_alpha_bgn
cm_alpha_end ->jconf.annotate.cm_alpha_end
cm_alpha_num ->jconf.annotate.cm_alpha_num
cm_alpha_step ->jconf.annotate.cm_alpha_step
cm_cut_thres ->jconf.annotate.cm_cut_thres
cm_cut_thres_pop ->jconf.annotate.cm_cut_thres_pop
cmn_loaded ->recog.cmn_loaded
cmn_map_weight ->jconf.frontend.cmn_map_weight
cmn_update ->jconf.frontend.cmn_update
cmnload_filename ->jconf.frontend.cmnload_filename
cmnsave_filename ->jconf.frontend.cmnsave_filename
compute_only_1pass ->jconf.sw.compute_only_1pass
dfa ->model.dfa
dfa_filename ->jconf.lm.dfa_filename
dictfilename ->jconf.lm.dictfilename
enable_iwsp ->jconf.lm.enable_iwsp
enable_iwspword ->jconf.lm.enable_iwspword
enveloped_bestfirst_width ->jconf.search.pass2.enveloped_bestfirst_width
force_realtime_flag ->jconf.search.pass1.force_realtime_flag
forced_realtime ->jconf.search.pass1.forced_realtime
forcedict_flag ->jconf.lm.forcedict_flag
framemaxscore ->recog.framemaxscore
from_code ->jconf.output.from_code
gmm ->model.gmm
gmm_filename ->jconf.reject.gmm_filename
gmm_gprune_num ->jconf.reject.gmm_gprune_num
gmm_reject_cmn_string ->jconf.reject.gmm_reject_cmn_string
gprune_method ->jconf.am.gprune_method
gramlist ->model.grammars
gramlist_root ->jconf.lm.gramlist_root
graph_merge_neighbor_range ->jconf.graph.graph_merge_neighbor_range
graph_totalwordnum ->recog.graph_totalwordnum
graphout_cut_depth ->jconf.graph.graphout_cut_depth
graphout_limit_boundary_loop_num ->jconf.graph.graphout_limit_boundary_loop_num
graphout_search_delay ->jconf.graph.graphout_search_delay
gs_statenum ->jconf.am.gs_statenum
head_margin_msec ->jconf.detect.head_margin_msec
head_silname ->jconf.lm.head_silname
hmm_gs ->model.hmm_gs
hmm_gs_filename ->jconf.am.hmm_gs_filename
hmmfilename ->jconf.am.hmmfilename
hmminfo ->model.hmminfo
hypo_overflow ->jconf.search.pass2.hypo_overflow
inputlist_filename ->jconf.input.inputlist_filename
iw_cache_rate ->jconf.search.pass1.iw_cache_rate
iwcdmaxn ->jconf.search.pass1.iwcdmaxn
iwcdmethod ->jconf.search.pass1.iwcdmethod
iwsp_penalty ->jconf.lm.iwsp_penalty
iwspentry ->jconf.lm.iwspentry
level_thres ->jconf.detect.level_thres
lm_penalty ->jconf.lm.lm_penalty
lm_penalty2 ->jconf.lm.lm_penalty2
lm_penalty_trans ->jconf.lm.lm_penalty_trans
lm_weight ->jconf.lm.lm_weight
lm_weight2 ->jconf.lm.lm_weight2
lmp_specified ->jconf.lm.lmp_specified
lmp2_specified ->jconf.lm.lmp2_specified
looktrellis_flag ->jconf.search.pass2.looktrellis_flag
lookup_range ->jconf.search.pass2.lookup_range
mapfilename ->jconf.am.mapfilename
mixnum_thres ->jconf.am.mixnum_thres
module_mode -> (app)
module_port -> (app)
module_sd -> (app)
multigramout_flag ->jconf.output.multigramout_flag
nbest ->jconf.search.pass2.nbest
netaudio_devname ->jconf.input.netaudio_devname
ngram ->model.ngram
ngram_filename ->jconf.lm.ngram_filename
ngram_filename_lr_arpa ->jconf.lm.ngram_filename_lr_arpa
ngram_filename_rl_arpa ->jconf.lm.ngram_filename_rl_arpa
old_iwcd_flag -> USE_OLD_IWCD (define.h)
old_tree_function_flag ->jconf.search.pass1.old_tree_function_flag
output_hypo_maxnum ->jconf.output.output_hypo_maxnum
para ->jconf.analysis.para
para_default ->jconf.analysis.para_default
para_hmm ->jconf.analysis.para_hmm
para_htk ->jconf.analysis.para_htk
paramtype_check_flag ->jconf.analysis.paramtype_check_flag
pass1_score ->recog.pass1_score
pass1_wnum ->recog.pass1_wnum
pass1_wseq ->recog.pass1_wseq
penalty1 ->jconf.lm.penalty1
penalty2 ->jconf.lm.penalty2
peseqlen ->recog.peseqlen
progout_flag ->jconf.output.progout_flag
progout_interval ->jconf.output.progout_interval
progout_interval_frame (beam.c) ->jconf.output.progout_interval
realtime_flag ->jconf.search.pass1.realtime_flag
record_dirname ->jconf.output.record_dirname
rejectshortlen ->jconf.reject.rejectshortlen
rest_param ->recog.rest_param
result_output -> (app)
scan_beam_thres ->jconf.search.pass2.scan_beam_thres
separate_score_flag ->jconf.output.separate_score_flag
separate_wnum ->jconf.search.pass1.separate_wnum
silence_cut ->jconf.detect.silence_cut
sp_break_2_begin_word ->recog.sp_break_2_begin_word
sp_break_2_end_word ->recog.sp_break_2_end_word
sp_break_last_nword ->recog.sp_break_last_nword
sp_break_last_nword_allow_override ->recog.sp_break_last_nword_allow_override
sp_break_last_word ->recog.sp_break_last_word
sp_frame_duration ->jconf.successive.sp_frame_duration
specified_trellis_beam_width ->jconf.search.pass1.specified_trellis_beam_width
speech ->recog.speech
speech_input ->jconf.input.speech_input
speechlen ->recog.speechlen
spmodel_name ->jconf.am.spmodel_name
ssbuf ->recog.ssbuf
sscalc ->jconf.frontend.sscalc
sscalc_len ->jconf.frontend.sscalc_len
sslen ->recog.sslen
ssload_filename ->jconf.frontend.ssload_filename
stack_size ->jconf.search.pass2.stack_size
strip_zero_sample ->jconf.frontend.strip_zero_sample
tail_margin_msec ->jconf.detect.tail_margin_msec
tail_silname ->jconf.lm.tail_silname
to_code ->jconf.output.to_code
trellis_beam_width ->recog.trellis_beam_width
trellis_check_flag ->jconf.sw.trellis_check_flag
triphone_check_flag ->jconf.sw.triphone_check_flag
use_ds48to16 ->jconf.input.use_ds48to16
use_zmean ->jconf.frontend.use_zmean
wchmm ->recog.wchmm
wchmm_check_flag ->jconf.sw.wchmm_check_flag
winfo ->model.winfo
wpair_keep_nlimit ->jconf.search.pass1.wpair_keep_nlimit
zero_cross_num ->jconf.detect.zero_cross_num

verbose_flag -> (remain in global.h)
debug2_flag -> (remain in global.h)

*/
 
