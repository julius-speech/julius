/**
 * @file   para.c
 * 
 * <JA>
 * @brief  特徴量抽出条件の扱い
 *
 * 音響分析の設定パラメータを保持する Value 構造体を扱う．
 * </JA>
 * 
 * <EN>
 * @brief  Acoustic analysis condition parameter handling
 * </EN>
 *
 * Value structure holds acoustic analysis configuration parameters.
 * 
 * @author Akinobu Lee
 * @date   Fri Oct 27 14:55:00 2006
 *
 * $Revision: 1.13 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/mfcc.h>
#include <sent/speech.h>

/** 
 * Reset configuration parameters for MFCC computation.
 * 
 * @param para [out] feature extraction parameters
 * 
 */
void
undef_para(Value *para)
{
  para->basetype   = F_ERR_INVALID;
  para->smp_period = -1;
  para->smp_freq   = -1;
  para->framesize  = -1;
  para->frameshift = -1;
  para->preEmph    = -1;
  para->mfcc_dim   = -1;
  para->lifter     = -1;
  para->fbank_num  = -1;
  para->delWin     = -1;
  para->accWin     = -1;
  para->silFloor   = -1;
  para->escale     = -1;
  para->enormal    = -1;
  para->hipass     = -2;	/* undef */
  para->lopass     = -2;	/* undef */
  para->cmn        = -1;
  para->cvn	   = -1;
  para->raw_e      = -1;
  para->c0         = -1;
  //para->ss_alpha   = -1;
  //para->ss_floor   = -1;
  para->vtln_alpha = -1;
  para->vtln_upper = -1;
  para->vtln_lower = -1;
  para->zmeanframe = -1;
  para->usepower   = -1;
  para->delta      = -1;
  para->acc        = -1;
  para->energy     = -1;
  para->absesup    = -1;
  para->baselen    = -1;
  para->vecbuflen  = -1;
  para->veclen     = -1;

  para->loaded     = 0;
}

/** 
 * Set Julius default parameters for MFCC computation.
 * 
 * @param para [out] feature extraction parameters
 * 
 */
void
make_default_para(Value *para)
{
  para->basetype   = F_MFCC;
  para->smp_period = 625;	/* 16kHz = 625 100ns unit */
  para->smp_freq   = 16000;	/* 16kHz = 625 100ns unit */
  para->framesize  = DEF_FRAMESIZE;
  para->frameshift = DEF_FRAMESHIFT;
  para->preEmph    = DEF_PREENPH;
  para->fbank_num  = DEF_FBANK;
  para->lifter     = DEF_CEPLIF;
  para->delWin     = DEF_DELWIN;
  para->accWin     = DEF_ACCWIN;
  para->raw_e      = FALSE;
  para->enormal    = FALSE;
  para->escale     = DEF_ESCALE;
  para->silFloor   = DEF_SILFLOOR;
  para->cvn	   = FALSE;
  para->hipass     = -1;	/* disabled */
  para->lopass     = -1;	/* disabled */
  //para->ss_alpha    = DEF_SSALPHA;
  //para->ss_floor    = DEF_SSFLOOR;
  para->vtln_alpha = 1.0;	/* disabled */
  para->zmeanframe = FALSE;
  para->usepower   = FALSE;
}

/** 
 * Set HTK default configuration parameters for MFCC computation.
 * This will be refered when parameters are given as HTK Config file.
 * 
 * @param para [out] feature extraction parameters
 * 
 */
void
make_default_para_htk(Value *para)
{
  para->framesize  = 256000.0;	/* dummy! */
  para->preEmph    = 0.97;
  para->fbank_num  = 20;
  para->lifter     = 22;
  para->delWin     = 2;
  para->accWin     = 2;
  para->raw_e      = TRUE;
  para->enormal    = TRUE;
  para->escale     = 0.1;
  para->silFloor   = 50.0;
  para->hipass     = -1;	/* disabled */
  para->lopass     = -1;	/* disabled */
  para->vtln_alpha = 1.0;	/* disabled */
  para->zmeanframe = FALSE;
  para->usepower   = FALSE;
}

/** 
 * Merge two configuration parameters for MFCC computation.
 * 
 * @param dst [out] feature extraction parameters to set to
 * @param src [out] feature extraction parameters to set from
 * 
 */
