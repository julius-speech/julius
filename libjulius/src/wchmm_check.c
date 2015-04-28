/**
 * @file   wchmm_check.c
 * 
 * <JA>
 * @brief  木構造化辞書のマニュアルチェック
 *
 * ここでは，与えられた単語辞書と言語モデルから生成された木構造化辞書の構造を
 * 対話的にチェックするための関数が定義されています. 起動時に "-check wchmm"
 * とすることで，木構造化辞書の構築後にプロンプトが表示され，ある単語が
 * 木構造化辞書のどこに位置するか，あるいはあるノードにどのような情報が
 * 付与されているかなどを調べることができます. 
 * </JA>
 * 
 * <EN>
 * @brief  Manual inspection of tree lexicon
 *
 * This file defines some functions to browse and check the structure
 * of the tree lexicon at startup time. When invoking with "-check wchmm",
 * it will enter to a prompt mode after tree lexicon is generated, and
 * you can check its structure, e.g. how the specified word is located in the
 * tree lexicon, or what kind of information a node has in it.
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Sat Sep 24 15:45:06 2005
 *
 * $Revision: 1.7 $
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
 * <JA>
 * 単語の辞書情報を出力する
 * 
 * @param winfo [in] 単語辞書
 * @param word [in] 出力する単語のID
 * @param ngram_exist [in] 同時に使用する言語制約が存在する場合TRUE
 * </JA>
 * <EN>
 * Display informations of a word in the dictionary.
 * 
 * @param winfo [in] word dictionary
 * @param word [in] ID of a word to be displayed
 * @param ngram_exist [in] TRUE when an N-gram was tied with this winfo
 * </EN>
 */
static void
print_winfo_w(WORD_INFO *winfo, WORD_ID word, boolean ngram_exist)
{
  int i;
  if (word >= winfo->num) return;
  printf("--winfo\n");
  printf("wname   = %s\n",winfo->wname[word]);
  printf("woutput = %s\n",winfo->woutput[word]);
  printf("\ntransp  = %s\n", (winfo->is_transparent[word]) ? "yes" : "no");
  printf("wlen    = %d\n",winfo->wlen[word]);
  printf("wseq    =");
  for (i=0;i<winfo->wlen[word];i++) {
    printf(" %s",winfo->wseq[word][i]->name);
  }
  printf("\nwseq_def=");
  for (i=0;i<winfo->wlen[word];i++) {
    if (winfo->wseq[word][i]->is_pseudo) {
      printf(" (%s)", winfo->wseq[word][i]->body.pseudo->name);
    } else {
      printf(" %s",winfo->wseq[word][i]->body.defined->name);
    }
  }
  if (ngram_exist) {
    printf("\nwton    = %d\n",winfo->wton[word]);
#ifdef CLASS_NGRAM
    printf("cprob   = %f(%f)\n", winfo->cprob[word], pow(10.0, winfo->cprob[word]));
#endif
  }
  
}

/** 
 * <JA>
 * 木構造化辞書上の単語の位置情報を出力する. 
 * 
 * @param wchmm [in] 木構造化辞書
 * @param word [in] 単語ID
 * </JA>
 * <EN>
 * Display the location of a word in the tree lexicon.
 * 
 * @param wchmm [in] tree lexicon
 * @param word [in] word ID
 * </EN>
 */
static void
print_wchmm_w(WCHMM_INFO *wchmm, WORD_ID word)
{
  int i;
  if (word >= wchmm->winfo->num) return;
  printf("--wchmm (word)\n");
  printf("offset  =");
  for (i=0;i<wchmm->winfo->wlen[word];i++) {
    printf(" %d",wchmm->offset[word][i]);
  }
  printf("\n");
  if (wchmm->hmminfo->multipath) {
    printf("wordbegin = %d\n",wchmm->wordbegin[word]);
  }
  printf("wordend = %d\n",wchmm->wordend[word]);
}

/** 
 * <JA>
 * 木構造化辞書上のあるノードの情報を出力する. 
 * 
 * @param wchmm [in] 木構造化辞書
 * @param node [in] ノード番号
 * </JA>
 * <EN>
 * Display informations assigned to a node in the tree lexicon.
 * 
 * @param wchmm [in] tree lexicon
 * @param node [in] node id
 * </EN>
 */
