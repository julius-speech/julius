/**
 * @file   recog.h
 * 
 * <JA>
 * @brief  エンジンインスタンスの定義
 *
 * 認識エンジンのインスタンス定義を行います．インスタンスは，
 * Recog をトップインスタンスとして，使用する音響モデル，言語モデル，
 * それらを組み合わせた認識処理インスタンスを複数持ちます．
 *
 * 各部のインスタンスは，対応する jconf 内の設定構造体，および
 * 使用するサブインスタンスへのポインタを持ちます．PROCESS_AM は音響モデル，
 * PROCESS_LM は言語モデルごとに定義されます．
 *
 * MFCCCalc は，
 * 音響モデルおよび GMM で要求されるパラメータタイプを調べたのち，
 * それらを生成するのに必要なだけ生成されます．同一のMFCC型および
 * その他のフロントエンド処理条件を持つ音響モデルおよびGMMどうしでは
 * 同じ MFCCCalc が共有されます．
 *
 * </JA>
 * 
 * <EN>
 * @brief  Enging instance definitions
 *
 * This file defines the engine instance and all its sub instances.
 * The top instance is Recog, and it consists of several
 * sub instances for LM, AM, and recognition process instances.
 *
 * Each sub-instance keeps pointer to corresponding jconf setting
 * part, and also has pointers to other instances to use.
 * PROCESS_AM will be generated for each acoustic model, and PROCESS_LM
 * will be for each language model.
 *
 * MFCCCalc will be generated for each required MFCC frontend types
 * by inspecting all AMs and GMM.  The AM's and GMMs that requires
 * exactly the same MFCC frontend will share the same MFCC frontend.
 *
 * </EN>
 *
 * <pre>
 * Recog
 *    +- *JCONF
 *    +- input related work area
 *    +- MFCCCalc[] (linked list) (generated from HMM + GMM)
 *    +- PROCESS_AM[] (linked list)
 *       +- *pointer to JCONF_AM
 *       +- *pointer to MFCCCalc
 *       +- hmminfo, hmm_gs
 *       +- hmmwrk
 *       +- multipath, ccd_flag, cmn_loaded
 *    +- PROCESS_LM[] (linked list)
 *       +- *pointer to JCONF_LM
 *       +- *pointer to PROCESS_AM
 *       +- lmtype, lmvar
 *       +- winfo
 *       +- ngram or grammars
 *       +- lmfunc
 *    +- RecogProcess process[] (linked list)
 *       +- *pointer to JCONF_SEARCH
 *       +- *pointer to PROCESS_AM
 *       +- *pointer to PROCESS_LM
 *       +- lmtype, lmvar
 *       +- misc. param
 *    +- GMMCalc
 *       +- *JCONF_AM for GMM
 *       +- *pointer to MFCCCalc
 * </pre>
 * 
 * @author Akinobu Lee
 * @date   Fri Feb 16 13:42:28 2007
 *
 * $Revision: 1.21 $
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

#ifndef __J_RECOG_H__
#define __J_RECOG_H__

#include <sent/stddefs.h>
#include <sent/hmm.h>
#include <sent/vocabulary.h>
#include <sent/ngram2.h>
#include <sent/dfa.h>
#include <julius/wchmm.h>
#include <julius/search.h>
#include <julius/callback.h>
#include <julius/jconf.h>

/*
  How tokens are managed:
   o  tlist[][] is a token stocker.  It holds all tokens in sequencial
      buffer.  They are malloced first on startup, and refered by ID while
      Viterbi procedure.  In word-pair mode, each token also has a link to
      another token to allow a node to have more than 1 token.
      
   o  token[n] holds the current ID number of a token associated to a
      lexicon tree node 'n'.

  */
/**
 * Work area for the first pass
 * 
 */
