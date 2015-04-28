/**
 * @file   adintool.c
 * 
 * <JA>
 * @brief  音声入力を記録/分割/送受信する汎用音声入力ツール
 *
 * このツールは Julius の音声ライブラリを用いて様々な音声の入力や出力を
 * 行います．マイクデバイス・ファイル・adinnetネットワーククライアント・
 * 標準入力から音声を読み込んで(必要であれば)音声区間検出を行い，
 * その結果のデータを順次，ファイル・adinnetネットワークサーバー・標準出力
 * などへ出力します．
 *
 * 応用例として，Julius/Julian へのネットワーク入力(入力=マイク,出力=adinnet)
 * ，音声ファイルの音声区間抽出(入力=ファイル,出力=ファイル)などに
 * 利用できます．
 * </JA>
 * 
 * <EN>
 * @brief  AD-in tool to record / split / send / receive speech data
 *
 * This tool is to handle speech input and output from/to various devices
 * using libsetn library in Julius.  It reads input from either of
 * microphone, file, adinnet network client, or standard input, and
 * perform speech detection based on level and zero cross (you can disable
 * this), and output them to either of file, adinnet network server, or
 * standard output.
 *
 * For example, you can send microphone input to Julius running on other host
 * by specifying input to microphone and output to adinnet (Julius should
 * be run with "-input adinnet"), or you can long recorded speech data by
 * long silence by specifying both input and output to file and enabling
 * segment detection.
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Mar 23 20:43:32 2005
 *
 * $Revision: 1.21 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2001-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius/juliuslib.h>
#include <signal.h>

#define MAXCONNECTION 10	///< Maximum number of server connection

/* input */
static int file_counter = 0;	///< num of input files (for SP_RAWFILE)
static int sfreq;		///< Temporal storage of sample rate

/* output */
enum{SPOUT_FILE, SPOUT_STDOUT, SPOUT_ADINNET, SPOUT_VECTORNET}; ///< value for speech_output
static int speech_output = SPOUT_FILE; ///< output device
static int total_speechlen;	///< total samples of recorded segments
static int speechlen;		///< samples of one recorded segments
static char *filename = NULL;	///< Output file name
static int fd = -1;		///< File descriptor for output
static FILE *fp = NULL;		///< File pointer for WAV output
static int  size;		///< Output file size
static boolean use_raw = FALSE;	///< Output in RAW format if TRUE
static boolean continuous_segment = TRUE; ///< enable/disable successive output
static short vecnet_paramtype = F_ERR_INVALID; ///< output parameter type
static int vecnet_veclen = 0;	///< output vector dimension
static int startid = 0;		///< output file numbering variable
static int sid = 0;		///< current file ID (for SPOUT_FILE)
static char *outpath = NULL;	///< work space for output file name formatting
static int adinnet_port_in = ADINNET_PORT; ///< Input server port
static int adinnet_port[MAXCONNECTION]; ///< Output server ports
static char *adinnet_serv[MAXCONNECTION]; ///< Server name to send recorded data
static int sd[MAXCONNECTION];	///< Output socket descriptors
static int adinnet_servnum = 0; ///< Number of server to connect
static int adinnet_portnum = 0; ///< Number of server to connect
static boolean writing_file = FALSE; ///< TRUE if writing to a file
static boolean stop_at_next = FALSE; ///< TRUE if need to stop at next input by server command.  Will be set when PAUSE or TERMINATE command received while input.

/* switch */
static boolean pause_each = FALSE; ///< If set to TRUE, adintool will pause automatically at each input end and wait for resume command
static boolean loose_sync = FALSE; ///< If set to TRUE, adintool will do loose synchronization for resume among servers
static int rewind_msec = 0;
static int trigger_sample;

/** 
 * <JA>ヘルプを表示して終了する</JA>
 * <EN>Print help and exit</EN>
 */
static boolean
opt_help(Jconf *jconf, char *arg[], int argnum)
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
  
  fprintf(stderr, "I/O options:\n");
#ifdef USE_NETAUDIO
  fprintf(stderr, "    -NA             (netaudio) NetAudio server host:unit\n");
#endif
  fprintf(stderr, "    -server host[,host,...] (adinnet-out) server hostnames\n");
  fprintf(stderr, "    -port num[,num,...]     (adinnet-out) port numbers (%d)\n", ADINNET_PORT);
  fprintf(stderr, "    -inport num     (adinnet-in) port number (%d)\n", ADINNET_PORT);
  fprintf(stderr, "    -filename foo   (file-out) filename to record\n");
  fprintf(stderr, "    -startid id     (file-out) recording start id (%04d)\n", startid);

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
  exit(1);			/* exit here */
  return TRUE;
}

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
static boolean
opt_out(Jconf *jconf, char *arg[], int argnum)
{
  switch(arg[0][0]) {
  case 'f':
    speech_output = SPOUT_FILE;
    break;
  case 's':
    speech_output = SPOUT_STDOUT;
    break;
  case 'a':
    speech_output = SPOUT_ADINNET;
    break;
  case 'v':
    speech_output = SPOUT_VECTORNET;
    break;
  default:
    fprintf(stderr,"Error: no such output device: %s\n", arg[0]);
    return FALSE;
  }
  return TRUE;
}
static boolean
opt_server(Jconf *jconf, char *arg[], int argnum)
{
  char *p, *q;
  if (speech_output == SPOUT_ADINNET || speech_output == SPOUT_VECTORNET) {
    p = (char *)malloc(strlen(arg[0]) + 1);
    strcpy(p, arg[0]);
    for (q = strtok(p, ","); q; q = strtok(NULL, ",")) {
      if (adinnet_servnum >= MAXCONNECTION) {
	fprintf(stderr, "Error: too many servers (> %d): %s\n", MAXCONNECTION, arg[0]);
	return FALSE;
      }
      adinnet_serv[adinnet_servnum] = (char *)malloc(strlen(q) + 1);
      strcpy(adinnet_serv[adinnet_servnum], q);
      adinnet_servnum++;
    }
    free(p);
  } else {
    fprintf(stderr, "Warning: server [%s] should be used with adinnet / vecnet\n", arg[0]);
    return FALSE;
  }
  return TRUE;
}
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
static boolean
opt_inport(Jconf *jconf, char *arg[], int argnum)
{
  adinnet_port_in = atoi(arg[0]);
  return TRUE;
}
static boolean
opt_port(Jconf *jconf, char *arg[], int argnum)
{
  char *p, *q;

  p = (char *)malloc(strlen(arg[0]) + 1);
  strcpy(p, arg[0]);
  for (q = strtok(p, ","); q; q = strtok(NULL, ",")) {
    if (adinnet_portnum >= MAXCONNECTION) {
      fprintf(stderr, "Error: too many server ports (> %d): %s\n", MAXCONNECTION, arg[0]);
      return FALSE;
    }
    adinnet_port[adinnet_portnum] = atoi(q);
    adinnet_portnum++;
  }
  free(p);
  return TRUE;
}