static void
print_wchmm_s(WCHMM_INFO *wchmm, int node)
{
  printf("--wchmm (node)\n");
  printf("stend   = %d\n",wchmm->stend[node]);
  if (wchmm->hmminfo->multipath) {
    if (wchmm->state[node].out.state == NULL) {
      printf("NO OUTPUT\n");
      return;
    }
  }
#ifdef PASS1_IWCD
  printf("outstyle= ");
  switch(wchmm->outstyle[node]) {
  case AS_STATE:
    printf("AS_STATE (id=%d)\n", (wchmm->state[node].out.state)->id);
    break;
  case AS_LSET:
    printf("AS_LSET  (%d variants)\n", (wchmm->state[node].out.lset)->num);
    break;
  case AS_RSET:
    if ((wchmm->state[node].out.rset)->hmm->is_pseudo) {
      printf("AS_RSET  (name=\"%s\", pseudo=\"%s\", loc=%d)\n",
	       (wchmm->state[node].out.rset)->hmm->name,
	       (wchmm->state[node].out.rset)->hmm->body.pseudo->name,
	       (wchmm->state[node].out.rset)->state_loc);
    } else {
      printf("AS_RSET  (name=\"%s\", defined=\"%s\", loc=%d)\n",
	       (wchmm->state[node].out.rset)->hmm->name,
	       (wchmm->state[node].out.rset)->hmm->body.defined->name,
	       (wchmm->state[node].out.rset)->state_loc);
    }
    break;
  case AS_LRSET:
    if ((wchmm->state[node].out.rset)->hmm->is_pseudo) {
      printf("AS_LRSET  (name=\"%s\", pseudo=\"%s\", loc=%d)\n",
	       (wchmm->state[node].out.lrset)->hmm->name,
	       (wchmm->state[node].out.lrset)->hmm->body.pseudo->name,
	       (wchmm->state[node].out.lrset)->state_loc);
    } else {
      printf("AS_LRSET  (name=\"%s\", defined=\"%s\", loc=%d)\n",
	       (wchmm->state[node].out.lrset)->hmm->name,
	       (wchmm->state[node].out.lrset)->hmm->body.defined->name,
	       (wchmm->state[node].out.lrset)->state_loc);
    }
    break;
  default:
    printf("UNKNOWN???\n");
  }
#endif /* PASS1_IWCD */
}

/** 
 * <JA>
 * 木構造化辞書上のあるノードについて，遷移先のリストを出力する. 
 * 
 * @param wchmm [in] 木構造化辞書
 * @param node [in] ノード番号
 * </JA>
 * <EN>
 * Display list of transition arcs from a node in the tree lexicon.
 * 
 * @param wchmm [in] tree lexicon
 * @param node [in] node ID
 * </EN>
 */
static void
print_wchmm_s_arc(WCHMM_INFO *wchmm, int node)
{
  A_CELL2 *ac;
  int i = 0;
  int j;
  printf("arcs:\n");
  if (wchmm->self_a[node] != LOG_ZERO) {
    printf(" %d %f(%f)\n", node, wchmm->self_a[node], pow(10.0, wchmm->self_a[node]));
    i++;
  }
  if (wchmm->next_a[node] != LOG_ZERO) {
    printf(" %d %f(%f)\n", node + 1, wchmm->next_a[node], pow(10.0, wchmm->next_a[node]));
    i++;
  }
  for(ac = wchmm->ac[node]; ac; ac = ac->next) {
    for (j=0;j<ac->n;j++) {
      printf(" %d %f(%f)\n",ac->arc[j],ac->a[j],pow(10.0, ac->a[j]));
      i++;
    }
  }
  printf(" total %d arcs\n",i);
}

/** 
 * <JA>
 * 木構造化辞書上のあるノードの持つ factoring 情報を出力する. 
 * 
 * @param wchmm [in] 木構造化辞書
 * @param node [in] ノード番号
 * </JA>
 * <EN>
 * Display factoring values on a node in the tree lexicon.
 * 
 * @param wchmm [in] tree lexicon
 * @param node [in] node ID
 * </EN>
 */