void
apply_para(Value *dst, Value *src)
{
  if (dst->basetype   == F_ERR_INVALID) dst->basetype = src->basetype;
  if (dst->smp_period == -1) dst->smp_period = src->smp_period;
  if (dst->smp_freq   == -1) dst->smp_freq = src->smp_freq; 
  if (dst->framesize  == -1) dst->framesize = src->framesize; 
  if (dst->frameshift == -1) dst->frameshift = src->frameshift; 
  if (dst->preEmph    == -1) dst->preEmph = src->preEmph; 
  if (dst->mfcc_dim   == -1) dst->mfcc_dim = src->mfcc_dim; 
  if (dst->lifter     == -1) dst->lifter = src->lifter; 
  if (dst->fbank_num  == -1) dst->fbank_num = src->fbank_num; 
  if (dst->delWin     == -1) dst->delWin = src->delWin; 
  if (dst->accWin     == -1) dst->accWin = src->accWin; 
  if (dst->silFloor   == -1) dst->silFloor = src->silFloor; 
  if (dst->escale     == -1) dst->escale = src->escale; 
  if (dst->enormal    == -1) dst->enormal = src->enormal; 
  if (dst->hipass     == -2) dst->hipass = src->hipass;
  if (dst->lopass     == -2) dst->lopass = src->lopass;
  if (dst->cmn        == -1) dst->cmn = src->cmn; 
  if (dst->cvn        == -1) dst->cvn = src->cvn; 
  if (dst->raw_e      == -1) dst->raw_e = src->raw_e; 
  if (dst->c0         == -1) dst->c0 = src->c0; 
  //if (dst->ss_alpha   == -1) dst->ss_alpha = src->ss_alpha; 
  //if (dst->ss_floor   == -1) dst->ss_floor = src->ss_floor; 
  if (dst->vtln_alpha == -1) dst->vtln_alpha = src->vtln_alpha; 
  if (dst->vtln_upper == -1) dst->vtln_upper = src->vtln_upper; 
  if (dst->vtln_lower == -1) dst->vtln_lower = src->vtln_lower; 
  if (dst->zmeanframe == -1) dst->zmeanframe = src->zmeanframe; 
  if (dst->usepower   == -1) dst->usepower = src->usepower; 
  if (dst->delta      == -1) dst->delta = src->delta; 
  if (dst->acc        == -1) dst->acc = src->acc; 
  if (dst->energy     == -1) dst->energy = src->energy; 
  if (dst->absesup    == -1) dst->absesup = src->absesup; 
  if (dst->baselen    == -1) dst->baselen = src->baselen; 
  if (dst->vecbuflen  == -1) dst->vecbuflen = src->vecbuflen; 
  if (dst->veclen     == -1) dst->veclen = src->veclen; 
}

#define ISTOKEN(A) (A == ' ' || A == '\t' || A == '\n') ///< Determine token characters

/** 
 * Read and parse an HTK Config file, and set the specified option values.
 * 
 * @param HTKconffile [in] HTK Config file path name
 * @param para [out] MFCC parameter to set
 *
 * @return TRUE on success, FALSE on failure.
 */
