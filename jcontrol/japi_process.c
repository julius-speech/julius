/**
 * @file   japi_procss.c.
 * 
 * <JA>
 * @brief  モジュールコマンド送信部
 * </JA>
 * 
 * <EN>
 * @brief  Sending module commands
 * </EN>
 * 
 * @author Akinobu LEE
 * @date   Thu Mar 24 11:24:18 2005
 *
 * $Revision: 1.4 $
 * 
 */
/*
 * Copyright (c) 2002-2013 Kawahara Lab., Kyoto University
 * Copyright (c) 2002-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include "japi.h"

/* list all recognition processes */
void
japi_list_process(int sd)
{
  do_send(sd, "LISTPROCESS\n");
}

/* switch the current operating process */
/* grammar commands will be issued to the current process */
void
japi_current_process(int sd, char *pname)
{
  do_send(sd, "CURRENTPROCESS\n");
  if (pname == NULL) {
    do_send(sd, "\n");
  } else {
    do_sendf(sd, "%s\n", pname);
  }
}

/* shift to the next process.  If reached to the end, go back to the first
   process */
void
japi_shift_process(int sd)
{
  do_send(sd, "SHIFTPROCESS\n");
}

/* Add an LM and SR process defined in a jconf file */
void
japi_add_process(int sd, char *jconffile)
{
  if (jconffile == NULL) {
    fprintf(stderr, "Error: addprocess needs jconf file name as argument\n");
    return;
  }
  do_send(sd, "ADDPROCESS\n");
  do_sendf(sd, "%s\n", jconffile);
}

/* Delete the process */
void
japi_del_process(int sd, char *pname)
{
  if (pname == NULL) {
    fprintf(stderr, "Error: delprocess needs process name as argument\n");
    return;
  }
  do_send(sd, "DELPROCESS\n");
  do_sendf(sd, "%s\n", pname);
}

/* Activate a process previously deactivated */
void
japi_activate_process(int sd, char *pname)
{
  if (pname == NULL) {
    fprintf(stderr, "Error: activateprocess needs process name as argument\n");
    return;
  }
  do_send(sd, "ACTIVATEPROCESS\n");
  do_sendf(sd, "%s\n", pname);
}

/* Deactivate a process */
void
japi_deactivate_process(int sd, char *pname)
{
  if (pname == NULL) {
    fprintf(stderr, "Error: deactivateprocess needs process name as argument\n");
    return;
  }
  do_send(sd, "DEACTIVATEPROCESS\n");
  do_sendf(sd, "%s\n", pname);
}
