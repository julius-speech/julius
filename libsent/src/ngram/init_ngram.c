/**
 * @file   init_ngram.c
 * 
 * <JA>
 * @brief  N-gramファイルをメモリに読み込み単語辞書と対応を取る
 * </JA>
 * 
 * <EN>
 * @brief  Load N-gram file into memory and setup with word dictionary
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Wed Feb 16 07:40:53 2005
 *
 * $Revision: 1.10 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/ngram2.h>
#include <sent/vocabulary.h>

/** 
 * Read and setup N-gram data from binary format file.
 * 
 * @param ndata [out] pointer to N-gram data structure to store the data
 * @param bin_ngram_file [in] file name of the binary N-gram
 */
boolean
init_ngram_bin(NGRAM_INFO *ndata, char *bin_ngram_file)
{
  FILE *fp;
  
  jlog("Stat: init_ngram: reading in binary n-gram from %s\n", bin_ngram_file);
  if ((fp = fopen_readfile(bin_ngram_file)) == NULL) {
    jlog("Error: init_ngram: failed to open \"%s\"\n", bin_ngram_file);
    return FALSE;
  }
  if (ngram_read_bin(fp, ndata) == FALSE) {
    jlog("Error: init_ngram: failed to read \"%s\"\n", bin_ngram_file);
    return FALSE;
  }
  if (fclose_readfile(fp) == -1) {
    jlog("Error: init_ngram: failed to close \"%s\"\n", bin_ngram_file);
    return FALSE;
  }

  /* set default unknown (=OOV) word id */
  set_default_unknown_id(ndata);

  jlog("Stat: init_ngram: finished reading n-gram\n");
  return TRUE;
}

/** 
 * Read and setup N-gram data from ARPA format file.
 * 
 * @param ndata [out] pointer to N-gram data structure to store the data
 * @param ngram_file [in] file name of ARPA (reverse) 3-gram file
 * @param dir [in] direction (DIR_LR | DIR_RL)
 */
boolean
init_ngram_arpa(NGRAM_INFO *ndata, char *ngram_file, int dir)
{
  FILE *fp;

  ndata->root = NULL;
  ndata->dir = dir;

  jlog("Stat: init_ngram: reading in ARPA %s n-gram from %s\n", (ndata->dir == DIR_LR) ? "forward" : "backward", ngram_file);
  /* read RL n-gram */
  if ((fp = fopen_readfile(ngram_file)) == NULL) {
    jlog("Error: init_ngram: failed to open \"%s\"\n", ngram_file);
    return FALSE;
  }
  if (ngram_read_arpa(fp, ndata, FALSE) == FALSE) {
    jlog("Error: init_ngram: failed to read \"%s\"\n", ngram_file);
    return FALSE;
  }
  if (fclose_readfile(fp) == -1) {
    jlog("Error: init_ngram: failed to close \"%s\"\n", ngram_file);
    return FALSE;
  }

  /* set default unknown (=OOV) word id */
  set_default_unknown_id(ndata);

  jlog("Stat: init_ngram: finished reading n-gram\n");
  return TRUE;
}

/** 
 * Read additional LR 2-gram for 1st pass.
 * 
 * @param ndata [out] pointer to N-gram data structure to store the data
 * @param bigram_file [in] file name of ARPA 2-gram file
 */
boolean
init_ngram_arpa_additional(NGRAM_INFO *ndata, char *bigram_file)
{
  FILE *fp;

  jlog("Stat: init_ngram: reading in additional LR 2-gram for the 1st pass from %s\n", bigram_file);
  if ((fp = fopen_readfile(bigram_file)) == NULL) {
    jlog("Error: init_ngram: failed to open \"%s\"\n", bigram_file);
    return FALSE;
  }
  if (ngram_read_arpa(fp, ndata, TRUE) == FALSE) {
    jlog("Error: init_ngram: failed to read \"%s\"\n", bigram_file);
    return FALSE;
  }
  if (fclose_readfile(fp) == -1) {
    jlog("Error: init_ngram: failed to close \"%s\"\n", bigram_file);
    return FALSE;
  }
  jlog("Stat: init_ngram: finished reading LR 2-gram\n");

  return TRUE;
}

/** 
 * Make correspondence between word dictionary and N-gram vocabulary.
 * 
 * @param ndata [i/o] word/class N-gram, the unknown word information will be set.
 * @param winfo [i/o] word dictionary, the word-to-ngram-entry mapping will be done here.
 */
