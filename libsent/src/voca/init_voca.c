/**
 * @file   init_voca.c
 * 
 * <JA>
 * @brief  単語辞書ファイルをメモリに読み込む
 * </JA>
 * 
 * <EN>
 * @brief  Load a word dictionary into memory
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Fri Feb 18 19:41:12 2005
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
#include <sent/vocabulary.h>

/** 
 * Load and initialize a word dictionary.
 * 
 * @param winfo [out] pointer to a word dictionary data to store the read data
 * @param filename [in] file name of the word dictionary to read
 * @param hmminfo [in] %HMM definition data, needed for triphone conversion.
 * @param not_conv_tri [in] TRUE if not converting monophone to triphone.
 * @param force_dict [in] TRUE if want to ignore the error words in the dictionary
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
init_voca(WORD_INFO *winfo, char *filename, HTK_HMM_INFO *hmminfo, boolean not_conv_tri, boolean force_dict)
{
  FILE *fd;

  if ((fd = fopen_readfile(filename)) == NULL) {
    jlog("Error: init_voca: failed to open %s\n",filename);
    return(FALSE);
  }
  if (!voca_load_htkdict(fd, winfo, hmminfo, not_conv_tri)) {
    if (force_dict) {
      jlog("Warning: init_voca: the word errors are ignored\n");
    } else {
      jlog("Error: init_voca: error in reading %s: %d words failed out of %d words\n",filename, winfo->errnum, winfo->num);
      fclose_readfile(fd);
      return(FALSE);
    }
  }
  if (fclose_readfile(fd) == -1) {
    jlog("Error: init_voca: failed to close\n");
    return(FALSE);
  }

  jlog("Stat: init_voca: read %d words\n", winfo->num);
  return(TRUE);
}

/** 
 * Load and initialize a word list for isolated word recognition.
 * 
 * @param winfo [out] pointer to a word dictionary data to store the read data
 * @param filename [in] file name of the word dictionary to read
 * @param hmminfo [in] %HMM definition data, needed for triphone conversion.
 * @param headphone [in] word head silence phone name
 * @param tailphone [in] word tail silence phone name
 * @param conextphone [in] silence context name at head and tail phoneme
 * @param force_dict [in] TRUE if want to ignore the error words in the dictionary
 * 
 * @return TRUE on success, FALSE on failure.
 */
boolean
init_wordlist(WORD_INFO *winfo, char *filename, HTK_HMM_INFO *hmminfo, char *headphone, char *tailphone, char *contextphone, boolean force_dict)
{
  FILE *fd;

  jlog("Stat: init_wordlist: reading in word list\n");
  if ((fd = fopen_readfile(filename)) == NULL) {
    jlog("Error: init_wordlist: failed to open %s\n",filename);
    return(FALSE);
  }
  if (!voca_load_wordlist(fd, winfo, hmminfo, headphone, tailphone, contextphone)) {
    if (force_dict) {
      jlog("Warning: init_wordlist: the word errors are ignored\n");
    } else {
      jlog("Error: init_wordlist: error in reading %s: %d words failed out of %d words\n",filename, winfo->errnum, winfo->num);
      fclose_readfile(fd);
      return(FALSE);
    }
  }
  if (fclose_readfile(fd) == -1) {
    jlog("Error: init_wordlist: failed to close\n");
    return(FALSE);
  }

  jlog("Stat: init_wordlist: read %d words\n", winfo->num);
  return(TRUE);
}
