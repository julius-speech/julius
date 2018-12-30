/*
 * Copyright (c) 1991-2016 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2016 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */


#include "adintool.h"

extern AdinTool *global_a;

// show descriptions to stderr
boolean
show_help_and_exit(Jconf *jconf, char *arg[], int argnum)
{
  fprintf(stderr, "adintool --- AD-in tool to record/split/send/receive speech data\n");
  fprintf(stderr, "Usage: adintool [options] -in inputdev -out outputdev\n");
  fprintf(stderr, "inputdev: read speech data from:\n");
#ifdef USE_MIC
  fprintf(stderr, "    mic         microphone (default)\n");
#endif
#ifdef USE_NETAUDIO
  fprintf(stderr, "    netaudio    DatLink (NetAudio) server\n");
#endif
  fprintf(stderr, "    file        speech file (filename given from prompt)\n");
  fprintf(stderr, "    adinnet     from adinnet client (I'm server)\n");
  fprintf(stderr, "    stdin       standard tty input\n");
  fprintf(stderr, "  (other input can be specified by \"-input xxx\" as in Julius)\n");
  fprintf(stderr, "outputdev: output data to:\n");
  fprintf(stderr, "    file        speech file (\"foo.0000.wav\" - \"foo.N.wav\"\n");
  fprintf(stderr, "    adinnet     to adinnet server (I'm client)\n");
  fprintf(stderr, "    vecnet      to vecnet server as feature vector (I'm client)\n");
  fprintf(stderr, "    stdout      standard tty output\n");
  fprintf(stderr, "    none        output nothing\n");
  
  fprintf(stderr, "I/O options:\n");
#ifdef USE_NETAUDIO
  fprintf(stderr, "    -NA             (netaudio) NetAudio server host:unit\n");
#endif
  fprintf(stderr, "    -server host[,host,...] (adinnet-out) server hostnames\n");
  fprintf(stderr, "    -port num[,num,...]     (adinnet-out) port numbers (%d)\n", ADINNET_PORT);
  fprintf(stderr, "    -inport num     (adinnet-in) port number (%d)\n", ADINNET_PORT);
  fprintf(stderr, "    -filename foo   (file-out) filename to record\n");
  fprintf(stderr, "    -startid id     (file-out) recording start id\n");

  fprintf(stderr, "Feature extraction options (other than in jconf):\n");
  fprintf(stderr, "    -paramtype desc     parameter type in HTK format\n");
  fprintf(stderr, "    -veclen num         total vector length\n");
  
  fprintf(stderr, "Recording and Pause segmentation options:\n");

  fprintf(stderr, " (input segmentation: on for file/mic/stdin, off for adinnet)\n");
  fprintf(stderr, "  [-nosegment]          not segment input speech\n");
  fprintf(stderr, "  [-segment]            force segmentation of input speech\n");
  fprintf(stderr, "  [-cutsilence]         (same as \"-segment\")\n");
  fprintf(stderr, "  [-oneshot]            record only the first segment\n");
  fprintf(stderr, "  [-freq frequency]     sampling frequency in Hz    (%d)\n", jconf->am_root->analysis.para_default.smp_freq);
  fprintf(stderr, "  [-48]                 48000Hz recording with down sampling (16kHz only)\n");
  fprintf(stderr, "  [-lv unsignedshort]   silence cut level threshold (%d)\n", jconf->detect.level_thres);
  fprintf(stderr, "  [-zc zerocrossnum]    silence cut zerocross num   (%d)\n", jconf->detect.zero_cross_num);
  fprintf(stderr, "  [-headmargin msec]    head margin length          (%d)\n", jconf->detect.head_margin_msec);
  fprintf(stderr, "  [-tailmargin msec]    tail margin length          (%d)\n", jconf->detect.tail_margin_msec);
  fprintf(stderr, "  [-chunksize sample]   chunk size for processing   (%d)\n", jconf->detect.chunk_size);
#ifdef HAVE_LIBFVAD
  fprintf(stderr, "  [-fvad]               FVAD sw (-1=off, 0 - 3)     (%d)\n", jconf->detect.fvad_mode);
  fprintf(stderr, "  [-fvad_param i f]     FVAD parameter (dur/thres)  (%d %.2f)\n", jconf->detect.fvad_smoothnum, jconf->detect.fvad_thres);
#endif /* HAVE_LIBFVAD */
  
  fprintf(stderr, "  [-nostrip]            do not strip zero samples\n");
  fprintf(stderr, "  [-zmean]              remove DC by zero mean\n");
  fprintf(stderr, "  [-raw]                output in RAW format\n");
  fprintf(stderr, "  [-autopause]          automatically pause at each input end\n");
  fprintf(stderr, "  [-loosesync]          loose sync of resume among servers\n");
  fprintf(stderr, "  [-rewind msec]        rewind input if spoken while pause at resume\n");
  fprintf(stderr, "  [-C jconffile]        load jconf to set parameters (ignore other options\n");
  
  fprintf(stderr, "\nLibrary configuration: ");
  confout_version(stderr);
  confout_audio(stderr);
  confout_process(stderr);
  fprintf(stderr, "\n");
  exit(1);
}

