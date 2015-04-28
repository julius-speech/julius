/**
 * @file   visual.c
 * 
 * <JA>
 * @brief  探索空間の可視化 (GTK/X11 を使用)
 *
 * 探索空間可視化機能は Linux でサポートされています. 
 * "--enable-visualize" で ON にしてコンパイルできます. 
 * なお，使用には gtk のバージョン 1.x が必要です. 
 * 動作確認は gtk-1.2 で行いました. 
 * </JA>
 * 
 * <EN>
 * @brief  Visualization of search space using GTK/X11.
 *
 * This visualization feature is supported on Linux.
 * To enable, specify "--enable-visualize" at configure.
 * It needs gtk version = 1.x, (tested on gtk-1.2).
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Mon Sep 12 01:49:44 2005
 *
 * $Revision: 1.5 $
 * 
 */
/*
 * Copyright (c) 2003-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2013 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 */

#include <libmain.h>
#include "app.h"

#ifdef VISUALIZE

#include <gtk/gtk.h>

/* Window constant properties */
#define WINTITLE "Julius word trellis viewer" ///< Window title
#define DEFAULT_WINDOW_WIDTH 800 ///< Default window width
#define DEFAULT_WINDOW_HEIGHT 600 ///< Default window height
#define CANVAS_MARGIN 16 ///< Margin of drawable
#define WAVE_HEIGHT 48 ///< Height of wave drawing canvas
#define WAVE_MARGIN 6 ///< Height margin of wave drawing canvas
#define RESULT_HEIGHT 0 ///< Offset of text output for each node
#define FONTSET "-*-fixed-medium-r-normal--10-*-*-*-*-*-*-*" ///< Fontset

/* global valuables for window handling */
static GdkFont *fontset;	///< Fontset
static GdkPixmap *pixmap = NULL; ///< Pixmap for the drawable
static gint canvas_width;	///< Current width of the drawable
static gint canvas_height;	///< Current height of the drawable


/**********************************************************************/
/* view configuration switches */
/**********************************************************************/
static boolean sw_wid_axis = TRUE; ///< Y axis is wid (FALSE: score)
static boolean sw_score_beam = FALSE; ///< Y axis is beam score (FALSE: normalized accumulated score
static boolean sw_text = TRUE;	///< Text display on/off
static boolean sw_line = TRUE;	///< Arc line display on/off
static boolean sw_level_thres = FALSE; ///< Show level thres on waveform
static boolean sw_hypo = FALSE;		///< Y axis is hypothesis score

/**********************************************************************/
/* data to plot (1st pass / 2nd pass) */
/**********************************************************************/
static Recog *re;		///< Local pointer to the whole instance
/* data to plot on 1st pass */
static BACKTRELLIS *btlocal = NULL; ///< Local pointer to the word trellis
/* data to plot on 2nd pass */
static POPNODE *popped = NULL;	///< List of information for popped nodes
static int pnum;		///< Total number of popped nodes
static POPNODE *lastpop = NULL;	///< Pointer to the last popped node


/**********************************************************************/
/* GTK color allocation */
/**********************************************************************/

#define NCOLS 15
static GdkColor cols[NCOLS] = {
  {0, 63000, 63000, 63000},	/* background */
  {0,     0,     0, 40000},	/* waveform */
  {0, 50000, 20000,     0},	/* waveform level threshold line */
  {0,     0,     0, 65535},	/* begin node of word arc */
  {0, 60000, 60000,     0},	/* end node of word arc */
  {0, 24000, 32000, 24000},	/* line (context word / word graph) */
  {0, 10000, 10000, 40000},	/* text (context word / word graph) */
  {0, 50000, 54000, 50000},	/* line (word indexed trellis = all survived words) */
  {0, 50000, 30000,     0},	/* line (best path) */
  {0, 55535,     0,     0},	/* end node (best path) */
  {0, 50000, 30000,     0},	/* text (best path) */
  {0, 12000, 20000, 12000},	/* line and text (2nd pass) */
  {0, 50000, 50000, 12000},	/* line and text (2nd pass popped) */
  {0, 50000,     0,     0},	/* 2nd pass best */
  {0,     0,     0,     0}	/* shadow for text decoration */
};
typedef enum {C_BG, C_WAVEFORM, C_LEVELTHRES, C_BGN, C_END, C_LINE, C_TEXT, C_LINE_FAINT, C_LINE_BEST, C_END_BEST, C_TEXT_BEST, C_PASS2_NEXT, C_PASS2, C_PASS2_BEST, C_SHADOW} UserColors;

static GdkGC *dgc = NULL;		///< gc for drawing

/** 
 * <JA>
 * 色の割付
 * </JA>
 * <EN>
 * Assign color.
 * </EN>
 */
static void
color_init()
{
  GdkColormap *defcolmap;
  gboolean success[NCOLS];
  
  defcolmap = gdk_colormap_get_system();
  if (gdk_colormap_alloc_colors(defcolmap, cols, NCOLS, FALSE, TRUE, success) > 0) {
    fprintf(stderr, "Warning: some colors are not allocated\n");
  }

}


/**********************************************************************/
/* graph scaling */
/**********************************************************************/

static LOGPROB *ftop = NULL;	///< Top score for each frame
static LOGPROB *fbottom = NULL;	///< Bottom maximum score for each frame
static LOGPROB lowest;		///< Lowest value (lower bound)
static LOGPROB maxrange;	///< Maximum value of (top - bottom)
static LOGPROB maxrange2;	///< Maximum distance from normalization line

/** 
 * <JA>
 * スケーリング用に最大スコアと最小スコアを得る. 
 * 
 * @param bt [in] 単語トレリス
 * </JA>
 * <EN>
 * Get the top and bottom scores for scaling.
 * 
 * @param bt [in] word trellis
 * </EN>
 */
static void
get_max_frame_score(BACKTRELLIS *bt)
{
  int t, i;
  TRELLIS_ATOM *tre;
  LOGPROB x,y;

  /* allocate */
  if (ftop != NULL) free(ftop);
  ftop = mymalloc(sizeof(LOGPROB) * bt->framelen);
  if (fbottom != NULL) free(fbottom);
  fbottom = mymalloc(sizeof(LOGPROB) * bt->framelen);

  /* get maxrange, ftop[], fbottom[] */
  maxrange = 0.0;
  for (t=0;t<bt->framelen;t++) {
    x = LOG_ZERO;
    y = 0.0;
    for (i=0;i<bt->num[t];i++) {
      tre = bt->rw[t][i];
      if (x < tre->backscore) x = tre->backscore;
      if (y > tre->backscore) y = tre->backscore;
    }
    ftop[t] = x;
    fbottom[t] = y;
    if (maxrange < x - y) maxrange = x - y;
  }

  /* get the lowest score and range around y=(lowest/framelen)x */
  lowest = 0.0;
  for (t=0;t<bt->framelen;t++) {
    if (lowest > fbottom[t]) lowest = fbottom[t];
  }
  maxrange2 = 0.0;
  for (t=0;t<bt->framelen;t++) {
    x = lowest * (float)t / (float)bt->framelen;
    if (ftop[t] == LOG_ZERO) continue;
    if (maxrange2 < abs(ftop[t] - x)) maxrange2 = abs(ftop[t] - x);
    if (maxrange2 < abs(fbottom[t] - x)) maxrange2 = abs(fbottom[t] - x);
  }
}

/** 
 * <JA>
 * 時間フレームを X 座標値に変換する. 
 * 
 * @param t [in] 時間フレーム
 * 
 * @return 対応する X 座標値を返す. 
 * </JA>
 * <EN>
 * Scale X axis by time to fullfill in the canvas width.
 * 
 * @param t [in] time frame
 * 
 * @return the converted X position.
 * </EN>
 */
static gint
scale_x(int t)
{
  return(t * (canvas_width - CANVAS_MARGIN * 2) / btlocal->framelen + CANVAS_MARGIN);
}

/** 
 * <JA>
 * スコアを Y 座標値に変換する. 
 * 
 * @param s [in] スコア
 * @param t [in] 対応する時間フレーム
 * 
 * @return Y 座標値を返す. 
 * </JA>
 * <EN>
 * Scale Y axis from score to fulfill in the canvas height.
 * 
 * @param s [in] score to plot
 * @param t [in] corresponding time frame
 * 
 * @return the converted Y position.
 * </EN>
 */