typedef struct __FSBeam__ {
  /* token stocker */
  TOKEN2 *tlist[2];     ///< Token space to hold all token entities.
  TOKENID *tindex[2];   ///< Token index corresponding to @a tlist for sort
  int maxtnum;          ///< Allocated number of tokens (will grow)
  int expand_step;      ///< Number of tokens to be increased per expansion
  boolean expanded;     ///< TRUE if the tlist[] and tindex[] has been expanded at last create_token();
  int tnum[2];          ///< Current number of tokens used in @a tlist
  int n_start;          ///< Start index of in-beam nodes on @a tindex
  int n_end;            ///< end index of in-beam nodes on @a tindex
  int tl;               ///< Current work area id (0 or 1, swapped for each frame)
  int tn;               ///< Next work area id (0 or 1, swapped for each frame)
#ifdef SCORE_PRUNING
  LOGPROB score_pruning_max;	  ///< Maximum score at current frame
  LOGPROB score_pruning_threshold;///< Score threshold for score pruning
  int score_pruning_count;	  ///< Number of tokens pruned by score (debug)
#endif
    
  /* Active token list */
  TOKENID *token;       ///< Active token list that holds currently assigned tokens for each tree node
#ifdef UNIGRAM_FACTORING
  /* for wordend processing with 1-gram factoring */
  LOGPROB wordend_best_score; ///< Best score of word-end nodes
  int wordend_best_node;        ///< Node id of the best wordend nodes
  TRELLIS_ATOM *wordend_best_tre; ///< Trellis word corresponds to above
  WORD_ID wordend_best_last_cword;      ///< Last context-aware word of above
#endif

  int totalnodenum;     ///< Allocated number of nodes in @a token
  TRELLIS_ATOM bos;     ///< Special token for beginning-of-sentence
  boolean nodes_malloced; ///< Flag to check if tokens already allocated
  LOGPROB lm_weight;           ///< Language score weight (local copy)
  LOGPROB lm_penalty;          ///< Word insertion penalty (local copy)
  LOGPROB lm_penalty_trans; ///< Additional insertion penalty for transparent words (local copy)
  LOGPROB penalty1; ///< Word insertion penalty for DFA (local copy)
#if defined(WPAIR) && defined(WPAIR_KEEP_NLIMIT)
  boolean wpair_keep_nlimit; ///< Keeps only N token on word-pair approx. (local copy from jconf)
#endif
  /* for short-pause segmentation */
  boolean in_sparea;         ///< TRUE when we are in a pause area now
  int tmp_sparea_start;         ///< Memorize where the current pause area begins
#ifdef SP_BREAK_RESUME_WORD_BEGIN
  WORD_ID tmp_sp_break_last_word; ///< Keep the max word hypothesis at beginning of this segment as the starting word of next segment
#else
  WORD_ID last_tre_word;        ///< Keep ths max word hypothesis at the end of this segment for as the starting word of the next segment
#endif
  boolean first_sparea;  ///< TRUE when we are in the first pause area
  int sp_duration;   ///< Number of current successive sp frame
#ifdef SPSEGMENT_NAIST
  boolean after_trigger;        ///< TRUE if speech already triggered 
  int trigger_duration;         ///< Current speech duration at uptrigger detection
  boolean want_rewind;          ///< TRUE if process wants mfcc rewinding
  int rewind_frame;             ///< Place to rewind to
  boolean want_rewind_reprocess; ///< TRUE if requires re-processing after rewind
#endif
  char *pausemodelnames;        ///< pause model name string to detect segment
  char **pausemodel;            ///< each pause model name to detect segment
  int pausemodelnum;            ///< num of pausemodel
} FSBeam;


/**
 * Work area for realtime processing of 1st pass
 * 
 */
typedef struct __RealBeam__ {
  /* input parameter */
  int maxframelen;              ///< Maximum allowed input frame length

  SP16 *window;         ///< Window buffer for MFCC calculation
  int windowlen;                ///< Buffer length of @a window
  int windownum;                ///< Currently left samples in @a window

  /* for short-pause segmentation */
  boolean last_is_segmented; ///<  TRUE if last pass was a segmented input
  SP16 *rest_Speech; ///< Speech samples left unprocessed by segmentation at previous segment
  int rest_alloc_len;   ///< Allocated length of rest_Speech
  int rest_len;         ///< Current stored length of rest_Speech

} RealBeam;

/**
 * Work area for the 2nd pass
 * 
 */