// option "-in" handler
static boolean
opt_in(Jconf *jconf, char *arg[], int argnum)
{
  jconf->input.plugin_source = -1;
  jconf->input.type = INPUT_WAVEFORM;
  switch(arg[0][0]) {
  case 'm':
#ifdef USE_MIC
    jconf->input.speech_input = SP_MIC;
#else
    fprintf(stderr,"Error: mic input not available\n");
    return FALSE;
#endif
    break;
  case 'f':
    jconf->input.speech_input = SP_RAWFILE;
    jconf->detect.silence_cut = 1;
    break;
  case 's':
    jconf->input.speech_input = SP_STDIN;
    jconf->detect.silence_cut = 1;
    break;
  case 'a':
    jconf->input.speech_input = SP_ADINNET;
    break;
  case 'n':
#ifdef USE_NETAUDIO
    jconf->input.speech_input = SP_NETAUDIO;
#else
    fprintf(stderr,"Error: netaudio input not available\n");
    return FALSE;
#endif
    break;
  default:
    fprintf(stderr,"Error: no such input device: %s\n", arg[0]);
    return FALSE;
  }
  return TRUE;
}

// option "-out" handler
static boolean
opt_out(Jconf *jconf, char *arg[], int argnum)
{
  switch(arg[0][0]) {
  case 'f':
    global_a->conf.speech_output = SPOUT_FILE;
    break;
  case 's':
    global_a->conf.speech_output = SPOUT_STDOUT;
    break;
  case 'a':
    global_a->conf.speech_output = SPOUT_ADINNET;
    break;
  case 'v':
    global_a->conf.speech_output = SPOUT_VECTORNET;
    break;
  case 'n':
    global_a->conf.speech_output = SPOUT_NONE;
    break;
  default:
    fprintf(stderr,"Error: no such output device: %s\n", arg[0]);
    return FALSE;
  }
  return TRUE;
}

// option "-server" handler
static boolean
opt_server(Jconf *jconf, char *arg[], int argnum)
{
  char *p, *q;
  if (global_a->conf.speech_output == SPOUT_ADINNET || global_a->conf.speech_output == SPOUT_VECTORNET) {
    p = (char *)malloc(strlen(arg[0]) + 1);
    strcpy(p, arg[0]);
    for (q = strtok(p, ","); q; q = strtok(NULL, ",")) {
      if (global_a->conf.adinnet_servnum >= MAXCONNECTION) {
	fprintf(stderr, "Error: too many servers (> %d): %s\n", MAXCONNECTION, arg[0]);
	return FALSE;
      }
      global_a->conf.adinnet_serv[global_a->conf.adinnet_servnum] = (char *)malloc(strlen(q) + 1);
      strcpy(global_a->conf.adinnet_serv[global_a->conf.adinnet_servnum], q);
      global_a->conf.adinnet_servnum++;
    }
    free(p);
  } else {
    fprintf(stderr, "Warning: server [%s] should be used with adinnet / vecnet\n", arg[0]);
    return FALSE;
  }
  return TRUE;
}

// option "-NA" handler
static boolean
opt_NA(Jconf *jconf, char *arg[], int argnum)
{
#ifdef USE_NETAUDIO
  if (jconf->input.speech_input == SP_NETAUDIO) {
    jconf->input.netaudio_devname = arg[0];
  } else {
    fprintf(stderr, "Error: use \"-NA\" with \"-in netaudio\"\n");
    return FALSE;
  }
  return TRUE;
#else  /* ~USE_NETAUDIO */
  fprintf(stderr, "Error: NetAudio(DatLink) not supported\n");
  return FALSE;
#endif
}

// option "-inport" handler
static boolean
opt_inport(Jconf *jconf, char *arg[], int argnum)
{
  global_a->conf.adinnet_port_in = atoi(arg[0]);
  return TRUE;
}

// option "-port" handler
static boolean
opt_port(Jconf *jconf, char *arg[], int argnum)
{
  char *p, *q;

  p = (char *)malloc(strlen(arg[0]) + 1);
  strcpy(p, arg[0]);
  for (q = strtok(p, ","); q; q = strtok(NULL, ",")) {
    if (global_a->conf.adinnet_portnum >= MAXCONNECTION) {
      fprintf(stderr, "Error: too many server ports (> %d): %s\n", MAXCONNECTION, arg[0]);
      return FALSE;
    }
    global_a->conf.adinnet_port[global_a->conf.adinnet_portnum] = atoi(q);
    global_a->conf.adinnet_portnum++;
  }
  free(p);
  return TRUE;
}