boolean
htk_config_file_parse(char *HTKconffile, Value *para)
{
  FILE *fp;
  char buf[512];
  char *p, *d, *a;
  float srate;
  boolean skipped;

  jlog("Stat: para: parsing HTK Config file: %s\n", HTKconffile);
  
  /* convert the content into argument list c_argv[1..c_argc-1] */
  /* c_argv[0] will be the original conffile name */
  if ((fp = fopen(HTKconffile, "r")) == NULL) {
    jlog("Error: para: failed to open HTK Config file: %s\n", HTKconffile);
    return FALSE;
  }

  srate = 0.0;

  while (getl_fp(buf, 512, fp) != NULL) {
    p = buf;
    if (*p == 35) { /* skip comment line */
      continue;
    }

    /* parse the input line to get directive and argument */
    while (*p != '\0' && ISTOKEN(*p)) p++;
    if (*p == '\0') continue;
    d = p;
    while (*p != '\0' && (!ISTOKEN(*p)) && *p != '=') p++;
    if (*p == '\0') continue;
    *p = '\0'; p++;
    while (*p != '\0' && ((ISTOKEN(*p)) || *p == '=')) p++;
    if (*p == '\0') continue;
    a = p;
    while (*p != '\0' && (!ISTOKEN(*p))) p++;
    *p = '\0';

    /* process arguments */
    skipped = FALSE;
    if (strmatch(d, "SOURCERATE")) { /* -smpPeriod */
      srate = atof(a);
    } else if (strmatch(d, "TARGETRATE")) { /* -fshift */
      para->frameshift = atof(a);
    } else if (strmatch(d, "WINDOWSIZE")) { /* -fsize */
      para->framesize = atof(a);
    } else if (strmatch(d, "ZMEANSOURCE")) { /* -zmeansource */
      para->zmeanframe = (a[0] == 'T') ? TRUE : FALSE;
    } else if (strmatch(d, "USEPOWER")) { /* -usepower */
      para->usepower = (a[0] == 'T') ? TRUE : FALSE;
    } else if (strmatch(d, "PREEMCOEF")) { /* -preemph */
      para->preEmph = atof(a);
    } else if (strmatch(d, "USEHAMMING")) { /* (fixed to T) */
      if (a[0] != 'T') {
	jlog("Error: para: USEHAMMING should be T\n", HTKconffile);
	return FALSE;
      }
    } else if (strmatch(d, "NUMCHANS")) { /* -fbank */
      para->fbank_num = atoi(a);
    } else if (strmatch(d, "CEPLIFTER")) { /* -ceplif */
      para->lifter = atoi(a);
    } else if (strmatch(d, "DELTAWINDOW")) { /* -delwin */
      para->delWin = atoi(a);
    } else if (strmatch(d, "ACCWINDOW")) { /* -accwin */
      para->accWin = atoi(a);
    } else if (strmatch(d, "LOFREQ")) { /* -lofreq */
      para->lopass = atof(a);
    } else if (strmatch(d, "HIFREQ")) { /* -hifreq */
      para->hipass = atof(a);
    } else if (strmatch(d, "RAWENERGY")) { /* -rawe */
      para->raw_e = (a[0] == 'T') ? TRUE : FALSE;
    } else if (strmatch(d, "ENORMALISE")) { /* -enormal */
      para->enormal = (a[0] == 'T') ? TRUE : FALSE;
    } else if (strmatch(d, "ESCALE")) { /* -escale */
      para->escale = atof(a);
    } else if (strmatch(d, "SILFLOOR")) { /* -silfloor */
      para->silFloor = atof(a);
    } else if (strmatch(d, "WARPFREQ")) { /* -vtln (1) */
      para->vtln_alpha = atof(a);
    } else if (strmatch(d, "WARPLCUTOFF")) { /* -vtln (2) */
      para->vtln_lower = atof(a);
    } else if (strmatch(d, "WARPUCUTOFF")) { /* -vtln (3) */
      para->vtln_upper = atof(a);
    } else if (strmatch(d, "TARGETKIND")) {
      jlog("Warning: para: TARGETKIND skipped (will be determined by AM header)\n");
      skipped = TRUE;
    } else if (strmatch(d, "NUMCEPS")) {
      jlog("Warning: para: NUMCEPS skipped (will be determined by AM header)\n");
      skipped = TRUE;
    } else {
      jlog("Warning: para: \"%s\" ignored (not supported, or irrelevant)\n", d);
      skipped = TRUE;
    }
    if (!skipped) {
      jlog("Stat: para: %s=%s\n", d, a);
    }
  }

  if (srate == 0.0) {
    jlog("Warning: no SOURCERATE found\n");
    jlog("Warning: assume source waveform sample rate to 625 (16kHz)\n");
    srate = 625;
  }

  para->smp_period = srate;
  para->smp_freq = period2freq(para->smp_period);
  para->frameshift /= srate;
  para->framesize /= srate;

  if (fclose(fp) == -1) {
    jlog("Error: para: failed to close file\n");
    return FALSE;
  }

  para->loaded = 1;

  return TRUE;
}

/** 
 * Set acoustic analysis parameters from HTK HMM definition header information.
 * 
 * @param para [out] acoustic analysis parameters
 * @param param_type [in] parameter type specified at HMM header
 * @param vec_size [in] vector size type specified at HMM header
 */
void
calc_para_from_header(Value *para, short param_type, short vec_size)
{
  int dim;

  /* decode required parameter extraction types */
  para->basetype = param_type & F_BASEMASK;
  para->delta = (param_type & F_DELTA) ? TRUE : FALSE;
  para->acc = (param_type & F_ACCL) ? TRUE : FALSE;
  para->energy = (param_type & F_ENERGY) ? TRUE : FALSE;
  para->c0 = (param_type & F_ZEROTH) ? TRUE : FALSE;
  para->absesup = (param_type & F_ENERGY_SUP) ? TRUE : FALSE;
  para->cmn = (param_type & F_CEPNORM) ? TRUE : FALSE;

  /* guess MFCC dimension from the vector size and parameter type in the
     acoustic HMM */
  dim = vec_size;
  if (para->absesup) dim++;
  dim /= 1 + (para->delta ? 1 : 0) + (para->acc ? 1 : 0);
  if (para->energy) dim--;
  if (para->c0) dim--;
  para->mfcc_dim = dim;
    
  /* determine base size */
  para->baselen = para->mfcc_dim + (para->c0 ? 1 : 0) + (para->energy ? 1 : 0);
  /* set required size of parameter vector for MFCC computation */
  para->vecbuflen = para->baselen * (1 + (para->delta ? 1 : 0) + (para->acc ? 1 : 0));
  /* set size of final parameter vector */
  para->veclen = para->vecbuflen - (para->absesup ? 1 : 0);

  /* on filter-bank output, also overwrite the number of filterbank */
  if (para->basetype == F_FBANK || para->basetype == F_MELSPEC) {
    if (para->fbank_num != dim) {
      jlog("Warning: number of filterbank is set to %d, but AM requires %d\n", para->fbank_num, dim);
      jlog("Warning: use value of AM: %d\n", dim);
      para->fbank_num = dim;
    }
  }
  
}