static boolean
opt_filename(Jconf *jconf, char *arg[], int argnum)
{
  filename = arg[0];
  return TRUE;
}
static boolean
opt_paramtype(Jconf *jconf, char *arg[], int argnum)
{
  short code;

  vecnet_paramtype = param_str2code(arg[0]);

  return TRUE;
}
static boolean
opt_veclen(Jconf *jconf, char *arg[], int argnum)
{
  short code;
  
  vecnet_veclen = atoi(arg[0]);

  return TRUE;
}
static boolean
opt_startid(Jconf *jconf, char *arg[], int argnum)
{
  startid = atoi(arg[0]);
  return TRUE;
}
static boolean
opt_freq(Jconf *jconf, char *arg[], int argnum)
{
  jconf->amnow->analysis.para.smp_freq = atoi(arg[0]);
  jconf->amnow->analysis.para.smp_period = freq2period(jconf->amnow->analysis.para.smp_freq);
  return TRUE;
}
static boolean
opt_nosegment(Jconf *jconf, char *arg[], int argnum)
{
  jconf->detect.silence_cut = 0;
  return TRUE;
}
static boolean
opt_segment(Jconf *jconf, char *arg[], int argnum)
{
  jconf->detect.silence_cut = 1;
  return TRUE;
}
static boolean
opt_oneshot(Jconf *jconf, char *arg[], int argnum)
{
  continuous_segment = FALSE;
  return TRUE;
}
static boolean
opt_raw(Jconf *jconf, char *arg[], int argnum)
{
  use_raw = TRUE;
  return TRUE;
}
static boolean
opt_autopause(Jconf *jconf, char *arg[], int argnum)
{
  pause_each = TRUE;
  return TRUE;
}
static boolean
opt_loosesync(Jconf *jconf, char *arg[], int argnum)
{
  loose_sync = TRUE;
  return TRUE;
}
static boolean
opt_rewind(Jconf *jconf, char *arg[], int argnum)
{
  rewind_msec = atoi(arg[0]);
  return TRUE;
}

/** 
 * <JA>
 * 確認のため入出力設定をテキスト出力する
 * 
 * </JA>
 * <EN>
 * Output input/output configuration in text for a confirmation.
 * 
 * </EN>
 */
void
put_status(Recog *recog)
{
  int i;
  Jconf *jconf = recog->jconf;

  fprintf(stderr, "----------------------------------------\n");
  fprintf(stderr, "INPUT\n");
  fprintf(stderr, "\t   InputType: ");
  switch(jconf->input.type) {
  case INPUT_WAVEFORM:
    fprintf(stderr, "waveform\n");
    break;
  case INPUT_VECTOR:
    fprintf(stderr, "feature vector sequence\n");
    break;
  }
  fprintf(stderr, "\t InputSource: ");
  if (jconf->input.plugin_source != -1) {
    fprintf(stderr, "plugin\n");
  } else if (jconf->input.speech_input == SP_RAWFILE) {
    fprintf(stderr, "waveform file\n");
  } else if (jconf->input.speech_input == SP_MFCFILE) {
    fprintf(stderr, "feature vector file (HTK format)\n");
  } else if (jconf->input.speech_input == SP_OUTPROBFILE) {
    fprintf(stderr, "output probability file (HTK format)\n");
  } else if (jconf->input.speech_input == SP_STDIN) {
    fprintf(stderr, "standard input\n");
  } else if (jconf->input.speech_input == SP_ADINNET) {
    fprintf(stderr, "adinnet client\n");
#ifdef USE_NETAUDIO
  } else if (jconf->input.speech_input == SP_NETAUDIO) {
    char *p;
    fprintf(stderr, "NetAudio server on ");
    if (jconf->input.netaudio_devname != NULL) {
      fprintf(stderr, "%s\n", jconf->input.netaudio_devname);
    } else if ((p = getenv("AUDIO_DEVICE")) != NULL) {
      fprintf(stderr, "%s\n", p);
    } else {
      fprintf(stderr, "local port\n");
    }
#endif
  } else if (jconf->input.speech_input == SP_MIC) {
    fprintf(stderr, "microphone\n");
    fprintf(stderr, "\t   DeviceAPI: ");
    switch(jconf->input.device) {
    case SP_INPUT_DEFAULT: fprintf(stderr, "default\n"); break;
    case SP_INPUT_ALSA: fprintf(stderr, "alsa\n"); break;
    case SP_INPUT_OSS: fprintf(stderr, "oss\n"); break;
    case SP_INPUT_ESD: fprintf(stderr, "esd\n"); break;
    case SP_INPUT_PULSEAUDIO: fprintf(stderr, "pulseaudio\n"); break;
    }
  }

  fprintf(stderr, "\tSegmentation: ");
  if (jconf->detect.silence_cut) {
    if (continuous_segment) {
      fprintf(stderr,"on, continuous\n");
    } else {
      fprintf(stderr,"on, only one snapshot\n");
    }
    if (recog->adin->down_sample) {
      fprintf(stderr, "\t  SampleRate: 48000Hz -> %d Hz\n", sfreq);
    } else {
      fprintf(stderr, "\t  SampleRate: %d Hz\n", sfreq);
    }
    fprintf(stderr, "\t       Level: %d / 32767\n", jconf->detect.level_thres);
    fprintf(stderr, "\t   ZeroCross: %d per sec.\n", jconf->detect.zero_cross_num);
    fprintf(stderr, "\t  HeadMargin: %d msec.\n", jconf->detect.head_margin_msec);
    fprintf(stderr, "\t  TailMargin: %d msec.\n", jconf->detect.tail_margin_msec);
  } else {
    fprintf(stderr,"OFF\n");
  }
  if (jconf->preprocess.strip_zero_sample) {
    fprintf(stderr, "\t  ZeroFrames: drop\n");
  } else {
    fprintf(stderr, "\t  ZeroFrames: keep\n");
  }
  if (jconf->preprocess.use_zmean) {
    fprintf(stderr, "\t   DCRemoval: on\n");
  } else {
    fprintf(stderr, "\t   DCRemoval: off\n");
  }
  if (pause_each) {
    fprintf(stderr, "\t   AutoPause: on\n");
  } else {
    fprintf(stderr, "\t   Auto`ause: off\n");
  }
  if (loose_sync) {
    fprintf(stderr, "\t   LooseSync: on\n");
  } else {
    fprintf(stderr, "\t   LooseSync: off\n");
  }
  if (rewind_msec > 0) {
    fprintf(stderr, "\t      Rewind: %d msec\n", rewind_msec);
  } else {
    fprintf(stderr, "\t      Rewind: no\n");
  }
  fprintf(stderr, "OUTPUT\n");
  switch(speech_output) {
  case SPOUT_FILE:
    fprintf(stderr, "\t  OutputType: waveform\n");
    fprintf(stderr, "\t    OutputTo: file\n");
    fprintf(stderr, "\t    FileName: ");
    if (jconf->detect.silence_cut) {
      if (continuous_segment) {
	if (use_raw) {
	  fprintf(stderr,"%s.%04d.raw, %s.%04d.raw, ...\n", filename,startid, filename, startid+1);
	} else {
	  fprintf(stderr,"%s.%04d.wav, %s.%04d.wav, ...\n", filename,startid, filename, startid+1);
	}
      } else {
	fprintf(stderr,"%s\n", outpath);
      }
    } else {
      fprintf(stderr,"%s (warning: inifinite recording: be care of disk space!)\n", outpath);
    }
    break;
  case SPOUT_STDOUT:
    fprintf(stderr, "\t  OutputType: waveform\n");
    fprintf(stderr, "\t    OutputTo: standard output\n");
    use_raw = TRUE;
    break;
  case SPOUT_ADINNET:
    fprintf(stderr, "\t  OutputType: waveform\n");
    fprintf(stderr, "\t    OutputTo: adinnet server\n");
    fprintf(stderr, "\t      SendTo:");
    for(i=0;i<adinnet_servnum;i++) {
      fprintf(stderr, " (%s:%d)", adinnet_serv[i], adinnet_port[i]);
    }
    fprintf(stderr, "\n");
    break;
  case SPOUT_VECTORNET:
    fprintf(stderr, "\t  OutputType: feature vector sequence\n");
    fprintf(stderr, "\t    OutputTo: vecnet server\n");
    fprintf(stderr, "\t      SendTo:");
    for(i=0;i<adinnet_servnum;i++) {
      fprintf(stderr, " (%s:%d)", adinnet_serv[i], adinnet_port[i]);
    }
    fprintf(stderr, "\n");
    {
      char buf[80];
      fprintf(stderr, "\t   ParamType: %s\n", param_code2str(buf, vecnet_paramtype, FALSE));
      fprintf(stderr, "\t   VectorLen: %d\n", vecnet_veclen);
    }
    break;
  }

  fprintf(stderr, "----------------------------------------\n");

  if (speech_output == SPOUT_VECTORNET) {
    MFCCCalc *mfcc;

    fprintf(stderr, "Detailed parameter setting for feature extraction\n");
    for(mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
      fprintf(stderr, "[MFCC%02d]\n", mfcc->id);
      print_mfcc_info(stderr, mfcc, recog->jconf);
    }
    fprintf(stderr, "----------------------------------------\n");
  }

}    

