#include <sent/stddefs.h>
#include <sent/ngram2.h>
#include <sys/stat.h>
#include <time.h>

#define DEFAULT_NUM 10
static char *bos_default = "<s>";
static char *eos_default = "</s>";
static char *ignore_default = "<UNK>";

#define MAXLEN 500
#define DELIM "+:"

#if defined(_WIN32) && !defined(__CYGWIN32__)
#define srandom srand
#define random rand
#endif

NGRAM_INFO *ngram;
WORD_INFO *dict;
static LOGPROB *findex;
static char buf[4096];

void
usage(char *s)
{
  fprintf(stderr,"%s: generate sentence using N-gram\n", s);
  fprintf(stderr,"usage: %s [options]  bingram\n", s);
  fprintf(stderr,"options:\n");
  fprintf(stderr,"   -n num             num of sentence to generate (10)\n");
  fprintf(stderr,"   -bos string        beginning of sentence word (<s>)\n");
  fprintf(stderr,"   -eos string        end of sentence (</s>)\n");
  fprintf(stderr,"   -ignore string     skip words (<UNK>)\n");
  fprintf(stderr,"   -N N		use N-gram (available max)\n");
  fprintf(stderr,"   -v                 verbose output\n");
  fprintf(stderr,"   -debug             debug output\n");
  exit(1);
}

static int
s1(void *p, void *q)
{
  WORD_ID *a, *b;
  WORD_ID x, y;

  a = p; b = q;
  x = *a; y = *b;
  if (findex[x] > findex[y]) return -1;
  else if (findex[x] < findex[y]) return 1;
  else return 0;
}

