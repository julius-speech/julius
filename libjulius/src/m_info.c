/**
 * @file   m_info.c
 * 
 * <JA>
 * @brief  システム情報の出力
 * </JA>
 * 
 * <EN>
 * @brief  Output system informations.
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Thu May 12 14:14:01 2005
 *
 * $Revision: 1.23 $
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
 * <EN>
 * Output module overview in a global configuration variables to log.
 * </EN>
 * <JA>
 * 全体設定パラメータ内のモジュール構成の概要をログに出力する. 
 * </JA>
 * 
 * @param jconf [in] global configuration variables
 *
 * @callgraph
 * @callergraph
 * 
 */
void
print_jconf_overview(Jconf *jconf)
{
  JCONF_AM *amconf;
  JCONF_LM *lmconf;
  JCONF_SEARCH *sconf;
  GRAMLIST *g;
  int i, n;

  jlog("------------------------------------------------------------\n");
  jlog("Configuration of Modules\n\n");
  jlog(" Number of defined modules:");
  i = 0; for(amconf=jconf->am_root;amconf;amconf=amconf->next) i++;
  jlog(" AM=%d,", i);
  i = 0; for(lmconf=jconf->lm_root;lmconf;lmconf=lmconf->next) i++;
  jlog(" LM=%d,", i);
  i = 0; for(sconf=jconf->search_root;sconf;sconf=sconf->next) i++;
  jlog(" SR=%d\n", i);
  
  jlog("\n");
  
  jlog(" Acoustic Model (with input parameter spec.):\n");
  for(amconf=jconf->am_root;amconf;amconf=amconf->next) {
    if (amconf->name[0] != '\0') {
      jlog(" - AM%02d \"%s\"\n", amconf->id, amconf->name);
    } else {
      jlog(" - AM%02d\n", amconf->id);
    }
    jlog("\thmmfilename=%s\n",amconf->hmmfilename);
    if (amconf->mapfilename != NULL) {
      jlog("\thmmmapfilename=%s\n",amconf->mapfilename);
    }
    if (amconf->hmm_gs_filename != NULL) {
      jlog("\thmmfile for Gaussian Selection: %s\n", amconf->hmm_gs_filename);
    }
  }
  jlog("\n");
  
  jlog(" Language Model:\n");
  for(lmconf=jconf->lm_root;lmconf;lmconf=lmconf->next) {
    if (lmconf->name[0] != '\0') {
      jlog(" - LM%02d \"%s\"\n", lmconf->id, lmconf->name);
    } else {
      jlog(" - LM%02d\n", lmconf->id);
    }
    if (lmconf->lmtype == LM_PROB) {
      jlog("\tvocabulary filename=%s\n",lmconf->dictfilename);
      if (lmconf->ngram_filename != NULL) {
	jlog("\tn-gram  filename=%s (binary format)\n", lmconf->ngram_filename);
      } else {
	if (lmconf->ngram_filename_rl_arpa != NULL) {
	  jlog("\tbackward n-gram filename=%s\n", lmconf->ngram_filename_rl_arpa);
	  if (lmconf->ngram_filename_lr_arpa != NULL) {
	    jlog("\tforward 2-gram for pass1=%s\n", lmconf->ngram_filename_lr_arpa);
	  }
	} else if (lmconf->ngram_filename_lr_arpa != NULL) {
	  jlog("\tforward n-gram filename=%s\n", lmconf->ngram_filename_lr_arpa);
	}
      }
    }
    if (lmconf->lmtype == LM_DFA) {
      switch(lmconf->lmvar) {
      case LM_DFA_GRAMMAR:
	n = 1;
	for(g = lmconf->gramlist_root; g; g = g->next) {
	  jlog("\tgrammar #%d:\n", n++);
	  jlog("\t    dfa  = %s\n", g->dfafile);
	  jlog("\t    dict = %s\n", g->dictfile);
	}
	break;
      case LM_DFA_WORD:
	n = 1;
	for(g = lmconf->wordlist_root; g; g = g->next) {
	  jlog("\twordlist #%d: %s\n", n++, g->dictfile);
	}
	break;
      }
    }
  }
  jlog("\n");
  jlog(" Recognizer:\n");
  for(sconf=jconf->search_root; sconf; sconf=sconf->next) {
    if (sconf->name[0] != '\0') {
      jlog(" - SR%02d \"%s\"", sconf->id, sconf->name);
    } else {
      jlog(" - SR%02d", sconf->id);
    }
    jlog(" (AM%02d, LM%02d)\n", sconf->amconf->id, sconf->lmconf->id);
  }
  jlog("\n");
}


/**
 * Output feature parameter processing information to log.
 *
 * @param mfcc [in] MFCC instance
 *
 * @callgraph
 * @callergraph
 */