static char *
new_output_filename(char *filename, char *suffix)
{
  int len, slen;
  char *buf;

  len = strlen(filename);
  slen = strlen(suffix);
  if (len < slen || strcmp(&(filename[len-slen]), suffix) != 0) {
    buf = strcpy((char *)mymalloc(len+slen+1), filename);
    strcat(buf, suffix);
  } else {
    buf = strcpy((char *)mymalloc(len+1), filename);
  }
  return buf;
}


/**
 * <JA>
 * 読み込んだサンプル列を  fd もしくは fp に記録
 * するコールバック関数
 * 
 * @param now [in] 録音されたサンプル列
 * @param len [in] 長さ（サンプル数）
 * 
 * @return エラー時 -1，処理成功時 0，処理成功＋区間終端検出時 1 を返す．
 * </JA>
 * <EN>
 * Callback handler to record the sample fragments to file pointed by
 * the file descriptor "fd".
 * 
 * @param now [in] recorded fragments of speech sample
 * @param len [in] length of above in samples
 * 
 * @return -1 on device error (require caller to exit and terminate input),
 * 0 on success (allow caller to continue),
 * 1 on succeeded but segmentation detected (require caller to exit but
 * input will continue in the next call.
 * </EN>
 */
static int
adin_callback_file(SP16 *now, int len, Recog *recog)
{
  int count;
  int start;
  int w;

  start = 0;

  if (recog->jconf->input.speech_input == SP_MIC && speechlen == 0) {
    /* this is first up-trigger */
    if (rewind_msec > 0 && !recog->adin->is_valid_data) {
      /* not spoken currently but has data to process at first trigger */
      /* it means that there are old spoken segments */
      /* disgard them */
      printf("disgard already recorded %d samples\n", len);
      return 0;
    }
    /* erase "<<<please speak>>>" text on tty */
    fprintf(stderr, "\r                    \r");
    if (rewind_msec > 0) {
      /* when -rewind value set larger than 0, the speech data spoken
	 while pause will be considered back to the specified msec.
	 */
      printf("buffered samples=%d\n", len);
      w = rewind_msec * sfreq / 1000;
      if (len > w) {
	start = len - w;
	len = w;
      } else {
	start = 0;
      }
      printf("will process from %d\n", start);
    }
  }

  /* open files for recording at first trigger */
  if (speech_output == SPOUT_FILE && speechlen == 0) {
    if (continuous_segment) {
      if (use_raw) {
	sprintf(outpath, "%s.%04d.raw", filename, sid);
      } else {
	sprintf(outpath, "%s.%04d.wav", filename, sid);
      }
    }
    fprintf(stderr,"[%s]", outpath);
    if (access(outpath, F_OK) == 0) {
      if (access(outpath, W_OK) == 0) {
	fprintf(stderr, "(override)");
      } else {
	perror("adintool");
	return(-1);
      }
    }
    if (use_raw) {
      if ((fd = open(outpath, O_CREAT | O_RDWR
#ifdef O_BINARY
		     | O_BINARY
#endif
		     , 0644)) == -1) {
	perror("adintool");
	return -1;
      }
    } else {
      if ((fp = wrwav_open(outpath, sfreq)) == NULL) {
	perror("adintool");
	return -1;
      }
    }
    writing_file = TRUE;
  }

  /* write recorded sample to file */
  if (use_raw) {
    count = wrsamp(fd, &(now[start]), len);
    if (count < 0) {
      perror("adinrec: cannot write");
      return -1;
    }
    if (count < len * sizeof(SP16)) {
      fprintf(stderr, "adinrec: cannot write more %d bytes\ncurrent length = %lu\n", count, speechlen * sizeof(SP16));
      return -1;
    }
  } else {
    if (wrwav_data(fp, &(now[start]), len) == FALSE) {
      fprintf(stderr, "adinrec: cannot write\n");
      return -1;
    }
  }
  
  /* accumulate sample num of this segment */
  speechlen += len;

  /* if input length reaches limit, rehash the ad-in buffer */
  if (recog->jconf->input.speech_input == SP_MIC) {
    if (speechlen > MAXSPEECHLEN - 16000) {
      recog->adin->rehash = TRUE;
    }
  }
  
  /* progress bar in dots */
  fprintf(stderr, ".");

  return(0);
}