static gint
scale_y(LOGPROB s, int t)
{
  gint y;
  LOGPROB top, bottom;
  gint yoffset, height;
  
  if (sw_score_beam) {
    /* beam threshold-based: upper is the maximum score on the frame */
    top = ftop[t];
    if (top == LOG_ZERO) {	/* no token found on the time */
      bottom = top;
    } else {
      bottom = ftop[t] - maxrange;
    }
  } else {
    /* total score based: show around (lowest/framelen) x time */
    top = lowest * (float)t / (float)btlocal->framelen + maxrange2;
    bottom = lowest * (float)t / (float)btlocal->framelen - maxrange2;
  }

  yoffset = CANVAS_MARGIN + RESULT_HEIGHT + (re->speechlen != 0 ? (WAVE_MARGIN + WAVE_HEIGHT): 0);
  height = canvas_height - yoffset - CANVAS_MARGIN;
  if (top <= bottom) {	/* single or no token on the time */
    y = yoffset;
  } else {
    y = (top - s) * height / (top - bottom) + yoffset;
  }
  return(y);
}

/** 
 * <JA>
 * 単語IDを Y 座標値に変換する. 
 * 
 * @param wid [in] 単語ID
 * 
 * @return Y 座標値を返す. 
 * </JA>
 * <EN>
 * Scale Y axis from word id.
 * 
 * @param wid [in] word id
 * 
 * @return the converted Y position.
 * </EN>
 */
static gint
scale_y_wid(WORD_ID wid)
{
  gint y;
  gint yoffset, height;
  
  yoffset = CANVAS_MARGIN + RESULT_HEIGHT + (re->speechlen != 0 ? (WAVE_MARGIN + WAVE_HEIGHT) : 0);
  height = canvas_height - yoffset - CANVAS_MARGIN;
  if (wid == WORD_INVALID) {
    y = yoffset;
  } else {
    y = wid * height / re->model->winfo->num + yoffset;
  }
  return(y);
}


/**********************************************************************/
/* Draw wave data */
/**********************************************************************/
static SP16 max_level;		///< Maximum level of input waveform

/** 
 * <JA>
 * 波形表示用に時間をX座標に変換する. 
 * 
 * @param t [in] 時間フレーム
 * 
 * @return 変換後の X 座標を返す. 
 * </JA>
 * <EN>
 * Scale time to X position.
 * 
 * @param t [in] time frame
 * 
 * @return converted X position.
 * </EN>
 */
static gint
scale_x_wave(int t)
{
  return(t * (canvas_width - CANVAS_MARGIN * 2) / re->speechlen + CANVAS_MARGIN);
}

/** 
 * <JA>
 * 波形表示用に振幅をY座標に変換する. 
 * 
 * @param x [in] 波形の振幅
 * 
 * @return 変換後の X 座標を返す. 
 * </JA>
 * <EN>
 * Scale wave level to Y position
 * 
 * @param x [in] wave level
 * 
 * @return converted Y position.
 * </EN>
 */
static gint
scale_y_wave(SP16 x)
{
  return(WAVE_HEIGHT / 2 + WAVE_MARGIN - (x * WAVE_HEIGHT / (max_level * 2)));
}

/** 
 * <JA>
 * 振幅の最大値を speech[] より求める. 
 * 
 * </JA>
 * <EN>
 * Get the maximum level of input waveform from speech[].
 * 
 * </EN>
 */
static void
get_max_waveform_level()
{
  int t;
  SP16 maxl;
  
  if (re->speechlen == 0) return;	/* no waveform data (MFCC) */
  
  maxl = 0;
  for(t=0;t<re->speechlen;t++) {
    if (maxl < abs(re->speech[t])) {
      maxl = abs(re->speech[t]);
    }
  }

  max_level = maxl;
  if (max_level < 3000) max_level = 3000;
}

/** 
 * <JA>
 * 入力波形 speech[] を描画する. 
 * 
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Draw input waveform in speech[].
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
draw_waveform(GtkWidget *widget)
{
  int t;
  gint text_width;
  static char buf[20];

  if (re->speechlen == 0) return;	/* no waveform data (MFCC) */

  /* first time, make gc for drawing */
  if (dgc == NULL) {
    dgc = gdk_gc_new(widget->window);
    gdk_gc_copy(dgc, widget->style->fg_gc[GTK_STATE_NORMAL]);
  }
  

  /* draw frame */
  gdk_gc_set_foreground(dgc, &(cols[C_WAVEFORM]));
  gdk_draw_rectangle(pixmap, dgc, FALSE,
		     scale_x_wave(0), scale_y_wave(max_level),
		     scale_x_wave(re->speechlen-1) - scale_x_wave(0),
		     scale_y_wave(-max_level) - scale_y_wave(max_level));

  if (sw_level_thres) {
    /* draw level threshold line */
    gdk_gc_set_foreground(dgc, &(cols[C_LEVELTHRES]));
    gdk_draw_line(pixmap, dgc,
		  scale_x_wave(0), scale_y_wave(re->jconf->detect.level_thres),
		  scale_x_wave(re->speechlen-1), scale_y_wave(re->jconf->detect.level_thres));
    gdk_draw_line(pixmap, dgc,
		  scale_x_wave(0), scale_y_wave(- re->jconf->detect.level_thres),
		  scale_x_wave(re->speechlen-1), scale_y_wave(- re->jconf->detect.level_thres));
    snprintf(buf, 20, "-lv %d", re->jconf->detect.level_thres);
    text_width = gdk_string_width(fontset, buf) + 1;
    gdk_draw_string(pixmap, fontset, dgc,
		    canvas_width - CANVAS_MARGIN - text_width - 2,
		    scale_y_wave(-max_level) - 2,
		    buf);
  }
  
  /* draw text */
  snprintf(buf, 20, "max: %d", max_level);
  text_width = gdk_string_width(fontset, buf) + 1;
  gdk_gc_set_foreground(dgc, &(cols[C_WAVEFORM]));
  gdk_draw_string(pixmap, fontset, dgc,
		  canvas_width - CANVAS_MARGIN - text_width - 2,
		  scale_y_wave(max_level) + 12,
		  buf);

  /* draw waveform */
  for(t=1;t<re->speechlen;t++) {
    gdk_gc_set_line_attributes(dgc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
    gdk_draw_line(pixmap, dgc,
		  scale_x_wave(t-1), scale_y_wave(re->speech[t-1]),
		  scale_x_wave(t), scale_y_wave(re->speech[t]));
  }
}


/**********************************************************************/
/* GTK primitive functions to draw a trellis atom */
/**********************************************************************/

/** 
 * <JA>
 * アークを描画する. 
 * 
 * @param widget [in] 描画ウィジェット
 * @param x1 [in] 第1点のX座標
 * @param y1 [in] 第1点のY座標
 * @param x2 [in] 第2点のX座標
 * @param y2 [in] 第2点のY座標
 * @param sw [in] 描画レベルの指定
 * </JA>
 * <EN>
 * Draw an arc.
 * 
 * @param widget [in] drawing widget
 * @param x1 [in] x of 1st point
 * @param y1 [in] y of 1st point
 * @param x2 [in] x of 2nd point
 * @param y2 [in] y of 2nd point
 * @param sw [in] draw strength level
 * </EN>
 */
