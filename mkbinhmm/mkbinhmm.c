/*
 * Copyright (c) 2003-2013 Kawahara Lab., Kyoto University 
 * Copyright (c) 2003-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* mkbinhmm --- read in ascii hmmdefs file and write in binary format */

/* $Id: mkbinhmm.c,v 1.7 2013/06/20 17:14:27 sumomo Exp $ */

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>


HTK_HMM_INFO *hmminfo;
Value para, para_htk;


static void
usage(char *s)
{
  printf("mkbinhmm: convert HMM definition file to binary format for Julius\n");
  printf("usage: %s [-htkconf HTKConfig] hmmdefs binhmm\n", s);
  printf("\nLibrary configuration: ");
  confout_version(stdout);
  confout_am(stdout);
  printf("\n");
}


int
main(int argc, char *argv[])
{
  FILE *fp;
  char *infile;
  char *outfile;
  char *conffile;
  int i;

  infile = outfile = conffile = NULL;
  for(i=1;i<argc;i++) {
    if (strmatch(argv[i], "-C") || strmatch(argv[i], "-htkconf")) {
      if (++i >= argc) {
	usage(argv[0]);
	return -1;
      }
      conffile = argv[i];
    } else {
      if (infile == NULL) {
	infile = argv[i];
      } else if (outfile == NULL) {
	outfile = argv[i];
      } else {
	usage(argv[0]);
	return -1;
      }
    }
  }
  if (infile == NULL || outfile == NULL) {
    usage(argv[0]);
    return -1;
  }

  hmminfo = hmminfo_new();

  printf("---- reading hmmdefs ----\n");
  printf("filename: %s\n", infile);

  /* read hmmdef file */
  undef_para(&para);
  if (init_hmminfo(hmminfo, infile, NULL, &para) == FALSE) {
    fprintf(stderr, "--- terminated\n");
    return -1;
  }

  if (conffile != NULL) {
    /* if input HMMDEFS already has embedded parameter
       they will be overridden by the parameters in the config file */
    printf("\n---- reading HTK Config ----\n");
    if (para.loaded == 1) {
      printf("Warning: input hmmdefs has acoustic analysis parameter information\n");
      printf("Warning: they are overridden by the HTK Config file...\n");
    }
    /* load HTK config file */
    undef_para(&para);
    if (htk_config_file_parse(conffile, &para) == FALSE) {
      fprintf(stderr, "Error: failed to read %s\n", conffile);
      return(-1);
    }
    /* set some parameters from HTK HMM header information */
    printf("\nsetting TARGETKIND and NUMCEPS from HMM definition header...");
    calc_para_from_header(&para, hmminfo->opt.param_type, hmminfo->opt.vec_size);
    printf("done\n");
    /* fulfill unspecified values with HTK defaults */
    printf("fulfill unspecified values with HTK defaults...");
    undef_para(&para_htk);
    make_default_para_htk(&para_htk);
    apply_para(&para, &para_htk);
    printf("done\n");
  }

  printf("\n------------------------------------------------------------\n");
  print_hmmdef_info(stdout, hmminfo);
  printf("\n");

  if (para.loaded == 1) {
    put_para(stdout, &para);
  }
  printf("------------------------------------------------------------\n");

  printf("---- writing ----\n");
  printf("filename: %s\n", outfile);
  
  if ((fp = fopen_writefile(outfile)) == NULL) {
    fprintf(stderr, "failed to open %s for writing\n", outfile);
    return -1;
  }
  if (write_binhmm(fp, hmminfo, (para.loaded == 1) ? &para : NULL) == FALSE) {
    fprintf(stderr, "failed to write to %s\n", outfile);
    return -1;
  }
  if (fclose_writefile(fp) != 0) {
    fprintf(stderr, "failed to close %s\n", outfile);
    return -1;
  }

  printf("\n");
  if (para.loaded == 1) {
    printf("binary HMM written to \"%s\", with acoustic parameters embedded for Julius.\n", outfile);
  } else {
    printf("binary HMM written to \"%s\"\n", outfile);
  }

  return 0;
}