typedef struct __StackDecode__ {
  int hypo_len_count[MAXSEQNUM+1];      ///< Count of popped hypothesis per each length
  int maximum_filled_length; ///< Current least beam-filled depth
#ifdef SCAN_BEAM
  LOGPROB *framemaxscore; ///< Maximum score of each frame on 2nd pass for score enveloping
#endif
  NODE *stocker_root; ///< Node stocker for recycle
  int popctr;           ///< Num of popped hypotheses from stack
  int genectr;          ///< Num of generated hypotheses
  int pushctr;          ///< Num of hypotheses actually pushed to stack
  int finishnum;        ///< Num of found sentence hypothesis
  NODE *current;                ///< Current node for debug

#ifdef CONFIDENCE_MEASURE
  LOGPROB cm_alpha;             ///< alpha scaling value from jconf
# ifdef CM_MULTIPLE_ALPHA
  LOGPROB *cmsumlist;        ///< Sum of cm score for each alpha coef.
  int cmsumlistlen;             ///< Allocated length of cmsumlist.
# endif
# ifdef CM_SEARCH
  LOGPROB cm_tmpbestscore; ///< Temporal best score for summing up scores
#  ifndef CM_MULTIPLE_ALPHA
  LOGPROB cm_tmpsum;            ///< Sum of CM score
#  endif
  int l_stacksize;              ///< Local stack size for CM
  int l_stacknum;               ///< Num of hypo. in local stack for CM
  NODE *l_start;        ///< Top node of local stack for CM
  NODE *l_bottom;       ///< bottom node of local stack for CM
# endif
# ifdef CM_NBEST
  LOGPROB *sentcm = NULL;       ///< Confidence score of each sentence
  LOGPROB *wordcm = NULL;       ///< Confidence score of each word voted from @a sentcm
  int sentnum;          ///< Allocated length of @a sentcm
  int wordnum;          ///< Allocated length of @a wordcm
# endif
#endif /* CONFIDENCE_MEASURE */

  LOGPROB *wordtrellis[2]; ///< Buffer to compute viterbi path of a word
  LOGPROB *g;           ///< Buffer to hold source viterbi scores
  HMM_Logical **phmmseq;        ///< Phoneme sequence to be computed
  int phmmlen_max;              ///< Maximum length of @a phmmseq.
  boolean *has_sp;              ///< Mark which phoneme allow short pause for multi-path mode
#ifdef GRAPHOUT_PRECISE_BOUNDARY
  short *wend_token_frame[2]; ///< Propagating token of word-end frame to detect corresponding end-of-words at word head
  LOGPROB *wend_token_gscore[2]; ///< Propagating token of scores at word-end to detect corresponding end-of-words at word head
  short *wef;           ///< Work area for word-end frame tokens for v2
  LOGPROB *wes;         ///< Work area for word-end score tokens for v2
#endif
  WORD_ID *cnword;		///< Work area for N-gram computation
  WORD_ID *cnwordrev;		///< Work area for N-gram computation

} StackDecode;

/**
 * User LM function entry point
 * 
 */
typedef struct {
  LOGPROB (*uniprob)(WORD_INFO *, WORD_ID, LOGPROB); ///< Pointer to function returning word occurence probability
  LOGPROB (*biprob)(WORD_INFO *, WORD_ID, WORD_ID, LOGPROB); ///< Pointer to function returning a word probability given a word context (corresponds to bi-gram)
  LOGPROB (*lmprob)(WORD_INFO *, WORD_ID *, int, WORD_ID, LOGPROB); ///< Pointer to function returning LM probability
} LMFunc;

/**
 * Work area for GMM calculation
 * 
 */
typedef struct __gmm_calc__{
  LOGPROB *gmm_score;   ///< Current accumurated scores for each GMM
  boolean *is_voice;            ///< True if corresponding model designates speech, FALSE if noise
  int framecount;               ///< Current frame count

  short OP_nstream;             ///< Number of input stream for GMM
  VECT *OP_vec_stream[MAXSTREAMNUM]; ///< input vector for each stream at that frame
  short OP_veclen_stream[MAXSTREAMNUM]; ///< vector length for each stream

  LOGPROB *OP_calced_score; ///< Work area for Gaussian pruning on GMM: scores
  int *OP_calced_id; ///< Work area for Gaussian pruning on GMM: id
  int OP_calced_num; ///< Work area for Gaussian pruning on GMM: number of above
  int OP_calced_maxnum; ///< Work area for Gaussian pruning on GMM: size of allocated area
  int OP_gprune_num; ///< Number of Gaussians to be computed in Gaussian pruning
  VECT *OP_vec;         ///< Local workarea to hold the input vector of current frame
  short OP_veclen;              ///< Local workarea to hold the length of above
  HTK_HMM_Data *max_d;  ///< Hold model of the maximum score
  int max_i;                    ///< Index of max_d
#ifdef CONFIDENCE_MEASURE
  LOGPROB gmm_max_cm;   ///< Hold maximum score
#endif
#ifdef GMM_VAD
  LOGPROB *rates;   ///< voice rate of recent N frames (cycle buffer)
  int nframe;                   ///< Length of rates
  boolean filled;
  int framep;                   ///< Current frame pointer

  boolean in_voice;             ///< TRUE if currently in voice area
  boolean up_trigger;           ///< TRUE when detect up trigger
  boolean down_trigger;         ///< TRUE when detect down trigger
  boolean after_trigger;        ///< TRUE when currently we are processing speech segment
  boolean want_rewind;          ///< TRUE if GMM wants rewinding its MFCC
  boolean want_rewind_reprocess; ///< TRUE if GMM wants re-processing after rewind
  int rewind_frame;             ///< Frame to rewind
  int duration;                 ///< Current GMM duration work
#endif
} GMMCalc;