static void
mygdk_draw_arc(GtkWidget *widget, int x1, int y1, int x2, int y2, int sw)
{
  int width;
  UserColors c;

  /* first time, make gc for drawing */
  if (dgc == NULL) {
    dgc = gdk_gc_new(widget->window);
    gdk_gc_copy(dgc, widget->style->fg_gc[GTK_STATE_NORMAL]);
  }

  /* change arc style by sw */
  switch(sw) {
  case 0: c = C_LINE_FAINT; width = 1; break;
  case 1: c = C_LINE; width = 1; break;
  case 2: c = C_LINE_BEST; width = 3; break;
  case 3: c = C_PASS2_NEXT; width = 1; break; /* next */
  case 4: c = C_PASS2; width = 1; break; /* popper (realigned) */
  case 5: c = C_PASS2_NEXT; width = 2; break; /* popped (original) */
  case 6: c = C_PASS2_BEST; width = 3; break;
  default: c = C_LINE; width = 1; break;
  }
  
  /* draw arc line */
  if (sw_line) {
    gdk_gc_set_line_attributes(dgc, width, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
    gdk_gc_set_foreground(dgc, &(cols[c]));
    gdk_draw_line(pixmap, dgc, x1, y1, x2, y2);
    gdk_gc_set_line_attributes(dgc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
  }

  /* draw begin/end point rectangle */
  if (sw != 0 && sw != 5) {
    /* make edge */
    gdk_gc_set_foreground(dgc, &(cols[C_SHADOW]));
    gdk_draw_rectangle(pixmap, dgc,
		       TRUE,
		       x1 - width/2 - 2,
		       y1 - width/2 -2,
		       width+4,
		       width+4);
    gdk_draw_rectangle(pixmap, dgc,
		       TRUE,
		       x2 - width/2 - 2,
		       y2 - width/2 -2,
		       width+4,
		       width+4);
  }
  gdk_gc_set_foreground(dgc, &(cols[C_BGN]));
  gdk_draw_rectangle(pixmap, dgc,
		     TRUE,
		     x1 - width/2 - 1,
		     y1 - width/2 -1,
		     width+2,
		     width+2);
  if (c == C_LINE_BEST || c == C_PASS2_BEST) {
    gdk_gc_set_foreground(dgc, &(cols[C_END_BEST]));
  } else {
    gdk_gc_set_foreground(dgc, &(cols[C_END]));
  }
  gdk_draw_rectangle(pixmap, dgc,
		     TRUE,
		     x2 - width/2 - 1,
		     y2 - width/2 - 1,
		     width+2,
		     width+2);
  
}

/** 
 * <JA>
 * トレリス単語を描画するサブ関数. 
 * 
 * @param widget [in] 描画ウィジェット
 * @param tre [in] 描画するトレリス単語
 * @param last_tre [in] @a tre の直前のトレリス単語
 * @param sw [in] 描画の強さ
 * </JA>
 * <EN>
 * Sub-function to draw a trellis word.
 * 
 * @param widget [in] drawing widget
 * @param tre [in] trellis word to be drawn
 * @param last_tre [in] previous word of @a tre
 * @param sw [in] drawing strength
 * </EN>
 */
static void
draw_atom_sub(GtkWidget *widget, TRELLIS_ATOM *tre, TRELLIS_ATOM *last_tre, int sw)
{
  int from_t;
  LOGPROB from_s;
  int from_w;
  
  /* draw word arc */
  if (sw_wid_axis) {
    if (tre->begintime <= 0) {
      from_t = 0;
      from_w = WORD_INVALID;
    } else {
      from_t = last_tre->endtime;
      from_w = last_tre->wid;
    }
    mygdk_draw_arc(widget,
		   scale_x(from_t),
		   scale_y_wid(from_w),
		   scale_x(tre->endtime),
		   scale_y_wid(tre->wid),
		   sw);
  } else {
    if (tre->begintime <= 0) {
      from_t = 0;
      from_s = 0.0;
    } else {
      from_t = last_tre->endtime;
      from_s = last_tre->backscore;
    }
    mygdk_draw_arc(widget,
		   scale_x(from_t),
		   scale_y(from_s, from_t),
		   scale_x(tre->endtime),
		   scale_y(tre->backscore, tre->endtime),
		   sw);
  }
}

/** 
 * <JA>
 * 第1パスのあるトレリス単語を描画する
 * 
 * @param widget [in] 描画ウィジェット
 * @param tre [in] トレリス単語
 * @param sw [in] 描画の強さ
 * </JA>
 * <EN>
 * Draw a trellis word at the 1st pass.
 * 
 * @param widget [in] drawing widget
 * @param tre [in] trellis word
 * @param sw [in] strength of drawing
 * </EN>
 */
static void
draw_atom(GtkWidget *widget, TRELLIS_ATOM *tre, int sw)
{
  draw_atom_sub(widget, tre, tre->last_tre, sw);
}

/* draw word output text */
/** 
 * <JA>
 * トレリス単語の単語読みを描画する. 
 * 
 * @param widget [in] 描画ウィジェット
 * @param tre [in] トレリス単語
 * @param sw [in] 描画の強さ
 * </JA>
 * <EN>
 * Draw a word output string of a trellis word.
 * 
 * @param widget [in] drawing widget
 * @param tre [in] trellis word
 * @param sw [in] strength of drawing
 * </EN>
 */
static void
draw_atom_text(GtkWidget *widget, TRELLIS_ATOM *tre, int sw)
{
  gint text_width;
  UserColors c;
  int style;
  int dx, dy, x, y;
  WORD_INFO *winfo;

  winfo = re->model->winfo;
  
  if (winfo->woutput[tre->wid] != NULL && strlen(winfo->woutput[tre->wid]) > 0) {
    switch(sw) {
    case 0: style = -1; break;
    case 1: c = C_TEXT; style = 0; break;
    case 2: c = C_TEXT_BEST; style = 1; break;
    case 3: c = C_PASS2_NEXT; style = 0; break;
    case 4: c = C_PASS2; style = 0; break;
    case 5: c = C_PASS2; style = 1; break;
    case 6: c = C_PASS2_BEST; style = 1; break;
    default: c = C_TEXT; style = 0; break;
    }
    if (style == -1) return;	/* do not draw text */
    
    text_width = gdk_string_width(fontset, winfo->woutput[tre->wid]) + 1;
    x = scale_x(tre->endtime) - text_width;
    if (sw_wid_axis) {
      y = scale_y_wid(tre->wid) - 4;
    } else {
      y = scale_y(tre->backscore, tre->endtime) - 4;
    }
    
    if (style == 1) {		/* make edge */
      gdk_gc_set_foreground(dgc, &cols[C_SHADOW]);
      for (dx = -1; dx <= 1; dx++) {
	for (dy = -1; dy <= 1; dy++) {
	  if (dx == 0 && dy == 0) continue;
	  gdk_draw_string(pixmap, fontset, dgc, x + dx, y + dy, 
			  winfo->woutput[tre->wid]);
	}
      }
    }
    gdk_gc_set_foreground(dgc, &cols[c]);
    gdk_draw_string(pixmap, fontset, dgc, x, y, 
		    winfo->woutput[tre->wid]);
  }
}


/**********************************************************************/
/* wrapper for narrowing atoms to be drawn */
/**********************************************************************/
static WORD_ID *wordlist = NULL; ///< List of drawn words for search
static WORD_ID wordlistnum = 0;	///< Length of @a wordlist

/** 
 * <JA>
 * 描画単語リストの中に単語があるかどうか検索する. 
 * 
 * @param wid [in] 単語ID
 * 
 * @return リストに見つかれば TRUE，見つからなければ FALSE を返す. 
 * </JA>
 * <EN>
 * Check if the given word exists in the drawn word list.
 * 
 * @param wid [in] word id
 * 
 * @return TRUE if found in list, or FALSE if not.
 * </EN>
 */
static boolean 
wordlist_find(WORD_ID wid)
{
  int left, right, mid;
  
  if (wordlistnum == 0) return FALSE;

  left = 0;
  right = wordlistnum - 1;
  while (left < right) {
    mid = (left + right) / 2;
    if (wordlist[mid] < wid) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }
  if (wordlist[left] == wid) return TRUE;
  return FALSE;
}


/**********************************************************************/
/* Top trellis atom drawing functions including wrapper */
/* All functions below should call this */
/**********************************************************************/

/** 
 * <JA>
 * 単語を描画する（描画単語リストにあるもののみ）. 
 * 
 * @param widget [in] 描画ウィジェット
 * @param tre [in] 描画するトレリス単語
 * @param sw [in] 描画の強さ
 * </JA>
 * <EN>
 * Draw a word on canvas (only words in wordlist).
 * 
 * @param widget [in] drawing widget
 * @param tre [in] trellis word to be drawn
 * @param sw [in] strength of drawing
 * </EN>
 */
static void
draw_atom_top(GtkWidget *widget, TRELLIS_ATOM *tre, int sw)
{
  if (wordlistnum == 0 || wordlist_find(tre->wid)) {
    draw_atom(widget, tre, sw);
  }
}

/** 
 * <JA>
 * 単語を描画する（描画単語リストにあるもののみ）. 
 * 単語のテキストを描画する（リストになければ描画しない）. 
 * 
 * @param widget [in] 描画ウィジェット
 * @param tre [in] 描画するトレリス単語
 * @param sw [in] 描画の強さ
 * </JA>
 * <EN>
 * 単語のテキストを描画する（描画単語リストにあるもののみ）. 
 * Draw text of a word on canvas (only words in wordlist).
 * 
 * @param widget [in] drawing widget
 * @param tre [in] trellis word to be drawn
 * @param sw [in] strength of drawing
 * </EN>
 */
static void
draw_atom_text_top(GtkWidget *widget, TRELLIS_ATOM *tre, int sw)
{
  if (wordlistnum == 0 || wordlist_find(tre->wid)) {
    draw_atom_text(widget, tre, sw);
  }
}


/**********************************************************************/
/* Draw a set of atom according to their properties */
/**********************************************************************/

/** 
 * <JA>
 * 全てのトレリス単語を描画する. 
 * 
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Draw all survived words in trellis.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
draw_all_atom(GtkWidget *widget)
{
  int t, i;
  TRELLIS_ATOM *tre;
  
  for (t=0;t<btlocal->framelen;t++) {
    for (i=0;i<btlocal->num[t];i++) {
      tre = btlocal->rw[t][i];
      draw_atom_top(widget, tre, 0);
    }
  }
  if (sw_text) {
    for (t=0;t<btlocal->framelen;t++) {
      for (i=0;i<btlocal->num[t];i++) {
	tre = btlocal->rw[t][i];
	draw_atom_text_top(widget, tre, 0);
      }
    }
  }
}

/** 
 * <JA>
 * 第1パスにおいてその次単語が生き残ったトレリス単語のみ描画する. 
 * 
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Draw words whose next word was survived on the 1st pass.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
draw_context_valid_atom(GtkWidget *widget)
{
  int t, i;
  TRELLIS_ATOM *tre;

  for (t=0;t<btlocal->framelen;t++) {
    for (i=0;i<btlocal->num[t];i++) {
      tre = btlocal->rw[t][i];
      if (tre->last_tre != NULL && tre->last_tre->wid != WORD_INVALID) {
	draw_atom_top(widget, tre->last_tre, 1);
      }
    }
  }
  if (sw_text) {
    for (t=0;t<btlocal->framelen;t++) {
      for (i=0;i<btlocal->num[t];i++) {
	tre = btlocal->rw[t][i];
	if (tre->last_tre != NULL && tre->last_tre->wid != WORD_INVALID) {
	  draw_atom_text_top(widget, tre->last_tre, 1);
	}
      }
    }
  }
}

#ifdef WORD_GRAPH
/** 
 * <JA>
 * 単語グラフとして描画する
 * 
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Draw words as word graph.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
draw_word_graph(GtkWidget *widget)
{
  int t, i;
  TRELLIS_ATOM *tre;

  /* assume (1)word atoms in word graph are already marked in
     generate_lattice() in beam.c, and (2) backtrellis is wid-sorted */
  for (t=0;t<btlocal->framelen;t++) {
    for (i=0;i<btlocal->num[t];i++) {
      tre = btlocal->rw[t][i];
      if (tre->within_wordgraph) {
	draw_atom_top(widget, tre, 1);
      }
    }
  }
  if (sw_text) {
    for (t=0;t<btlocal->framelen;t++) {
      for (i=0;i<btlocal->num[t];i++) {
	tre = btlocal->rw[t][i];
	if (tre->within_wordgraph) {
	  draw_atom_text_top(widget, tre, 1);
	}
      }
    }
  }
  
}
#endif

/** 
 * <JA>
 * 第1パスの一位文仮説を描画する. 
 * 
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Draw the best path at the 1st pass.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
draw_best_path(GtkWidget *widget)
{
  int last_time;
  LOGPROB maxscore;
  TRELLIS_ATOM *tre, *last_tre;
  int i;

  /* look for the beginning trellis word at end of speech */
  for (last_time = btlocal->framelen - 1; last_time >= 0; last_time--) {
#ifdef USE_NGRAM
    /* it is fixed to the tail silence model (winfo->tail_silwid) */
    last_tre = bt_binsearch_atom(btlocal, last_time, re->model->winfo->tail_silwid);
    if (last_tre != NULL) break;
#else /* USE_DFA */
    /* the best trellis word on the last frame (not use cp_end[]) */
    maxscore = LOG_ZERO;
    for (i=0;i<btlocal->num[last_time];i++) {
      tre = btlocal->rw[last_time][i];
      /*      if (dfa->cp_end[winfo->wton[tmp->wid]] == TRUE) {*/
	if (maxscore < tre->backscore) {
	  maxscore = tre->backscore;
	  last_tre = tre;
	}
	/*      }*/
    }
    if (maxscore != LOG_ZERO) break;
#endif
  }
  if (last_time < 0) return;		/* last_tre not found */

  /* parse from the beginning word to find the best path */
  draw_atom_top(widget, last_tre, 2);
  tre = last_tre;
  while (tre->begintime > 0) {
    tre = tre->last_tre;
    draw_atom_top(widget, tre, 2);
  }
  if (sw_text) {
    draw_atom_text_top(widget, last_tre, 2);
    tre = last_tre;
    while (tre->begintime > 0) {
      tre = tre->last_tre;
      draw_atom_text_top(widget, tre, 2);
    }
  }
}


