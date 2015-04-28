/*
 * Copyright (c) 2003-2013 Kawahara Lab., Kyoto University 
 * Copyright (c) 2003-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* mkbinhmmlist --- read in ascii hmmlist file and write in binary format */

/* $Id: mkbinhmmlist.c,v 1.5 2013/06/20 17:14:27 sumomo Exp $ */

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>


HTK_HMM_INFO *hmminfo;
Value para, para_htk;


static void
usage(char *s)
{
  printf("mkbinhmmlist: convert HMMList file to binary format for Julius\n");
  printf("usage: %s hmmdefs hmmlist binhmmlist\n", s);
  printf("\nLibrary configuration: ");
  confout_version(stdout);
  confout_am(stdout);
  printf("\n");
}


int
main(int argc, char *argv[])
{
  FILE *fp;
  char *hmmdefs_file;
  char *hmmlist_file;
  char *outfile;
  int i;

  hmmdefs_file = hmmlist_file = outfile = NULL;
  for(i=1;i<argc;i++) {
    if (hmmdefs_file == NULL) {
      hmmdefs_file = argv[i];
    } else if (hmmlist_file == NULL) {
      hmmlist_file = argv[i];
    } else if (outfile == NULL) {
      outfile = argv[i];
    } else {
      usage(argv[0]);
      return -1;
    }
  }
  if (hmmdefs_file == NULL || hmmlist_file == NULL || outfile == NULL) {
    usage(argv[0]);
    return -1;
  }

  hmminfo = hmminfo_new();

  printf("---- reading hmmdefs ----\n");
  printf("filename: %s\n", hmmdefs_file);

  /* read hmmdef file */
  undef_para(&para);
  if (init_hmminfo(hmminfo, hmmdefs_file, hmmlist_file, &para) == FALSE) {
    fprintf(stderr, "--- terminated\n");
    return -1;
  }

  if (hmminfo->is_triphone) {
    fprintf(stderr, "making pseudo bi/mono-phone for IW-triphone\n");
    if (make_cdset(hmminfo) == FALSE) {
      fprintf(stderr, "ERROR: m_fusion: failed to make context-dependent state set\n");
      return -1;
    }
  }

  printf("\n------------------------------------------------------------\n");
  print_hmmdef_info(stdout, hmminfo);
  printf("\n");

  printf("------------------------------------------------------------\n");

  printf("---- writing logical-to-physical mapping and pseudo phone info ----\n");
  printf("filename: %s\n", outfile);

  if ((fp = fopen_writefile(outfile)) == NULL) {
    fprintf(stderr, "failed to open %s for writing\n", outfile);
    return -1;
  }
  if (save_hmmlist_bin(fp, hmminfo) == FALSE) {
    fprintf(stderr, "failed to write to %s\n", outfile);
    return -1;
  }
  if (fclose_writefile(fp) != 0) {
    fprintf(stderr, "failed to close %s\n", outfile);
    return -1;
  }

  printf("\n");
  printf("binary HMMList and pseudo phone definitions are written to \"%s\"\n", outfile);

  return 0;
}