/**
 * Alignment result, valid when forced alignment was done
 * 
 */
typedef struct __sentence_align__ {
  int num;                    ///< Number of units
  short unittype;             ///< Unit type (one of PER_*)
  WORD_ID *w;                 ///< word sequence by id (PER_WORD)
  HMM_Logical **ph;     ///< Phone sequence (PER_PHONEME, PER_STATE)
  short *loc; ///< sequence of state location in a phone (PER_STATE)
  boolean *is_iwsp;           ///< TRUE if PER_STATE and this is the inter-word pause state at multipath mode
  int *begin_frame;           ///< List of beginning frame
  int *end_frame;             ///< List of ending frame
  LOGPROB *avgscore;          ///< Score averaged by frames
  LOGPROB allscore;           ///< Re-computed acoustic score
  struct __sentence_align__ *next; ///< data chain pointer
} SentenceAlign;

/**
 * Output result structure
 * 
 */
typedef struct __sentence__ {
  WORD_ID word[MAXSEQNUM];      ///< Sequence of word ID 
  int word_num;                 ///< Number of words in the sentence
  LOGPROB score;                ///< Likelihood (LM+AM)
  LOGPROB confidence[MAXSEQNUM]; ///< Word confidence scores
  LOGPROB score_lm;             ///< Language model likelihood (scaled) for N-gram
  LOGPROB score_am;             ///< Acoustic model likelihood for N-gram
  int gram_id;                  ///< The grammar ID this sentence belongs to for DFA
  SentenceAlign *align;

#ifdef USE_MBR
  LOGPROB score_mbr; ///< MBR score
#endif 

} Sentence;

/** 
 * A/D-in work area
 * 
 */
