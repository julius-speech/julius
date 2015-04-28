/**
 * @file   plugin.c
 * 
 * <EN>
 * @brief  Load plugin
 * </EN>
 * 
 * <JA>
 * @brief  プラグイン読み込み
 * </JA>
 * 
 * @author Akinobu Lee
 * @date   Sat Aug  2 09:46:09 2008
 * 
 * $Revision: 1.11 $
 * 
 */
/*
 * Copyright (c) 1991-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <julius/julius.h>

#ifdef ENABLE_PLUGIN

#if defined(_WIN32) && !defined(__CYGWIN32__) && !defined(__MINGW32__)
#include <windows.h>
#else
#include <dirent.h>
#endif
#include <stdarg.h>

/**
 * Plugin file path suffix
 * 
 */
static char *plugin_suffix = PLUGIN_SUFFIX;

/**
 * Function names to be loaded
 * 
 */
static char *plugin_function_namelist[] = PLUGIN_FUNCTION_NAMELIST;


/**************************************************************/

#if defined(_WIN32) && !defined(__CYGWIN32__)
/** 
 * Return error string.
 * 
 * @return the error string.
 */
static const char* dlerror()
{
  static char szMsgBuf[256];
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		szMsgBuf,
		sizeof szMsgBuf,
		NULL);
  return szMsgBuf;
}
#endif

/**************************************************************/
static int
plugin_namelist_num()
{
  return(sizeof(plugin_function_namelist) / sizeof(char *));
}

static void
plugin_free_all()
{
  PLUGIN_ENTRY *p, *ptmp;
  int i, num;

  if (global_plugin_list == NULL) return;

  num = plugin_namelist_num();
  for(i=0;i<num;i++) {
    p = global_plugin_list[i];
    while(p) {
      ptmp = p->next;
      free(p);
      p = ptmp;
    }
  }
  free(global_plugin_list);
  global_plugin_list = NULL;
}    


int
plugin_get_id(char *name)
{
  int i, num;

  num = plugin_namelist_num();
  for(i=0;i<num;i++) {
    if (strmatch(plugin_function_namelist[i], name)) {
      return i;
    }
  }
  jlog("InternalError: no plugin entry named %s\n", name);
  return -1;
}

void
plugin_init()
{
  int i, num;

  if (global_plugin_list != NULL) {
    plugin_free_all();
  }
  num = plugin_namelist_num();
  global_plugin_list = (PLUGIN_ENTRY **)mymalloc(sizeof(PLUGIN_ENTRY *) * num);
  for(i=0;i<num;i++) {
    global_plugin_list[i] = NULL;
  }
  global_plugin_loaded_file_num = 0;
}

/**************************************************************/
/** 
 * Guess if it is a file name of julius plugin
 * 
 * @param filename [in] file name
 * 
 * @return TRUE if it has suffix of julius plugin, else return FALSE.
 */
static boolean
is_plugin_obj(char *filename)
{
  char *p, *x;
  x = plugin_suffix + strlen(plugin_suffix) - 1;
  p = filename + strlen(filename) - 1;

  while (x >= plugin_suffix && p >= filename && *x == *p) {
    x--; p--;
  }
  if (x < plugin_suffix) {
    return TRUE;
  }

  return FALSE;
}

/** 
 * Load a plugin file.
 * 
 * @param file [in] plugin file path
 *
 * @return TRUE on success, FALSE on failure.
 */
