/*
 * Copyright (c) 1991-2016 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2016 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

/* binlm2arpa --- convert Julius binary N-gram to ARPA format */

#include <sent/stddefs.h>
#include <sent/ngram2.h>
#include <sys/stat.h>

static NGRAM_INFO *ngram;

void
usage(char *s)
{
  printf("binlm2arpa: convert Julius binary N-gram to ARPA format\n");
  printf("\nUsage: %s infile outfile_prefix\n", s);
  printf("\nLibrary configuration: ");
  confout_version(stdout);
  confout_lm(stdout);
  printf("\n");
}

int
main(int argc, char *argv[])
{
  FILE *fp = NULL;
  FILE *fp2 = NULL;
  char outfile1[512];
  char outfile2[512];
  char *binfile = NULL;

  if (argc < 3) {
    usage(argv[0]);
    return -1;
  }
  binfile = argv[1];

  ngram = ngram_info_new();
  if (init_ngram_bin(ngram, binfile) == FALSE) return -1;
  print_ngram_info(stdout, ngram);

  fprintf(stderr, "----------------\n");

  if (ngram->dir == DIR_RL) {
    snprintf(outfile1, 512, "%s.rev-%dgram.arpa", argv[2], ngram->n);
    printf("writing reverse %d-gram to \"%s\"\n", ngram->n, outfile1);
  } else {
    snprintf(outfile1, 512, "%s.ngram.arpa", argv[2]);
    printf("writing forward %d-gram to \"%s\"\n", ngram->n, outfile1);
  }
  if ((fp = fopen_writefile(outfile1)) == NULL) {
    printf("failed to open \"%s\"\n", outfile1);
    return -1;
  }
  if (ngram->bo_wt_1 != NULL) {
    snprintf(outfile2, 512, "%s.2gram.arpa", argv[2]);
    printf("writing forward 2-gram to \"%s\"\n", outfile2);
    if ((fp2 = fopen_writefile(outfile2)) == NULL) {
      printf("failed to open \"%s\"\n", outfile2);
      return -1;
    }
  }
  if (ngram_write_arpa(ngram, fp, fp2) == FALSE) {/* failed */
    printf("failed to write file\n");
    return -1;
  }

  if (fp2) fclose_writefile(fp2);
  if (fp) fclose_writefile(fp);

  return 0;

}