typedef struct __adin__ {
  /* functions */
  /// Pointer to function for device initialization (call once on startup)
  boolean (*ad_standby)(int, void *);
  /// Pointer to function to open audio stream for capturing
  boolean (*ad_begin)(char *);
  /// Pointer to function to close audio stream capturing
  boolean (*ad_end)();
  /// Pointer to function to begin / restart recording
  boolean (*ad_resume)();
  /// Pointer to function to pause recording
  boolean (*ad_pause)();
  /// Pointer to function to terminate current recording immediately
  boolean (*ad_terminate)();
  /// Pointer to function to read samples
  int (*ad_read)(SP16 *, int);
  /// Pointer to function to return current input source name (filename, devname, etc.)
  char * (*ad_input_name)();

  /* configuration parameters */
  int thres;            ///< Input Level threshold (0-32767)
  int noise_zerocross;  ///< Computed threshold of zerocross num in the cycle buffer
  int nc_max;           ///< Computed number of fragments for tail margin
  int chunk_size;	///< audio process unit
  boolean adin_cut_on;  ///< TRUE if do input segmentation by silence
  boolean silence_cut_default; ///< Device-dependent default value of adin_cut_on()
  boolean strip_flag;   ///< TRUE if skip invalid zero samples
  boolean enable_thread;        ///< TRUE if input device needs threading
  boolean need_zmean;   ///< TRUE if perform zmeansource
  float level_coef;     ///< Input level scaling factor

  /* work area */
  int c_length; ///< Computed length of cycle buffer for zero-cross, actually equals to head margin length
  int c_offset; ///< Static data DC offset (obsolute, should be 0)
  SP16 *swapbuf;                ///< Buffer for re-triggering in tail margin
  int sbsize;    ///< Size of @a swapbuf
  int sblen;    ///< Current length of @a swapbuf
  int rest_tail;                ///< Samples not processed yet in swap buffer

  ZEROCROSS zc;                 ///< Work area for zero-cross computation

#ifdef HAVE_PTHREAD
  /* Variables related to POSIX threading */
  pthread_t adin_thread;	///< Thread information
  pthread_mutex_t mutex;        ///< Lock primitive
  SP16 *speech;         ///< Unprocessed samples recorded by A/D-in thread
  int speechlen;                ///< Current length of @a speech
  int freezelen;        ///< Number of samples to abondon processing
/*
 * Semaphore to start/stop recognition.
 * 
 * If TRUE, A/D-in thread will store incoming samples to @a speech and
 * main thread will detect and process them.
 * If FALSE, A/D-in thread will still get input and check trigger as the same
 * as TRUE case, but does not store them to @a speech.
 * 
 */
  boolean transfer_online;
  /**
   * TRUE if buffer overflow occured in adin thread.
   * 
   */
  boolean adinthread_buffer_overflowed;
  /**
   * TRUE if adin thread ended
   * 
   */
  boolean adinthread_ended;

  boolean ignore_speech_while_recog; ///< TRUE if ignore speech input between call, while waiting recognition process

#endif

  /* Input data buffer */
  SP16 *buffer; ///< Temporary buffer to hold input samples
  int bpmax;            ///< Maximum length of @a buffer
  int bp;                       ///< Current point to store the next data
  int current_len;              ///< Current length of stored samples
  SP16 *cbuf;           ///< Buffer for flushing cycle buffer just after detecting trigger 
  boolean down_sample; ///< TRUE if perform down sampling from 48kHz to 16kHz
  SP16 *buffer48; ///< Another temporary buffer to hold 48kHz inputs
  int io_rate; ///< frequency rate (should be 3 always for 48/16 conversion

  boolean is_valid_data;        ///< TRUE if we are now triggered
  int nc;               ///< count of current tail silence segments
  boolean end_of_stream;        ///< TRUE if we have reached the end of stream
  boolean need_init;    ///< if TRUE, initialize buffer on startup

  DS_BUFFER *ds;           ///< Filter buffer for 48-to-16 conversion

  boolean rehash; ///< TRUE is want rehash at rewinding on decoder-based VAD

  boolean input_side_segment;   ///< TRUE if segmentation requested by ad_read

  unsigned int total_captured_len; ///< Total number of recorded samples from start until now
  unsigned int last_trigger_sample; ///< Last speech area was triggeed at this sample
  unsigned int last_trigger_len; // Length of last speech area 

  char current_input_name[MAXPATHLEN]; ///< File or device name of current input

} ADIn;

/**
 * Recognition result output structure.  You may want to use with model data
 * to get fully detailed results.
 * 
 */
typedef struct __Output__ {
  /**
   * 1: recognition in progress
   * 0: recognition succeeded (at least one candidate has been found)
   * -1: search failed, no candidate has been found
   * -2: input rejected by short input
   * -3: input rejected by GMM
   * 
   */
  int status;

  int num_frame;                ///< Number of frames of the recognized part
  int length_msec;              ///< Length of the recognized part

  Sentence *sent;               ///< List of (N-best) recognition result sentences
  int sentnum;                  ///< Number of sentences

  WordGraph *wg1;               ///< List of word graph generated on 1st pass
  int wg1_num;                  ///< Num of words in the wg1

  WordGraph *wg;                ///< List of word graph

  CN_CLUSTER *confnet;          ///< List of confusion network clusters

  Sentence pass1;               ///< Recognition result on the 1st pass

} Output;  


/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

/**
 * instance for a parameter vector computation
 * 
 */
