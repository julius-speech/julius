/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <sent/stddefs.h>
#include <sent/htk_hmm.h>
#include <sent/htk_param.h>
#include <sent/hmm.h>
#include <sent/hmm_calc.h>

/************************************************************************/
/* .npy file load */

static boolean load_npy(float *array, char *filename, int x, int y)
{
  FILE *fp;
  unsigned char code;
  char magic[6];
  unsigned char major_version;
  unsigned char minor_version;
  unsigned short header_len;
  char *header;
  size_t len;
  boolean fortran_order;
  int i, j;

  if ((fp = fopen_readfile(filename)) == NULL) {
    jlog("Error: load_npy: unable to open: %s\n", filename);
    return FALSE;
  }
  if ((len = myfread(&code, 1, 1, fp)) < 1) {
    jlog("Error: load_npy: failed to read header: %s\n", filename);
    fclose_readfile(fp);
    return FALSE;
  }
  if (code != 0x93) {
    jlog("Error: load_npy: wrong magic number, not an npy file: %s\n", filename);
    return FALSE;
  }
  if ((len = myfread(magic, 1, 5, fp)) < 5) {
    jlog("Error: load_npy: failed to read header: %s\n", filename);
    fclose_readfile(fp);
    return FALSE;
  }
  magic[5] = '\0';
  if (strmatch(magic, "NUMPY") == FALSE) {
    jlog("Error: load_npy: wrong magic header, not an npy file: %s\n", filename);
    return FALSE;
  }
  if ((len = myfread(&major_version, 1, 1, fp)) < 1) {
    jlog("Error: load_npy: failed to read header: %s\n", filename);
    fclose_readfile(fp);
    return FALSE;
  }
  /* we only assume Version 1.x format */
  /* not check subversion x */
  if (major_version != 1) {
    jlog("Error: load_npy: can read only Version 1.0 but this file is Version %d\n", major_version);
    fclose_readfile(fp);
    return FALSE;
  }
  if ((len = myfread(&minor_version, 1, 1, fp)) < 1) {
    jlog("Error: load_npy: failed to read header: %s\n", filename);
    fclose_readfile(fp);
    return FALSE;
  }

  /* currently not support all conversion */
  /* accept only littlen endian 4byte float, with fortran order */
  /* produce error if the file has other format */
  if ((len = myfread(&header_len, 2, 1, fp)) < 1) {
    jlog("Error: load_npy: failed to read header length: %s\n", filename);
    fclose_readfile(fp);
    return FALSE;
  }
#ifdef WORDS_BIGENDIAN
  swap_bytes(&header_len, 2, 1);
#endif  
  header = (char *)mymalloc(header_len + 1);
  if ((len = myfread(header, 1, header_len, fp)) < header_len) {
    jlog("Error: load_npy: failed to read header (%d bytes): %s\n", header_len, filename);
    free(header);
    fclose_readfile(fp);
    return FALSE;
  }
  header[header_len] = '\0';
  if (strstr(header, "'descr': '<f4'") == NULL) {
    jlog("Error: load_npy: not a little-endian float array: %s\n", filename);
    free(header);
    fclose_readfile(fp);
    return FALSE;
  }

  /* fortran order: data are stored per columns */
  /* C order: data are stored per row */
  if (strstr(header, "'fortran_order': True")) {
    fortran_order = TRUE;
  } else {
    fortran_order = FALSE;
  }

  /* all arrays are transposed when used in the original python script, so */
  /* we can assume all data in fortran-order, and read them as is */
  if (fortran_order == FALSE) {
    jlog("Error: load_npy: data array should be in fortran order: %s\n", filename);
    free(header);
    fclose_readfile(fp);
    return FALSE;
  }
    
  char buf[100];
  sprintf(buf, "'shape': (%d, %d)", x, y);
  if (strstr(header, buf) == NULL) {
    jlog("Error: load_npy: not a (%d, %d) array? %s\n", x, y, filename);
    free(header);
    fclose_readfile(fp);
    return FALSE;
  }
  free(header);

#if 1
  /* just read them in the order */
  if ((len = myfread(array, 4, x * y, fp)) < x * y) {
    jlog("Error: load_npy: failed to read %d bytes: %s\n", x * y, filename);
    fclose_readfile(fp);
    return FALSE;
  }
#else
  float *f;
  f = (float *)mymalloc(sizeof(float) * y);
  for (i = 0; i < x; i++) {
    if ((len = myfread(f, 4, y, fp)) < y) {
      jlog("Error: load_npy: failed to read %d bytes: %s\n", y * 4, filename);
      fclose_readfile(fp);
      free(f);
      return FALSE;
    }
    for (j = 0; j < y; j++) {
      array[i * y + j] = f[j];
    }
  }
  free(f);
#endif

  fclose_readfile(fp);
  return TRUE;
}


