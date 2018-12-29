/**
 * @file   main.c
 * 
 * <JA>
 * @brief  Julius/Julian �ᥤ��
 * </JA>
 * 
 * <EN>
 * @brief  Main function of Julius/Julian
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Wed May 18 15:02:55 2005
 *
 * $Revision: 1.8 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 1997-2000 Information-technology Promotion Agency, Japan
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include "app.h"
#include "config.h"

boolean separate_score_flag = FALSE;
boolean outfile_enabled = FALSE;
boolean noxmlescape_enabled = FALSE;

static char *logfile = NULL;
static boolean nolog = FALSE;

/************************************************************************/

#ifdef VISUALIZE

/**
 * Callbacks for the graphical interface.
 *
 */

static boolean show_gui = FALSE;

static void
show_visual(Recog *recog, void *dummy)
{
  RecogProcess *process = recog->process_list;

  if (show_gui)
    visual_show(process->backtrellis);
}

static void
init_visual2(Recog *recog, void *dummy)
{
  JCONF_SEARCH *conf = recog->process_list->config;

  if (show_gui)
    visual2_init(conf->pass2.hypo_overflow);
}

static void
pop_visual2(Recog *recog, void *dummy)
{
  StackDecode *pass2 = &(recog->process_list->pass2);

  if (show_gui)
    visual2_popped(pass2->current, pass2->popctr);
}

static void
next_word_visual2(Recog *recog, void *dummy)
{
  RecogProcess *process;
  StackDecode *pass2;
  NODE *prev, *next;
  int popctr;

  if (!show_gui)
    return;

  process = recog->process_list;
  pass2 = &(process->pass2);
  prev = pass2->current;
  popctr = pass2->popctr;

  process = process->next;
  pass2 = &(process->pass2);
  next = pass2->current;

  visual2_next_word(prev, next, popctr);
}

static boolean
opt_visualize(Jconf *jconf, char *arg[], int argnum)
{
  show_gui = TRUE;
  return TRUE;
}

#endif

/**
 * Callbacks for application option handling.
 * 
 */
static boolean
opt_help(Jconf *jconf, char *arg[], int argnum)
{
  fprintf(stderr, "Julius rev.%s - based on ", JULIUS_VERSION);
  j_output_argument_help(stderr);
  exit(1);			/* terminates here! */
  return TRUE;
}
static boolean
opt_separatescore(Jconf *jconf, char *arg[], int argnum)
{
  separate_score_flag = TRUE;
  return TRUE;
}
static boolean
opt_logfile(Jconf *jconf, char *arg[], int argnum)
{
  logfile = (char *)malloc(strlen(arg[0]) + 1);
  strcpy(logfile, arg[0]);
  return TRUE;
}
static boolean
opt_nolog(Jconf *jconf, char *arg[], int argnum)
{
  nolog = TRUE;
  return TRUE;
}
static boolean
opt_outfile(Jconf *jconf, char *arg[], int argnum)
{
  outfile_enabled = TRUE;
  return TRUE;
}
static boolean
opt_noxmlescape(Jconf *jconf, char *arg[], int argnum)
{
  noxmlescape_enabled = TRUE;
  return TRUE;
}
   