typedef struct __mfcc_calc__ {

  /**
   * Unique id
   * 
   */
  short id;

  /**
   * Parameter setting (entity in JCONF_AM)
   * 
   */
  Value *para;

  /**
   * TRUE if the para came from "-htkconf"
   * 
   */
  boolean htk_loaded;
  /**
   * TRUE if the para came from binhmm embedded header
   * 
   */
  boolean hmm_loaded;

  /**
   * Check input parameter type with header of the hmmdefs
   * (-notypecheck to unset)
   */
  boolean paramtype_check_flag;

  /**
   * Parameter extraction work area
   * 
   */
  MFCCWork *wrk;

  /**
   * Parameter vector sequence to be recognized
   * 
   */
  HTK_Param *param;

  /**
   * Rest parameter for next segment for short-pause segmentation
   */
  HTK_Param *rest_param;

  /**
   * Work area and setting for cepstral mean normalization
   * 
   */
  struct {
    /**
     * CMN: load initial cepstral mean from file at startup (-cmnload)
     */
    char *load_filename;
    /**
     * CMN: update cepstral mean while recognition
     * (-cmnnoupdate to unset)
     */
    boolean update;
    /**
     * CMN: save cepstral mean to file at end of every recognition (-cmnsave)
     */
    char *save_filename;     
    /**
     * CMN: MAP weight for initial cepstral mean on (-cmnmapweight)
   */
    float map_weight;

    /**
     * TRUE if CMN parameter loaded from file at boot up
     */
    boolean loaded;

    /**
     * realtime CMN work area
     * 
     */
    CMNWork *wrk;

  } cmn;

  /**
   * Work area for front-end processing
   * 
   */
  struct {
    /**
     * Estimated noise spectrum
     */
    float *ssbuf;
    
    /**
     * Length of @a ssbuf
     */
    int sslen;
    
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

    /**
     * Parameter extraction work area for spectral subtraction
     * 
     */
    MFCCWork *mfccwrk_ss;
    
  } frontend;

  /**
   * work area for energy normalization on real time processing
   * 
   */
  ENERGYWork ewrk;

  /**
   * delta MFCC cycle buffer
   * 
   */
  DeltaBuf *db;
  /**
   * accel MFCC cycle buffer
   * 
   */
  DeltaBuf *ab;
  /**
   * working buffer holding current computing mfcc vector
   * 
   */
  VECT *tmpmfcc;

  /**
   * FALSE indicates that the current frame (f) is not valid and should
   * not be used for recognition
   * 
   */
  boolean valid;

  /**
   * Current frame
   * 
   */
  int f;

  /**
   * Processed frame length when segmented
   * 
   */
  int last_time;

  /**
   * Re-start frame if segmenetd
   * 
   */
  int sparea_start;

  /**
   * TRUE if a parent instance has decided segmented
   * 
   */
  boolean segmented;

  /**
   * TRUE if an input functionhas decided segmented
   * 
   */
  boolean segmented_by_input;

  /**
   * id of an plugin module if MFCC should be obtained via plugin
   * 
   */
  int plugin_source;

  /**
   * Function entry points for plugin input
   * 
   */
  struct {
    /// Pointer to function for device initialization (call once on startup)
    boolean (*fv_standby)();
    /// Pointer to function to open audio stream for capturing
    boolean (*fv_begin)();
    /// Pointer to function to read samples
    int (*fv_read)(VECT *, int);
    /// Pointer to function to close audio stream capturing
    boolean (*fv_end)();
    /// Pointer to function to begin / restart recording
    boolean (*fv_resume)();
    /// Pointer to function to pause recording
    boolean (*fv_pause)();
    /// Pointer to function to terminate current recording immediately
    boolean (*fv_terminate)();
    /// Pointer to function to return current input name
    char * (*fv_input_name)();
  } func;

#ifdef POWER_REJECT
  float avg_power;
#endif

  /**
   * pointer to next
   * 
   */
  struct __mfcc_calc__ *next;

} MFCCCalc;

/**
 * instance for an AM.
 * 
 */
typedef struct __process_am__ {

  /**
   * Configuration parameters
   * 
   */
  JCONF_AM *config;

  /**
   * Corresponding input parameter vector instance
   * 
   */
  MFCCCalc *mfcc;

  /**
   * Main phoneme HMM 
   */
  HTK_HMM_INFO *hmminfo;

  /**
   * HMM for Gaussian Selection
   */
  HTK_HMM_INFO *hmm_gs;

  /**
   * Work area and outprob cache for HMM output probability computation
   */
  HMMWork hmmwrk;

  /**
   * pointer to next
   * 
   */
  struct __process_am__ *next;
  
} PROCESS_AM;

/**
 * instance for a LM.
 * 
 */
