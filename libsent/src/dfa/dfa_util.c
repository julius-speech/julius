/**
 * @file   dfa_util.c
 * 
 * <JA>
 * @brief  文法の情報をテキストで出力
 * </JA>
 * 
 * <EN>
 * @brief  Output text informations about the grammar
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Tue Feb 15 14:18:36 2005
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

#include <sent/stddefs.h>
#include <sent/dfa.h>

/** 
 * Output overall grammar information to stdout.
 *
 * @param fp [in] file pointer
 * @param dinfo [in] DFA grammar
 */
void
print_dfa_info(FILE *fp, DFA_INFO *dinfo)
{
  unsigned long size, allocsize;
  if (fp == NULL) return;
  fprintf(fp, " DFA grammar info:\n");
  fprintf(fp, "      %d nodes, %d arcs, %d terminal(category) symbols\n",
	 dinfo->state_num, dinfo->arc_num, dinfo->term_num);
  
  dfa_cp_count_size(dinfo, &size, &allocsize);
  fprintf(fp, "      category-pair matrix: %ld bytes (%ld bytes allocated)\n", size, allocsize);
}

/** 
 * Output the category-pair matrix in text format to stdout
 * 
 * @param fp [in] file pointer
 * @param dinfo [in] DFA grammar that holds category pair matrix
 */
void
print_dfa_cp(FILE *fp, DFA_INFO *dinfo)
{
  if (fp == NULL) return;
  fprintf(fp, "---------- terminal(category)-pair matrix ----------\n");
  dfa_cp_output_rawdata(fp, dinfo);
}