/**********************************************************************/
/* 2nd pass drawing data collection functions */
/* will be called from search_bestfirst_main.c to gather the atoms
   referred to in the search process of the 2nd pass */
/**********************************************************************/

/** 
 * <JA>
 * 第2パス可視化のための初期化を行う. 
 * 
 * @param maxhypo [in] 第2パスにおいてポップされた仮説の最大数
 * </JA>
 * <EN>
 * Initialize for visualization of the 2nd pass.
 * 
 * @param maxhypo [in] maximum number of popped hypothesis on the 2nd pass.
 * </EN>
 */
void
visual2_init(int maxhypo)
{
  POPNODE *p, *ptmp;
  int i;

  if (popped == NULL) {
    popped = (POPNODE *)mymalloc(sizeof(POPNODE) * (maxhypo + 1));
  } else {
    for(i=0;i<pnum;i++) {
      p = popped[i].next;
      while(p) {
	ptmp = p->next;
	free(p);
	p = ptmp;
      }
    }
  }
  pnum = 1;
  /* for start words */
  popped[0].tre = NULL;
  popped[0].score = LOG_ZERO;
  popped[0].last = NULL;
  popped[0].next = NULL;

  /* for bests */
  p = lastpop;
  while(p) {
    ptmp = p->next;
    free(p);
    p = ptmp;
  }
  lastpop = NULL;
}

/** 
 * <JA>
 * スタックから取り出した仮説をローカルに保存する. 
 * 
 * @param n [in] 仮説
 * @param popctr [in] 現在のポップ数カウント
 * </JA>
 * <EN>
 * Store popped nodes to local buffer.
 * 
 * @param n [in] hypothesis node
 * @param popctr [in] current number of popped hypo.
 * </EN>
 */
void
visual2_popped(NODE *n, int popctr)
{
  if (pnum < popctr + 1) pnum = popctr + 1;
  
  popped[popctr].tre = n->popnode->tre;
  popped[popctr].score = n->popnode->score;
  popped[popctr].last = n->popnode->last;
  popped[popctr].next = NULL;

  n->popnode = &(popped[popctr]);
}
  
/** 
 * <JA>
 * 生成された仮説のノードを保存する. 
 * 
 * @param next [in] 生成された次単語仮説
 * @param prev [in] 展開元の仮説
 * @param popctr [in] 現在の生成仮説数カウンタ
 * </JA>
 * <EN>
 * Store generated nodes.
 * 
 * @param next [in] generated next word hypothesis
 * @param prev [in] source hypothesis from which @a next was expanded
 * @param popctr [in] current popped num 
 * </EN>
 */
void
visual2_next_word(NODE *next, NODE *prev, int popctr)
{
  POPNODE *new;

  /* allocate new popnode info */
  new = (POPNODE *)mymalloc(sizeof(POPNODE));
  new->tre = next->tre;
  new->score = next->score;
  /* link between previous POPNODE */
  new->last = (prev) ? prev->popnode : NULL;
  next->popnode = new;
  /* store */
  new->next = popped[popctr].next;
  popped[popctr].next = new;
}