typedef struct __process_lm__ {

  /**
   * Configuration parameters
   * 
   */
  JCONF_LM *config;

  /**
   * Corresponding AM
   * 
   */
  PROCESS_AM *am;


  /**
   * the LM type of this Model holder: will be set from Jconf used for loading
   * 
   */
  int lmtype;

  /**
   * the LM variation type of this Model holder: will be set from
   * Jconf used for loading
   * 
   */
  int lmvar;

  /**
   * Main Word dictionary for all LM types
   */
  WORD_INFO *winfo;

  /**
   * Main N-gram language model (do not use with grammars)
   */
  NGRAM_INFO *ngram;

  /**
   * List of all loaded grammars (do not use with ngram)
   */
  MULTIGRAM *grammars;

  /**
   * Current maximum value of assigned grammar ID.
   * A new grammar ID will be assigned to each new grammar.
   * 
   */
  int gram_maxid;

  /**
   * Global DFA for recognition.  This will be generated from @a grammars,
   * concatinating each DFA into one.
   */
  DFA_INFO *dfa;

  /**
   * TRUE if modified in multigram_update()
   * 
   */
  boolean global_modified;

  /**
   * LM User function entry point
   * 
   */
  LMFunc lmfunc;

  /**
   * pointer to next
   * 
   */
  struct __process_lm__ *next;

} PROCESS_LM;

/**
 * instance for a decoding, i.e. set of LM, AM and parameters
 * 
 */
typedef struct __recogprocess__ {

  /**
   * TRUE is this instance is alive, or FALSE when temporary disabled.
   * 
   */
  boolean live;

  /**
   * 1 if this instance should be made alive in the next recognition,
   * -1 if should become dead in the next recognition,
   * or 0 to leave unchanged.
   * 
   */
  short active;

  /**
   * search configuration data
   * 
   */
  JCONF_SEARCH *config;

  /**
   * acoustic model instance to use
   * 
   */
  PROCESS_AM *am;

  /**
   * language model instance to use
   * 
   */
  PROCESS_LM *lm;

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
   * Whether handle phone context dependency (local copy from jconf)
   */
  boolean ccd_flag;

  /**
   * Word-conjunction HMM as tree lexicon
   */
  WCHMM_INFO *wchmm;

  /**
   * Actual beam width of 1st pass (will be set on startup)
   */
  int trellis_beam_width;

  /**
   * Word trellis index generated at the 1st pass
   */
  BACKTRELLIS *backtrellis;

  /**
   * Work area for the first pass
   */
  FSBeam pass1;

  /**
   * Work area for second pass
   * 
   */
  StackDecode pass2;

  /**
   * Word sequence of best hypothesis on 1st pass
   */
  WORD_ID pass1_wseq[MAXSEQNUM];

  /**
   * Number of words in @a pass1_wseq
   */
  int pass1_wnum;

  /**
   * Score of @a pass1_wseq
   */
  LOGPROB pass1_score;

  /**
   * Last maximum word hypothesis on the begin point for short-pause segmentation
   */
  WORD_ID sp_break_last_word;
  /**
   * Last (not transparent) context word for LM for short-pause segmentation
   */
  WORD_ID sp_break_last_nword;
  /**
   * Allow override of last context word from result of 2nd pass for short-pause segmentation
   */
  boolean sp_break_last_nword_allow_override;
  /**
   * Search start word on 2nd pass for short-pause segmentation
   */
  WORD_ID sp_break_2_begin_word;
  /**
   * Search end word on 2nd pass for short-pause segmentation
   */
  WORD_ID sp_break_2_end_word;

  /**
   * Input length in frames
   */
  int peseqlen;         

  /**
   * GraphOut: total number of words in the generated graph
   */
  int graph_totalwordnum;

  /**
   * Recognition results
   * 
   */
  Output result;

  /**
   * graphout: will be set from value from jconf->graph.enabled
   * 
   */
  boolean graphout;

  /**
   * Temporal matrix work area to hold the order relations between words
   * for confusion network construction.
   * 
   */
  char *order_matrix;

  /**
   * Number of words to be expressed in the order matrix for confusion network
   * construction.
   * 
   */
  int order_matrix_count;

#ifdef DETERMINE
  int determine_count;
  LOGPROB determine_maxnodescore;
  boolean determined;
  LOGPROB determine_last_wid;
  boolean have_determine;
#endif

  /**
   * TRUE if has something to output at CALLBACK_RESULT_PASS1_INTERIM.
   * 
   */
  boolean have_interim;

  /**
   * User-defined data hook.  JuliusLib does not concern about its content.
   * 
   */
  void *hook;

  /**
   * Pointer to next instance
   * 
   */
  struct __recogprocess__ *next;

} RecogProcess;

