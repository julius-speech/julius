/**
 * @file   paramtypes.c
 *
 * <JA>
 * @brief  特徴パラメータ型の文字列表現とバイナリ表現の相互変換
 *
 * このファイルの関数は，特徴パラメータ型の文字列表現（"MFCC_E_D_Z" など）
 * と HTK の short 型で表される内部バイナリ形式との相互変換を行ないます．
 * </JA>
 * <EN>
 * @brief  Convert between string and binary expression of parameter type
 *
 * The functions in this file converts the expression of parameter type,
 * between string (ex. "MFCC_E_D_Z") and internal binary format used in HTK.
 * </EN>
 *
 * @author Akinobu LEE
 * @date   Tue Feb 15 00:06:26 2005
 *
 * $Revision: 1.5 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/htk_defs.h>
#include <sent/htk_param.h>


/// Database that relates base type strings to binary code and description string.
static OptionStr pbase[] = {
  {"WAVEFORM", F_WAVEFORM, "sampled waveform", FALSE},
  {"DISCRETE", F_DISCRETE, "Discrete", FALSE},
  {"LPC", F_LPC, "LPC", TRUE},
  {"LPCEPSTRA", F_LPCEPSTRA, "LPC cepstral", TRUE},
  {"MFCC", F_MFCC, "mel-frequency cepstral", TRUE},
  {"FBANK", F_FBANK, "log mel-filter bank", TRUE},
  {"MELSPEC", F_MELSPEC, "linear mel-filter bank", TRUE},
  {"LPREFC", F_LPREFC, "LPC(reflection)", TRUE},
  {"LPDELCEP", F_LPDELCEP, "LPC+Delta", TRUE},
  {"USER", F_USER, "user defined sample kind", TRUE},
  {NULL,0,NULL,FALSE}
};
/// Database that relates qualifier type strings to binary code and description string.
static OptionStr pqual[] = {
  {"_E", F_ENERGY, "log energy coef.", TRUE},
  {"_N", F_ENERGY_SUP, "uppress absolute energy", TRUE},
  {"_D", F_DELTA, "delta coef.", TRUE},
  {"_A", F_ACCL, "acceleration coef.", TRUE},
  {"_C", F_COMPRESS, "compressed", TRUE},
  {"_Z", F_CEPNORM, "cepstral mean normalization", TRUE},
  {"_K", F_CHECKSUM, "CRC checksum added", TRUE},
  {"_0", F_ZEROTH, "0'th cepstral parameter", TRUE},
  {NULL,0,NULL,FALSE}
};

/** 
 * Convert a qualifier string to a binary type code.
 * 
 * @param s [in] a string that contains qualifier strings like "_E_D_Z"
 * 
 * @return the converted internal binary type code, F_ERR_INVALID if failed.
 */
short
param_qualstr2code(char *s)
{
  int i, qlen;
  char *p;
  short qual_type;

  qual_type = 0;
  p = s;

  /* parse qualifiers */
  while (*p == '_') {
    for (i=0;pqual[i].name!=NULL;i++) {
      qlen = strlen(pqual[i].name);
      if (strncasecmp(p, pqual[i].name, qlen) == 0) {
	qual_type |= pqual[i].type;
	break;
      }
    }
    if (pqual[i].name == NULL) {	/* qualifier not found */
      jlog("Error: paramtypes: unknown parameter qualifier: %2s\n", p);
      return(F_ERR_INVALID);
    }
    p += 2;
  }

  return(qual_type);
}

/** 
 * Convert a type string that contains basename and qualifiers to a binary type code.
 * 
 * @param s [in] a string that contains base and qualifier string like "MFCC_E_D_Z"
 * 
 * @return the converted internal binary type code, F_ERR_INVALID if failed.
 */
short
param_str2code(char *s)
{
  int i;
  short param_type, qual_type;
  char *p, *buf;

  /* determine base type */
  /* cutout base part to *buf */
  buf = strcpy((char *)mymalloc(strlen(s)+1), s);
  p = strchr(buf, '_');
  if (p != NULL) *p = '\0';
  
  for (i=0;pbase[i].name!=NULL;i++) {
    if (strcasecmp(buf, pbase[i].name) == 0) {
      param_type = pbase[i].type;
      /* qualifiers */
      qual_type = param_qualstr2code(s + strlen(buf));
      if (qual_type == F_ERR_INVALID) {
	free(buf);
	return(F_ERR_INVALID);
      } else {
	param_type |= qual_type;
	free(buf);
	return(param_type);
      }
    }
  }
  /* base type not found */
  free(buf);
  return(F_ERR_INVALID);
}

/** 
 * Convert the qualifier part of a binary type code to string.
 *
 * @param buf [out] buffer to store the resulting string (must have enough length)
 * @param type [in] binary type code to convert.
 * @param descflag [in] set to TRUE if you want result in description string
 * instead of qualifier string.
 * 
 * @return @a buf on success, NULL on failure.
 */
char *
param_qualcode2str(char *buf, short type, boolean descflag)
{
  int i;

  /* qualifier */
  for (i=0;pqual[i].name!=NULL;i++) {
    if (type & pqual[i].type) {
      if (descflag) {
	sprintf(buf, " %s %s\n", pqual[i].desc,
		(pqual[i].supported ? "" : "(not supported)"));
      } else {
	strcat(buf, pqual[i].name);
      }
    }
  }
  return(buf);
}

/** 
 * Convert a binary type code to string.
 *
 * @param buf [out] buffer to store the resulting string (must have enough length)
 * @param type [in] binary type code to convert.
 * @param descflag [in] set to TRUE if you want result in description string
 * instead of base and qualifier string.
 * 
 * @return @a buf on success, NULL on failure.
 */
char *
param_code2str(char *buf, short type, boolean descflag)
{
  int i;
  short btype;

  /* basetype */
  btype = type & F_BASEMASK;
  for (i = 0; pbase[i].name != NULL; i++) {
    if (pbase[i].type == btype) {
      if (descflag) {
	sprintf(buf, "%s %s with:\n", pbase[i].desc,
		(pbase[i].supported ? "" : "(not supported)"));
      } else {
	strcpy(buf, pbase[i].name);
      }
      break;
    }
  }
  if (pbase[i].name  == NULL) {	/* not found */
    sprintf(buf, "ERROR: unknown basetype ID: %d\n", btype);
    return(buf);
  }

  /* add qualifier string to buf */
  param_qualcode2str(buf, type, descflag);

  return(buf);
}