boolean
make_voca_ref(NGRAM_INFO *ndata, WORD_INFO *winfo)
{
  int i;
  boolean ok_flag = TRUE;
  int count = 0;

  jlog("Stat: init_ngram: mapping dictonary words to n-gram entries\n");
  ndata->unk_num = 0;
  for (i = 0; i < winfo->num; i++) {
    winfo->wton[i] = make_ngram_ref(ndata, winfo->wname[i]);
    if (winfo->wton[i] == WORD_INVALID) {
      ok_flag = FALSE;
      count++;
      continue;
    }
    if (winfo->wton[i] == ndata->unk_id) {
      (ndata->unk_num)++;
    }
  }
  if (ok_flag == FALSE) {
    jlog("Error: --- Failed to map %d words in dictionary to N-gram\n", count);
    jlog("Error: --- Specify the word to which those words are mapped with \"-mapunk\" (default: \"<unk>\" or \"<UNK>\"\n");
    return FALSE;
  }
      
  if (ndata->unk_num == 0) {
    ndata->unk_num_log = 0.0;	/* for safe */
  } else {
    ndata->unk_num_log = (float)log10(ndata->unk_num);
  }
  jlog("Stat: init_ngram: finished word-to-ngram mapping\n");
  return TRUE;
}

/** 
 * @brief  Set default unknown word ID to the N-gram data.
 * If default "<unk>" is not found, also try "<UNK>".
 * 
 * @param ndata [out] N-gram data to set unknown word ID.
 */
void
set_default_unknown_id(NGRAM_INFO *ndata)
{
  ndata->unk_id = ngram_lookup_word(ndata, UNK_WORD_DEFAULT);
  if (ndata->unk_id != WORD_INVALID) {
    jlog("Stat: init_ngram: found unknown word entry \"%s\"\n", UNK_WORD_DEFAULT);
    ndata->isopen = TRUE;
  } else {
    ndata->unk_id = ngram_lookup_word(ndata, UNK_WORD_DEFAULT2);
    if (ndata->unk_id != WORD_INVALID) {
      jlog("Stat: init_ngram: found unknown word entry \"%s\"\n", UNK_WORD_DEFAULT2);
      ndata->isopen = TRUE;
    } else{
      jlog("Stat: init_ngram: neither \"%s\" nor \"%s\" was found, assuming close vocabulary LM\n", UNK_WORD_DEFAULT, UNK_WORD_DEFAULT2);
      ndata->isopen = FALSE;
    }
  }
  ndata->unk_num = 0;
}

/** 
 * @brief  Set user-specified word ID to the N-gram data.
 *
 * @param ndata [out] N-gram data to set unknown word ID.
 * @param str [in] word name string of unknown word
 */
void
set_unknown_id(NGRAM_INFO *ndata, char *str)
{
  WORD_ID w;
  w = ngram_lookup_word(ndata, str);
  if (w == WORD_INVALID) {
    jlog("Stat: init_ngram: \"%s\" not found", str);
  } else {
    jlog("Stat: init_ngram: unknown word entry was set to \"%s\"\n", str);
    ndata->unk_id = w;
    ndata->isopen = TRUE;
  }
}

/** 
 * @brief  Fix unigram probability of BOS / EOS word.
 *
 * This function checks the probabilities of BOS / EOS word, and
 * if it is set to "-99", give the same as another one.
 * This is the case when the LM is trained by SRILM, which assigns
 * unigram probability of "-99" to the beginning-of-sentence word,
 * and causes search on reverse direction to fail.
 * 
 * @param ndata [i/o] N-gram data
 * @param winfo [i/o] Vocabulary information
 * 
 */
void
fix_uniprob_srilm(NGRAM_INFO *ndata, WORD_INFO *winfo)
{
  WORD_ID wb, we;

  wb = winfo->wton[winfo->head_silwid];
  we = winfo->wton[winfo->tail_silwid];
  if (ndata->d[0].prob[wb] == -99.0) {
    jlog("Warning: BOS word \"%s\" has unigram prob of \"-99\"\n", ndata->wname[wb]);
    jlog("Warning: assigining value of EOS word \"%s\": %f\n", ndata->wname[we], ndata->d[0].prob[we]);
    ndata->d[0].prob[wb] = ndata->d[0].prob[we];
  } else if (ndata->d[0].prob[we] == -99.0) {
    jlog("Warning: EOS word \"%s\" has unigram prob of \"-99\"\n", ndata->wname[we]);
    jlog("Warning: assigining value of BOS word \"%s\": %f\n", ndata->wname[wb], ndata->d[0].prob[wb]);
    ndata->d[0].prob[we] = ndata->d[0].prob[wb];
  }
}
  