/**
 * Top level instance for the whole recognition process
 * 
 */
typedef struct __Recog__ {

  /*******************************************/
  /**
   * User-specified configuration parameters
   * 
   */
  Jconf *jconf;

  /*******************************************/
  /**
   * A/D-in buffers
   * 
   */
  ADIn *adin;

  /**
   * Work area for the realtime processing of first pass
   */
  RealBeam real;

  /**
   * Linked list of MFCC calculation/reading instances
   * 
   */
  MFCCCalc *mfcclist;

  /**
   * Linked list of acoustic model instances
   * 
   */
  PROCESS_AM *amlist;

  /**
   * Linked list of language model instances
   * 
   */
  PROCESS_LM *lmlist;

  /**
   * Linked list of recognition process instances
   * 
   */
  RecogProcess *process_list;


  /**
   * TRUE when engine is processing a segment (for short-pause segmentation)
   * 
   */
  boolean process_segment;

  /*******************************************/
  /* inputs */

  /**
   * Input speech data
   */
  SP16 *speech;

  /**
   * Allocate length of speech 
   * 
   */
  int speechalloclen;

  /**
   * Input length in samples
   */
  int speechlen;                

  /**
   * Input length in frames
   */
  int peseqlen;         

  /*******************************************/

  /**
   * GMM definitions
   * 
   */
  HTK_HMM_INFO *gmm;

  /**
   * Pointer to MFCC instance for GMM
   * 
   */
  MFCCCalc *gmmmfcc;

  /**
   * Work area for GMM calculation
   * 
   */
  GMMCalc *gc;

  /*******************************************/
  /* misc. */

  /**
   * Status flag indicating whether the recognition is alive or not.  If
   * TRUE, the process is currently activated, either monitoring an
   * audio input or recognizing the current input.  If FALSE, the recognition
   * is now disabled until some activation command has been arrived from
   * client.  While disabled, all the inputs are ignored.
   *
   * If set to FALSE in the program, Julius/Julian will stop after
   * the current recognition ends, and enter the disabled status.
   * 
   */
  boolean process_active;

  /**
   * If set to TRUE, Julius/Julian stops recognition immediately, terminating
   * the currenct recognition process, and enter into disabled status.
   * 
   */
  boolean process_want_terminate;

  /**
   * If set to TRUE, Julius/Julian stops recognition softly.  If it is
   * performing recognition of the 1st pass, it immediately segments the
   * current input, process the 2nd pass, and output the result.  Then it
   * enters the disabled status.
   * 
   */
  boolean process_want_reload;

  /**
   * When to refresh the global lexicon if received while recognition for
   * DFA
   * 
   */
  short gram_switch_input_method;

  /**
   * TRUE if audio stream is now open and engine is either listening
   * audio stream or recognizing a speech.  FALSE on startup or when
   * in pause specified by a module command.
   * 
   */
  boolean process_online;

  /**
   * Function pointer to parameter vector computation for realtime 1st pass.
   * default: RealTimeMFCC() in realtime-1stpass.c
   * 
   */
  boolean (*calc_vector)(MFCCCalc *, SP16 *, int);

  /**
   * TRUE when recognition triggered and some recognition started,
   * FALSE if engine terminated with no input.
   * 
   */
  boolean triggered;

  /**
   * Callback entry point
   * 
   */
  void (*callback_function[SIZEOF_CALLBACK_ID][MAX_CALLBACK_HOOK])();
  /**
   * Callback user data
   * 
   */
  void *callback_user_data[SIZEOF_CALLBACK_ID][MAX_CALLBACK_HOOK];
  /**
   * Numbers of callbacks registered
   * 
   */
  int callback_function_num[SIZEOF_CALLBACK_ID];
  /**
   * Callback function code list
   * 
   */
  int callback_list_code[MAX_CALLBACK_HOOK*SIZEOF_CALLBACK_ID];
  /**
   * Callback function location list
   * 
   */
  int callback_list_loc[MAX_CALLBACK_HOOK*SIZEOF_CALLBACK_ID];
  /**
   * Number of callbacks
   * 
   */
  int callback_num;

  /*******************************************/

  /**
   * User-defined data hook.  JuliusLib does not concern about its content.
   * 
   */
  void *hook;

} Recog;

#endif /* __J_RECOG_H__ */