/** 
 * Output acoustic analysis configuration parameters to stdout.
 *
 * @param fp [in] file pointer
 * @param para [in] configuration parameter
 * 
 */
void
put_para(FILE *fp, Value *para)
{
  fprintf(fp, " Acoustic analysis condition:\n");
  fprintf(fp, "\t       parameter = ");
  switch(para->basetype) {
  case F_MFCC:
    fprintf(fp, "MFCC");
    break;
  case F_FBANK:
    fprintf(fp, "FBANK");
    break;
  case F_MELSPEC:
    fprintf(fp, "MELSPEC");
    break;
  default:
    fprintf(fp, "(UNKNOWN_OR_NOT_SUPPORTED)");
    break;
  }
  if (para->c0) fprintf(fp, "_0");
  if (para->energy) fprintf(fp, "_E");
  if (para->delta) fprintf(fp, "_D");
  if (para->acc) fprintf(fp, "_A");
  if (para->absesup) fprintf(fp, "_N");
  if (para->cmn) fprintf(fp, "_Z");
  fprintf(fp, " (%d dim. from %d cepstrum", para->veclen, para->mfcc_dim);
  if (para->c0) fprintf(fp, " + c0");
  if (para->energy) fprintf(fp, " + energy");
  if (para->absesup) fprintf(fp, ", abs energy supressed");
  if (para->cmn) fprintf(fp, " with CMN");
  fprintf(fp, ")\n");
  fprintf(fp, "\tsample frequency = %5d Hz\n", para->smp_freq);
  fprintf(fp, "\t   sample period = %4d  (1 = 100ns)\n", para->smp_period);
  fprintf(fp, "\t     window size = %4d samples (%.1f ms)\n", para->framesize,
           (float)para->smp_period * (float)para->framesize / 10000.0);
  fprintf(fp, "\t     frame shift = %4d samples (%.1f ms)\n", para->frameshift,
           (float)para->smp_period * (float)para->frameshift / 10000.0);
  fprintf(fp, "\t    pre-emphasis = %.2f\n", para->preEmph);
  fprintf(fp, "\t    # filterbank = %d\n", para->fbank_num);
  fprintf(fp, "\t   cepst. lifter = %d\n", para->lifter);
  fprintf(fp, "\t      raw energy = %s\n", para->raw_e ? "True" : "False");
  if (para->enormal) {
    fprintf(fp, "\tenergy normalize = True (scale = %.1f, silence floor = %.1f dB)\n", para->escale, para->silFloor);
  } else {
    fprintf(fp, "\tenergy normalize = False\n");
  }
  if (para->delta) {
    fprintf(fp, "\t    delta window = %d frames (%.1f ms) around\n", para->delWin,  (float)para->delWin * (float)para->smp_period * (float)para->frameshift / 10000.0);
  }
  if (para->acc) {
    fprintf(fp, "\t      acc window = %d frames (%.1f ms) around\n", para->accWin, (float)para->accWin * (float)para->smp_period * (float)para->frameshift / 10000.0);
  }
  fprintf(fp, "\t     hi freq cut = ");
  if (para->hipass < 0) fprintf(fp, "OFF\n"); 
  else fprintf(fp, "%5d Hz\n", para->hipass);
  fprintf(fp, "\t     lo freq cut = ");
  if (para->lopass < 0) fprintf(fp, "OFF\n"); 
  else fprintf(fp, "%5d Hz\n", para->lopass);
  fprintf(fp, "\t zero mean frame = ");
  if (para->zmeanframe) fprintf(fp, "ON\n");
  else fprintf(fp, "OFF\n");
  fprintf(fp, "\t       use power = ");
  if (para->usepower) fprintf(fp, "ON\n");
  else fprintf(fp, "OFF\n");
  fprintf(fp, "\t             CVN = ");
  switch (para->cvn) {
  case TRUE:
    fprintf(fp, "ON\n");
    break;
  case FALSE:
    fprintf(fp, "OFF\n");
    break;
  default:
    fprintf(fp, "UNKNOWN\n");
    break;
  }
  fprintf(fp, "\t            VTLN = ");
  if(para->vtln_alpha != 1.0) {
    fprintf(fp, "ON, alpha=%.3f, f_low=%.1f, f_high=%.1f\n", para->vtln_alpha, para->vtln_lower, para->vtln_upper);
  } else fprintf(fp, "OFF\n");
}