/**
 * <JA>
 * 読み込んだサンプル列をソケットデスクリプタ "fd" 上のadinnetサーバに送信
 * するコールバック関数
 * 
 * @param now [in] 録音されたサンプル列
 * @param len [in] 長さ（サンプル数）
 * 
 * @return エラー時 -1，処理成功時 0，処理成功＋区間終端検出時 1 を返す．
 * </JA>
 * <EN>
 * Callback handler to record the sample fragments to adinnet server
 * pointed by the socket descriptor "fd".
 * 
 * @param now [in] recorded fragments of speech sample
 * @param len [in] length of above in samples
 * 
 * @return -1 on device error (require caller to exit and terminate input),
 * 0 on success (allow caller to continue),
 * 1 on succeeded but segmentation detected (require caller to exit but
 * input will continue in the next call.
 * </EN>
 */
static int
adin_callback_adinnet(SP16 *now, int len, Recog *recog)
{
  int count;
  int start, w;
  int i;

  start = 0;

  if (recog->jconf->input.speech_input == SP_MIC && speechlen == 0) {
    /* this is first up-trigger */
    if (rewind_msec > 0 && !recog->adin->is_valid_data) {
      /* not spoken currently but has data to process at first trigger */
      /* it means that there are old spoken segments */
      /* disgard them */
      printf("disgard already recorded %d samples\n", len);
      return 0;
    }
    /* erase "<<<please speak>>>" text on tty */
    fprintf(stderr, "\r                    \r");
    if (rewind_msec > 0) {
      /* when -rewind value set larger than 0, the speech data spoken
	 while pause will be considered back to the specified msec.
	 */
      printf("buffered samples=%d\n", len);
      w = rewind_msec * sfreq / 1000;
      if (len > w) {
	start = len - w;
	len = w;
      } else {
	start = 0;
      }
      printf("will process from %d\n", start);
    }
  }

#ifdef WORDS_BIGENDIAN
  swap_sample_bytes(&(now[start]), len);
#endif
  for (i=0;i<adinnet_servnum;i++) {
    count = wt(sd[i], (char *)&(now[start]), len * sizeof(SP16));
    if (count < 0) {
      perror("adintool: cannot write");
      fprintf(stderr, "failed to send data to %s:%d\n", adinnet_serv[i], adinnet_port[i]);
    }
  }
#ifdef WORDS_BIGENDIAN
  swap_sample_bytes(&(now[start]), len);
#endif
  /* accumulate sample num of this segment */
  speechlen += len;
#ifdef HAVE_PTHREAD
  if (recog->adin->enable_thread) {
    /* if input length reaches limit, rehash the ad-in buffer */
    if (recog->adin->speechlen > MAXSPEECHLEN - 16000) {
      recog->adin->rehash = TRUE;
      fprintf(stderr, "+");
    }
  }
#endif
  
  /* display progress in dots */
  fprintf(stderr, ".");
  return(0);
}

boolean
vecnet_init(Recog *recog)
{
  Jconf *jconf = recog->jconf;
  JCONF_AM *amconf = jconf->am_root;
  MFCCCalc *mfcc;
  PROCESS_AM *am;
  
  am = j_process_am_new(recog, amconf);
  calc_para_from_header(&(jconf->am_root->analysis.para), vecnet_paramtype, vecnet_veclen);

  /* from j_final_fusion() */
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    create_mfcc_calc_instances(recog);
  }
  for(mfcc=recog->mfcclist;mfcc;mfcc=mfcc->next) {
    mfcc->param = new_param();
  }
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    for(mfcc=recog->mfcclist;mfcc;mfcc=mfcc->next) {
      if (mfcc->frontend.sscalc) {
        mfcc->frontend.mfccwrk_ss = WMP_work_new(mfcc->para);
        if (mfcc->frontend.mfccwrk_ss == NULL) {
          return FALSE;
        }
        if (mfcc->frontend.sscalc_len * recog->jconf->input.sfreq / 1000 < mfcc->para->framesize) {
          return FALSE;
        }
      }
    }
  }
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    if (RealTimeInit(recog) == FALSE) {
      fprintf(stderr, "Error: failed to initialize feature extraction module\n");
      return FALSE;
    }
  }
}

int vecnet_send_data(int sd, void *buf, int bytes)
{
  /* send data size header (4 byte) */
  if (send(sd, &bytes, sizeof(int), 0) != sizeof(int)) {
    fprintf(stderr, "Error: failed to send %lu bytes\n", sizeof(int));
    return -1;
  }

  /* send data body */
  if (send(sd, buf, bytes, 0) != bytes) {
    fprintf(stderr, "Error: failed to send %lu bytes\n", sizeof(int));
    return -1;
  }

  return 0;
}

typedef struct {
  int veclen;                 ///< (4 byte)Vector length of an input
  int fshift;                 ///< (4 byte) Frame shift in msec of the vector
  char outprob_p;             ///< (1 byte) != 0 if input is outprob vector
} ConfigurationHeader;

void
vecnet_send_header(Recog *recog)
{
  ConfigurationHeader conf;
  int i;

  conf.veclen = recog->jconf->am_root->analysis.para.veclen;
  conf.fshift = 1000.0 * recog->jconf->am_root->analysis.para.frameshift / recog->jconf->am_root->analysis.para.smp_freq;
  conf.outprob_p = 0;		/* feature output */
  for (i=0;i<adinnet_servnum;i++) {
    vecnet_send_data(sd[i], &conf, sizeof(ConfigurationHeader));
  }
}

boolean
vecnet_prepare(Recog *recog)
{
  RealBeam *r = &(recog->real);
  MFCCCalc *mfcc;

  r->windownum = 0;
  for(mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
    mfcc->param->veclen = vecnet_veclen;
    if (mfcc->para->cmn || mfcc->para->cvn) CMN_realtime_prepare(mfcc->cmn.wrk);
    param_alloc(mfcc->param, 1, mfcc->param->veclen);
    mfcc->f = 0;
  }
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    reset_mfcc(recog);
  }
  recog->triggered = FALSE;

  return TRUE;
  
}