// option "-filename" handler
static boolean
opt_filename(Jconf *jconf, char *arg[], int argnum)
{
  global_a->conf.filename = arg[0];
  return TRUE;
}

// option "-paramtype" handler
static boolean
opt_paramtype(Jconf *jconf, char *arg[], int argnum)
{
  global_a->conf.vecnet_paramtype = param_str2code(arg[0]);

  return TRUE;
}

// option "-veclen" handler
static boolean
opt_veclen(Jconf *jconf, char *arg[], int argnum)
{
  global_a->conf.vecnet_veclen = atoi(arg[0]);

  return TRUE;
}

// option "-startid" handler
static boolean
opt_startid(Jconf *jconf, char *arg[], int argnum)
{
  global_a->conf.startid = atoi(arg[0]);
  return TRUE;
}

// option "-freq" handler
static boolean
opt_freq(Jconf *jconf, char *arg[], int argnum)
{
  jconf->amnow->analysis.para.smp_freq = atoi(arg[0]);
  jconf->amnow->analysis.para.smp_period = freq2period(jconf->amnow->analysis.para.smp_freq);
  return TRUE;
}

// option "-nosegment" handler
static boolean
opt_nosegment(Jconf *jconf, char *arg[], int argnum)
{
  jconf->detect.silence_cut = 0;
  return TRUE;
}

// option "-segment" handler
static boolean
opt_segment(Jconf *jconf, char *arg[], int argnum)
{
  jconf->detect.silence_cut = 1;
  return TRUE;
}

// option "-oneshot" handler
static boolean
opt_oneshot(Jconf *jconf, char *arg[], int argnum)
{
  global_a->conf.continuous_segment = FALSE;
  return TRUE;
}

// option "-raw" handler
static boolean
opt_raw(Jconf *jconf, char *arg[], int argnum)
{
  global_a->conf.use_raw = TRUE;
  return TRUE;
}

// option "-autopause" handler
static boolean
opt_autopause(Jconf *jconf, char *arg[], int argnum)
{
  global_a->conf.pause_each = TRUE;
  return TRUE;
}

// option "-loosesync" handler
static boolean
opt_loosesync(Jconf *jconf, char *arg[], int argnum)
{
  global_a->conf.loose_sync = TRUE;
  return TRUE;
}

// option "-rewind" handler
static boolean
opt_rewind(Jconf *jconf, char *arg[], int argnum)
{
  global_a->conf.rewind_msec = atoi(arg[0]);
  return TRUE;
}

// option handler loader
void register_options_to_julius()
{
  /* register additional options */
  j_add_option("-in", 1, 1, "input from", opt_in);
  j_add_option("-out", 1, 1, "output to", opt_out);
  j_add_option("-server", 1, 1, "hostname (-out adinnet)", opt_server);
  j_add_option("-NA", 1, 1, "NetAudio server host:unit (-in netaudio)", opt_NA);
  j_add_option("-port", 1, 1, "port number (-out adinnet)", opt_port);
  j_add_option("-inport", 1, 1, "port number (-in adinnet)", opt_inport);
  j_add_option("-filename", 1, 1, "(base) filename to record (-out file)", opt_filename);
  j_add_option("-paramtype", 1, 1, "feature parameter type in HTK format", opt_paramtype);
  j_add_option("-veclen", 1, 1, "feature parameter vector length", opt_veclen);
  j_add_option("-startid", 1, 1, "recording start id (-out file)", opt_startid);
  j_add_option("-freq", 1, 1, "sampling frequency in Hz", opt_freq);
  j_add_option("-nosegment", 0, 0, "not segment input speech, record all", opt_nosegment);
  j_add_option("-segment", 0, 0, "force segment input speech", opt_segment);
  j_add_option("-oneshot", 0, 0, "exit after the first input", opt_oneshot);
  j_add_option("-raw", 0, 0, "save in raw (BE) format", opt_raw);
  j_add_option("-autopause", 0, 0, "automatically pause at each input end", opt_autopause);
  j_add_option("-loosesync", 0, 0, "loose sync of resume among servers", opt_loosesync);
  j_add_option("-rewind", 1, 1, "rewind to the msec", opt_rewind);
  j_add_option("-h", 0, 0, "display this help", show_help_and_exit);
  j_add_option("-help", 0, 0, "display this help", show_help_and_exit);
  j_add_option("--help", 0, 0, "display this help", show_help_and_exit);
  
}

/* end of options.c */