static void
print_wchmm_s_successor(WCHMM_INFO *wchmm, int node)
{
  int i = 0, j;
  int scid;

  scid = wchmm->state[node].scid;
  if (scid == 0) {
    printf("no successors\n");
  } else if (scid < 0) {
    printf("successor id: %d\n", scid);
#ifdef UNIGRAM_FACTORING
    if (wchmm->lmtype == LM_PROB) {
      printf("1-gram factoring node: score=%f\n",wchmm->fscore[-scid]);
    }
#endif
  } else {
#ifdef UNIGRAM_FACTORING
    printf("successor id: %d\n", scid);
    printf(" %d\n", wchmm->scword[scid]);
#else
    printf("successor id: %d\n", scid);
    for (j = 0; j < wchmm->sclen[scid]; j++) {
      printf(" %d\n", wchmm->sclist[scid][j]);
      i++;
    }
    printf(" total %d successors\n",i);
#endif
  }
}

/** 
 * <JA>
 * 指定された論理名のHMMを検索し，その情報を出力する. 
 * 
 * @param name [in] 論理HMMの名前
 * @param hmminfo [in] HMM定義
 * </JA>
 * <EN>
 * Lookup an HMM of given name, and display specs of it.
 * 
 * @param name [in] HMM logical name
 * @param hmminfo [in] HMM definition
 * </EN>
 */
static void
print_hmminfo(char *name, HTK_HMM_INFO *hmminfo)
{
  HMM_Logical *l;

  l = htk_hmmdata_lookup_logical(hmminfo, name);
  if (l == NULL) {
    printf("no HMM named \"%s\"\n", name);
  } else {
    put_logical_hmm(stdout, l);
  }
}

/** 
 * <JA>
 * 単語N-gramのある単語の情報を出力する. 
 * 
 * @param ngram [in] 単語N-gram
 * @param nid [in] N-gram単語のID
 * </JA>
 * <EN>
 * Display specs of a word in the word N-gram
 * 
 * @param ngram [in] word N-gram
 * @param nid [in] N-gram word ID
 * </EN>
 */
static void
print_ngraminfo(NGRAM_INFO *ngram, int nid)
{
  printf("-- N-gram entry --\n");
  printf("nid  = %d\n", nid);
  printf("name = %s\n", ngram->wname[nid]);
}


/** 
 * <JA>
 * 木構造化辞書の構造を起動時に対話的にチェックする際のコマンドループ
 * 
 * @param wchmm [in] 木構造化辞書
 * </JA>
 * <EN>
 * Command loop to browse and check the structure of the constructed tree
 * lexicon on startup.
 * 
 * @param wchmm [in] tree lexicon
 * </EN>
 * @callgraph
 * @callergraph
 */
void
wchmm_check_interactive(WCHMM_INFO *wchmm) /* interactive check */
{
#define MAXNAMELEN 24
  char buf[MAXNAMELEN], *name;
  int arg, newline;
  WORD_ID argw;
  boolean endflag;

  printf("\n\n");
  printf("********************************************\n");
  printf("********  LM & LEXICON CHECK MODE  *********\n");
  printf("********************************************\n");
  printf("\n");

  for (endflag = FALSE; endflag == FALSE;) {
    printf("===== syntax: command arg (\"H\" for help) > ");
    if (fgets(buf, MAXNAMELEN, stdin) == NULL) break;
    name = "";
    arg = 0;
    if (isalpha(buf[0]) != 0 && buf[1] == ' ') {
      newline = strlen(buf)-1;
      if (buf[newline] == '\n') {
	buf[newline] = '\0';
      }
      if (buf[2] != '\0') {
	name = buf + 2;
	arg = atoi(name);
      }
    }
    switch(buf[0]) {
    case 'w':			/* word info */
      argw = arg;
      print_winfo_w(wchmm->winfo, argw, (wchmm->ngram) ? TRUE : FALSE);
      print_wchmm_w(wchmm, argw);
      break;
    case 'n':			/* node info */
      print_wchmm_s(wchmm, arg);
      break;
    case 'a':			/* arc list */
      print_wchmm_s_arc(wchmm, arg);
      break;
#if 0
    case 'r':			/* reverse arc list */
      print_wchmm_r_arc(arg);
      break;
#endif
    case 's':			/* successor word list */
      if (wchmm->category_tree) {
	printf("Error: this is category tree (no successor list)\n");
      } else {
	print_wchmm_s_successor(wchmm, arg);
      }
      break;
    case 't':			/* node total info of above */
      print_wchmm_s(wchmm, arg);
      print_wchmm_s_arc(wchmm, arg);
#if 0
      print_wchmm_r_arc(arg);
#endif
      if (!wchmm->category_tree) {
	print_wchmm_s_successor(wchmm, arg);
      }
      break;
    case 'h':			/* hmm state info */
      print_hmminfo(name, wchmm->hmminfo);
      break;
    case 'l':			/* N-gram language model info */
      if (wchmm->lmtype == LM_PROB) {
	print_ngraminfo(wchmm->ngram, arg);
      } else {
	printf("Error: this is not an N-gram model\n");
      }
      break;
    case 'q':			/* quit */
      endflag = TRUE;
      break;
    default:			/* help */
      printf("syntax: [command_character] [number(#)]\n");
      printf("  w [word_id] ... show word info\n");
      printf("  n [state]   ... show wchmm state info\n");
      printf("  a [state]   ... show arcs from the state\n");
#if 0
      printf("  r [state]   ... show arcs  to  the state\n");
#endif
      printf("  s [state]   ... show successor list of the state\n");
      printf("  h [hmmname] ... show HMM info of the name\n");
      printf("  l [nwid]    ... N-gram entry info\n");
      printf("  H           ... print this help\n");
      printf("  q           ... quit\n");
      break;
    }
  }
  printf("\n");
  printf("********************************************\n");
  printf("*****  END OF LM & LEXICON CHECK MODE  *****\n");
  printf("********************************************\n");
  printf("\n");
}