int
main(int argc, char *argv[])
{
  char *binfile;
  char *bos_str = NULL, *eos_str = NULL, *ignore_str = NULL;
  int i;
  int n = 0;
  int num = DEFAULT_NUM;
  boolean verbose = FALSE;
  boolean debug = FALSE;
  boolean quiet = FALSE;
  boolean reverse = FALSE;

  /* set random seed */
  srandom(getpid());

  /* option parsing */
   binfile = NULL;
  for(i=1;i<argc;i++) {
    if (argv[i][0] == '-') {
      if (argv[i][1] == 'd') {
	debug = TRUE;
      } else if (argv[i][1] == 'n') {
	if (++i >= argc) usage(argv[0]);
	num = atoi(argv[i]);
      } else if (argv[i][1] == 'b') {
	if (++i >= argc) usage(argv[0]);
	bos_str = argv[i];
      } else if (argv[i][1] == 'e') {
	if (++i >= argc) usage(argv[0]);
	eos_str = argv[i];
      } else if (argv[i][1] == 'i') {
	if (++i >= argc) usage(argv[0]);
	ignore_str = argv[i];
      } else if (argv[i][1] == 'N') {
	if (++i >= argc) usage(argv[0]);
	n = atoi(argv[i]);
      } else if (argv[i][1] == 'v') {
	verbose = TRUE;
      } else if (argv[i][1] == 'q') {
	quiet = TRUE;
      } else {
	usage(argv[0]);
      }
    } else {
      if (binfile == NULL) {
	binfile = argv[i];
      } else {
	usage(argv[0]);
      }
    }
  }
  if (binfile == NULL) {
    usage(argv[0]);
  }
  if (bos_str == NULL) bos_str = bos_default;
  if (eos_str == NULL) eos_str = eos_default;
  if (ignore_str == NULL) ignore_str = ignore_default;

  if (verbose) {
    printf("bingram: %s\n", binfile);
  }

  /* read in N-gram */
  ngram = ngram_info_new();
  /* read in bingram */
  if (init_ngram_bin(ngram, binfile) == FALSE) return -1;

  /* output N-gram statistics */
  print_ngram_info(stdout, ngram);

  if (n == 0) {
    n = ngram->n;
  } else if (ngram->n < n) {
    printf("Error: you requested %d-gram but this is %d-gram\n", n, ngram->n);
    return -1;
  }

  if (ngram->dir == DIR_RL) reverse = TRUE;

  printf("--- sentence generation using %d-gram (%s) ---\n", n,
	 reverse ? "backward" : "forward");

  /* generate */
  {
    int sent;
    WORD_ID w_start, w_end;
    WORD_ID *windex;
    int i, j, ntmp;
    WORD_ID *wlist;
    int len;
    double rnd;
    double fsum;
    char *p;

    windex = (WORD_ID *)mymalloc(sizeof(WORD_ID) * ngram->max_word_num);
    findex = (LOGPROB *)mymalloc(sizeof(LOGPROB) * ngram->max_word_num);

    wlist = (WORD_ID *)mymalloc(sizeof(WORD_ID) * MAXLEN);

    /* first word */
    if ((w_start = ngram_lookup_word(ngram, bos_str)) == WORD_INVALID) {
      printf("Error: word \"%s\" not found as beginning-of-sentence\n", bos_str);
      return -1;
    }
    if (verbose) printf("BOS = %s\n", ngram->wname[w_start]);
    if ((w_end = ngram_lookup_word(ngram, eos_str)) == WORD_INVALID) {
      printf("Error: word \"%s\" not found as end-of-sentence\n", eos_str);
      return -1;
    }
    if (verbose) printf("EOS = %s\n", ngram->wname[w_end]);


    /* main loop */
    for (sent = 0; sent < num; sent++) {

      /* set first word */
      wlist[0] = reverse ? w_end : w_start;
      len = 1;

      /* loop to predict next words */
      while (1) {
	/* create word index and store ngram prob */
	for(i=0;i<ngram->max_word_num;i++) {
	  windex[i] = i;
	  wlist[len] = i;
	  if (len < n - 1) {
	    ntmp = len + 1;
	  } else {
	    ntmp = n;
	  }
	  findex[i] = ngram_prob(ngram, ntmp, &(wlist[len-ntmp+1]));
	}
	if (debug) {
	  if (ntmp > 1) {
	    printf("context=");
	    for(i=0;i<ntmp-1;i++) printf("[%s]", ngram->wname[wlist[len-ntmp+1+i]]);
	    printf("\n");
	  }
	}

	/* sort the index by the ngram prob */
	qsort(windex, ngram->max_word_num, sizeof(WORD_ID), (int (*)(const void *, const void *))s1);
	if (debug) {
	  for(i=0;i<5;i++) {
	    printf(" #%d: %f %s\n", i, findex[windex[i]], ngram->wname[windex[i]]);
	  }
	}
	/* get random number [0..1] */
	rnd = random() / (float) RAND_MAX;
	if (debug) printf("random prob: %f\n", rnd);
	
	/* find next word */
	fsum = 0.0;
	i = 0;
	while (fsum < rnd && i < ngram->max_word_num) {
	  fsum += pow(10, findex[windex[i++]]);
	  //printf("\t%f %f\n", pow(10, findex[windex[i++]]), fsum);
	}
	i--;
	/* if sum of prob. not reached the rnd, assign the least one */
	if (i == ngram->max_word_num) {
	  i = ngram->max_word_num - 1;
	}
	/* if hits word to be ignored, immediate last word will be chosen */
	j = i;
	while (strmatch(ngram->wname[windex[i]], ignore_str) ||
	       (!reverse && strmatch(ngram->wname[windex[i]], bos_str)) ||
	       (reverse && strmatch(ngram->wname[windex[i]], eos_str))) {
	  i--;
	  if (i < 0) break;
	}
	if (i < 0) {
	  i = j;
	  while (strmatch(ngram->wname[windex[i]], ignore_str) ||
		 (!reverse && strmatch(ngram->wname[windex[i]], bos_str)) ||
		 (reverse && strmatch(ngram->wname[windex[i]], eos_str))) {
	    i++;
	    if (i >= ngram->max_word_num) break;
	  }
	  if (i >= ngram->max_word_num) {
	    i = ngram->max_word_num - 1;
	  }
	}

	if (debug) printf("\t%dth/%d hit\n", i+1, ngram->max_word_num);
	if (debug) printf("\t-> [%s]\n", ngram->wname[windex[i]]);
      
	/* store */
	wlist[len++] = windex[i];

	/* length limit */
	if (len + 1 >= MAXLEN) break;
	
	/* end of sentence */
	if (reverse) {
	  //if (windex[i] == w_start) break;
	  if (strmatch(ngram->wname[windex[i]], bos_str)) break;
	} else {
	  //if (windex[i] == w_end) break;
	  if (strmatch(ngram->wname[windex[i]], eos_str)) break;
	}
      }

      /* output */
      for(j=0;j<len;j++) {
	if (reverse) i = len - 1 - j;
	else i = j;
	if (verbose || debug) {
	  printf(" %s", ngram->wname[wlist[i]]);
	} else {
	  strcpy(buf, ngram->wname[wlist[i]]);
	  if ((p = strtok(buf, DELIM)) == NULL) p = &(buf[0]);
	  printf(" %s", p);
	}
      }
      printf("\n");

    }

    free(wlist);
    free(findex);
    free(windex);
  }  

  
  return 0;
}