/** 
 * <JA>
 * ポップされた仮説候補を保存する. 
 * 
 * @param now [in] 文仮説
 * @param winfo [in] 単語辞書
 * </JA>
 * <EN>
 * Store last popped hypothesis of best hypothesis.
 * 
 * @param now [in] 文仮説
 * @param winfo [in] 単語辞書
 * </EN>
 */
void
visual2_best(NODE *now, WORD_INFO *winfo)
{
  POPNODE *new;
  
  new = (POPNODE *)mymalloc(sizeof(POPNODE));
  new->tre = now->popnode->tre;
  new->score = now->popnode->score;
  new->last = now->popnode->last;
  new->next = lastpop;
  lastpop = new;
}

/**********************************************************************/
/* Draw atoms refered at the 2nd pass */
/**********************************************************************/

/* draw 2nd pass results */
/** 
 * <JA>
 * 第2パス探索中に，スタックから取り出された仮説とその次単語集合を描画する. 
 * 
 * @param widget 描画ウィジェット
 * </JA>
 * <EN>
 * Draw popped hypotheses and their next candidates appeared while search.
 * 
 * @param widget 描画ウィジェット
 * </EN>
 */
static void
draw_final_results(GtkWidget *widget)
{
  POPNODE *firstp, *lastp, *p;

  for(firstp = lastpop; firstp; firstp = firstp->next) {
    if (firstp->tre != NULL) {
      draw_atom(widget, firstp->tre, 6);
    }
    lastp = firstp;
    for(p = firstp->last; p; p = p->last) {
      if (p->tre != NULL) {
	draw_atom_sub(widget, p->tre, lastp->tre, 6);
	draw_atom_text_top(widget, p->tre, 6);
      }
      lastp = p;
    }
  }
}

static LOGPROB maxscore;	///< Maximum score of popped hypotheses while search
static LOGPROB minscore;	///< Minimum score of popped hypotheses while search
/** 
 * <JA>
 * 第2パスで出現した展開元仮説のスコアの最大値と最小値を求める. 
 * 
 * </JA>
 * <EN>
 * Get the maximum and minumum score of popped hypotheses appeared while search.
 * 
 * </EN>
 */
static void
get_max_hypo_score()
{
  POPNODE *p;
  int i;

  maxscore = LOG_ZERO;
  minscore = 0.0;
  for(i=1;i<pnum;i++) {
    if (maxscore < popped[i].score) maxscore = popped[i].score;
    if (minscore > popped[i].score) minscore = popped[i].score;
  }
}

/** 
 * <JA>
 * 仮説表示時，仮説スコアを Y 座標値に変換する. 
 * 
 * @param s [in] 仮説スコア
 * 
 * @return 対応するY座標を返す
 * </JA>
 * <EN>
 * Scale hypothesis score to Y position.
 * 
 * @param s [in] hypothesis score
 * 
 * @return the corresponding Y position.
 * </EN>
 */
static gint
scale_hypo_y(LOGPROB s)
{
  gint y;
  gint yoffset, height;

  yoffset = CANVAS_MARGIN + RESULT_HEIGHT + (re->speechlen != 0 ? (WAVE_MARGIN + WAVE_HEIGHT) : 0);
  height = canvas_height - yoffset - CANVAS_MARGIN;
  y = (maxscore - s) * height / (maxscore - minscore) + yoffset;
  return(y);
}

/* draw popped words */
/** 
 * <JA>
 * スタックから取り出された仮説を描画する. 
 * 
 * @param widget [in] 描画ウィジェット
 * @param p [in] スタックから取り出された仮説の情報
 * @param c [in] 線の色
 * @param width [in] 線の幅
 * @param style [in] 線のスタイル
 * </JA>
 * <EN>
 * Draw a popped hypothesis.
 * 
 * @param widget [in] drawing widget
 * @param p [in] information of popped hypothesis
 * @param c [in] line color
 * @param width [in] line width
 * @param style [in] line style
 * </EN>
 */
static void
draw_popped(GtkWidget *widget, POPNODE *p, UserColors c, int width, int style)
{
  int text_width;
  gint x, y;

  if (p->tre == NULL) return;

  if (p->last != NULL && p->last->tre != NULL) {
    gdk_gc_set_line_attributes(dgc, width, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
    gdk_gc_set_foreground(dgc, &(cols[c]));
    gdk_draw_line(pixmap, dgc,
		  scale_x(p->last->tre->endtime),
		  scale_hypo_y(p->last->score),
		  scale_x(p->tre->endtime),
		  scale_hypo_y(p->score));
    gdk_gc_set_line_attributes(dgc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_MITER);
  }
  
  if (p->tre != NULL) {
    x = scale_x(p->tre->endtime);
    y = scale_hypo_y(p->score);
    if (style == 1) {
      gdk_gc_set_foreground(dgc, &(cols[C_SHADOW]));
      gdk_draw_rectangle(pixmap, dgc,
			 TRUE,
			 x - 3,
			 y - 3,
			 7,
			 7);
    } else if (style == 2) {
      gdk_gc_set_foreground(dgc, &(cols[c]));
      gdk_draw_rectangle(pixmap, dgc,
			 TRUE,
			 x - 3,
			 y - 3,
			 7,
			 7);
    }
    gdk_gc_set_foreground(dgc, &(cols[c]));
    gdk_draw_rectangle(pixmap, dgc,
		       TRUE,
		       x - 2,
		       y - 2,
		       5,
		       5);
    if (p->tre->wid != WORD_INVALID) {
      text_width = gdk_string_width(fontset, re->model->winfo->woutput[p->tre->wid]) + 1;
      gdk_draw_string(pixmap, fontset, dgc, x - text_width-1, y - 5, 
		      re->model->winfo->woutput[p->tre->wid]);
    }
  }
    
  
}

/* draw popped words at one hypothesis expantion */

static int old_popctr;		///< popctr of previously popped hypo.

/** 
 * <JA>
 * 第2パスの単語展開の様子を描画する. 
 * 
 * @param widget [in] 描画ウィジェット
 * @param popctr [in] 展開仮説の番号
 * </JA>
 * <EN>
 * Draw a popped word and their expanded candidates for 2nd pass replay.
 * 
 * @param widget [in] drawing widget
 * @param popctr [in] counter of popped hypothesis to draw
 * </EN>
 */
static void
draw_popnodes(GtkWidget *widget, int popctr)
{
  POPNODE *p, *porg;

  if (popctr < 0 || popctr >= pnum) {
    fprintf(stderr, "invalid popctr (%d > %d)!\n", popctr, pnum);
    return;
  }
  
  porg = &(popped[popctr]);

  /* draw expanded atoms */
  for(p = porg->next; p; p = p->next) {
    draw_popped(widget, p, C_LINE_BEST, 1, 0);
  }

  /* draw hypothesis context */
  for(p = porg->last; p; p = p->last) {
    draw_popped(widget, p, C_PASS2_BEST, 2, 0);
  }
  draw_popped(widget, porg, C_PASS2_BEST, 3, 1);

  old_popctr = popctr;
}
/** 
 * <JA>
 * 直前に draw_popnodes() で描画したのを上書きして消す. 
 * 
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Erase previous one drawn at draw_popnodes(), by overwriting.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
draw_popnodes_old(GtkWidget *widget)
{
  POPNODE *p, *porg;

  porg = &(popped[old_popctr]);

  /* draw expanded atoms */
  for(p = porg->next; p; p = p->next) {
    draw_popped(widget, p, C_LINE_FAINT, 1, 0);
  }

  /* draw hypothesis context */
  for(p = porg->last; p; p = p->last) {
    draw_popped(widget, p, C_PASS2, 2, 0);
  }
  draw_popped(widget, porg, C_PASS2, 3, 2);
}

/**********************************************************************/
/* GTK TopLevel draw/redraw functions */
/* will be called for each exposure/configure event */
/**********************************************************************/
static boolean fitscreen = TRUE; ///< フィットスクリーン指定時 TRUE