void
vecnet_sub(SP16 *Speech, int nowlen, Recog *recog)
{
  int i, j, k, now, ret;
  MFCCCalc *mfcc;
  RealBeam *r = &(recog->real);

  now = 0;
  
  while (now < nowlen) {
    for(i = min(r->windowlen - r->windownum, nowlen - now); i > 0 ; i--)
      r->window[r->windownum++] = (float) Speech[now++];
    if (r->windownum < r->windowlen) break;
    for (mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
      mfcc->valid = FALSE;
      if (RealTimeMFCC(mfcc, r->window, r->windowlen)) {
        mfcc->valid = TRUE;
	param_alloc(mfcc->param, mfcc->f + 1, mfcc->param->veclen);
	memcpy(mfcc->param->parvec[mfcc->f], mfcc->tmpmfcc, sizeof(VECT) * mfcc->param->veclen);
#if 0
	{
	  int i;
	  for (i = 0; i < vecnet_veclen; i++) {
	    printf(" %f", mfcc->tmpmfcc[i]);
	  }
	  printf("\n");
	}
#endif
	/* send 1 frame */
	for (j=0;j<adinnet_servnum;j++) {
	  vecnet_send_data(sd[j], mfcc->tmpmfcc, sizeof(VECT) * vecnet_veclen);
	}
	mfcc->f++;
      }
    }
    /* shift window */
    memmove(r->window, &(r->window[recog->jconf->input.frameshift]), sizeof(SP16) * (r->windowlen - recog->jconf->input.frameshift));
    r->windownum -= recog->jconf->input.frameshift;
  }
}

void
vecnet_param_update(Recog *recog)
{
  MFCCCalc *mfcc;

  for (mfcc = recog->mfcclist; mfcc; mfcc = mfcc->next) {
    mfcc->param->header.samplenum = mfcc->f;
    mfcc->param->samplenum = mfcc->f;
  }
  if (recog->jconf->input.type == INPUT_WAVEFORM) {
    for(mfcc=recog->mfcclist;mfcc;mfcc=mfcc->next) {
      if (mfcc->f > 0 && mfcc->para && mfcc->para->cmn) {
	if (mfcc->cmn.update) {
	  CMN_realtime_update(mfcc->cmn.wrk, mfcc->param);
	}
	if (mfcc->cmn.save_filename) {
	  CMN_save_to_file(mfcc->cmn.wrk, mfcc->cmn.save_filename);
	}
      }
    }
  }
}


static int
adin_callback_vecnet(SP16 *now, int len, Recog *recog)
{
  int count;
  int start, w;
  int i;

  start = 0;

  if (recog->jconf->input.speech_input == SP_MIC && speechlen == 0) {
    /* this is first up-trigger */
    if (rewind_msec > 0 && !recog->adin->is_valid_data) {
      /* not spoken currently but has data to process at first trigger */
      /* it means that there are old spoken segments */
      /* disgard them */
      printf("disgard already recorded %d samples\n", len);
      return 0;
    }
    /* erase "<<<please speak>>>" text on tty */
    fprintf(stderr, "\r                    \r");
    if (rewind_msec > 0) {
      /* when -rewind value set larger than 0, the speech data spoken
	 while pause will be considered back to the specified msec.
	 */
      printf("buffered samples=%d\n", len);
      w = rewind_msec * sfreq / 1000;
      if (len > w) {
	start = len - w;
	len = w;
      } else {
	start = 0;
      }
      printf("will process from %d\n", start);
    }
  }

  vecnet_sub(&(now[start]), len, recog);

  /* accumulate sample num of this segment */
  speechlen += len;
#ifdef HAVE_PTHREAD
  if (recog->adin->enable_thread) {
    /* if input length reaches limit, rehash the ad-in buffer */
    if (recog->adin->speechlen > MAXSPEECHLEN - 16000) {
      recog->adin->rehash = TRUE;
      fprintf(stderr, "+");
    }
  }
#endif
  
  /* display progress in dots */
  fprintf(stderr, ".");
  return(0);
}

static void
vecnet_send_end_of_segment()
{
  int i, j;
  
  /* send header value of '0' as an end-of-utterance marker */
  i = 0;
  for (j=0;j<adinnet_servnum;j++) {
    if (send(sd[j], &i, sizeof(int), 0) != sizeof(int)) {
      fprintf(stderr, "Error: failed to send %lu bytes\n", sizeof(int));
      return ;
    }
  }
}

static void
vecnet_send_end_of_session()
{
  int i, j;

  /* send negative header value as an end-of-session marker */
  i = -1;
  for (j=0;j<adinnet_servnum;j++) {
    if (send(sd[j], &i, sizeof(int), 0) != sizeof(int)) {
      fprintf(stderr, "Error: failed to send %lu bytes\n", sizeof(int));
      return;
    }
  }
}

/**********************************************************************/
/** 
 * <JA>
 * adinnetサーバにセグメント終了信号を送信する
 * 
 * </JA>
 * <EN>
 * Send end-of-segment singal to adinnet server.
 * 
 * </EN>
 */
static void
adin_send_end_of_segment()
{
  char p;
  int i;

  for(i=0;i<adinnet_servnum;i++) {
    if (wt(sd[i], &p,  0) < 0) {
      perror("adintool: cannot write");
      fprintf(stderr, "failed to send EOS to %s:%d\n", adinnet_serv[i], adinnet_port[i]);
    }
  }
}

/**********************************************************************/
/* reveice resume/pause command from adinnet server */
/* (for SPOUT_ADINNET only) */
/*'1' ... resume  '0' ... pause */

static int unknown_command_counter = 0;	///< Counter to detect broken connection

/** 
 * <JA>
 * 音声取り込み中にサーバからの中断/再開コマンドを受け取るための
 * コールバック関数
 * 
 * @return コマンド無しか再開コマンドで録音続行の場合 0,
 * エラー時 -2, 中断コマンドを受信して録音を中断すべきとき -1 を返す．
 * </JA>
 * <EN>
 * Callback function for A/D-in processing to check pause/resume
 * command from adinnet server.
 * 
 * @return 0 when no command or RESUME command to tell caller to
 * continue recording, -1 when received a PAUSE command and tell caller to
 * stop recording, or -2 when error.
 * </EN>
 */