void
print_mfcc_info(FILE *fp, MFCCCalc *mfcc, Jconf *jconf)
{
  put_para(fp, mfcc->para);

  jlog("\n");

  if (jconf->input.type == INPUT_WAVEFORM) {
    jlog("    spectral subtraction = ");
    if (mfcc->frontend.ssload_filename || mfcc->frontend.sscalc) {
      if (mfcc->frontend.sscalc) {
	jlog("use head silence of each input\n");
	jlog("\t     head sil length = %d msec\n", mfcc->frontend.sscalc_len);
      } else {			/* ssload_filename != NULL */
	jlog("use a constant value from file\n");
	jlog("         noise spectrum file = \"%s\"\n", mfcc->frontend.ssload_filename);
      }
      jlog("\t         alpha coef. = %f\n", mfcc->frontend.ss_alpha);
      jlog("\t      spectral floor = %f\n", mfcc->frontend.ss_floor);
    } else {
      jlog("off\n");
    }
  }
  jlog("\n");
  jlog(" cep. mean normalization = ");
  if (mfcc->para->cmn) {
    jlog("yes, ");
    if (jconf->decodeopt.realtime_flag) {
      jlog("real-time MAP-CMN, updating mean with last %.1f sec. input\n");
      jlog("  initial mean from file = ");
      if (mfcc->cmn.loaded) {
	jlog("%s\n", mfcc->cmn.load_filename);
      } else {
	jlog("N/A\n");
      }
      jlog("   beginning data weight = %6.2f\n", mfcc->cmn.map_weight);
    } else {
      if (mfcc->cmn.loaded) {
	jlog("with a static mean\n");
	jlog("   static mean from file = %s\n", mfcc->cmn.load_filename);
      } else {
	jlog("with per-utterance self mean\n");
      }
    }
  } else {
    jlog("no\n");
  }
  jlog(" cep. var. normalization = ");
  if (mfcc->para->cvn) {
    jlog("yes, ");
    if (mfcc->cmn.loaded) {
      jlog("with a static variance\n");
      jlog("static variance from file = %s\n", mfcc->cmn.load_filename);
    } else {
      if (jconf->decodeopt.realtime_flag) {
	jlog("estimating long-term variance from all speech input from start\n");
      } else {
	jlog("with per-utterance self variance\n");
      }
    }
  } else {
    jlog("no\n");
  }
  if (mfcc->cmn.save_filename) {
    jlog("        save cep. data to = \"%s\", update at the end of each input\n", mfcc->cmn.save_filename);
  }
  jlog("\n");
  
  jlog("\t base setup from =");
  if (mfcc->htk_loaded == 1 || mfcc->hmm_loaded == 1) {
    if (mfcc->hmm_loaded == 1) {
      jlog(" binhmm-embedded");
      if (mfcc->htk_loaded == 1) {
	jlog(", then overridden by HTK Config and defaults");
      }
    } else {
      if (mfcc->htk_loaded == 1) {
	jlog(" HTK Config (and HTK defaults)");
      }
    }
  } else {
    jlog(" Julius defaults");
  }
  jlog("\n");
}


/** 
 * <JA>
 * エンジンインスタンスの全情報をログに出力する. 
 * </JA>
 * <EN>
 * Output all informations of an engine instance to log.
 * </EN>
 *
 * @param recog [in] engine instance
 * 
 * @callgraph
 * @callergraph
 */