boolean
plugin_load_file(char *file)
{
  PLUGIN_MODULE handle;
  FUNC_INT func;
  FUNC_VOID entfunc;
  int ret, number, num;
  char buf[256];
  int buflen = 256;
  PLUGIN_ENTRY *p;
  int i;

  if (global_plugin_list == NULL) plugin_init();

  /* open file */
  handle = dlopen(file, RTLD_LAZY);
  if (!handle) {
    jlog("ERROR: plugin_load: failed to open: %s\n", dlerror());
    return(FALSE);
  }

  /* call initialization function */
  func = dlsym(handle, "initialize");
  if (func) {
    ret = (*func)();
    if (ret == -1) {
      jlog("WARNING: plugin_load: %s: initialize() returns no, skip this file\n", file);
      dlclose(handle);
      return(FALSE);
    }
  }

  /* call information function */
  func = dlsym(handle, "get_plugin_info");
  if (func == NULL) {
    jlog("ERROR: plugin_load: %s: get_plugin_info(): %s\n", file, dlerror());
    dlclose(handle);
    return(FALSE);
  }
  number = 0;
  ret = (*func)(number, buf, buflen);
  if (ret == -1) {
    jlog("ERROR: plugin_load: %s: get_plugin_info(0) returns error\n", file);
    dlclose(handle);
    return(FALSE);
  }
  buf[buflen-1] = '\0';
  jlog("#%d [%s]\n", global_plugin_loaded_file_num, buf);
  
  /* register plugin functions */
  num = plugin_namelist_num();
  for(i=0;i<num;i++) {
    entfunc = dlsym(handle, plugin_function_namelist[i]);
    if (entfunc) {
      if (debug2_flag) {
	jlog("     (%s)\n", plugin_function_namelist[i]);
      }
      p = (PLUGIN_ENTRY *)mymalloc(sizeof(PLUGIN_ENTRY));
      p->id = i;
      p->source_id = global_plugin_loaded_file_num;
      p->func = entfunc;
      p->next = global_plugin_list[i];
      global_plugin_list[i] = p;
    }
  }

  /* increment file counter */
  global_plugin_loaded_file_num++;

  return(TRUE);
}

/** 
 * Search for plugin file in a directory and load them.
 * 
 * @param dir [in] directory
 *
 * @return TRUE on success, FALSE on failure
 */
boolean
plugin_load_dir(char *dir)
{
#if defined(_WIN32) && !defined(__CYGWIN32__) && !defined(__MINGW32__)

  WIN32_FIND_DATA FindFileData;
  HANDLE hFind;
  static char buf[512];
  int cnt;

  strncpy(buf, dir, 505);
  strcat(buf, "\\*.dll");
  if ((hFind = FindFirstFile(buf, &FindFileData)) == INVALID_HANDLE_VALUE) {
    jlog("ERROR: plugin_load: cannot open plugins dir \"%s\"\n", dir);
    return FALSE;
  }

  cnt = 0;
  do {
    jlog("STAT: file: %-23s ", FindFileData.cFileName);
    sprintf_s(buf, 512, "%s\\%s", dir, FindFileData.cFileName);
    if (plugin_load_file(buf)) cnt++;
  } while (FindNextFile(hFind, &FindFileData));

  FindClose(hFind);
  jlog("STAT: %d files loaded\n", cnt);

  return TRUE;
  
#else
  
  DIR *d;
  struct dirent *f;
  static char buf[512];
  int cnt;

  if ((d = opendir(dir)) == NULL) {
    jlog("ERROR: plugin_load: cannot open plugins dir \"%s\"\n", dir);
    return FALSE;
  }
  cnt = 0;
  while((f = readdir(d)) != NULL) {
    if (is_plugin_obj(f->d_name)) {
      snprintf(buf, 512, "%s/%s", dir, f->d_name);
      jlog("STAT: file: %-23s ", f->d_name);
      if (plugin_load_file(buf)) cnt++;
    }
  }
  closedir(d);
  jlog("STAT: %d files loaded\n", cnt);

  return TRUE;

#endif
}

/** 
 * read in plugins in multiple directories
 * 
 * @param dirent [i/o] directory entry in form of
 * "dir1:dir2:dir3:...".
 *
 */