static int
adinnet_check_command()
{
  fd_set rfds;
  struct timeval tv;
  int status;
  int cnt, ret;
  char com;
  int i, max_sd;
  
  /* check if some commands are waiting in queue */
  FD_ZERO(&rfds);
  max_sd = 0;
  for(i=0;i<adinnet_servnum;i++) {
    if (max_sd < sd[i]) max_sd = sd[i];
    FD_SET(sd[i], &rfds);
  }
  tv.tv_sec = 0;
  tv.tv_usec = 1;
  status = select(max_sd+1, &rfds, NULL, NULL, &tv);
  if (status < 0) {           /* error */
    fprintf(stderr, "adintool: cannot check command from adinnet server\n");
    return -2;                        /* error return */
  }
  if (status > 0) {           /* there are some data */
    for (i=0;i<adinnet_servnum;i++) {
      if (FD_ISSET(sd[i], &rfds)) {
	ret = rd(sd[i], &com, &cnt, 1); /* read in command */
	switch (com) {
	case '0':                       /* pause */
	  fprintf(stderr, "<#%d: PAUSE>\n", i+1);
	  stop_at_next = TRUE;	/* mark to pause at the end of this input */
	  /* tell caller to stop recording */
	  return -1;
	case '1':                       /* resume */
	  fprintf(stderr, "<#%d: RESUME - already running, ignored>\n", i+1);
	  /* we are already running, so just continue */
	  break;
	case '2':                       /* terminate */
	  fprintf(stderr, "<#%d: TERMINATE>\n", i+1);
	  stop_at_next = TRUE;	/* mark to pause at the end of this input */
	  /* tell caller to stop recording immediately */
	  return -2;
	  break;
	default:
	  fprintf(stderr, "adintool: unknown command from #%d: %d\n", i+1,com);
	  unknown_command_counter++;
	  /* avoid infinite loop in that case... */
	  /* this may happen when connection is terminated from server side  */
	  if (unknown_command_counter > 100) {
	    fprintf(stderr, "killed by a flood of unknown commands from server\n");
	    exit(1);
	  }
	}
      }
    }
  }
  return 0;			/* continue ad-in */
}

static int resume_count[MAXCONNECTION];       ///< Number of incoming resume commands for resume synchronization

/** 
 * <JA>
 * サーバから再開コマンドを受信するまで待つ．再開コマンドを受信したら
 * 終了する．
 * 
 * @return エラー時 -1, 通常終了は 0 を返す．
 * </JA>
 * <EN>
 * Wait for resume command from server.
 * 
 * @return 0 on normal exit, or -1 on error.
 * </EN>
 */
static int
adinnet_wait_command()
{
  fd_set rfds;
  int status;
  int cnt, ret;
  char com;
  int i, count, max_sd;

  fprintf(stderr, "<<< waiting RESUME >>>");

  while(1) {
    /* check for synchronized resume */
    if (loose_sync) {
      for(i=0;i<adinnet_servnum;i++) {
	if (resume_count[i] == 0) break;
      }
      if (i >= adinnet_servnum) { /* all count > 0 */
	for(i=0;i<adinnet_servnum;i++) resume_count[i] = 0;
	fprintf(stderr, ">>RESUME\n");
	return 1;                       /* restart recording */
      }
    } else {
      /* force same resume count among servers */
      count = resume_count[0];
      for(i=1;i<adinnet_servnum;i++) {
	if (count != resume_count[i]) break;
      }
      if (i >= adinnet_servnum && count > 0) {
	/* all resume counts are the same, actually resume */
	for(i=0;i<adinnet_servnum;i++) resume_count[i] = 0;
	fprintf(stderr, ">>RESUME\n");
	return 1;                       /* restart recording */
      }
    }
    /* not all host send me resume command */
    FD_ZERO(&rfds);
    max_sd = 0;
    for(i=0;i<adinnet_servnum;i++) {
      if (max_sd < sd[i]) max_sd = sd[i];
      FD_SET(sd[i], &rfds);
    }
    status = select(max_sd+1, &rfds, NULL, NULL, NULL); /* block when no command */
    if (status < 0) {         /* error */
      fprintf(stderr, "adintool: cannot check command from adinnet server\n");
      return -1;                      /* error return */
    } else {                  /* there are some data */
      for(i=0;i<adinnet_servnum;i++) {
	if (FD_ISSET(sd[i], &rfds)) {
	  ret = rd(sd[i], &com, &cnt, 1);
	  switch (com) {
	  case '0':                       /* pause */
	    /* already paused, so just wait for next command */
	    if (loose_sync) {
	      fprintf(stderr, "<#%d: PAUSE - already paused, reset sync>\n", i+1);
	      for(i=0;i<adinnet_servnum;i++) resume_count[i] = 0;
	    } else {
	      fprintf(stderr, "<#%d: PAUSE - already paused, ignored>\n", i+1);
	    }
	    break;
	  case '1':                       /* resume */
	    /* do resume */
	    resume_count[i]++;
	    if (loose_sync) {
	      fprintf(stderr, "<#%d: RESUME>\n", i+1);
	    } else {
	      fprintf(stderr, "<#%d: RESUME @%d>\n", i+1, resume_count[i]);
	    }
	    break;
	  case '2':                       /* terminate */
	    /* already paused, so just wait for next command */
	    if (loose_sync) {
	      fprintf(stderr, "<#%d: TERMINATE - already paused, reset sync>\n", i+1);
	      for(i=0;i<adinnet_servnum;i++) resume_count[i] = 0;
	    } else {
	      fprintf(stderr, "<#%d: TERMINATE - already paused, ignored>\n", i+1);
	    }
	    break;
	  default:
	    fprintf(stderr, "adintool: unknown command from #%d: %d\n", i+1, com);
	    unknown_command_counter++;
	    /* avoid infinite loop in that case... */
	    /* this may happen when connection is terminated from server side  */
	    if (unknown_command_counter > 100) {
	      fprintf(stderr, "killed by a flood of unknown commands from server\n");
	      exit(1);
	    }
	  }
	}
      }
    }
  }
  return 0;
} 

/* close file */
static boolean
close_files()
{
  size = sizeof(SP16) * speechlen;

  if (writing_file) {
    if (use_raw) {
      if (close(fd) != 0) {
	perror("adinrec");
	return FALSE;
      }
    } else {
      if (wrwav_close(fp) == FALSE) {
	fprintf(stderr, "adinrec: failed to close file\n");
	return FALSE;
      }
    }
    printf("%s: %d samples (%.2f sec.) [%6d (%5.2fs) - %6d (%5.2fs)]\n", 
	   outpath, speechlen, 
	   (float)speechlen / (float)sfreq,
	   trigger_sample, (float)trigger_sample / (float)sfreq, 
	   trigger_sample + speechlen, (float)(trigger_sample + speechlen) / (float)sfreq);
    
    writing_file = FALSE;
  }

  return TRUE;
}  

/* Interrupt signal handling */
static void
interrupt_record(int signum)
{
  fprintf(stderr, "[Interrupt]");
  if (speech_output == SPOUT_FILE) {
    /* close files */
    close_files();
  }
  if (speech_output == SPOUT_VECTORNET) {
    vecnet_send_end_of_session();
  }
  /* terminate program */
  exit(1);
}

static void
record_trigger_time(Recog *recog, void *data)
{
  trigger_sample = recog->adin->last_trigger_sample;
}

/** 
 * <JA>
 * メイン関数
 * 
 * @param argc [in] 引数列の長さ
 * @param argv [in] 引数列
 * 
 * @return 
 * </JA>エラー時 1，通常終了時 0 を返す．
 * <EN>
 * Main function.
 * 
 * @param argc [in] number of argument.
 * @param argv [in] array of arguments.
 * 
 * @return 1 on error, 0 on success.
 * </EN>
 */
