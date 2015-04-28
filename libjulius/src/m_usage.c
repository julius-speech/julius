/**
 * @file   m_usage.c
 * 
 * <JA>
 * @brief  ヘルプを表示する
 * </JA>
 * 
 * <EN>
 * @brief  Print help.
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Fri May 13 15:04:34 2005
 *
 * $Revision: 1.25 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius/julius.h>

/** 
 * <JA>
 * ヘルプを表示する. 
 * 
 * </JA>
 * <EN>
 * Output help document.
 * 
 * </EN>
 *
 * @param fp [in] file pointer to output help
 *
 * @callgraph
 * @callergraph
 * @ingroup engine
 * 
 */
void
j_output_argument_help(FILE *fp)
{
  Jconf *jconf;
#ifdef ENABLE_PLUGIN
  int id;
  char buf[64];
  PLUGIN_ENTRY *p;
  FUNC_VOID func;
#endif
    
  /* load default values */
  jconf = j_jconf_new();

  j_put_header(fp);
  j_put_compile_defs(fp);
  fprintf(fp, "\nOptions:\n");

  fprintf(fp, "\n--- Global Options -----------------------------------------------\n");

  fprintf(fp, "\n Feature Vector Input:\n");
  fprintf(fp, "    [-input devname]       input source  (default = htkparam)\n");
  fprintf(fp, "         htkparam/mfcfile  feature vectors in HTK parameter file format\n");
  fprintf(fp, "         outprob           outprob vectors in HTK parameter file format\n");
  fprintf(fp, "         vecnet            receive vectors from client (TCP/IP)\n");
#ifdef ENABLE_PLUGIN
  if (global_plugin_list) {
    if ((id = plugin_get_id("fvin_get_optname")) >= 0) {
      for(p=global_plugin_list[id];p;p=p->next) {
	func = (FUNC_VOID) p->func;
	(*func)(buf, (int)64);
	fprintf(fp, "         %-18s(feature vector input plugin #%d)\n", buf, p->source_id);
      }
    }
  }
#endif
  fprintf(fp, "    [-filelist file]    filename of input file list\n");

  fprintf(fp, "\n Speech Input:\n");
  fprintf(fp, "    (Can extract MFCC/FBANK/MELSPEC features from waveform)\n");
  fprintf(fp, "    [-input devname]    input source  (default = htkparam)\n");
  fprintf(fp, "         file/rawfile      waveform file (%s)\n", SUPPORTED_WAVEFILE_FORMAT);
#ifdef USE_MIC
  fprintf(fp, "         mic               default microphone device\n");
# ifdef HAS_ALSA
  fprintf(fp, "         alsa              use ALSA interface\n");
# endif
# ifdef HAS_OSS
  fprintf(fp, "         oss               use OSS interface\n");
# endif
# ifdef HAS_ESD
  fprintf(fp, "         esd               use ESounD interface\n");
# endif
# ifdef HAS_PULSEAUDIO
  fprintf(fp, "         pulseaudio        use PulseAudio interface\n");
# endif
#endif
#ifdef USE_NETAUDIO
  fprintf(fp, "         netaudio          DatLink/NetAudio server\n");
#endif
  fprintf(fp, "         adinnet           adinnet client (TCP/IP)\n");
  fprintf(fp, "         stdin             standard input\n");
#ifdef ENABLE_PLUGIN
  if (global_plugin_list) {
    if ((id = plugin_get_id("adin_get_optname")) >= 0) {
      for(p=global_plugin_list[id];p;p=p->next) {
	func = (FUNC_VOID) p->func;
	(*func)(buf, (int)64);
	fprintf(fp, "         %-18s(adin plugin #%d)\n", buf, p->source_id);
      }
    }
  }
#endif
  fprintf(fp, "    [-filelist file]    filename of input file list\n");
#ifdef USE_NETAUDIO
  fprintf(fp, "    [-NA host:unit]     get audio from NetAudio server at host:unit\n");
#endif
  fprintf(fp, "    [-adport portnum]   adinnet port number to listen         (%d)\n", jconf->input.adinnet_port);
  fprintf(fp, "    [-48]               enable 48kHz sampling with internal down sampler (OFF)\n");
  fprintf(fp, "    [-zmean/-nozmean]   enable/disable DC offset removal      (OFF)\n");
  fprintf(fp, "    [-lvscale]          input level scaling factor (1.0: OFF) (%.1f)\n", jconf->preprocess.level_coef);
  fprintf(fp, "    [-nostrip]          disable stripping off zero samples\n");
  fprintf(fp, "    [-record dir]       record triggered speech data to dir\n");
  fprintf(fp, "    [-rejectshort msec] reject an input shorter than specified\n");
  fprintf(fp, "    [-rejectlong msec]  reject an input longer than specified\n");
#ifdef POWER_REJECT
  fprintf(fp, "    [-powerthres value] rejection threshold of average power  (%.1f)\n", jconf->reject.powerthres);
#endif
  
  fprintf(fp, "\n Speech Detection: (default: on=mic/net off=files)\n");
  /*fprintf(fp, "    [-pausesegment]     turn on (force) pause detection\n");*/
  /*fprintf(fp, "    [-nopausesegment]   turn off (force) pause detection\n");*/
  fprintf(fp, "    [-cutsilence]       turn on (force) skipping long silence\n");
  fprintf(fp, "    [-nocutsilence]     turn off (force) skipping long silence\n");
  fprintf(fp, "    [-lv unsignedshort] input level threshold (0-32767)       (%d)\n", jconf->detect.level_thres);
  fprintf(fp, "    [-zc zerocrossnum]  zerocross num threshold per sec.      (%d)\n", jconf->detect.zero_cross_num);
  fprintf(fp, "    [-headmargin msec]  header margin length in msec.         (%d)\n", jconf->detect.head_margin_msec);
  fprintf(fp, "    [-tailmargin msec]  tail margin length in msec.           (%d)\n", jconf->detect.tail_margin_msec);
  fprintf(fp, "    [-chunksize sample] unit length for processing            (%d)\n", jconf->detect.chunk_size);

  fprintf(fp, "\n GMM utterance verification:\n");
  fprintf(fp, "    -gmm filename       GMM definition file\n");
  fprintf(fp, "    -gmmnum num         GMM Gaussian pruning num              (%d)\n", jconf->reject.gmm_gprune_num);
  fprintf(fp, "    -gmmreject string   comma-separated list of noise model name to reject\n");
#ifdef GMM_VAD
  fprintf(fp, "\n GMM-based VAD:\n");
  fprintf(fp, "    -gmmmargin frames   backstep margin on speech trigger     (%d)\n", jconf->detect.gmm_margin);
  fprintf(fp, "    -gmmup score        up-trigger threshold                  (%.1f)\n", jconf->detect.gmm_uptrigger_thres);
  fprintf(fp, "    -gmmdown score      down-trigger threshold                (%.1f)\n", jconf->detect.gmm_downtrigger_thres);
#endif

  fprintf(fp, "\n On-the-fly Decoding: (default: on=mic/net off=files)\n");
  fprintf(fp, "    [-realtime]         turn on, input streamed with MAP-CMN\n");
  fprintf(fp, "    [-norealtime]       turn off, input buffered with sentence CMN\n");

  fprintf(fp, "\n Others:\n");
  fprintf(fp, "    [-C jconffile]      load options from jconf file\n");
  fprintf(fp, "    [-quiet]            reduce output to only word string\n");
  fprintf(fp, "    [-demo]             equal to \"-quiet -progout\"\n");
  fprintf(fp, "    [-debug]            (for debug) dump numerous log\n");
  fprintf(fp, "    [-callbackdebug]    (for debug) output message per callback\n");
  fprintf(fp, "    [-check (wchmm|trellis)] (for debug) check internal structure\n");
  fprintf(fp, "    [-check triphone]   triphone mapping check\n");
  fprintf(fp, "    [-outprobout file]  Output state probabilities to file\n");
  fprintf(fp, "    [-setting]          print engine configuration and exit\n");
  fprintf(fp, "    [-help]             print this message and exit\n");

  fprintf(fp, "\n--- Instance Declarations ----------------------------------------\n\n");

  fprintf(fp, "    [-AM]               start a new acoustic model instance\n");
  fprintf(fp, "    [-LM]               start a new language model instance\n");
  fprintf(fp, "    [-SR]               start a new recognizer (search) instance\n");
  fprintf(fp, "    [-AM_GMM]           start an AM feature instance for GMM\n");
  fprintf(fp, "    [-GLOBAL]           start a global section\n");
  fprintf(fp, "    [-nosectioncheck]   disable option location check\n");
  fprintf(fp, "\n--- Acoustic Model Options (-AM) ---------------------------------\n");

  fprintf(fp, "\n Acoustic analysis:\n");
  fprintf(fp, "    [-htkconf file]     load parameters from the HTK Config file\n");
  fprintf(fp, "    [-smpFreq freq]     sample period (Hz)                    (%d)\n", jconf->am_root->analysis.para_default.smp_freq);
  fprintf(fp, "    [-smpPeriod period] sample period (100ns)                 (%d)\n", jconf->am_root->analysis.para_default.smp_period);
  fprintf(fp, "    [-fsize sample]     window size (sample)                  (%d)\n", jconf->am_root->analysis.para_default.framesize);
  fprintf(fp, "    [-fshift sample]    frame shift (sample)                  (%d)\n", jconf->am_root->analysis.para_default.frameshift);
  fprintf(fp, "    [-preemph]          pre-emphasis coef.                    (%.2f)\n", jconf->am_root->analysis.para_default.preEmph);
  fprintf(fp, "    [-fbank]            number of filterbank channels         (%d)\n", jconf->am_root->analysis.para_default.fbank_num);
  fprintf(fp, "    [-ceplif]           cepstral liftering coef.              (%d)\n", jconf->am_root->analysis.para_default.lifter);
  fprintf(fp, "    [-rawe] [-norawe]   toggle using raw energy               (no)\n");
  fprintf(fp, "    [-enormal] [-noenormal] toggle normalizing log energy     (no)\n");
  fprintf(fp, "    [-escale]           scaling log energy for enormal        (%.1f)\n", jconf->am_root->analysis.para_default.escale);
  fprintf(fp, "    [-silfloor]         energy silence floor in dB            (%.1f)\n", jconf->am_root->analysis.para_default.silFloor);
  fprintf(fp, "    [-delwin frame]     delta windows length (frame)          (%d)\n", jconf->am_root->analysis.para_default.delWin);
  fprintf(fp, "    [-accwin frame]     accel windows length (frame)          (%d)\n", jconf->am_root->analysis.para_default.accWin);
  fprintf(fp, "    [-hifreq freq]      freq. of upper band limit, off if <0  (%d)\n", jconf->am_root->analysis.para_default.hipass);
  fprintf(fp, "    [-lofreq freq]      freq. of lower band limit, off if <0  (%d)\n", jconf->am_root->analysis.para_default.lopass);
  fprintf(fp, "    [-sscalc]           do spectral subtraction (file input only)\n");
  fprintf(fp, "    [-sscalclen msec]   length of head silence for SS (msec)  (%d)\n", jconf->am_root->frontend.sscalc_len);
  fprintf(fp, "    [-ssload filename]  load constant noise spectrum from file for SS\n");
  fprintf(fp, "    [-ssalpha value]    alpha coef. for SS                    (%f)\n", jconf->am_root->frontend.ss_alpha);
  fprintf(fp, "    [-ssfloor value]    spectral floor for SS                 (%f)\n", jconf->am_root->frontend.ss_floor);
  fprintf(fp, "    [-zmeanframe/-nozmeanframe] frame-wise DC removal like HTK(OFF)\n");
  fprintf(fp, "    [-usepower/-nousepower] use power in fbank analysis       (OFF)\n");
  fprintf(fp, "    [-cmnload file]     load initial CMN param from file on startup\n");
  fprintf(fp, "    [-cmnsave file]     save CMN param to file after each input\n");
  fprintf(fp, "    [-cmnnoupdate]      not update CMN param while recog. (use with -cmnload)\n");
  fprintf(fp, "    [-cmnmapweight]     weight value of initial cm for MAP-CMN (%6.2f)\n", jconf->am_root->analysis.cmn_map_weight);
  fprintf(fp, "    [-cvn]              cepstral variance normalisation       (%s)\n", jconf->amnow->analysis.para.cvn ? "on" : "off");
  fprintf(fp, "    [-vtln alpha lowcut hicut] enable VTLN (1.0 to disable)   (%f)\n", jconf->am_root->analysis.para_default.vtln_alpha);

  fprintf(fp, "\n Acoustic Model:\n");
  fprintf(fp, "    -h hmmdefsfile      HMM definition file name\n");
  fprintf(fp, "    [-hlist HMMlistfile] HMMlist filename (must for triphone model)\n");
  fprintf(fp, "    [-iwcd1 methodname] switch IWCD triphone handling on 1st pass\n");
  fprintf(fp, "             best N     use N best score (default of n-gram, N=%d)\n", jconf->am_root->iwcdmaxn);
  fprintf(fp, "             max        use maximum score\n");
  fprintf(fp, "             avg        use average score (default of dfa)\n");
  fprintf(fp, "    [-force_ccd]        force to handle IWCD\n");
  fprintf(fp, "    [-no_ccd]           don't handle IWCD\n");
  fprintf(fp, "    [-notypecheck]      don't check input parameter type\n");
  fprintf(fp, "    [-spmodel HMMname]  name of short pause model             (\"%s\")\n", SPMODEL_NAME_DEFAULT);
  fprintf(fp, "    [-multipath]        switch decoding for multi-path HMM    (auto)\n");

  fprintf(fp, "\n Acoustic Model Computation Method:\n");
  fprintf(fp, "    [-gprune methodname] select Gaussian pruning method:\n");
#ifdef GPRUNE_DEFAULT_SAFE
  fprintf(fp, "             safe          safe pruning (default for TM/PTM)\n");
#else
  fprintf(fp, "             safe          safe pruning\n");
#endif
#if GPRUNE_DEFAULT_HEURISTIC
  fprintf(fp, "             heuristic     heuristic pruning (default for TM/PTM)\n");
#else
  fprintf(fp, "             heuristic     heuristic pruning\n");
#endif
#if GPRUNE_DEFAULT_BEAM
  fprintf(fp, "             beam          beam pruning (default for TM/PTM)\n");
#else
  fprintf(fp, "             beam          beam pruning\n");
#endif
  fprintf(fp, "             none          no pruning (default for non tmix models)\n");
#ifdef ENABLE_PLUGIN
  if (global_plugin_list) {
    if ((id = plugin_get_id("calcmix_get_optname")) >= 0) {
      for(p=global_plugin_list[id];p;p=p->next) {
	func = (FUNC_VOID) p->func;
	(*func)(buf, (int)64);
	fprintf(fp, "             %-14s(calculation plugin #%d)\n", buf, p->source_id);
      }
    }
  }
#endif
  fprintf(fp, "    [-tmix gaussnum]    Gaussian num threshold per mixture for pruning (%d)\n", jconf->am_root->mixnum_thres);
  fprintf(fp, "    [-gshmm hmmdefs]    monophone hmmdefs for GS\n");
  fprintf(fp, "    [-gsnum N]          N-best state will be selected        (%d)\n", jconf->am_root->gs_statenum);

  fprintf(fp, "\n--- Language Model Options (-LM) ---------------------------------\n");

  fprintf(fp, "\n N-gram:\n");
  fprintf(fp, "    -d file.bingram     n-gram file in Julius binary format\n");
  fprintf(fp, "    -nlr file.arpa      forward n-gram file in ARPA format\n");
  fprintf(fp, "    -nrl file.arpa      backward n-gram file in ARPA format\n");
  fprintf(fp, "    [-lmp float float]  weight and penalty (tri: %.1f %.1f mono: %.1f %1.f)\n", DEFAULT_LM_WEIGHT_TRI_PASS1, DEFAULT_LM_PENALTY_TRI_PASS1, DEFAULT_LM_WEIGHT_MONO_PASS1, DEFAULT_LM_PENALTY_MONO_PASS1);
  fprintf(fp, "    [-lmp2 float float]       for 2nd pass (tri: %.1f %.1f mono: %.1f %1.f)\n", DEFAULT_LM_WEIGHT_TRI_PASS2, DEFAULT_LM_PENALTY_TRI_PASS2, DEFAULT_LM_WEIGHT_MONO_PASS2, DEFAULT_LM_PENALTY_MONO_PASS2);
  fprintf(fp, "    [-transp float]     penalty for transparent word (%+2.1f)\n", jconf->search_root->lmp.lm_penalty_trans);

  fprintf(fp, "\n DFA Grammar:\n");
  fprintf(fp, "    -dfa file.dfa       DFA grammar file\n");
  fprintf(fp, "    -gram file[,file2...] (list of) grammar prefix(es)\n");
  fprintf(fp, "    -gramlist filename  filename of grammar list\n");
  fprintf(fp, "    [-penalty1 float]   word insertion penalty (1st pass)     (%.1f)\n", jconf->search_root->lmp.penalty1);
  fprintf(fp, "    [-penalty2 float]   word insertion penalty (2nd pass)     (%.1f)\n", jconf->search_root->lmp.penalty2);

  fprintf(fp, "\n Word Dictionary for N-gram and DFA:\n");
  fprintf(fp, "    -v dictfile         dictionary file name\n");
  fprintf(fp, "    [-silhead wordname] (n-gram) beginning-of-sentence word   (%s)\n", BEGIN_WORD_DEFAULT);
  fprintf(fp, "    [-siltail wordname] (n-gram) end-of-sentence word         (%s)\n", END_WORD_DEFAULT);
  fprintf(fp, "    [-mapunk wordname]  (n-gram) map unknown words to this    (%s)\n", UNK_WORD_DEFAULT);
  fprintf(fp, "    [-forcedict]        ignore error entry and keep running\n");
  fprintf(fp, "    [-iwspword]         (n-gram) add short-pause word for inter-word CD sp\n");
  fprintf(fp, "    [-iwspentry entry]  (n-gram) word entry for \"-iwspword\" (%s)\n", IWSPENTRY_DEFAULT);
  fprintf(fp, "    [-adddict dictfile] (n-gram) load extra dictionary\n");
  fprintf(fp, "    [-addentry entry]   (n-gram) load extra word entry\n");
  
  fprintf(fp, "\n Isolated Word Recognition:\n");
  fprintf(fp, "    -w file[,file2...]  (list of) wordlist file name(s)\n");
  fprintf(fp, "    -wlist filename     file that contains list of wordlists\n");
  fprintf(fp, "    -wsil head tail sp  name of silence/pause model\n");
  fprintf(fp, "                          head - BOS silence model name       (%s)\n", jconf->lm_root->wordrecog_head_silence_model_name);
  fprintf(fp, "                          tail - EOS silence model name       (%s)\n", jconf->lm_root->wordrecog_tail_silence_model_name);
  fprintf(fp, "                           sp  - their name as context or \"NULL\" (%s)\n", (jconf->lm_root->wordrecog_silence_context_name[0] == '\0') ? "NULL" : jconf->lm_root->wordrecog_silence_context_name);
#ifdef DETERMINE
  fprintf(fp, "    -wed float int      thresholds for early word determination\n");
  fprintf(fp, "                        float: score threshold    (%.1f)\n", jconf->search_root->pass1.determine_score_thres);
  fprintf(fp, "                        int: frame duration thres (%d)\n", jconf->search_root->pass1.determine_duration_thres);
#endif

  fprintf(fp, "\n--- Recognizer / Search Options (-SR) ----------------------------\n");


  fprintf(fp, "\n Search Parameters for the First Pass:\n");
  fprintf(fp, "    [-b beamwidth]      beam width (by state num)             (guessed)\n");
  fprintf(fp, "                        (0: full search, -1: force guess)\n");
#ifdef SCORE_PRUNING
  fprintf(fp, "    [-bs score_width]   beam width (by score offset)          (disabled)\n");
  fprintf(fp, "                        (-1: disable)\n");
#endif
#ifdef WPAIR
# ifdef WPAIR_KEEP_NLIMIT
  fprintf(fp, "    [-nlimit N]         keeps only N tokens on each state     (%d)\n", jconf->search_root->pass1.wpair_keep_nlimit);
# endif
#endif
#ifdef SEPARATE_BY_UNIGRAM
  fprintf(fp, "    [-sepnum wordnum]   (n-gram) # of hi-freq word isolated from tree (%d)\n", jconf->lm_root->separate_wnum);
#endif
#ifdef HASH_CACHE_IW
  fprintf(fp, "    [-iwcache percent]  (n-gram) amount of inter-word LM cache (%3d)\n", jconf->search_root->pass1.iw_cache_rate);
#endif
  fprintf(fp, "    [-1pass]            do 1st pass only, omit 2nd pass\n");
  fprintf(fp, "    [-inactive]         recognition process not active on startup\n");

  fprintf(fp, "\n Search Parameters for the Second Pass:\n");
  fprintf(fp, "    [-b2 hyponum]       word envelope beam width (by hypo num) (%d)\n",jconf->search_root->pass2.enveloped_bestfirst_width);
  fprintf(fp, "    [-n N]              # of sentence to find                 (%d)\n", jconf->search_root->pass2.nbest);
  fprintf(fp, "    [-output N]         # of sentence to output               (%d)\n",jconf->search_root->output.output_hypo_maxnum);
#ifdef SCAN_BEAM
  fprintf(fp, "    [-sb score]         score beam threshold (by score)       (%.1f)\n", jconf->search_root->pass2.scan_beam_thres);
#endif
  fprintf(fp, "    [-s hyponum]        global stack size of hypotheses       (%d)\n", jconf->search_root->pass2.stack_size);
  fprintf(fp, "    [-m hyponum]        hypotheses overflow threshold num     (%d)\n", jconf->search_root->pass2.hypo_overflow);

  fprintf(fp, "    [-lookuprange N]    frame lookup range in word expansion  (%d)\n", jconf->search_root->pass2.lookup_range);
  fprintf(fp, "    [-looktrellis]      (dfa) expand only backtrellis words\n");
  fprintf(fp, "    [-[no]multigramout] (dfa) output per-grammar results\n");
  fprintf(fp, "    [-oldtree]          (dfa) use old build_wchmm()\n");
#ifdef PASS1_IWCD
  fprintf(fp, "    [-oldiwcd]          (dfa) use full lcdset\n");
#endif
  fprintf(fp, "    [-iwsp]             insert sp for all word end (multipath)(off)\n");
  fprintf(fp, "    [-iwsppenalty]      trans. penalty for iwsp (multipath)   (%.1f)\n", jconf->am_root->iwsp_penalty);

  fprintf(fp, "\n Short-pause Segmentation:\n");
  fprintf(fp, "    [-spsegment]        enable short-pause segmentation\n");
  fprintf(fp, "    [-spdur]            length threshold of sp frames         (%d)\n", jconf->search_root->successive.sp_frame_duration);
#ifdef SPSEGMENT_NAIST
  fprintf(fp, "    [-spmargin]         backstep margin on speech trigger     (%d)\n", jconf->search_root->successive.sp_margin);
  fprintf(fp, "    [-spdelay]          delay on speech trigger               (%d)\n", jconf->search_root->successive.sp_delay);
#endif
  fprintf(fp, "    [-pausemodels str]  comma-delimited list of pause models for segment\n");

  fprintf(fp, "\n Graph Output with graph-oriented search:\n");
  fprintf(fp, "    [-lattice]          enable word graph (lattice) output\n");
  fprintf(fp, "    [-confnet]          enable confusion network output\n");
  fprintf(fp, "    [-nolattice]][-noconfnet] disable lattice / confnet output\n");
  fprintf(fp, "    [-graphrange N]     merge same words in graph (%d)\n", jconf->search_root->graph.graph_merge_neighbor_range);
  fprintf(fp, "                        -1: not merge, leave same loc. with diff. score\n");
  fprintf(fp, "                         0: merge same words at same location\n");
  fprintf(fp, "                        >0: merge same words around the margin\n");
#ifdef GRAPHOUT_DEPTHCUT
  fprintf(fp, "    [-graphcut num]     graph cut depth at postprocess (-1: disable)(%d)\n", jconf->search_root->graph.graphout_cut_depth);
#endif
#ifdef GRAPHOUT_LIMIT_BOUNDARY_LOOP
  fprintf(fp, "    [-graphboundloop num] max. num of boundary adjustment loop (%d)\n", jconf->search_root->graph.graphout_limit_boundary_loop_num);
#endif
#ifdef GRAPHOUT_SEARCH_DELAY_TERMINATION
  fprintf(fp, "    [-graphsearchdelay] inhibit search termination until 1st sent. found\n");
  fprintf(fp, "    [-nographsearchdelay] disable it (default)\n");
#endif

  fprintf(fp, "\n Forced Alignment:\n");
  fprintf(fp, "    [-walign]           optionally output word alignments\n");
  fprintf(fp, "    [-palign]           optionally output phoneme alignments\n");
  fprintf(fp, "    [-salign]           optionally output state alignments\n");

#ifdef USE_MBR
  fprintf(fp, "\n Minimum Bayes Risk Decoding:\n");
  fprintf(fp, "    [-mbr]              enable rescoring sentence on MBR(WER)\n");
  fprintf(fp, "    [-mbr_wwer]         enable rescoring sentence on MBR(WWER)\n");
  fprintf(fp, "    [-nombr]            disable rescoring sentence on MBR\n");
  fprintf(fp, "    [-mbr_weight float float] score and loss func. weight on MBR (%.1f %.1f)\n", jconf->search_root->mbr.score_weight, jconf->search_root->mbr.loss_weight);
#endif

#ifdef CONFIDENCE_MEASURE
  fprintf(fp, "\n Confidence Score:\n");
#ifdef CM_MULTIPLE_ALPHA
  fprintf(fp, "    [-cmalpha f t s]    CM smoothing factor        (from, to, step)\n");
#else
  fprintf(fp, "    [-cmalpha value]    CM smoothing factor                    (%f)\n", jconf->search_root->annotate.cm_alpha);
#endif
#ifdef CM_SEARCH_LIMIT
  fprintf(fp, "    [-cmthres value]    CM threshold to cut hypo on 2nd pass   (%f)\n", jconf->search_root->annotate.cm_cut_thres);
#endif
#endif /* CONFIDENCE_MEASURE */
  fprintf(fp, "\n Message Output:\n");
  fprintf(fp, "    [-fallback1pass]    use 1st pass result when search failed\n");
  fprintf(fp, "    [-progout]          progressive output in 1st pass\n");
  fprintf(fp, "    [-proginterval]     interval of progout in msec           (%d)\n", jconf->search_root->output.progout_interval);

  fprintf(fp, "\n-------------------------------------------------\n");

  j_jconf_free(jconf);

  /* output application-side options */
  useropt_show_desc(fp);

}

/* end of file */