/** 
 * <JA>
 * キャンバスの背景を pixmap に作成する
 * 
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Make the white background to the pixmap of the given canvas widget.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
draw_background(GtkWidget *widget)
{
  static char buf[MAX_HMMNAME_LEN];
  
  /* first time, make gc for drawing */
  if (dgc == NULL) {
    dgc = gdk_gc_new(widget->window);
    gdk_gc_copy(dgc, widget->style->fg_gc[GTK_STATE_NORMAL]);
  }
  /* clear pixmap background */
  gdk_gc_set_foreground(dgc, &(cols[C_BG]));
  gdk_draw_rectangle(pixmap, dgc, TRUE,
		     0,0,canvas_width,canvas_height);

  /* display view mode and zoom status */
  gdk_gc_set_foreground(dgc, &(cols[C_TEXT]));
  if (sw_hypo) {
    gdk_draw_string(pixmap, fontset, dgc, 0, canvas_height - 16,
		    "Hypothesis score (2nd pass)");
  } else {
    gdk_draw_string(pixmap, fontset, dgc, 0, canvas_height - 16,
		    sw_wid_axis ? "Word ID" : (sw_score_beam ? "Beam score" : "Accumulated score (normalized by time)"));
  }
  snprintf(buf, 50, "x%3.1f", (float)canvas_width / (float)btlocal->framelen);
  gdk_draw_string(pixmap, fontset, dgc, 0, canvas_height - 3, buf);
}

/** 
 * <JA>
 * 描画キャンパス内に表示すべき全ての単語情報を pixmap に描画する. 
 * 
 * @param widget 
 * </JA>
 * <EN>
 * Draw all the contents to the pixmap.
 * 
 * @param widget 
 * </EN>
 */
static void
drawarea_draw(GtkWidget *widget)
{
  /* allocate (new) pixmap */
  if (pixmap) {
    gdk_pixmap_unref(pixmap);	/* destroy old one */
  }
  pixmap = gdk_pixmap_new(widget->window, canvas_width, canvas_height, -1);
  
  /* make background */
  draw_background(widget);

  if (re->speechlen != 0) {
    draw_waveform(widget);
  }

  if (!sw_hypo) {
    if (btlocal != NULL) {
      /* draw objects */
      draw_all_atom(widget);
#ifdef WORD_GRAPH
      draw_word_graph(widget);
#else
      draw_context_valid_atom(widget);
#endif
      draw_best_path(widget);
    }
    if (popped != NULL) {
      /* draw 2nd pass objects */
      draw_final_results(widget);
    }
  }
}

/** 
 * <JA>
 * Expose イベントを発行する. 
 * 
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Tell X to issue expose event on this window.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
drawarea_expose(GtkWidget *widget)
{
  GdkRectangle r;

  r.x = 0;
  r.y = 0;
  r.width = canvas_width;
  r.height = canvas_height;
  gtk_widget_draw(widget, &r);
}

/** 
 * <JA>
 * Expose イベント処理：あらかじめ描画された pixmap を画面に表示する. 
 * 
 * @param widget [in] 描画ウィジェット
 * @param event [in] イベント情報
 * @param user_data [in] ユーザデータ（未使用）
 * 
 * @return gboolean を返す. 
 * </JA>
 * <EN>
 * Expose event handler: show the already drawn pixmap to the screen.
 * 
 * @param widget [in] drawing widget
 * @param event [in] event information
 * @param user_data [in] user data (unused)
 * 
 * @return gboolean value.
 * </EN>
 */
static gboolean
event_drawarea_expose(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
  if (pixmap != NULL) {
    gdk_draw_pixmap(widget->window,
		    widget->style->fg_gc[GTK_STATE_NORMAL],
		    pixmap,
		    event->area.x, event->area.y,
		    event->area.x, event->area.y,
		    event->area.width, event->area.height);
  }
}

/** 
 * <JA>
 * Configure イベント処理：resize または map 時に再描画する. 
 * 
 * @param widget [in] 描画ウィジェット
 * @param event [in] イベント情報
 * @param user_data [in] ユーザデータ（未使用）
 * 
 * @return gboolean を返す. 
 * </JA>
 * <EN>
 * Configure event handler: redraw objects when resized or mapped.
 * 
 * @param widget [in] drawing widget
 * @param event [in] event information
 * @param user_data [in] user data (unused)
 * 
 * @return gboolean value.
 * </EN>
 */
static gboolean
event_drawarea_configure(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
  if (fitscreen) {		/* if in zoom mode, resizing window does not cause resize of the canvas */
    canvas_width = widget->allocation.width; /* get canvas size */
  }
  /* canvas height will be always automatically changed by resizing */
  canvas_height = widget->allocation.height;

  /* redraw objects to pixmap */
  drawarea_draw(widget);
}


/**********************************************************************/
/* GTK callbacks for buttons */
/**********************************************************************/

/** 
 * <JA>
 * [show threshold] ボタンクリック時の callback: トリガしきい値線の描画
 * の ON/ OFF. 
 * 
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Callback when [show threshold] button is clicked: toggle trigger
 * threshold line.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
action_toggle_thres(GtkWidget *widget)
{

  if (re->speechlen == 0) return;
  /* toggle switch */
  if (sw_level_thres) sw_level_thres = FALSE;
  else sw_level_thres = TRUE;

  /* redraw objects to pixmap */
  drawarea_draw(widget);

  /* tell X to issue expose event on this window */
  drawarea_expose(widget);
}

#ifdef PLAYCOMMAND
/** 
 * <JA>
 * [play] ボタンクリック時の callback: 音声を再生する. 
 * 
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Callback when [play] button is clicked: play waveform.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
action_play_waveform(GtkWidget *widget)
{
  char buf[80];
  static char command[250];
  int fd;

  if (re->speechlen == 0) return;
  
  /* play waveform */
  snprintf(buf, 250, "/var/tmp/julius_visual_play.%d", getpid());
  if ((fd = open(buf, O_CREAT | O_TRUNC | O_WRONLY, 0644)) < 0) {
    fprintf(stderr, "cannot open %s for writing\n", buf);
    return;
  }
  if (wrsamp(fd, re->speech, re->speechlen) < 0) {
    fprintf(stderr, "failed to write to %s for playing\n", buf);
    return;
  }
  close(fd);
  
  snprintf(command, 250, PLAYCOMMAND, re->jconf->analysis.para.smp_freq, buf);
  printf("play: [%s]\n", command);
  system(command);

  unlink(buf);
}
#endif

/** 
 * <JA>
 * [Word view] ボタンクリック時の callback: Y軸を単語IDにする. 
 * 
 * @param button [in] ボタンウィジェット
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Callback when [Word view] button is clicked: set Y axis to word ID.
 * 
 * @param button [in] button widget
 * @param widget [in] drawing widget
 * </EN>
 */
static void
action_view_wid(GtkWidget *button, GtkWidget *widget)
{
  if (GTK_TOGGLE_BUTTON(button)->active) {
    /* set switch */
    sw_wid_axis = TRUE;
    sw_hypo = FALSE;
    /* redraw objects to pixmap */
    drawarea_draw(widget);
    /* tell X to issue expose event on this window */
    drawarea_expose(widget);
  } else {
    sw_wid_axis = FALSE;
  }
}

/** 
 * <JA>
 * [Score view] ボタンクリック時の callback: Y軸をスコアにする. 
 * 
 * @param button [in] ボタンウィジェット
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Callback when [Score view] button is clicked: set Y axis to score.
 * 
 * @param button [in] button widget
 * @param widget [in] drawing widget
 * </EN>
 */
static void
action_view_score(GtkWidget *button, GtkWidget *widget)
{
  if (GTK_TOGGLE_BUTTON(button)->active) {
    /* set switch */
    sw_score_beam = FALSE;
    sw_hypo = FALSE;
    /* redraw objects to pixmap */
    drawarea_draw(widget);
    /* tell X to issue expose event on this window */
    drawarea_expose(widget);
  }
}

/** 
 * <JA>
 * [Beam view] ボタンクリック時の callback: Y軸をフレームごとの最大スコア
 * からの差分にする. 
 * 
 * @param button [in] ボタンウィジェット
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Callback when [Beam view] button is clicked: set Y axis to offsets
 * from the maximum score on each frame.
 * 
 * @param button [in] button widget
 * @param widget [in] drawing widget
 * </EN>
 */