/** 
 * <JA>
 * 木構造化辞書内のリンク情報の一貫性をチェックする（内部デバッグ用）
 * 
 * @param wchmm [in] 木構造化辞書
 * </JA>
 * <EN>
 * Check coherence of tree lexicon (for internal debug only!)
 * 
 * @param wchmm [in] tree lexicon
 * </EN>
 * @callgraph
 * @callergraph
 */
void
check_wchmm(WCHMM_INFO *wchmm)
{
  int i;
  boolean ok_flag;
  int node;
  WORD_ID w;

  ok_flag = TRUE;

  if (wchmm->hmminfo->multipath) {
  
    /* check word-beginning nodes */
    for(i=0;i<wchmm->startnum;i++) {
      node = wchmm->startnode[i];
      if (wchmm->state[node].out.state != NULL) {
	printf("Error: word-beginning node %d has output function!\n", node);
	ok_flag = FALSE;
      }
    }
    /* examine if word->state and state->word mapping is correct */
    for(w=0;w<wchmm->winfo->num;w++) {
      if (wchmm->stend[wchmm->wordend[w]] != w) {
	printf("Error: no match of word end for word %d!!\n", w);
	ok_flag = FALSE;
      }
    }
    
  } else {
  
    /* examine if word->state and state->word mapping is correct */
    for (i=0;i<wchmm->winfo->num;i++) {
      if (wchmm->stend[wchmm->wordend[i]]!=i) {
	printf("end ga awanai!!!: word=%d, node=%d, value=%d\n",
	       i, wchmm->wordend[i], wchmm->stend[wchmm->wordend[i]]);
	ok_flag = FALSE;
      }
    }
  }

#if 0
  /* check if the last state is unique and has only one output arc */
  {
    int n;
    A_CELL *ac;

    i = 0;
    for (n=0;n<wchmm->n;n++) {
      if (wchmm->stend[n] != WORD_INVALID) {
	i++;
	for (ac=wchmm->state[n].ac; ac; ac=ac->next) {
	  if (ac->arc == n) continue;
	  if (!wchmm->hmminfo->multipath && wchmm->ststart[ac->arc] != WORD_INVALID) continue;
	  break;
	}
	if (ac != NULL) {
	  printf("node %d is shared?\n",n);
	  ok_flag = FALSE;
	}
      }
    }
    if (i != wchmm->winfo->num ) {
      printf("num of heads of words in wchmm not match word num!!\n");
      printf("from wchmm->stend:%d != from winfo:%d ?\n",i,wchmm->winfo->num);
      ok_flag = FALSE;
    }
  }
#endif

  /* if check failed, go into interactive mode */
  if (!ok_flag) {
    wchmm_check_interactive(wchmm);
  }

  jlog("STAT: coordination check passed\n");
}

/* end of file */
