/**
 * @file   cdhmm.c
 * 
 * <JA>
 * @brief  音素列からコンテキスト依存音素モデルにアクセスするためのサブ関数群
 * </JA>
 * 
 * <EN>
 * @brief  Sub functions to access context dependent %HMM from phones
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Tue Feb 15 17:33:47 2005
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
#include <sent/htk_hmm.h>

/** 
 * @brief  Generate context-dependent phone name from base phone name
 * and its right context phone name.
 *
 * The center phone name of the right context name will be appended to the
 * base phone name.  If the phone name already has right context, it will
 * be renamed to the new one.
 *
 *    - Example 1: "a" | "r" -> "a+r"
 *    - Example 2: "a" | "e-k+i" -> "a+k"
 *    - Example 3: "k-a" | "e" -> "k-a+e"
 *    - Example 4: "k-a+e" | "b" -> "k-a+b"
 *    - Example 5: "k-a+e" | "r-u+k" -> "k-a+u"
 * 
 * @param name [i/o] string of phone name to be the base name (will be modified)
 * @param rc [in] right context phone name
 */
void
add_right_context(char name[], char *rc)
{
  char *p;
  int i;

  if ((p = strchr(name, HMM_RC_DLIM_C)) != NULL) {
    p++;
    *p = '\0';
  } else {
    strcat(name, HMM_RC_DLIM);
  }
  i = strlen(name);
  center_name(rc, &(name[i]));
}

/** 
 * @brief  Generate context-dependent phone name from base phone name
 * and its left context phone name.
 *
 * The center phone name of the left context name will be appended to the
 * base phone name.  If the phone name already has left context, it will
 * be renamed to the new one.
 * 
 * @param name [i/o] string of phone name to be the base name (will be modified)
 * @param lc [in] left context phone name
 */
void
add_left_context(char name[], char *lc)
{
  char *p;
  static char buf[MAX_HMMNAME_LEN];

  if ((p = strchr(name, HMM_LC_DLIM_C)) != NULL) {
    p++;
  } else {
    p = name;
  }
  center_name(lc, buf);
  strcat(buf, HMM_LC_DLIM);
  strcat(buf, p);
  strcpy(name, buf);
}

static char gbuf[MAX_HMMNAME_LEN]; ///< Work area for get_{right|left}_context_HMM

/**
 *
 * @brief  Search for right context %HMM in logical %HMM
 *
 * The name of a new right context %HMM, given base phone %HMM and a
 * right context phone string, will be generated, and search it in
 * the list of logical %HMM.
 * If found, return the pointer to the logical %HMM. 
 * 
 * @param base [in] base phone %HMM
 * @param rc_name [in] right context phone name (allow context-dependent name)
 * @param hmminfo [in] HTK %HMM definition data
 * 
 * @return the pointer to the logical %HMM, or NULL if not found.
 */
HMM_Logical *
get_right_context_HMM(HMM_Logical *base, char *rc_name, HTK_HMM_INFO *hmminfo)
{
  strcpy(gbuf, base->name);
  add_right_context(gbuf, rc_name);
  return(htk_hmmdata_lookup_logical(hmminfo, gbuf));
}

/**
 * @brief  Search for left context %HMM in logical %HMM
 *
 * The name of a new left context %HMM, given base phone %HMM and a
 * left context phone string, will be generated, and search it in
 * the list of logical %HMM.
 * If found, return the pointer to the logical %HMM. 
 * 
 * @param base [in] base phone %HMM
 * @param lc_name [in] left context phone name (allow context-dependent name)
 * @param hmminfo [in] HTK %HMM definition data
 * 
 * @return the pointer to the logical %HMM, or NULL if not found.
 */
HMM_Logical *
get_left_context_HMM(HMM_Logical *base, char *lc_name, HTK_HMM_INFO *hmminfo)
{
  strcpy(gbuf, base->name);
  add_left_context(gbuf, lc_name);
  return(htk_hmmdata_lookup_logical(hmminfo, gbuf));
}
  
/** 
 * Extract the center phone name and copy to the specified buffer.
 * 
 * @param hmmname [in] string from which the center phone name will be extracted
 * @param buf [out] the extracted phone name will be written here
 * 
 * @return the argument @a buf.
 */
char *
center_name(char *hmmname, char *buf)
{
  char *p, *s, *d;

  p = hmmname;
  d = buf;

  /* move next to '-' */
  while (*p != HMM_LC_DLIM_C && *p != '\0') p++;
  if (*p == '\0') s = hmmname;
  else s = ++p;

  while (*s != HMM_RC_DLIM_C && *s != '\0') {
    *d = *s;
    d++;
    s++;
  }
  *d = '\0';

  return (buf);
}

/** 
 * Return "left - center" phone name, modifying @a buf.
 * 
 * @param hmmname [in] context-dependent phone name string
 * @param buf [out] resulting phone name
 * 
 * @return the argument @a buf.
 */
char *
leftcenter_name(char *hmmname, char *buf)
{
  char *p;
  /* strip off "+..." */
  strcpy(buf, hmmname);
  if ((p = strchr(buf, HMM_RC_DLIM_C)) != NULL) {
    *p = '\0';
  }
  return(buf);
}

/* return right+center(base) phone name */
/* modify content of buf[] */
/** 
 * Return "center + right" phone name, modifying @a buf.
 * 
 * @param hmmname [in] context-dependent phone name string
 * @param buf [out] resulting phone name
 * 
 * @return the argument @a buf.
 */
char *
rightcenter_name(char *hmmname, char *buf)
{
  char *p;
  /* strip off "...-" */
  if ((p = strchr(hmmname, HMM_LC_DLIM_C)) != NULL && *(p+1) != '\0') {
    strcpy(buf, p+1);
  } else {
    strcpy(buf, hmmname);
  }
  return(buf);
}