static void
action_view_beam(GtkWidget *button, GtkWidget *widget)
{
  if (GTK_TOGGLE_BUTTON(button)->active) {
    /* set switch */
    sw_score_beam = TRUE;
    sw_hypo = FALSE;
    /* redraw objects to pixmap */
    drawarea_draw(widget);
    /* tell X to issue expose event on this window */
    drawarea_expose(widget);
  }
}

/** 
 * <JA>
 * [Arc on/off] ボタンクリック時の callback: 単語先頭と単語終端の間の線の
 * 描画を on/off する. 
 * 
 * @param button [in] ボタンウィジェット
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Callback when [Arc on/off] button is clicked: toggle lines between
 * word head node and word tail node.
 * 
 * @param button [in] button widget
 * @param widget [in: drawing widget
 * </EN>
 */
static void
action_toggle_arc(GtkWidget *button, GtkWidget *widget)
{
  if (GTK_TOGGLE_BUTTON(button)->active) {
    sw_text = TRUE;
    sw_line = TRUE;
  } else {
    sw_text = FALSE;
    sw_line = FALSE;
  }
  /* redraw objects to pixmap */
  drawarea_draw(widget);
  /* tell X to issue expose event on this window */
  drawarea_expose(widget);
}

/** 
 * <JA>
 * テキストウィジェットに単語名が入力されたときのcallback: 描画単語リストを
 * 更新する. 
 * 
 * @param widget [in] テキストウィジェット
 * @param draw [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Callback when a word string is entered on the text widget: update the
 * drawing word list to show only the word.
 * 
 * @param widget [in] text widget
 * @param draw [in] drawing widget
 * </EN>
 */
static void
action_set_wid(GtkWidget *widget, GtkWidget *draw)
{
  gchar *entry_text;
  WORD_ID i;
  
  entry_text = gtk_entry_get_text(GTK_ENTRY(widget));

  /* allocate */
  if (wordlist == NULL) {
    wordlist = mymalloc(sizeof(WORD_ID) * re->model->winfo->num);
  }
  wordlistnum = 0;

  /* pickup words with the specified output text and regiter them to the lsit */
  if (strlen(entry_text) == 0) {
    wordlistnum = 0;
  } else {
    for (i=0;i<re->model->winfo->num;i++) {
      if (strmatch(entry_text, re->model->winfo->woutput[i])) {
	wordlist[wordlistnum] = i;
	wordlistnum++;
      }
    }
    if (wordlistnum == 0) {
      fprintf(stderr, "word \"%s\" not found, show all\n", entry_text);
    } else {
      fprintf(stderr, "%d words found for \"%s\"\n", wordlistnum, entry_text);
    }
  }
  
  /* redraw objects to pixmap */
  drawarea_draw(draw);
  /* tell X to issue expose event on this window */
  drawarea_expose(draw);
}

/** 
 * <JA>
 * [x2] ズームボタンクリック時のcallback: X軸を2倍に伸張する. なおFITSCREENは
 * OFFになる. 
 * 
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Callback when [x2] zoom button is clicked: expand X axis to "x2".
 * If FITSCREEN is enabled, it will be disabled.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
action_zoom(GtkWidget *widget)
{
  fitscreen = FALSE;
  if (btlocal != NULL) {
    canvas_width = btlocal->framelen * 2;
    gtk_drawing_area_size(GTK_DRAWING_AREA(widget), canvas_width, canvas_height);

  }
  drawarea_draw(widget);
  drawarea_expose(widget);
}

/** 
 * <JA>
 * [x4] ズームボタンクリック時のcallback: X軸を4倍に伸張する. なおFITSCREENは
 * OFFになる. 
 * 
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Callback when [x4] zoom button is clicked: expand X axis to "x4".
 * If FITSCREEN is enabled, it will be disabled.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
action_zoom_4(GtkWidget *widget)
{
  fitscreen = FALSE;
  if (btlocal != NULL) {
    canvas_width = btlocal->framelen * 4;
    gtk_drawing_area_size(GTK_DRAWING_AREA(widget), canvas_width, canvas_height);
  }
 
  drawarea_draw(widget);
  drawarea_expose(widget);
}

/** 
 * <JA>
 * [x8] ズームボタンクリック時のcallback: X軸を8倍に伸張する. なおFITSCREENは
 * OFFになる. 
 * 
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Callback when [x8] zoom button is clicked: expand X axis to "x8".
 * If FITSCREEN is enabled, it will be disabled.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
action_zoom_8(GtkWidget *widget)
{
  fitscreen = FALSE;
  if (btlocal != NULL) {
    canvas_width = btlocal->framelen * 8;
    gtk_drawing_area_size(GTK_DRAWING_AREA(widget), canvas_width, canvas_height);
  }
  
  drawarea_draw(widget);
  drawarea_expose(widget);
}

/** 
 * <JA>
 * [Fit] ボタンクリック時のコールバック：キャンパスサイズをウィンドウサイズに
 * 自動的に合わせるようにする. 他の zoom 指定時，それらを off にする. 
 * 
 * @param widget 
 * </JA>
 * <EN>
 * Callback for [Fit] button: make canvas automatically fit to the window size.
 * If other zoom mode is enabled, they will be disabled.
 * 
 * @param widget 
 * </EN>
 */
static void
action_fit_screen(GtkWidget *widget)
{
  fitscreen = TRUE;
  canvas_width = widget->parent->allocation.width;
  gtk_drawing_area_size(GTK_DRAWING_AREA(widget), canvas_width, canvas_height);
  
  drawarea_draw(widget);
  drawarea_expose(widget);
}

/** 
 * <JA>
 * 第2パス再現用の [start] ボタンのcallback: 第2パス再現用に準備する. 
 * 
 * @param button [in] ボタンウィジェット
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Callback for [start] button for pass2 replay: prepare flag and canvas
 * for the replaying of the 2nd pass.
 * 
 * @param button [in] button widget
 * @param widget [in] drawing widget
 * </EN>
 */
static void
action_toggle_popctr(GtkWidget *button, GtkWidget *widget)
{
  if (GTK_TOGGLE_BUTTON(button)->active) {
    sw_hypo = TRUE;
  } else {
    sw_hypo = FALSE;
  }
  drawarea_draw(widget);
  drawarea_expose(widget);
}

/** 
 * <JA>
 * 第2パス再現用のスケールのcallback: 値が N に変更されたときに，
 * N 番目の単語展開の様子を描画する. 
 * 
 * @param adj [in] アジャスタ
 * @param widget [in] 描画ウィジェット
 * </JA>
 * <EN>
 * Callback for pop counter scale for pass2 replay: when the scale value
 * was changed to N, draw the details of the Nth word expansion on the
 * 2nd pass.
 * 
 * @param adj [in] adjuster of the scale
 * @param widget [in] drawing widget
 * </EN>
 */
static void
action_change_popctr(GtkAdjustment *adj, GtkWidget *widget)
{
  int popctr;

  if (sw_hypo) {
    popctr = adj->value;
    draw_popnodes_old(widget);
    draw_popnodes(widget, popctr);
    drawarea_expose(widget);
  }
}

/**********************************************************************/
/* GTK delete/destroy event handler */
/**********************************************************************/

/** 
 * <JA>
 * GTKの削除イベントハンドラ
 * 
 * @param widget [in] ウィジェット
 * @param event [in] イベント
 * @param data [in] ユーザデータ
 * 
 * @return FALSEを常に返す（destroy signalを発行するため）. 
 * </JA>
 * <EN>
 * GTK delete event handler.
 * 
 * @param widget [in] widget
 * @param event [in] event
 * @param data [in] user data
 * 
 * @return always FALSE, to emit destroy signal.
 * </EN>
 */
static gint
delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  return (FALSE);		/* emit destroy signal */
}

/** 
 * <JA>
 * GTKの終了イベントハンドラ. アプリケーションを終了する. 
 * 
 * @param widget [in] ウィジェット
 * @param data [in] ユーザデータ
 * </JA>
 * <EN>
 * GTK destroy event handler.  Quit application here.
 * 
 * @param widget [in] widget
 * @param data [in] user data
 * </EN>
 */
static void
destroy(GtkWidget *widget, gpointer data)
{
  gtk_main_quit();
}

/**********************************************************************/
/* Main public functions for visualization */
/**********************************************************************/

/** 
 * <JA>
 * 起動時，可視化機能を初期化する. 
 * </JA>
 * <EN>
 * Initialize visualization functions at startup.
 * </EN>
 */