/************************************************************************/
/* standard logistic function value table: take range x[-6,6] */
/* table size: LOGISTIC_TABLE_FACTOR * 12 * 4 (bytes) */
#define LOGISTIC_TABLE_FACTOR 20000
#define LOGISTIC_TABLE_MAX (12 * LOGISTIC_TABLE_FACTOR)

static float logistic_table[LOGISTIC_TABLE_MAX+1]; /* logistic value table */

/* build logistic function value table */
static void logistic_table_build()
{
  int i;
  double d;
  double x;
  
  for (i = 0; i <= LOGISTIC_TABLE_MAX; i++) {
    x = (double)i / (double)LOGISTIC_TABLE_FACTOR - 6.0;
    d = 1.0 / (1.0 + exp(-x));
    logistic_table[i] = (float)d;
  }
}

/* return logistic function value, consulting table */
static float logistic_func(float x)
{
  if (x <= -6.0f) return 0.0f;
  if (x >=  6.0f) return 1.0f;
  return logistic_table[(int)((x + 6.0f) * LOGISTIC_TABLE_FACTOR)];
}

/* initialize dnn layer */
static void dnn_layer_init(DNNLayer *l)
{
  l->w = NULL;
  l->b = NULL;
  l->in = 0;
  l->out = 0;
}

/* load dnn layer parameter from files */
static boolean dnn_layer_load(DNNLayer *l, int in, int out, char *wfile, char *bfile)
{
  l->in = in;
  l->out = out;
  l->w = (float *)mymalloc(sizeof(float) * l->out * l->in);
  l->b = (float *)mymalloc(sizeof(float) * l->out);
  if (! load_npy(l->w, wfile, l->in, l->out)) return FALSE;
  jlog("Stat: dnn_layer_load: loaded %s\n", wfile);
  if (! load_npy(l->b, bfile, l->out, 1)) return FALSE;
  jlog("Stat: dnn_layer_load: loaded %s\n", bfile);
  return TRUE;
}

/* clear dnn layer */
static void dnn_layer_clear(DNNLayer *l)
{
  if (l->w != NULL) free(l->w);
  if (l->b != NULL) free(l->b);
  dnn_layer_init(l);
}

/*********************************************************************/
DNNData *dnn_new()
{
  DNNData *d;

  d = (DNNData *)mymalloc(sizeof(DNNData));
  memset(d, 0, sizeof(DNNData));

  return d;
}

void dnn_clear(DNNData *dnn)
{
  int i;

  if (dnn->h) {
    for (i = 0; i < dnn->hnum; i++) {
      dnn_layer_clear(&(dnn->h[i]));
    }
    free(dnn->h);
  }
  if (dnn->state_prior) free(dnn->state_prior);
  for (i = 0; i < 2; i++) {
    if (dnn->work[i]) free(dnn->work[i]);
  }
  memset(dnn, 0, sizeof(DNNData));
}

void dnn_free(DNNData *dnn)
{
  dnn_clear(dnn);
  free(dnn);
}