/**********************************************************************/
int
main(int argc, char *argv[])
{
  FILE *fp;
  Recog *recog;
  Jconf *jconf;

  /* inihibit system log output (default: stdout) */
  //jlog_set_output(NULL);
  /* output system log to a file */
  // FILE *fp = fopen(logfile, "w"); jlog_set_output(fp);

  /* if no option argument, output julius usage and exit */
  if (argc == 1) {
    fprintf(stderr, "Julius rev.%s - based on ", JULIUS_VERSION);
    j_put_version(stderr);
    fprintf(stderr, "Try '-setting' for built-in engine configuration.\n");
    fprintf(stderr, "Try '-help' for run time options.\n");
    return -1;
  }

  /* add application options */
  record_add_option();
  module_add_option();
  charconv_add_option();
  j_add_option("-separatescore", 0, 0, "output AM and LM scores separately", opt_separatescore);
  j_add_option("-noxmlescale", 0, 0, "disable XML escape", opt_noxmlescape);
  j_add_option("-logfile", 1, 1, "output log to file", opt_logfile);
  j_add_option("-nolog", 0, 0, "not output any log", opt_nolog);
  j_add_option("-outfile", 0, 0, "save result in separate .out file", opt_outfile);
  j_add_option("-help", 0, 0, "display this help", opt_help);
  j_add_option("--help", 0, 0, "display this help", opt_help);
#ifdef VISUALIZE
  j_add_option("-visualize", 0, 0, "show a visual interface for the parsed input", opt_visualize);
#endif /*VISUALIZE*/

  /* create a configuration variables container */
  jconf = j_jconf_new();
  // j_config_load_file(jconf, jconffile);
  if (j_config_load_args(jconf, argc, argv) == -1) {
    fprintf(stderr, "Try `-help' for more information.\n");
    return -1;
  }

  /* output system log to a file */
  if (nolog) {
    jlog_set_output(NULL);
  } else if (logfile) {
    fp = fopen(logfile, "w");
    jlog_set_output(fp);
  }

  /* here you can set/modify any parameter in the jconf before setup */
  // jconf->input.input_speech = SP_MIC;

  /* Fixate jconf parameters: it checks whether the jconf parameters
     are suitable for recognition or not, and set some internal
     parameters according to the values for recognition.  Modifying
     a value in jconf after this function may be errorous.
  */
  if (j_jconf_finalize(jconf) == FALSE) {
    if (logfile) fclose(fp);
    return -1;
  }

  /* create a recognition instance */
  recog = j_recog_new();
  /* assign configuration to the instance */
  recog->jconf = jconf;
  /* load all files according to the configurations */
  if (j_load_all(recog, jconf) == FALSE) {
    fprintf(stderr, "ERROR: Error in loading model\n");
    if (logfile) fclose(fp);
    return -1;
  }
  
#ifdef USER_LM_TEST
  {
    PROCESS_LM *lm;
    for(lm=recog->lmlist;lm;lm=lm->next) {
      if (lm->lmtype == LM_PROB) {
	j_regist_user_lm_func(lm, my_uni, my_bi, my_lm);
      }
    }
#endif

  /* checkout for recognition: build lexicon tree, allocate cache */
  if (j_final_fusion(recog) == FALSE) {
    fprintf(stderr, "ERROR: Error while setup work area for recognition\n");
    j_recog_free(recog);
    if (logfile) fclose(fp);
    return -1;
  }
  
  /* Set up some application functions */
  /* set character conversion mode */
  if (charconv_setup() == FALSE) {
    if (logfile) fclose(fp);
    return -1;
  }
  if (is_module_mode()) {
    /* set up for module mode */
    /* register result output callback functions to network module */
    module_setup(recog, NULL);
  } else {
    /* register result output callback functions to stdout */
    setup_output_tty(recog, NULL);
  }
  /* if -outfile option specified, callbacks for file output will be
     regitered */
  if (outfile_enabled) {
    if (jconf->input.speech_input == SP_MFCFILE || jconf->input.speech_input == SP_RAWFILE || jconf->input.speech_input == SP_OUTPROBFILE) {
      setup_output_file(recog, NULL);
    } else {
      fprintf(stderr, "Warning: -outfile works only for file input, disabled now\n");
      outfile_enabled = FALSE;
    }
  }

  /* setup recording if option was specified */
  record_setup(recog, NULL);

  /* on module connect with client */
  if (is_module_mode()) module_server();

  /* initialize and standby the specified audio input source */
  /* for microphone or other threaded input, ad-in thread starts here */
  if (j_adin_init(recog) == FALSE) return -1;

  /* output system information to log */
  j_recog_info(recog);

#ifdef VISUALIZE
  /* Visualize: initialize GTK */
  visual_init(recog);
  callback_add(recog, CALLBACK_RESULT, show_visual, NULL);
  callback_add(recog, CALLBACK_EVENT_PASS2_BEGIN, init_visual2, jconf);
  callback_add(recog, CALLBACK_DEBUG_PASS2_POP, pop_visual2, NULL);
  callback_add(recog, CALLBACK_DEBUG_PASS2_PUSH, next_word_visual2, NULL);
#endif
  
  /* if no grammar specified on startup, start with pause status */
  {
    RecogProcess *r;
    for(r=recog->process_list;r;r=r->next) {
      if (r->lmtype == LM_DFA) {
	if (r->lm->winfo == NULL) { /* stop when no grammar found */
	  j_request_pause(recog);
	}
      }
    }
  }

  /* enter recongnition loop */
  main_recognition_stream_loop(recog);

  /* end proc */
  if (is_module_mode()) module_disconnect();

  /* release all */
  j_recog_free(recog);

  if (logfile) fclose(fp);
  return(0);
}