void
plugin_load_dirs(char *dirent)
{
  char *p, *s;
  char c;

  if (dirent == NULL) return;

  if (debug2_flag) {
    jlog("DEBUG: loading dirs: %s\n", dirent);
  }

  p = dirent;
  do {
    s = p;
    while(*p != '\0' && *p != ':') p++;
    c = *p;
    *p = '\0';
    jlog("STAT: loading plugins at \"%s\":\n", dirent);
    plugin_load_dir(s);
    if (c != '\0') {
      *p = c;
      p++;
    }
  } while (*p != '\0');
}


/************************************************************************/

int
plugin_find_optname(char *optfuncname, char *str)
{
  char buf[64];
  int id;
  PLUGIN_ENTRY *p;
  FUNC_VOID func;

  if (global_plugin_list == NULL) return -1;

  if ((id = plugin_get_id(optfuncname)) < 0) return -1;
  for(p=global_plugin_list[id];p;p=p->next) {
    func = (FUNC_VOID) p->func;
    (*func)(buf, (int)64);
    if (strmatch(buf, str)) {
      return p->source_id;
    }
  }
  return -1;
}

FUNC_VOID
plugin_get_func(int sid, char *name)
{
  int id;
  PLUGIN_ENTRY *p;
  FUNC_VOID func;

  if (global_plugin_list == NULL) return NULL;

  if ((id = plugin_get_id(name)) < 0) return NULL;

  for(p=global_plugin_list[id];p;p=p->next) {
    if (p->source_id == sid) return p->func;
  }
  return NULL;
}

/************************************************************************/
boolean
plugin_exec_engine_startup(Recog *recog)
{
  int id;
  PLUGIN_ENTRY *p;
  FUNC_INT func;
  boolean ok_p;

  if (global_plugin_list == NULL) return TRUE;

  if ((id = plugin_get_id("startup")) < 0) return FALSE;

  ok_p = TRUE;
  for(p=global_plugin_list[id];p;p=p->next) {
    func = (FUNC_INT) p->func;
    if ((*func)(recog) != 0) {
      jlog("WARNING: plugin #%d: failed in startup()\n", p->source_id);
      ok_p = FALSE;
    }
  }

  return ok_p;
}


/************************************************************************/
void
plugin_exec_adin_captured(short *buf, int len)
{
  int id;
  PLUGIN_ENTRY *p;
  FUNC_VOID adfunc;

  if (global_plugin_list == NULL) return;

  if ((id = plugin_get_id("adin_postprocess")) < 0) return;
  for(p=global_plugin_list[id];p;p=p->next) {
    adfunc = (FUNC_VOID) p->func;
    (*adfunc)(buf, len);
  }
}

void
plugin_exec_adin_triggered(short *buf, int len)
{
  int id;
  PLUGIN_ENTRY *p;
  FUNC_VOID adfunc;

  if (global_plugin_list == NULL) return;

  if ((id = plugin_get_id("adin_postprocess_triggered")) < 0) return;
  for(p=global_plugin_list[id];p;p=p->next) {
    adfunc = (FUNC_VOID) p->func;
    (*adfunc)(buf, len);
  }
}

void
plugin_exec_vector_postprocess(VECT *vecbuf, int veclen, int nframe)
{
  int id;
  PLUGIN_ENTRY *p;
  FUNC_INT func;

  if (global_plugin_list == NULL) return;

  if ((id = plugin_get_id("fvin_postprocess")) < 0) return;
  for(p=global_plugin_list[id];p;p=p->next) {
    func = (FUNC_INT) p->func;
    (*func)(vecbuf, veclen, nframe);
  }
}
void
plugin_exec_vector_postprocess_all(HTK_Param *param)
{
  int id;
  PLUGIN_ENTRY *p;
  FUNC_INT func;
  int t;

  if (global_plugin_list == NULL) return;

  if ((id = plugin_get_id("fvin_postprocess")) < 0) return;
  for(t=0;t<param->samplenum;t++) {
    for(p=global_plugin_list[id];p;p=p->next) {
      func = (FUNC_INT) p->func;
      (*func)(param->parvec[t], param->veclen, t);
    }
  }
}