void
print_engine_info(Recog *recog)
{
  FILE *fp;
  Jconf *jconf;
  MFCCCalc *mfcc;
  PROCESS_AM *am;
  PROCESS_LM *lm;
  RecogProcess *r;

  jconf = recog->jconf;
  
  /* set output file pointer to fp */
  fp = jlog_get_fp();
  if (fp == NULL) return;

  jlog("----------------------- System Information begin ---------------------\n");
  j_put_header(fp);
  j_put_compile_defs(fp);
  jlog("\n");
  
  /* print current argument setting to log */
  print_jconf_overview(jconf);

  if (jconf->input.type == INPUT_WAVEFORM) {

    /* acoustic parameter conditions for this model */
    jlog("------------------------------------------------------------\n");
    jlog("Speech Analysis Module(s)\n\n");
    
    for(mfcc=recog->mfcclist;mfcc;mfcc=mfcc->next) {

      jlog("[MFCC%02d]  for", mfcc->id);
      for(am=recog->amlist;am;am=am->next) {
	if (am->mfcc == mfcc) {
	  jlog(" [AM%02d %s]", am->config->id, am->config->name);
	}
      }
      if (recog->gmm != NULL) {
	if (recog->gmmmfcc == mfcc) {
	  jlog(" [GMM]");
	}
      }
      jlog("\n\n");

      print_mfcc_info(fp, mfcc, jconf);

      jlog("\n");

    }
  }


  if (recog->gmm != NULL) {
    jlog("------------------------------------------------------------\n");
    jlog("GMM\n");
    jlog("\n");
    jlog("     GMM definition file = %s\n", jconf->reject.gmm_filename);
    jlog("          GMM gprune num = %d\n", jconf->reject.gmm_gprune_num);
    if (jconf->reject.gmm_reject_cmn_string != NULL) {
      jlog("     GMM names to reject = %s\n", jconf->reject.gmm_reject_cmn_string);
    }
#ifdef GMM_VAD
    jlog("\n GMM-based VAD\n\n");
    jlog("       backstep on trigger = %d frames\n", jconf->detect.gmm_margin);
    jlog("    up-trigger thres score = %.1f\n", jconf->detect.gmm_uptrigger_thres);
    jlog("  down-trigger thres score = %.1f\n", jconf->detect.gmm_downtrigger_thres);
#endif
    jlog("\n GMM");
    print_hmmdef_info(fp, recog->gmm);
    jlog("\n");
  }

  jlog("------------------------------------------------------------\n");
  jlog("Acoustic Model(s)\n");
  jlog("\n");

  for(am = recog->amlist; am; am = am->next) {
    if (am->config->name[0] != '\0') {
      jlog("[AM%02d \"%s\"]\n\n", am->config->id, am->config->name);
    } else {
      jlog("[AM%02d]\n\n", am->config->id);
    }
    print_hmmdef_info(fp, am->hmminfo);
    jlog("\n");
    if (am->config->hmm_gs_filename != NULL) {
      jlog("GS ");
      print_hmmdef_info(fp, am->hmm_gs);
      jlog("\n");
    }

    jlog(" AM Parameters:\n");

    jlog("        Gaussian pruning = ");
    switch(am->config->gprune_method){
    case GPRUNE_SEL_NONE: jlog("none (full computation)"); break;
    case GPRUNE_SEL_BEAM: jlog("beam"); break;
    case GPRUNE_SEL_HEURISTIC: jlog("heuristic"); break;
    case GPRUNE_SEL_SAFE: jlog("safe"); break;
    case GPRUNE_SEL_USER: jlog("(use plugin function)"); break;
    }
    jlog("  (-gprune)\n");
    if (am->config->gprune_method != GPRUNE_SEL_NONE
	&& am->config->gprune_method != GPRUNE_SEL_USER) {
      jlog("  top N mixtures to calc = %d / %d  (-tmix)\n", am->config->mixnum_thres, am->hmminfo->maxcodebooksize);
    }
    if (am->config->hmm_gs_filename != NULL) {
      jlog("      GS state num thres = %d / %d selected  (-gsnum)\n", am->config->gs_statenum, am->hmm_gs->totalstatenum);
    }
    jlog("    short pause HMM name = \"%s\" specified", am->config->spmodel_name);
    if (am->hmminfo->sp != NULL) {
      jlog(", \"%s\" applied", am->hmminfo->sp->name);
      if (am->hmminfo->sp->is_pseudo) {
	jlog(" (pseudo)");
      } else {
	jlog(" (physical)");
      }
    } else {
      jlog(" but not assigned");
    }
    jlog("  (-sp)\n");
    jlog("  cross-word CD on pass1 = ");
#ifdef PASS1_IWCD
    jlog("handle by approx. ");
    switch(am->hmminfo->cdset_method) {
    case IWCD_AVG:
      jlog("(use average prob. of same LC)\n");
      break;
    case IWCD_MAX:
      jlog("(use max. prob. of same LC)\n");
      break;
    case IWCD_NBEST:
      jlog("(use %d-best of same LC)\n", am->hmminfo->cdmax_num);
      break;
    }
#else
    jlog("disabled\n");
#endif
    
    if (am->hmminfo->multipath) {
      jlog("   sp transition penalty = %+2.1f\n", am->config->iwsp_penalty);
    }

    jlog("\n");
  }

  jlog("------------------------------------------------------------\n");
  jlog("Language Model(s)\n");

  for(lm = recog->lmlist; lm; lm = lm->next) {
    jlog("\n");
    if (lm->config->name[0] != '\0') {
      jlog("[LM%02d \"%s\"]", lm->config->id, lm->config->name);
    } else {
      jlog("[LM%02d]", lm->config->id);
    }
    if (lm->lmtype == LM_PROB) {
      if (lm->lmvar == LM_NGRAM) {
	jlog(" type=n-gram\n\n");
	if (lm->ngram) {
	  print_ngram_info(fp, lm->ngram); jlog("\n");
	}
      } else if (lm->lmvar == LM_NGRAM_USER) {
	if (lm->ngram) {
	  jlog(" type=n-gram + user\n\n");
	  print_ngram_info(fp, lm->ngram); jlog("\n");
	} else {
	  jlog(" type=user\n\n");
	}
      } else {
	jlog(" type=UNKNOWN??\n\n");
      }
    } else if (lm->lmtype == LM_DFA) {
      if (lm->lmvar == LM_DFA_GRAMMAR) {
	jlog(" type=grammar\n\n");
	if (lm->dfa) {
	  print_dfa_info(fp, lm->dfa); jlog("\n");
	  if (debug2_flag) {
	    print_dfa_cp(fp, lm->dfa);
	    jlog("\n");
	  }
	}
      } else if (lm->lmvar == LM_DFA_WORD) {
	jlog(" type=word\n\n");
      } else {
	jlog(" type=UNKNOWN??\n\n");
      }
    } else {
      jlog(" type=UNKNOWN??\n\n");
    }
    if (lm->winfo != NULL) {
      print_voca_info(fp, lm->winfo); jlog("\n");
    }

    jlog(" Parameters:\n");

    if (lm->lmtype == LM_DFA && lm->lmvar == LM_DFA_GRAMMAR) {
      if (lm->dfa != NULL) {
	int i;
	jlog("   found sp category IDs =");
	for(i=0;i<lm->dfa->term_num;i++) {
	  if (lm->dfa->is_sp[i]) {
	    jlog(" %d", i);
	  }
	}
	jlog("\n");
      }
    }
    
    if (lm->lmtype == LM_PROB) {
      if (lm->config->enable_iwspword) {
	jlog("\tIW-sp word added to dict= \"%s\"\n", lm->config->iwspentry);
      }
      if (lm->config->additional_dict_files) {
	JCONF_LM_NAMELIST *nl;
	jlog("\tadditional dictionaries:\n");
	for(nl=lm->config->additional_dict_files;nl;nl=nl->next) {
	  jlog("\t\t\t%s\n", nl->name);
	}
	jlog("\n");
      }
      if (lm->config->additional_dict_entries) {
	JCONF_LM_NAMELIST *nl;
	int n = 0;
	jlog("\tadditional dict entries:\n");
	for(nl=lm->config->additional_dict_entries;nl;nl=nl->next) {
	  jlog("\t\t\t%s\n", nl->name);
	  n++;
	}
	jlog("--- total %d entries\n", n);
      }
    }

    if (lm->lmtype == LM_PROB) {    
      jlog("\t(-silhead)head sil word = ");
      put_voca(fp, lm->winfo, lm->winfo->head_silwid);
      jlog("\t(-siltail)tail sil word = ");
      put_voca(fp, lm->winfo, lm->winfo->tail_silwid);
    }

    if (lm->lmvar == LM_DFA_WORD) {
      jlog("     silence model names to add at word head / tail:  (-wsil)\n");
      jlog("\tword head          = \"%s\"\n", lm->config->wordrecog_head_silence_model_name);
      jlog("\tword tail          = \"%s\"\n", lm->config->wordrecog_tail_silence_model_name);
      jlog("\ttheir context name = \"%s\"\n", (lm->config->wordrecog_silence_context_name[0] == '\0') ? "NULL (blank)" : lm->config->wordrecog_silence_context_name);
      
    }

  }

  jlog("\n");
  jlog("------------------------------------------------------------\n");
  jlog("Recognizer(s)\n\n");

  for(r = recog->process_list; r; r = r->next) {
    jlog("[SR%02d", r->config->id);
    if (r->config->name[0] != '\0') {
      jlog(" \"%s\"", r->config->name);
    }
    jlog("]  ");
    if (r->am->config->name[0] != '\0') {
      jlog("AM%02d \"%s\"", r->am->config->id, r->am->config->name);
    } else {
      jlog("AM%02d", r->am->config->id);
    }
    jlog("  +  ");
    if (r->lm->config->name[0] != '\0') {
      jlog("LM%02d \"%s\"", r->lm->config->id, r->lm->config->name);
    } else {
      jlog("LM%02d", r->lm->config->id);
    }
    jlog("\n\n");

    if (r->wchmm != NULL) {
      print_wchmm_info(r->wchmm); jlog("\n");
    }
    if (r->lmtype == LM_PROB) {
      jlog(" Inter-word N-gram cache: \n");
      {
	int num, len;
#ifdef UNIGRAM_FACTORING
	len = r->wchmm->isolatenum;
	jlog("\troot node to be cached = %d / %d (isolated only)\n",
	     len, r->wchmm->startnum);
#else
	len = r->wchmm->startnum;
	jlog("\troot node to be cached = %d (all)\n", len);
#endif
#ifdef HASH_CACHE_IW
	num = (r->config->pass1.iw_cache_rate * r->lm->ngram->max_word_num) / 100;
	jlog("\tword ends to be cached = %d / %d\n", num, r->lm->ngram->max_word_num);
#else
	num = r->lm->ngram->max_word_num;
	jlog("\tword ends to be cached = %d (all)\n", num);
#endif
	jlog("\t  max. allocation size = %dMB\n", num * len / 1000 * sizeof(LOGPROB) / 1000);
      }
    }

    if (r->lmtype == LM_PROB) {
      jlog("\t(-lmp)  pass1 LM weight = %2.1f  ins. penalty = %+2.1f\n", r->config->lmp.lm_weight, r->config->lmp.lm_penalty);
      jlog("\t(-lmp2) pass2 LM weight = %2.1f  ins. penalty = %+2.1f\n", r->config->lmp.lm_weight2, r->config->lmp.lm_penalty2);
      jlog("\t(-transp)trans. penalty = %+2.1f per word\n", r->config->lmp.lm_penalty_trans);
    } else if (r->lmtype == LM_DFA && r->lmvar == LM_DFA_GRAMMAR) {
      jlog("\t(-penalty1) IW penalty1 = %+2.1f\n", r->config->lmp.penalty1);
      jlog("\t(-penalty2) IW penalty2 = %+2.1f\n", r->config->lmp.penalty2);
    }


#ifdef CONFIDENCE_MEASURE
#ifdef CM_MULTIPLE_ALPHA
    jlog("\t(-cmalpha)CM alpha coef = from %f to %f by step of %f (%d outputs)\n", r->config->annotate.cm_alpha_bgn, r->config->annotate.cm_alpha_end, r->config->annotate.cm_alpha_step, r->config->annotate.cm_alpha_num);
#else
    jlog("\t(-cmalpha)CM alpha coef = %f\n", r->config->annotate.cm_alpha);
#endif
#ifdef CM_SEARCH_LIMIT
    jlog("\t(-cmthres) CM cut thres = %f for hypo generation\n", r->config->annotate.cm_cut_thres);
#endif
#ifdef CM_SEARCH_LIMIT_POP
    jlog("\t(-cmthres2)CM cut thres = %f for popped hypo\n", r->config->annotate.cm_cut_thres_pop);
#endif
#endif /* CONFIDENCE_MEASURE */
    jlog("\n");

    if (r->am->hmminfo->multipath) {
      if (r->lm->config->enable_iwsp) {
	jlog("\t inter-word short pause = on (append \"%s\" for each word tail)\n", r->am->hmminfo->sp->name);
	jlog("\t  sp transition penalty = %+2.1f\n", r->am->config->iwsp_penalty);
      }
    }

    if (r->lmvar == LM_DFA_WORD) {
#ifdef DETERMINE
      jlog("    early word determination:  (-wed)\n");
      jlog("\tscore threshold    = %f\n", r->config->pass1.determine_score_thres);
      jlog("\tframe dur. thres   = %d\n", r->config->pass1.determine_duration_thres);
#endif
    }

    jlog(" Search parameters: \n");
    jlog("\t    multi-path handling = ");
    if (r->am->hmminfo->multipath) {
      jlog("yes, multi-path mode enabled\n");
    } else {
      jlog("no\n");
    }
    jlog("\t(-b) trellis beam width = %d", r->trellis_beam_width);
    if (r->config->pass1.specified_trellis_beam_width == -1) {
      jlog(" (-1 or not specified - guessed)\n");
    } else if (r->config->pass1.specified_trellis_beam_width == 0) {
      jlog(" (0 - full)\n");
    } else {
      jlog("\n");
    }
#ifdef SCORE_PRUNING
    if (r->config->pass1.score_pruning_width < 0.0) {
      jlog("\t(-bs)score pruning thres= disabled\n");
    } else {
      jlog("\t(-bs)score pruning thres= %f\n", r->config->pass1.score_pruning_width);
    }
#endif
    jlog("\t(-n)search candidate num= %d\n", r->config->pass2.nbest);
    jlog("\t(-s)  search stack size = %d\n", r->config->pass2.stack_size);
    jlog("\t(-m)    search overflow = after %d hypothesis poped\n", r->config->pass2.hypo_overflow);
    jlog("\t        2nd pass method = ");
    if (r->config->graph.enabled) {
#ifdef GRAPHOUT_DYNAMIC
#ifdef GRAPHOUT_SEARCH
      jlog("searching graph, generating dynamic graph\n");
#else
      jlog("searching sentence, generating dynamic graph\n");
#endif /* GRAPHOUT_SEARCH */
#else  /* ~GRAPHOUT_DYNAMIC */
      jlog("searching sentence, generating static graph from N-best\n");
#endif
    } else {
      jlog("searching sentence, generating N-best\n");
    }
    if (r->config->pass2.enveloped_bestfirst_width >= 0) {
      jlog("\t(-b2)  pass2 beam width = %d\n", r->config->pass2.enveloped_bestfirst_width);
    }
    jlog("\t(-lookuprange)lookup range= %d  (tm-%d <= t <tm+%d)\n",r->config->pass2.lookup_range,r->config->pass2.lookup_range,r->config->pass2.lookup_range);
#ifdef SCAN_BEAM
    jlog("\t(-sb)2nd scan beamthres = %.1f (in logscore)\n", r->config->pass2.scan_beam_thres);
#endif
    jlog("\t(-n)        search till = %d candidates found\n", r->config->pass2.nbest);
    jlog("\t(-output)    and output = %d candidates out of above\n", r->config->output.output_hypo_maxnum);

    if (r->ccd_flag) {
      jlog("\t IWCD handling:\n");
#ifdef PASS1_IWCD
      jlog("\t   1st pass: approximation ");
      switch(r->am->hmminfo->cdset_method) {
      case IWCD_AVG:
	jlog("(use average prob. of same LC)\n");
	break;
      case IWCD_MAX:
	jlog("(use max. prob. of same LC)\n");
	break;
      case IWCD_NBEST:
	jlog("(use %d-best of same LC)\n", r->am->hmminfo->cdmax_num);
	break;
      }
#else
      jlog("\t   1st pass: ignored\n");
#endif
#ifdef PASS2_STRICT_IWCD
      jlog("\t   2nd pass: strict (apply when expanding hypo. )\n");
#else
      jlog("\t   2nd pass: loose (apply when hypo. is popped and scanned)\n");
#endif
    }
    if (r->lmtype == LM_PROB) {
      jlog("\t factoring score: ");
#ifdef UNIGRAM_FACTORING
      jlog("1-gram prob. (statically assigned beforehand)\n");
#else
      jlog("2-gram prob. (dynamically computed while search)\n");
#endif
    }

    if (r->config->annotate.align_result_word_flag) {
      jlog("\t output word alignments\n");
    }
    if (r->config->annotate.align_result_phoneme_flag) {
      jlog("\t output phoneme alignments\n");
    }
    if (r->config->annotate.align_result_state_flag) {
      jlog("\t output state alignments\n");
    }
    if (r->lmtype == LM_DFA && r->lmvar == LM_DFA_GRAMMAR) {
      if (r->config->pass2.looktrellis_flag) {
	jlog("\t only words in backtrellis will be expanded in 2nd pass\n");
      } else {
	jlog("\t all possible words will be expanded in 2nd pass\n");
      }
    }
    if (r->wchmm != NULL) {
      if (r->wchmm->category_tree) {
	if (r->config->pass1.old_tree_function_flag) {
	  jlog("\t build_wchmm() used\n");
	} else {
	  jlog("\t build_wchmm2() used\n");
	}
#ifdef PASS1_IWCD
#ifdef USE_OLD_IWCD
	jlog("\t full lcdset used\n");
#else
	jlog("\t lcdset limited by word-pair constraint\n");
#endif
#endif /* PASS1_IWCD */
      }
    }
    if (r->config->output.progout_flag) {
      jlog("\tprogressive output on 1st pass\n");
    }
    if (r->config->compute_only_1pass) {
      jlog("\tCompute only 1-pass\n");
    }
    
    if (r->config->graph.enabled) {
      jlog("\n");
      jlog("Graph-based output with graph-oriented search:\n");
      jlog("\t(-lattice)      word lattice = %s\n", r->config->graph.lattice ? "yes" : "no");
      jlog("\t(-confnet) confusion network = %s\n", r->config->graph.confnet ? "yes" : "no");
      if (r->config->graph.lattice == TRUE) {
	jlog("\t(-graphrange)         margin = %d frames", r->config->graph.graph_merge_neighbor_range);
	if (r->config->graph.graph_merge_neighbor_range < 0) {
	  jlog(" (all post-marging disabled)\n");
	} else if (r->config->graph.graph_merge_neighbor_range == 0) {
	  jlog(" (merge same word with the same boundary)\n");
	} else {
	  jlog(" (merge same words around this margin)\n");
	}
      }
#ifdef GRAPHOUT_DEPTHCUT
      jlog("\t(-graphcut)cutoff depth      = ");
      if (r->config->graph.graphout_cut_depth < 0) {
	jlog("disabled (-1)\n");
      } else {
	jlog("%d words\n",r->config->graph.graphout_cut_depth);
      }
#endif
#ifdef GRAPHOUT_LIMIT_BOUNDARY_LOOP
      jlog("\t(-graphboundloop)loopmax     = %d for boundary adjustment\n",r->config->graph.graphout_limit_boundary_loop_num);
#endif
#ifdef GRAPHOUT_SEARCH_DELAY_TERMINATION
      jlog("\tInhibit graph search termination before 1st sentence found = ");
      if (r->config->graph.graphout_search_delay) {
	jlog("enabled\n");
      } else {
	jlog("disabled\n");
      }
#endif

    }
    
    if (r->config->successive.enabled) {
      jlog("\tshort pause segmentation = on\n");
      jlog("\t      sp duration length = %d frames\n", r->config->successive.sp_frame_duration);
#ifdef SPSEGMENT_NAIST
      jlog("      backstep margin on trigger = %d frames\n", r->config->successive.sp_margin);
      jlog("\t        delay on trigger = %d frames\n", r->config->successive.sp_delay);
#endif
      if (r->config->successive.pausemodelname) {
	jlog("\t   pause models for seg. = %s\n",  r->config->successive.pausemodelname);
      }
    } else {
      jlog("\tshort pause segmentation = off\n");
    }
    if (r->config->output.progout_flag) {
      jlog("\t        progout interval = %d msec\n", r->config->output.progout_interval);
    }
    jlog("\tfall back on search fail = ");
    if (r->config->sw.fallback_pass1_flag) {
      jlog("on, adopt 1st pass result as final\n");
    } else {
      jlog("off, returns search failure\n");
    }
#ifdef USE_MBR
    if (r->config->mbr.use_mbr) {
      jlog("\n");
      jlog("Minimum Bayes Risk Decoding:\n");
      jlog("\t(-mbr)        sentence rescoring on MBR = %s\n", r->config->mbr.use_mbr ? "yes" : "no");
      jlog("\t(-mbr_wwer)   use word weight on MBR = %s\n", r->config->mbr.use_word_weight ? "yes" : "no");
      jlog("\t(-mbr_weight) score weight = %2.1f  loss func. weight  = %2.1f\n", r->config->mbr.score_weight, r->config->mbr.loss_weight);
    }
#endif
    jlog("\n");
  }

  jlog("------------------------------------------------------------\n");
  jlog("Decoding algorithm:\n\n");
  jlog("\t1st pass input processing = ");
  if (jconf->decodeopt.force_realtime_flag) jlog("(forced) ");
  if (jconf->decodeopt.realtime_flag) {
    jlog("real time, on-the-fly\n");
  } else {
    jlog("buffered, batch\n");
  }
  jlog("\t1st pass method = ");
#ifdef WPAIR
# ifdef WPAIR_KEEP_NLIMIT
  jlog("word-pair approx., keeping only N tokens ");
# else
  jlog("word-pair approx. ");
# endif
#else
  jlog("1-best approx. ");
#endif
#ifdef WORD_GRAPH
  jlog("generating word_graph\n");
#else
  jlog("generating indexed trellis\n");
#endif
#ifdef CONFIDENCE_MEASURE
  jlog("\toutput word confidence measure ");
#ifdef CM_NBEST
  jlog("based on N-best candidates\n");
#endif
#ifdef CM_SEARCH
  jlog("based on search-time scores\n");
#endif
#endif /* CONFIDENCE_MEASURE */
  
  jlog("\n");

  jlog("------------------------------------------------------------\n");
  jlog("FrontEnd:\n\n");

  jlog(" Input stream:\n");
  jlog("\t             input type = ");
  switch(jconf->input.type) {
  case INPUT_WAVEFORM:
    jlog("waveform\n");
    break;
  case INPUT_VECTOR:
    jlog("feature vector sequence\n");
    break;
  }
  jlog("\t           input source = ");
  if (jconf->input.plugin_source != -1) {
    jlog("plugin\n");
  } else if (jconf->input.speech_input == SP_RAWFILE) {
    jlog("waveform file\n");
    jlog("\t          input filelist = ");
    if (jconf->input.inputlist_filename == NULL) {
      jlog("(none, get file name from stdin)\n");
    } else {
      jlog("%s\n", jconf->input.inputlist_filename);
    }
  } else if (jconf->input.speech_input == SP_MFCFILE) {
    jlog("feature vector file (HTK format)\n");
    jlog("\t                filelist = ");
    if (jconf->input.inputlist_filename == NULL) {
      jlog("(none, get file name from stdin)\n");
    } else {
      jlog("%s\n", jconf->input.inputlist_filename);
    }
  } else if (jconf->input.speech_input == SP_OUTPROBFILE) {
    jlog("output probability vector file (HTK format)\n");
    jlog("\t                filelist = ");
    if (jconf->input.inputlist_filename == NULL) {
      jlog("(none, get file name from stdin)\n");
    } else {
      jlog("%s\n", jconf->input.inputlist_filename);
    }
  } else if (jconf->input.speech_input == SP_MFCMODULE) {
    jlog("vector input module (feature or outprob)\n");
  } else if (jconf->input.speech_input == SP_STDIN) {
    jlog("standard input\n");
  } else if (jconf->input.speech_input == SP_ADINNET) {
    jlog("adinnet client\n");
#ifdef USE_NETAUDIO
  } else if (jconf->input.speech_input == SP_NETAUDIO) {
    char *p;
    jlog("NetAudio server on ");
    if (jconf->input.netaudio_devname != NULL) {
      jlog("%s\n", jconf->input.netaudio_devname);
    } else if ((p = getenv("AUDIO_DEVICE")) != NULL) {
      jlog("%s\n", p);
    } else {
      jlog("local port\n");
    }
#endif
  } else if (jconf->input.speech_input == SP_MIC) {
    jlog("microphone\n");
    jlog("\t    device API          = ");
    switch(jconf->input.device) {
    case SP_INPUT_DEFAULT: jlog("default\n"); break;
    case SP_INPUT_ALSA: jlog("alsa\n"); break;
    case SP_INPUT_OSS: jlog("oss\n"); break;
    case SP_INPUT_ESD: jlog("esd\n"); break;
    case SP_INPUT_PULSEAUDIO: jlog("pulseaudio\n"); break;
    }
  }
  if (jconf->input.type == INPUT_WAVEFORM) {
    if (jconf->input.speech_input == SP_RAWFILE || jconf->input.speech_input == SP_STDIN || jconf->input.speech_input == SP_ADINNET) {
      if (jconf->input.use_ds48to16) {
	jlog("\t          sampling freq. = assume 48000Hz, then down to %dHz\n", jconf->input.sfreq);
      } else {
	jlog("\t          sampling freq. = %d Hz required\n", jconf->input.sfreq);
      }
    } else {
      if (jconf->input.use_ds48to16) {
	jlog("\t          sampling freq. = 48000Hz, then down to %d Hz\n", jconf->input.sfreq);
      } else {
 	jlog("\t          sampling freq. = %d Hz\n", jconf->input.sfreq);
      }
    }
  }
  if (jconf->input.type == INPUT_WAVEFORM) {
    jlog("\t         threaded A/D-in = ");
#ifdef HAVE_PTHREAD
    if (recog->adin->enable_thread) {
      jlog("supported, on\n");
    } else {
      jlog("supported, off\n");
    }
#else
    jlog("not supported (live input may be dropped)\n");
#endif
  }
  if (jconf->input.speech_input == SP_OUTPROBFILE) {
    jlog("\t   zero frames stripping = disabled for outprob input\n");
  } else {
    if (jconf->preprocess.strip_zero_sample) {
      jlog("\t   zero frames stripping = on\n");
    } else {
      jlog("\t   zero frames stripping = off\n");
    }
  }
  if (jconf->input.type == INPUT_WAVEFORM) {
    if (recog->adin->adin_cut_on) {
      jlog("\t         silence cutting = on\n");
      jlog("\t             level thres = %d / 32767\n", jconf->detect.level_thres);
      jlog("\t         zerocross thres = %d / sec.\n", jconf->detect.zero_cross_num);
      jlog("\t             head margin = %d msec.\n", jconf->detect.head_margin_msec);
      jlog("\t             tail margin = %d msec.\n", jconf->detect.tail_margin_msec);
      jlog("\t              chunk size = %d samples\n", jconf->detect.chunk_size);
    } else {
      jlog("\t         silence cutting = off\n");
    }

    if (jconf->preprocess.use_zmean) {
      jlog("\t    long-term DC removal = on");
      if (jconf->input.speech_input == SP_RAWFILE) {
	jlog(" (will compute for each file)\n");
      } else {
	jlog(" (will compute from first %.1f sec)\n", (float)ZMEANSAMPLES / (float)jconf->input.sfreq);
      }
    } else {
      jlog("\t    long-term DC removal = off\n");
    }
    jlog("\t    long-term DC removal = off\n");
    if (jconf->preprocess.level_coef != 1.0) {
      jlog("\t    level scaling factor = %.2f\n", jconf->preprocess.level_coef);
    } else {
      jlog("\t    level scaling factor = %.2f (disabled)\n", jconf->preprocess.level_coef);
    }
  }
  jlog("\t      reject short input = ");
  if (jconf->reject.rejectshortlen > 0) {
    jlog("< %d msec\n", jconf->reject.rejectshortlen);
  } else {
    jlog("off\n");
  }
  jlog("\t      reject  long input = ");
  if (jconf->reject.rejectlonglen >= 0) {
    jlog("longer than %d msec\n", jconf->reject.rejectlonglen);
  } else {
    jlog("off\n");
  }
#ifdef POWER_REJECT
  jlog("\t   power rejection thres = %f", jconf->reject.powerthres);
#endif

  jlog("\n");

  jlog("----------------------- System Information end -----------------------\n");
  jlog("\n");

  if (jconf->input.type == INPUT_WAVEFORM) {

   if (jconf->decodeopt.realtime_flag) {

    /* warning for real-time decoding */
    for(mfcc=recog->mfcclist; mfcc; mfcc=mfcc->next) {
      if (mfcc->para->cmn || mfcc->para->cvn) {
	jlog("Notice for feature extraction (%02d),\n", mfcc->id);
	jlog("\t*************************************************************\n");
	if (mfcc->para->cmn && mfcc->para->cvn) {
	  jlog("\t* Cepstral mean and variance norm. for real-time decoding:  *\n");
	  if (mfcc->cmn.loaded) {
	    jlog("\t* initial mean loaded from file, updating per utterance.    *\n");
	    jlog("\t* static variance loaded from file, apply it constantly.    *\n");
	    jlog("\t* NOTICE: The first input may not be recognized, since      *\n");
	    jlog("\t*         cepstral mean is unstable on startup.             *\n");
	  } else {
	    jlog("\t* no static variance was given by file.                     *\n");
	    jlog("\t* estimating long-term variance from all speech from start. *\n");
	    jlog("\t* NOTICE: May not work on the first several minutes, since  *\n");
	    jlog("\t*         no cepstral variance is given on startup.         *\n");
	  }
	} else if (mfcc->para->cmn) {
	  jlog("\t* Cepstral mean normalization for real-time decoding:       *\n");
	  if (mfcc->cmn.loaded) {
	    jlog("\t* initial mean loaded from file, updating per utterance.    *\n");
	    jlog("\t* NOTICE: The first input may not good, since               *\n");
	    jlog("\t*         cepstral mean is unstable on startup.             *\n");
	  } else {
	    jlog("\t* NOTICE: The first input may not be recognized, since      *\n");
	    jlog("\t*         no initial mean is available on startup.          *\n");
	  }
	} else if (mfcc->para->cvn) {
	  jlog("\t* Cepstral variance normalization for real-time decoding:   *\n");
	  if (mfcc->cmn.loaded) {
	    jlog("\t* static variance loaded from file, apply it constantly.    *\n");
	  } else {
	    jlog("\t* no static variance is given by file.                      *\n");
	    jlog("\t* estimating long-term variance from all speech from start. *\n");
	    jlog("\t* NOTICE: The first minutes may not work well, since        *\n");
	    jlog("\t*         no cepstral variance is given on startup.         *\n");
	  }
	}
	jlog("\t*************************************************************\n");
      }
      if (mfcc->para->energy && mfcc->para->enormal) {
	jlog("Notice for energy computation (%02d),\n", mfcc->id);
	jlog("\t*************************************************************\n");
	jlog("\t* NOTICE: Energy normalization is activated on live input:  *\n");
	jlog("\t*         maximum energy of LAST INPUT will be used for it. *\n");
	jlog("\t*         So, the first input will not be recognized.       *\n");
	jlog("\t*************************************************************\n");
      }
    }

   } else {

    /* warning for batch decoding */
    for(mfcc=recog->mfcclist; mfcc; mfcc=mfcc->next) {
      if (mfcc->para->cmn || mfcc->para->cvn) {
	jlog("Notice for feature extraction (%02d),\n", mfcc->id);
	jlog("\t*************************************************************\n");
	if (mfcc->para->cmn && mfcc->para->cvn) {
	  jlog("\t* Cepstral mean and variance norm. for batch decoding:      *\n");
	  if (mfcc->cmn.loaded) {
	    jlog("\t* constant mean and variance was loaded from file.          *\n");
	    jlog("\t* they will be applied constantly for all input.            *\n");
	  } else {
	    jlog("\t* per-utterance mean and variance will be computed and      *\n");
	    jlog("\t* applied for each input.                                   *\n");
	  }
	} else if (mfcc->para->cmn) {
	  jlog("\t* Cepstral mean normalization for batch decoding:           *\n");
	  if (mfcc->cmn.loaded) {
	    jlog("\t* constant mean was loaded from file.                       *\n");
	    jlog("\t* they will be constantly applied for all input.            *\n");
	  } else {
	    jlog("\t* per-utterance mean will be computed and applied.          *\n");
	  }
	} else if (mfcc->para->cvn) {
	  jlog("\t* Cepstral variance normalization for batch decoding:       *\n");
	  if (mfcc->cmn.loaded) {
	    jlog("\t* constant variance was loaded from file.                   *\n");
	    jlog("\t* they will be constantly applied for all input.            *\n");
	  } else {
	    jlog("\t* per-utterance variance will be computed and applied.      *\n");
	  }
	}
	jlog("\t*************************************************************\n");
      }
    }

   }

  }

}

/* end of file */