int
main(int argc, char *argv[])
{
  Recog *recog;
  Jconf *jconf;
  int ret;
  int i;
  boolean is_continues;

  /* create instance */
  recog = j_recog_new();
  jconf = j_jconf_new();
  recog->jconf = jconf;

  /********************/
  /* setup parameters */
  /********************/
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
  j_add_option("-h", 0, 0, "display this help", opt_help);
  j_add_option("-help", 0, 0, "display this help", opt_help);
  j_add_option("--help", 0, 0, "display this help", opt_help);

  /* when no argument, output help and exit */
  if (argc <= 1) {
    opt_help(jconf, NULL, 0);
    return 0;
  }

  /* read arguments and set parameters */
  if (j_config_load_args(jconf, argc, argv) == -1) {
    fprintf(stderr, "Error reading arguments\n");
    return -1;
  }

  /* check needed arguments */
  if (speech_output == SPOUT_FILE && filename == NULL) {
    fprintf(stderr, "Error: output filename not specified\n");
    return(-1);
  }
  if ((speech_output == SPOUT_ADINNET || speech_output == SPOUT_VECTORNET) && adinnet_servnum < 1) {
    fprintf(stderr, "Error: server name for output not specified\n");
    return(-1);
  }
  if (jconf->input.speech_input == SP_ADINNET &&
      speech_output != SPOUT_ADINNET && speech_output != SPOUT_VECTORNET &&
      adinnet_servnum >= 1) {
    fprintf(stderr, "Warning: you specified port num by -port, but it's for output\n");
    fprintf(stderr, "Warning: you may specify input port by -inport instead.\n");
    fprintf(stderr, "Warning: now the default value (%d) will be used\n", ADINNET_PORT);
  }
#ifdef USE_NETAUDIO
  if (jconf->input.speech_input == SP_NETAUDIO && jconf->input.netaudio_devname == NULL) {
    fprintf(stderr, "Error: NetAudio server name not specified\n");
    return(-1);
  }
#endif
  if (adinnet_portnum != adinnet_servnum) {
    /* if only one server, use default */
    if (adinnet_servnum == 1) {
      if (speech_output == SPOUT_ADINNET) {
	adinnet_port[0] = ADINNET_PORT;
      } else if (speech_output == SPOUT_VECTORNET) {
	adinnet_port[0] = VECINNET_PORT;
      }
      adinnet_portnum = 1;
    } else {
      fprintf(stderr, "Error: you should specify both server names and different port for each!\n");
      fprintf(stderr, "\tserver:");
      for(i=0;i<adinnet_servnum;i++) fprintf(stderr, " %s", adinnet_serv[i]);
      fprintf(stderr, "\n\tport  :");
      for(i=0;i<adinnet_portnum;i++) fprintf(stderr, " %d", adinnet_port[i]);
      fprintf(stderr, "\n");
      return(-1);
    }
  }

  if (speech_output == SPOUT_VECTORNET) {
    if (vecnet_paramtype == F_ERR_INVALID || vecnet_veclen == 0) {
      fprintf(stderr, "Error: with \"-out vecnet\", \"-paramtype\" and \"-veclen\" is required\n");
      return -1;
    }
  }

  /* set Julius default parameters for unspecified acoustic parameters */
  if (jconf->am_root->analysis.para_htk.loaded == 1) {
    apply_para(&(jconf->am_root->analysis.para), &(jconf->am_root->analysis.para_htk));
  }
  apply_para(&(jconf->am_root->analysis.para), &(jconf->am_root->analysis.para_default));

  /* set some values */
  jconf->input.sfreq = jconf->am_root->analysis.para.smp_freq;
  jconf->input.period = jconf->am_root->analysis.para.smp_period;
  jconf->input.frameshift = jconf->am_root->analysis.para.frameshift;
  jconf->input.framesize = jconf->am_root->analysis.para.framesize;

  if (speech_output == SPOUT_VECTORNET) {
    if (vecnet_init(recog) == FALSE) exit(1);
  }

  /* disable successive segmentation when no segmentation available */
  if (!jconf->detect.silence_cut) continuous_segment = FALSE;
  /* store sampling rate locally */
  sfreq = jconf->am_root->analysis.para.smp_freq;

  /********************/
  /* setup for output */
  /********************/
  if (speech_output == SPOUT_FILE) {
    /* allocate work area for output file name */
    if (continuous_segment) {
      outpath = (char *)mymalloc(strlen(filename) + 10);
    } else {
      if (use_raw) {
	outpath = filename;
      } else {
	outpath = new_output_filename(filename, ".wav");
      }
    }
  }

  /**************************************/
  /* display input/output configuration */
  /**************************************/
  put_status(recog);

  /*********************/
  /* connect to server */
  /*********************/
  if (speech_output == SPOUT_ADINNET || speech_output == SPOUT_VECTORNET) {
    /* connect to adinnet server(s) */
    for(i=0;i<adinnet_servnum;i++) {
      fprintf(stderr, "connecting to #%d (%s:%d)...", i+1, adinnet_serv[i], adinnet_port[i]);
      sd[i] = make_connection(adinnet_serv[i], adinnet_port[i]);
      if (sd[i] < 0) return 1;	/* on error */
      fprintf(stderr, "connected\n");
    }
  } else if (speech_output == SPOUT_STDOUT) {
    /* output to stdout */
    fd = 1;
  }

  if (speech_output == SPOUT_VECTORNET) {
    vecnet_send_header(recog);
  }

  /**********************/
  /* interrupt handling */
  /**********************/
  if (signal(SIGINT, interrupt_record) == SIG_ERR) {
    fprintf(stderr, "Warning: signal interruption may collapse output\n");
  }
  if (signal(SIGTERM, interrupt_record) == SIG_ERR) {
    fprintf(stderr, "Warning: signal interruption may collapse output\n");
  }
#ifdef SIGPIPE
  if (signal(SIGPIPE, interrupt_record) == SIG_ERR) {
    fprintf(stderr, "Warning: signal interruption may collapse output\n");
  }
#endif
#ifdef SIGQUIT
  if (signal(SIGQUIT, interrupt_record) == SIG_ERR) {
    fprintf(stderr, "Warning: signal interruption may collapse output\n");
  }
#endif

  /***************************/
  /* initialize input device */
  /***************************/
  if (jconf->input.speech_input == SP_ADINNET) {
    jconf->input.adinnet_port = adinnet_port_in;
  }
  if (j_adin_init(recog) == FALSE) {
    fprintf(stderr, "Error in initializing adin device\n");
    return -1;
  }
  if (rewind_msec > 0) {
    /* allow adin module to keep triggered speech while pausing */
#ifdef HAVE_PTHREAD
    if (recog->adin->enable_thread) {
      recog->adin->ignore_speech_while_recog = FALSE;
    }
#endif
  }

  /*********************/
  /* add some callback */
  /*********************/
  callback_add(recog, CALLBACK_EVENT_SPEECH_START, record_trigger_time, NULL);


  /*******************/
  /* begin recording */
  /*******************/
  if (continuous_segment) {	/* reset parameter for successive output */
    total_speechlen = 0;
    sid = startid;
  }
  fprintf(stderr,"[start recording]\n");
  if (jconf->input.speech_input == SP_RAWFILE) file_counter = 0;

  /*********************/
  /* input stream loop */
  /*********************/
  while(1) {

    /* begin A/D input of a stream */
    ret = j_open_stream(recog, NULL);
    switch(ret) {
    case 0:			/* succeeded */
      break;
    case -1:      		/* error */
      /* go on to next input */
      continue;
    case -2:			/* end of recognition process */
      switch(jconf->input.speech_input) {
      case SP_RAWFILE:
	fprintf(stderr, "%d files processed\n", file_counter);
	break;
      case SP_STDIN:
	fprintf(stderr, "reached end of input on stdin\n");
	break;
      default:
	fprintf(stderr, "failed to begin input stream\n");
      }
      /* exit recording */
      goto record_end;
    }

    /*********************************/
    /* do VAD and recording */
    /*********************************/
    do {
      /* process one segment with segmentation */
      /* for incoming speech input, speech detection and segmentation are
	 performed and, adin_callback_* is called for speech output for each segment block.
      */
      /* adin_go() return when input segmented by long silence, or input stream reached to the end */
      speechlen = 0;
      stop_at_next = FALSE;
      if (jconf->input.speech_input == SP_MIC) {
	fprintf(stderr, "<<< please speak >>>");
      }
      if (speech_output == SPOUT_ADINNET) {
	ret = adin_go(adin_callback_adinnet, adinnet_check_command, recog);
      } else if (speech_output == SPOUT_VECTORNET) {
	if (vecnet_prepare(recog) == FALSE){
	  fprintf(stderr, "failed to init\n");
	  exit(1);
	}
	ret = adin_go(adin_callback_vecnet, adinnet_check_command, recog);
      } else {
	ret = adin_go(adin_callback_file, NULL, recog);
      }
      /* return value of adin_go:
	 -2: input terminated by pause command from adinnet server
	 -1: input device read error or callback process error
	 0:  paused by input stream (end of file, etc..)
	 >0: detected end of speech segment:
             by adin-cut, or by callback process
	 (or return value of ad_check (<0) (== not used in this program))
      */
      /* if PAUSE or TERMINATE command has been received while input,
	 stop_at_next is TRUE here  */
      switch(ret) {
      case -2:	     /* terminated by terminate command from server */
	fprintf(stderr, "[terminated by server]\n");
	break;
      case -1:		     /* device read error or callback error */
	fprintf(stderr, "[error]\n");
	break;
      case 0:			/* reached to end of input */
	fprintf(stderr, "[eof]\n");
	break;
      default:	  /* input segmented by silence or callback process */
	fprintf(stderr, "[segmented]\n");
	break;
      }
      
      if (ret == -1) {
	/* error in input device or callback function, so terminate program here */
	return 1;
      }

      /*************************/
      /* one segment processed */
      /*************************/
      if (speech_output == SPOUT_FILE) {
	/* close output files */
	if (close_files() == FALSE) return 1;
      } else if (speech_output == SPOUT_ADINNET) {
	if (speechlen > 0) {
	  if (ret >= 0 || stop_at_next) { /* segmented by adin-cut or end of stream or server-side command */
	    /* send end-of-segment ack to client */
	    adin_send_end_of_segment();
	  }
	  /* output info */
	  printf("sent: %d samples (%.2f sec.) [%6d (%5.2fs) - %6d (%5.2fs)]\n", 
		 speechlen, (float)speechlen / (float)sfreq,
		 trigger_sample, (float)trigger_sample / (float)sfreq, 
		 trigger_sample + speechlen, (float)(trigger_sample + speechlen) / (float)sfreq);
	}
      } else if (speech_output == SPOUT_VECTORNET) {
	if (speechlen > 0) {
	  if (ret >= 0 || stop_at_next) { /* segmented by adin-cut or end of stream or server-side command */
	    /* send end-of-segment ack to client */
	    vecnet_send_end_of_segment();
	    vecnet_param_update(recog);
	  }
	  /* output info */
	  printf("sent: %d samples (%.2f sec.) [%6d (%5.2fs) - %6d (%5.2fs)]\n", 
		 speechlen, (float)speechlen / (float)sfreq,
		 trigger_sample, (float)trigger_sample / (float)sfreq, 
		 trigger_sample + speechlen, (float)(trigger_sample + speechlen) / (float)sfreq);
	}
      }

      /*************************************/
      /* increment ID and total sample len */
      /*************************************/
      if (continuous_segment) {
	total_speechlen += speechlen;
	sid++;
      }

      /***************************************************/
      /* with adinnet server, if terminated by           */
      /* server-side PAUSE command, wait for RESUME here */
      /***************************************************/
      if (pause_each) {
	/* pause at each end */
	//if (speech_output == SPOUT_ADINNET && speechlen > 0) {
	if (speech_output == SPOUT_ADINNET || speech_output == SPOUT_VECTORNET) {
	  if (adinnet_wait_command() < 0) {
	    /* command error: terminate program here */
	    return 1;
	  }
	}
      } else {
	if ((speech_output == SPOUT_ADINNET || speech_output == SPOUT_VECTORNET) && stop_at_next) {
	  if (adinnet_wait_command() < 0) {
	    /* command error: terminate program here */
	    return 1;
	  }
	}
      }

      /* loop condition check */
      is_continues = FALSE;
      if (continuous_segment && (ret > 0 || ret == -2)) {
	is_continues = TRUE;
      }
      
    } while (is_continues); /* to the next segment in this input stream */

    /***********************/
    /* end of input stream */
    /***********************/
    adin_end(recog->adin);

  } /* to the next input stream (i.e. next input file in SP_RAWFILE) */

 record_end:

  if (speech_output == SPOUT_FILE) {
    if (continuous_segment) {
      printf("recorded total %d samples (%.2f sec.) segmented to %s.%04d - %s.%04d files\n", total_speechlen, (float)total_speechlen / (float)sfreq, filename, 0, filename, sid-1);
    }
  }

  if (speech_output == SPOUT_VECTORNET) {
    vecnet_send_end_of_session();
    vecnet_param_update(recog);
  }

  return 0;
}