void
plugin_exec_process_result(Recog *recog)
{
  int id;
  PLUGIN_ENTRY *p;
  FUNC_VOID func;

  RecogProcess *rtmp, *r;
  Sentence *s;
  int i;
  int len;
  char *str;

  if (global_plugin_list == NULL) return;

  /* for result_str(), return the best sentence string among processes */
  s = NULL;
  for(rtmp=recog->process_list;rtmp;rtmp=rtmp->next) {
    if (! rtmp->live) continue;
    if (rtmp->result.status >= 0 && rtmp->result.sentnum > 0) { /* recognition succeeded */
      if (s == NULL || rtmp->result.sent[0].score > s->score) {
	r = rtmp;
	s = &(r->result.sent[0]);
      }
    }
  }
  if (s == NULL) {
    str = NULL;
  } else {
    len = 0;
    for(i=0;i<s->word_num;i++) len += strlen(r->lm->winfo->woutput[s->word[i]]) + 1;
    str = (char *)mymalloc(len);
    str[0]='\0';
    for(i=0;i<s->word_num;i++) {
      if (strlen(r->lm->winfo->woutput[s->word[i]]) == 0) continue;
      if (strlen(str) > 0) strcat(str, " ");
      strcat(str, r->lm->winfo->woutput[s->word[i]]);
    }
  }

  if ((id = plugin_get_id("result_best_str")) < 0) return;
  for(p=global_plugin_list[id];p;p=p->next) {
    func = (FUNC_VOID) p->func;
    (*func)(str);
  }

  if (str != NULL) free(str);
}


#endif /* ENABLE_PLUGIN */

/************************************************************************/
/* assume only one MFCC module! */

/************************************************************************/

boolean
mfc_module_init(MFCCCalc *mfcc, Recog *recog)
{
  /* assign default functions */
  mfcc->func.fv_standby    = (boolean (*)()) vecin_standby;
  mfcc->func.fv_begin      = (boolean (*)()) vecin_open;
  mfcc->func.fv_read       = (int (*)(VECT *, int)) vecin_read;
  mfcc->func.fv_end        = (boolean (*)()) vecin_close;
  mfcc->func.fv_resume     = (boolean (*)()) vecin_resume;
  mfcc->func.fv_pause      = (boolean (*)()) vecin_pause;
  mfcc->func.fv_terminate  = (boolean (*)()) vecin_terminate;
  mfcc->func.fv_input_name = (char * (*)()) vecin_input_name;

#ifdef ENABLE_PLUGIN
  mfcc->plugin_source = recog->jconf->input.plugin_source;
  if (mfcc->plugin_source < 0) {
    /* no plugin, use the default functions */
    return TRUE;
  }
  mfcc->func.fv_standby  = (boolean (*)()) plugin_get_func(mfcc->plugin_source, "fvin_standby");
  mfcc->func.fv_begin    = (boolean (*)()) plugin_get_func(mfcc->plugin_source, "fvin_open");
  mfcc->func.fv_read 	   = (int (*)(VECT *, int)) plugin_get_func(mfcc->plugin_source, "fvin_read");
  mfcc->func.fv_end 	   = (boolean (*)()) plugin_get_func(mfcc->plugin_source, "fvin_close");
  mfcc->func.fv_resume   = (boolean (*)()) plugin_get_func(mfcc->plugin_source, "fvin_resume");
  mfcc->func.fv_pause    = (boolean (*)()) plugin_get_func(mfcc->plugin_source, "fvin_pause");
  mfcc->func.fv_terminate= (boolean (*)()) plugin_get_func(mfcc->plugin_source, "fvin_terminate");
  mfcc->func.fv_input_name= (char * (*)()) plugin_get_func(mfcc->plugin_source, "fvin_input_name");

  if (mfcc->func.fv_read == NULL) {
    jlog("ERROR: FEATURE_INPUT: fvin_read() not found!\n");
    return FALSE;
  }
#endif

  return TRUE;
}