void
visual_init(Recog *recog)
{
  POPNODE *p;

  /* hold recognition instance to local */
  re = recog;

  /* reset values */
  btlocal = NULL;
  
  /* initialize Gtk/Gdk libraries */
  /* no argument passed as gtk options */
  /*gtk_init (&argc, &argv);*/
  gtk_init(NULL, NULL);

  /* set locale */
  gtk_set_locale();

  /* load fontset */
  fontset = gdk_fontset_load(FONTSET);
  if (fontset == NULL) {
    fprintf(stderr, "cannot load X font \"%s\" for visualize\n", FONTSET); exit(-1);
  }

  /* initialize color */
  color_init();

  fprintf(stderr, "GTK initialized\n");

}

/** 
 * <JA>
 * 認識結果を元に，探索空間の可視化を実行する. 
 * 
 * @param bt [in] 単語トレリス
 * </JA>
 * <EN>
 * Start visualization of recognition result.
 * 
 * @param bt [in] word trellis
 * </EN>
 */
void
visual_show(BACKTRELLIS *bt)
{
  GtkWidget *window, *button, *draw, *entry, *scrolled_window, *scale;
  GtkWidget *box1, *box2, *label, *frame, *box3;
  GtkObject *adj;
  GSList *group;
  GList *glist;

  
  fprintf(stderr, "*** Showing word trellis view (close window to proceed)\n");

  /* store pointer to backtrellis data */
  btlocal = bt;

  /* prepare for Y axis score normalization */
  get_max_frame_score(bt);

  /* prepare for Y axis hypo score normalization */
  get_max_hypo_score();

  /* prepare for waveform */
  if (re->speechlen != 0) get_max_waveform_level();

  /* start with trellis view */
  sw_hypo = FALSE;

  /* reset value */
  fitscreen = TRUE;
  if (dgc != NULL) {
    gdk_gc_unref(dgc);
    dgc = NULL;
  }

  /* create main window */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_usize(GTK_WIDGET(window), DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
  gtk_window_set_title(GTK_WINDOW(window), WINTITLE);
  gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		     GTK_SIGNAL_FUNC(delete_event), NULL);
  gtk_signal_connect(GTK_OBJECT(window), "destroy",
		     GTK_SIGNAL_FUNC(destroy), NULL);
  gtk_container_border_width(GTK_CONTAINER(window), 10);

  /* create horizontal packing box */
  box1 = gtk_hbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(window), box1);

  /* create scrolled window */
  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_ALWAYS,GTK_POLICY_AUTOMATIC);

  /* create drawing area */
  draw = gtk_drawing_area_new();
  /*  gtk_drawing_area_size(GTK_DRAWING_AREA(draw), DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT);*/
  gtk_signal_connect(GTK_OBJECT(draw), "expose-event", GTK_SIGNAL_FUNC(event_drawarea_expose), NULL);
  gtk_signal_connect(GTK_OBJECT(draw), "configure-event", GTK_SIGNAL_FUNC(event_drawarea_configure), NULL);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), draw);
  gtk_box_pack_start(GTK_BOX(box1), scrolled_window, TRUE, TRUE, 0);

  /* create packing box for buttons */
  box2 = gtk_vbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, TRUE, 0);

  if (re->speechlen != 0) {
    /* create waveform related frame */
    frame = gtk_frame_new("waveform");
    gtk_box_pack_start(GTK_BOX(box2), frame, FALSE, FALSE, 0);
    box3 = gtk_hbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(box3), 5);
    gtk_container_add(GTK_CONTAINER(frame), box3);
    
    /* create play button if supported */
#ifdef PLAYCOMMAND
    button = gtk_button_new_with_label("play");
    gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			      GTK_SIGNAL_FUNC(action_play_waveform), GTK_OBJECT(draw));
    gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);
#endif
    
    /* create level thres toggle button */
    button = gtk_button_new_with_label("thres");
    gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			      GTK_SIGNAL_FUNC(action_toggle_thres), GTK_OBJECT(draw));
    gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);
  }
    
  /* create scaling frame */
  frame = gtk_frame_new("change view");
  gtk_box_pack_start(GTK_BOX(box2), frame, FALSE, FALSE, 0);
  box3 = gtk_hbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box3), 5);
  gtk_container_add(GTK_CONTAINER(frame), box3);

  /* create word view button */
  button = gtk_radio_button_new_with_label(NULL, "word");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     GTK_SIGNAL_FUNC(action_view_wid), GTK_OBJECT(draw));
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create score view button */
  group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
  button = gtk_radio_button_new_with_label(group, "score");
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     GTK_SIGNAL_FUNC(action_view_score), GTK_OBJECT(draw));
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create beam view button */
  group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
  button = gtk_radio_button_new_with_label(group, "beam");
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     GTK_SIGNAL_FUNC(action_view_beam), GTK_OBJECT(draw));
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create show/hide frame */
  frame = gtk_frame_new("show/hide");
  gtk_box_pack_start(GTK_BOX(box2), frame, FALSE, FALSE, 0);
  box3 = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box3), 5);
  gtk_container_add(GTK_CONTAINER(frame), box3);

  /* create text toggle button */
  button = gtk_toggle_button_new_with_label("arcs");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     GTK_SIGNAL_FUNC(action_toggle_arc), GTK_OBJECT(draw));
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create word entry frame */
  frame = gtk_frame_new("view words");
  gtk_box_pack_start(GTK_BOX(box2), frame, FALSE, FALSE, 0);
  box3 = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box3), 5);
  gtk_container_add(GTK_CONTAINER(frame), box3);

  /* create word ID entry */
  entry = gtk_entry_new_with_max_length(16);
  gtk_signal_connect(GTK_OBJECT(entry), "activate",
		     GTK_SIGNAL_FUNC(action_set_wid), GTK_OBJECT(draw));
  gtk_box_pack_start(GTK_BOX(box3), entry, FALSE, FALSE, 0);

  /* create zoom frame */
  frame = gtk_frame_new("zoom");
  gtk_box_pack_start(GTK_BOX(box2), frame, FALSE, FALSE, 0);
  box3 = gtk_hbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box3), 5);
  gtk_container_add(GTK_CONTAINER(frame), box3);

  /* create x zoom button */
  button = gtk_button_new_with_label("x2");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			    GTK_SIGNAL_FUNC(action_zoom), GTK_OBJECT(draw));
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create x zoom button */
  button = gtk_button_new_with_label("x4");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			    GTK_SIGNAL_FUNC(action_zoom_4), GTK_OBJECT(draw));
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create x more zoom button */
  button = gtk_button_new_with_label("x8");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			    GTK_SIGNAL_FUNC(action_zoom_8), GTK_OBJECT(draw));
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create fit screen button */
  button = gtk_button_new_with_label("fit");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			    GTK_SIGNAL_FUNC(action_fit_screen), GTK_OBJECT(draw));
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create replay frame */
  frame = gtk_frame_new("pass2 replay");
  gtk_box_pack_start(GTK_BOX(box2), frame, FALSE, FALSE, 0);
  box3 = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box3), 5);
  gtk_container_add(GTK_CONTAINER(frame), box3);

  adj = gtk_adjustment_new(0.0, 0.0, (pnum-1) + 5, 1.0, 1.0, 5.0);
  gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
		     GTK_SIGNAL_FUNC(action_change_popctr), GTK_OBJECT(draw));

  /* create replay start button */
  button = gtk_toggle_button_new_with_label("start");
  gtk_signal_connect(GTK_OBJECT(button), "toggled",
		     GTK_SIGNAL_FUNC(action_toggle_popctr), GTK_OBJECT(draw));
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);
  
  /* create replay scale widget */
  scale = gtk_hscale_new(GTK_ADJUSTMENT(adj));
  gtk_scale_set_digits(GTK_SCALE(scale), 0);
  gtk_box_pack_start(GTK_BOX(box3), scale, FALSE, FALSE, 0);
  
  /* create close button */
  button = gtk_button_new_with_label("close");
  /* connect click event to close the window */
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(window));
  
  gtk_box_pack_start(GTK_BOX(box2), button, FALSE, FALSE, 0);

  /* show all the widget */
  gtk_widget_show_all(window);

  /* enter the gtk event routine */
  gtk_main();
}

#endif /* VISUALIZE */
