/*
 * Copyright (c) 1991-2016 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2016 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#ifndef __J_ADINTOOL_H__
#define __J_ADINTOOL_H__

#ifdef USE_SDL
// EXPERIMENTAL: when enabledwith SDL, auto-adjust threshold
#undef AUTO_ADJUST_THRESHOLD
#endif

// headers
#ifdef USE_SDL
#ifdef NO_SDL_MAIN
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>
#endif
#include <julius/juliuslib.h>
#include <signal.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

// definition of speech output selection
enum{SPOUT_NONE, SPOUT_FILE, SPOUT_STDOUT, SPOUT_ADINNET, SPOUT_VECTORNET};

// maximum number of server connection
#define MAXCONNECTION 10

#ifdef USE_SDL
// default screen width
#define SCREEN_WIDTH 500
// default screen height
#define SCREEN_HEIGHT 600
// level threshold adjustment upper bound
#define THRESHOLD_ADJUST_MAX 32700
// level threshold adjustment lower bound
#define THRESHOLD_ADJUST_MIN 200
// level threshold adjustment moving step
#define THRESHOLD_ADJUST_STEP 200
// length of audio fragment for a tick in milliseconds
#define WAVE_TICK_TIME_MSEC 20
// display width for an audio tick
#define WAVE_TICK_WIDTH 2
// audio tick flag: set when the tick was triggered and processed
#define WAVE_TICK_FLAG_PROCESSED 0x01
// audio tick flag: set to indicate that an input segment was triggered down
#define WAVE_TICK_FLAG_TRIGGER 0x02

#ifdef AUTO_ADJUST_THRESHOLD
// mean / var computing window length in seconds
#define AUTOTHRES_WINDOW_SEC 0.2
// varofmean computing window length in seconds, should be longer than above
#define AUTOTHRES_STABLE_SEC 0.8
// varofmean stable(noise) -> unstable(voice) threshold
// the bigger, the more the threshold goes up to avoid trigger by loud noise
#define AUTOTHRES_ADAPT_THRES_1 0.008
// varofmean unstable(voice) -> stable(noise) threshold
// the smaller, the more the threshold goes down to capture quiet speech
#define AUTOTHRES_ADAPT_THRES_2 0.030
// silence duration length threshold to start moving to lower threshold
#define AUTOTHRES_DOWN_SEC 2      /* mean/var stable duration */
// ignore the first samples till this seconds
#define AUTOTHRES_START_IGNORE_SEC 0.5
// threshold smearing coef.  smaller, slower.
#define AUTOTHRES_ADAPT_SPEED_COEF 0.25
#endif


// SDL data holder
typedef struct {
#ifdef HAVE_PTHREAD
  pthread_mutex_t mutex;        /* mutex */
#endif
  SDL_Window *window;		/* SDL Window */
  SDL_Renderer *renderer;	/* SDL Renderer */
  int window_w;			/* current window screen width */
  int items;			/* number of ticks to be drawn */
  SP16 *tickbuf;		/* buffer to get samples of a tick */
  int ticklen;			/* length of tickbuf */
  int tickbp;			/* current pointer at tickbuf */
  float *maxlevel;		/* max levels of ticks */
  float *minlevel;		/* min levels of ticks */
  short *flag;			/* flags of ticks */
#ifdef AUTO_ADJUST_THRESHOLD
  float *mean;
  float *var;
  float *meanofmean;
  float *validmean;
  float *varofmean;
  float *triggerrate;
  float vvthres1;
  float vvthres2;
#endif
  int bp; /* current pointer at maxlevel/minlevel/flag (will circulate) */
  SDL_Rect *rects;  /* working area for drawing: tick coordinates */
  short *rectflags; /* working area for drawing: flags */
  short is_valid_flag; /* last input valid flag, used to find trigger up/down */
  int totaltick;
} SDLDATA;
#endif

// AdinTool structure
typedef struct {

  // configurations given via command line or Jconf configuration file
  struct {
    int speech_output;		/* speech output selection */
    int sfreq;	 /* sampling frequency, obtained from Julius config */
    boolean continuous_segment;	/* process only the first segment if FALSE */
    boolean pause_each;		/* always pause after each input segment and wait resume when TRUE */
    boolean loose_sync;		/* more loose way of resuming with multiple servers when TRUE */
    int rewind_msec;		/* rewind samples at re-trigger */
    
    char *filename;		/* output file name */
    int startid;		/* output file path numbering start with this value */
    boolean use_raw;		/* output file in raw format when TRUE, in wav otherwise */

    int adinnet_port_in;	/* input adinnet port number */
    char *adinnet_serv[MAXCONNECTION]; /* output adinnet server names */
    int adinnet_port[MAXCONNECTION]; /* output adinnet server port numbers */
    int adinnet_servnum;	     /* number of output adinnet server names */
    int adinnet_portnum;	     /* number of output adinnet server port numbers */

    short vecnet_paramtype;	/* output vector format */
    int vecnet_veclen;		/* output vector length */
  } conf;

  // status
  boolean on_processing; /* TRUE when processing of triggered samples are ready (connected) */
  boolean on_pause;      /* TRUE when pausing (not process input samples) */
  boolean writing_file;	 /* TRUE when writing to a file */
  boolean stop_at_next;	 /* TRUE when need to stop at next inpu by server request */
  boolean process_error;

  // counters
  int total_speechlen; /* total number of processed samples since start */
  int trigger_sample;  /* accumulated number of samples since input start at last trigger up */
  int unknown_command_counter;	/* Counter to detect broken connection */
  int resume_count[MAXCONNECTION]; /* Number of incoming resume commands for resume synchronization */

  // work area
  Recog *recog;	  /* Julius recognition instance */
  Jconf *jconf;	  /* Julius configuration instance */
  int speechlen;     /* number of processed samples in this segment */
  int fd;	     /* output raw file descriptor for SPOUT_FILE */
  FILE *fp;	     /* output wav file pointer for SPOUT_FILE */
  int sid;	     /* current file path numbering value */
  char *outpath;     /* string buffer to hold current output file path */
  int sd[MAXCONNECTION];	/* output adinnet socket descriptors */

#ifdef USE_SDL
  SDLDATA sdl;
#endif
  
} AdinTool;

/* adintool.c */
AdinTool *adintool_new();

/* options.c */
boolean show_help_and_exit(Jconf *jconf, char *arg[], int argnum);
void register_options_to_julius();

/* mainloop.c */
boolean vecnet_init(Recog *recog);
void mainloop();

#endif /* __J_ADINTOOL_H__ */