boolean
mfc_module_set_header(MFCCCalc *mfcc, Recog *recog)
{
  FUNC_INT func;
  unsigned int ret;

#ifdef ENABLE_PLUGIN
  if (mfcc->plugin_source < 0) {
    /* no plugin, use the default functions */
    func = vecin_get_configuration;
  } else {
    func = (FUNC_INT) plugin_get_func(mfcc->plugin_source, "fvin_get_configuration");
    if (func == NULL) {
      jlog("ERROR: feature vector input: fvin_get_configuration() not found\n");
      return FALSE;
    }
  }
#else
  func = vecin_get_configuration;
#endif

  /* vector length in unit */
  mfcc->param->veclen = (*func)(0);
  mfcc->param->header.sampsize = mfcc->param->veclen * sizeof(VECT);
  /* frame shift in msec */
  mfcc->param->header.wshift = (*func)(1) * 10000.0;
  /* parameter type for checking (return 0xffff to disable the check) */
  ret = (*func)(2);
  if (ret == 0xffff) {
    /* disable type checking */
    recog->jconf->input.paramtype_check_flag = FALSE;
  } else {
    mfcc->param->header.samptype = ret;
  }

  /* switch if the input vector is feature vector or outprob vector */
  mfcc->param->is_outprob = ( (*func)(3) > 0 ) ? TRUE : FALSE;

  return TRUE;
}

boolean
mfc_module_standby(MFCCCalc *mfcc)
{
  FUNC_INT func;
  int ret;

  if (mfcc->func.fv_standby) ret = mfcc->func.fv_standby();
  else ret = TRUE;
  mfcc->segmented_by_input = FALSE;
  return ret;
}

boolean
mfc_module_begin(MFCCCalc *mfcc)
{
  FUNC_INT func;
  int ret;

  if (mfcc->segmented_by_input) return TRUE; /* do nothing if last was segmented */

  if (mfcc->func.fv_begin) ret = mfcc->func.fv_begin();
  else ret = TRUE;
  return ret;
}

boolean
mfc_module_end(MFCCCalc *mfcc)
{
  FUNC_INT func;
  int ret;

  if (mfcc->segmented_by_input) return TRUE; /* do nothing if last was segmented */

  if (mfcc->func.fv_end) ret = mfcc->func.fv_end();
  else ret = TRUE;
  return ret;
}

int
mfc_module_read(MFCCCalc *mfcc, int *new_t)
{
  FUNC_INT func;
  int ret;

  /* expand area if needed */
  if (param_alloc(mfcc->param, mfcc->f + 1, mfcc->param->veclen) == FALSE) {
    jlog("ERROR: FEATURE_INPUT: failed to allocate memory\n");
    return -2;
  }
  /* get data */
  ret = mfcc->func.fv_read(mfcc->param->parvec[mfcc->f], mfcc->param->veclen);
  if (ret == -3) {
    /* function requests segmentation of the current recognition */
    mfcc->segmented_by_input = TRUE;
    *new_t = mfcc->f;
    return -3;
  } else if (ret == -1) {
    /* end of input */
    mfcc->segmented_by_input = FALSE;
    *new_t = mfcc->f;
    return -1;
  } else if (ret == -2) {
    /* error */
    jlog("ERROR: FEATURE_INPUT: fvin_read() returns error (-2)\n");
    return -2;
  }
    
  *new_t = mfcc->f + 1;

  return 0;
}  

char *
mfc_module_input_name(MFCCCalc *mfcc)
{
  int ret;

  if (mfcc->func.fv_input_name) return(mfcc->func.fv_input_name());
  return NULL;
}

/* end of file */