/* initialize dnn */
boolean dnn_setup(DNNData *dnn, int veclen, int contextlen, int inputnodes, int outputnodes, int hiddennodes, int hiddenlayernum, char **wfile, char **bfile, char *output_wfile, char *output_bfile, char *priorfile, float prior_factor, int batchsize)
{
  int i;

  if (dnn == NULL) return FALSE;

  /* clear old data if exist */
  dnn_clear(dnn);

  /* build logistic table */
  logistic_table_build();

  /* set values */
  dnn->batch_size = batchsize;
  dnn->veclen = veclen;
  dnn->contextlen = contextlen;
  dnn->inputnodenum = inputnodes;
  dnn->hiddennodenum = hiddennodes;
  dnn->outputnodenum = outputnodes;
  dnn->prior_factor = prior_factor;

  /* check for input length */
  int inputlen = veclen * contextlen;
  if (inputnodes != inputlen) {
    jlog("Error: dnn_init: veclen(%d) * contextlen(%d) != inputnodes(%d)\n", veclen, contextlen, inputnodes);
    return FALSE;
  }

  jlog("Stat: dnn_init: input: vec %d * context %d = %d dim\n", veclen, contextlen, inputlen);
  jlog("Stat: dnn_init: input layer: %d dim\n", inputnodes);
  jlog("Stat: dnn_init: %d hidden layer(s): %d dim\n", hiddenlayernum, hiddennodes);
  jlog("Stat: dnn_init: output layer: %d dim\n", outputnodes);
  
  /* initialize layers */
  dnn->hnum = hiddenlayernum;
  dnn->h = (DNNLayer *)mymalloc(sizeof(DNNLayer) * dnn->hnum);
  for (i = 0; i < dnn->hnum; i++) {
    dnn_layer_init(&(dnn->h[i]));
  }
  dnn_layer_init(&(dnn->o));

  /* load layer parameters */
  if (dnn_layer_load(&(dnn->h[0]), inputnodes, hiddennodes, wfile[0], bfile[0]) == FALSE) return FALSE;
  for (i = 1; i < dnn->hnum; i++) {
    if (dnn_layer_load(&(dnn->h[i]), hiddennodes, hiddennodes, wfile[i], bfile[i]) == FALSE) return FALSE;
  }
  if (dnn_layer_load(&(dnn->o), hiddennodes, outputnodes, output_wfile, output_bfile) == FALSE) return FALSE;

  /* load state prior */
  {
    FILE *fp;
    int id;
    float val;

    dnn->state_prior_num = outputnodes;
    dnn->state_prior = (float *)mymalloc(sizeof(float) * dnn->state_prior_num);
    for (i = 0; i < dnn->state_prior_num; i++) {
      dnn->state_prior[i] = 0.0f;
    }
    if ((fp = fopen(priorfile, "rb")) == NULL) {
      jlog("Error: cannot open %s\n", priorfile);
      return FALSE;
    }
    while (fscanf(fp, "%d %e", &id, &val) != EOF){
      if (id < 0 || id >= dnn->state_prior_num) {
	jlog("Error: wrong state id in prior file (%d)\n", id);
	fclose_readfile(fp);
	return FALSE;
      }
      dnn->state_prior[id] = val * prior_factor;
    }
    fclose(fp);
    jlog("Stat: dnn_init: state prior loaded: %s\n", priorfile);
  }

  /* allocate work area */
  dnn->work[0] = (float *)mymalloc(sizeof(float) * dnn->hiddennodenum);
  dnn->work[1] = (float *)mymalloc(sizeof(float) * dnn->hiddennodenum);

  return TRUE;
}

#if 0
void dnn_ff(DNNData *dnn, float *in, float *out_ret)
{
  int n;
  float *src, *dst;

  /* feed forward by standard logistic function */
  src = in;
  n = 0;
  for (int i = 0; i < dnn->hnum; i++) {
    dnn->work[n] = logistic_func(dnn->h[i].w * src + dnn->h[i].b);
    src = dnn->work[n];
    if (++n > 1) n = 0;
  }
  out_ret = dnn->o.w * src + dnn->o.b;
  /* do soft max */
#if 0
  /* log10( (exp(x)/sum(exp(x))) / state_prior) */
  out_ret = exp(out_ret);
  allsum = sum(out_ret);
  out_ret /= allsum;
  out_ret /= dnn->state_prior;
  out_ret = log10(out_ret);
#else
  /* INV_LOG_TEN * (x - addlogarray(x) - log(state_prior)) */
#endif
}

#endif

/* compute outprob by DNN for the current frame and store them to current frame state outprob cache */
boolean dnn_calc_outprob(HMMWork *wrk)
{
  /* frame = wrk->OP_time */
  /* param = wrk->OP_param */
  /* input vector = wrk->OP_param[wrk->OP_time][] */
  /* store state outprob to wrk->last_cache[]  */

  printf("%d\n", wrk->OP_time);

  {
    int s;
    for (s = 0; s < wrk->statenum; s++) {
      wrk->last_cache[s] = 0.0f;
    }
  }
}

