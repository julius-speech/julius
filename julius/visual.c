/**
 * @file   visual.c
 * 
 * <JA>
 * @brief  √µ∫˜∂ı¥÷§Œ≤ƒªÎ≤Ω (GTK/X11 §Úª»Õ—)
 *
 * √µ∫˜∂ı¥÷≤ƒªÎ≤Ωµ°«Ω§œ Linux §«•µ•›°º•»§µ§Ï§∆§§§ﬁ§π. 
 * "--enable-visualize" §« ON §À§∑§∆•≥•Û•—•§•Î§«§≠§ﬁ§π. 
 * § §™°§ª»Õ—§À§œ gtk §Œ•–°º•∏•Á•Û 1.x §¨…¨Õ◊§«§π. 
 * ∆∞∫Ó≥Œ«ß§œ gtk-1.2 §«π‘§§§ﬁ§∑§ø. 
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

#include "app.h"

#ifdef VISUALIZE

#include <gtk/gtk.h>

/* Window constant properties */
#define WINTITLE "Julius word trellis viewer" ///< Window title
#define DEFAULT_WINDOW_WIDTH 800 ///< Default window width
#define DEFAULT_WINDOW_HEIGHT 600 ///< Default window height
#define WAVE_HEIGHT 48 ///< Height of wave drawing canvas
#define WAVE_MARGIN 6 ///< Height margin of wave drawing canvas

/* global valuables for window handling */
static GtkAdjustment *adj;
static GtkWidget *zoom_label;
static GtkWidget *op_label;

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
static boolean draw_nodes = FALSE;

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

static gchar *css_colors =
".waveform {color: rgb(0, 0, 155);} \n"
".waveform-treshold {color: rgb(195, 78, 0);} \n"
".arc-begin {color: rgb(0, 0, 255);} \n"
".arc-end {color: rgb(245, 245, 0);} \n"
".line {color: rgb(93, 125, 93);} \n"
".word {color: rgb(39, 39, 155);} \n"
".line-faint {color: rgb(195, 210, 195);} \n"
".line-best {color: rgb(195, 117, 0);} \n"
".arc-end-best {color: rgb(245, 245, 0);} \n"
".text-best {color: rgb(195, 117, 0); font-weight: bold;} \n"
".pass2 {color: rgb(47, 47, 47);} \n"
".pass2-best {color: rgb(195, 195, 47);} \n"
".shadow {color: rgb(0, 0, 0);}";

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
 * •π•±°º•Í•Û•∞Õ—§À∫«¬Á•π•≥•¢§»∫«æÆ•π•≥•¢§Ú∆¿§Î. 
 * 
 * @param bt [in] √±∏Ï•»•Ï•Í•π
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
 * ª˛¥÷•’•Ï°º•‡§Ú X ∫¬…∏√Õ§À —¥π§π§Î. 
 * 
 * @param t [in] ª˛¥÷•’•Ï°º•‡
 * 
 * @return ¬–±˛§π§Î X ∫¬…∏√Õ§Ú ÷§π. 
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
  return(t * canvas_width / btlocal->framelen);
}

/** 
 * <JA>
 * •π•≥•¢§Ú Y ∫¬…∏√Õ§À —¥π§π§Î. 
 * 
 * @param s [in] •π•≥•¢
 * @param t [in] ¬–±˛§π§Îª˛¥÷•’•Ï°º•‡
 * 
 * @return Y ∫¬…∏√Õ§Ú ÷§π. 
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

  yoffset = (re->speechlen != 0 ? (WAVE_MARGIN + WAVE_HEIGHT): 0);
  height = canvas_height - yoffset;
  if (top <= bottom) {	/* single or no token on the time */
    y = yoffset;
  } else {
    y = (top - s) * height / (top - bottom) + yoffset;
  }
  return(y);
}

/** 
 * <JA>
 * √±∏ÏID§Ú Y ∫¬…∏√Õ§À —¥π§π§Î. 
 * 
 * @param wid [in] √±∏ÏID
 * 
 * @return Y ∫¬…∏√Õ§Ú ÷§π. 
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
  WORD_INFO *winfo;

  winfo = re->process_list->lm->winfo;
  
  yoffset = (re->speechlen != 0 ? (WAVE_MARGIN + WAVE_HEIGHT) : 0);
  height = canvas_height - yoffset;
  if (wid == WORD_INVALID) {
    y = yoffset;
  } else {
    y = wid * height / winfo->num + yoffset;
  }
  return(y);
}


/**********************************************************************/
/* Draw wave data */
/**********************************************************************/
static SP16 max_level;		///< Maximum level of input waveform

/** 
 * <JA>
 * «»∑¡…Ωº®Õ—§Àª˛¥÷§ÚX∫¬…∏§À —¥π§π§Î. 
 * 
 * @param t [in] ª˛¥÷•’•Ï°º•‡
 * 
 * @return  —¥π∏Â§Œ X ∫¬…∏§Ú ÷§π. 
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
  return(t * canvas_width / re->speechlen);
}

/** 
 * <JA>
 * «»∑¡…Ωº®Õ—§Àø∂…˝§ÚY∫¬…∏§À —¥π§π§Î. 
 * 
 * @param x [in] «»∑¡§Œø∂…˝
 * 
 * @return  —¥π∏Â§Œ X ∫¬…∏§Ú ÷§π. 
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
 * ø∂…˝§Œ∫«¬Á√Õ§Ú speech[] §Ë§Íµ·§·§Î. 
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
 * ∆˛Œœ«»∑¡ speech[] §Ú…¡≤Ë§π§Î. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
 * </JA>
 * <EN>
 * Draw input waveform in speech[].
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
draw_waveform(GtkWidget *widget,
              cairo_t   *cr)
{
  GtkStyleContext *ctx;
  PangoLayout *layout;
  int t;
  gint text_width;
  static char buf[20];

  if (re->speechlen == 0) return;	/* no waveform data (MFCC) */

  ctx = gtk_widget_get_style_context(widget);

  /* draw frame */
  gtk_style_context_save(ctx);
  gtk_style_context_add_class(ctx, "waveform");
  gtk_render_frame(ctx, cr,
                   scale_x_wave(0), scale_y_wave(max_level),
                   scale_x_wave(re->speechlen-1) - scale_x_wave(0),
                   scale_y_wave(-max_level) - scale_y_wave(max_level));
  gtk_style_context_restore(ctx);

  if (sw_level_thres) {
    /* draw level threshold line */
    gtk_style_context_save(ctx);
    gtk_style_context_add_class(ctx, "waveform-treshold");

    gtk_render_line(ctx, cr,
                    scale_x_wave(0), scale_y_wave(re->jconf->detect.level_thres),
                    scale_x_wave(re->speechlen-1), scale_y_wave(re->jconf->detect.level_thres));

    gtk_render_line(ctx, cr,
                    scale_x_wave(0), scale_y_wave(-re->jconf->detect.level_thres),
                    scale_x_wave(re->speechlen-1), scale_y_wave(-re->jconf->detect.level_thres));

    snprintf(buf, 20, "-lv %d", re->jconf->detect.level_thres);
    layout = gtk_widget_create_pango_layout(widget, buf);
    pango_layout_get_pixel_size(layout, &text_width, NULL);

    gtk_render_layout(ctx, cr,
                      canvas_width - text_width - 2,
                      scale_y_wave(-max_level) - 2,
                      layout);

    gtk_style_context_restore (ctx);
    g_clear_object(&layout);
  }
  
  /* draw text */
  gtk_style_context_save(ctx);
  gtk_style_context_add_class(ctx, "waveform");

  snprintf(buf, 20, "max: %d", max_level);
  layout = gtk_widget_create_pango_layout(widget, buf);
  pango_layout_get_pixel_size(layout, &text_width, NULL);

  gtk_render_layout(ctx, cr,
                    canvas_width - text_width - 2,
                    scale_y_wave(max_level) + 12,
                    layout);

  /* draw waveform */
  for(t=1;t<re->speechlen;t++) {
    gtk_render_line(ctx, cr,
                    scale_x_wave(t-1), scale_y_wave(re->speech[t-1]),
                    scale_x_wave(t), scale_y_wave(re->speech[t]));
  }

  gtk_style_context_restore (ctx);
  g_clear_object(&layout);
}


/**********************************************************************/
/* GTK primitive functions to draw a trellis atom */
/**********************************************************************/

/** 
 * <JA>
 * •¢°º•Ø§Ú…¡≤Ë§π§Î. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
 * @param x1 [in] ¬Ë1≈¿§ŒX∫¬…∏
 * @param y1 [in] ¬Ë1≈¿§ŒY∫¬…∏
 * @param x2 [in] ¬Ë2≈¿§ŒX∫¬…∏
 * @param y2 [in] ¬Ë2≈¿§ŒY∫¬…∏
 * @param sw [in] …¡≤Ë•Ï•Ÿ•Î§ŒªÿƒÍ
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
my_render_arc(GtkWidget *widget,
              cairo_t   *cr,
              int        x1,
              int        y1,
              int        x2,
              int        y2,
              int        sw)
{
  GtkStyleContext *ctx;
  const gchar *css_class;
  int width;

  ctx = gtk_widget_get_style_context(widget);

  /* change arc style by sw */
  switch(sw) {
  case 0:  css_class = "line-faint"; width = 1; break;
  case 1:  css_class = "line";       width = 1; break;
  case 2:  css_class = "line-best";  width = 3; break;
  case 3:  css_class = "pass2-next"; width = 1; break; /* next */
  case 4:  css_class = "pass2";      width = 1; break; /* popper (realigned) */
  case 5:  css_class = "pass2-next"; width = 2; break; /* popped (original) */
  case 6:  css_class = "pass2-best"; width = 3; break;
  default: css_class = "line";       width = 1; break;
  }

  /* draw arc line */
  if (sw_line) {
    GdkRGBA line_color;

    gtk_style_context_save(ctx);
    gtk_style_context_add_class(ctx, css_class);

    cairo_save(cr);

    gtk_style_context_get_color (ctx, gtk_style_context_get_state(ctx), &line_color);
    gdk_cairo_set_source_rgba(cr, &line_color);

    cairo_set_line_width(cr, width);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    cairo_move_to(cr, x1 + 0.5, y1 + 0.5);
    cairo_line_to(cr, x2 + 0.5, y2 + 0.5);

    cairo_stroke(cr);

    cairo_restore(cr);
    gtk_style_context_restore(ctx);
  }

  /* draw begin/end point rectangle */
  if (sw != 0 && sw != 5) {
    gtk_style_context_save(ctx);
    gtk_style_context_add_class(ctx, "shadow");

    /* make edge */
    gtk_render_frame(ctx, cr,
                     x1 - width/2 - 2,
                     y1 - width/2 - 2,
                     width + 4,
                     width + 4);

    gtk_render_frame(ctx, cr,
                     x2 - width/2 - 2,
                     y2 - width/2 - 2,
                     width + 4,
                     width + 4);

    gtk_style_context_restore(ctx);
  }

  gtk_style_context_save(ctx);
  gtk_style_context_add_class(ctx, "arc-begin");

  gtk_render_frame(ctx, cr,
                     x1 - width/2 - 1,
                     y1 - width/2 - 1,
                     width + 2,
                     width + 2);
  gtk_style_context_restore(ctx);

  gtk_style_context_save(ctx);
  if (g_strcmp0 (css_class, "line-best") == 0 || g_strcmp0 (css_class, "pass2-best") == 0) {
    gtk_style_context_add_class(ctx, "arc-end-best");
  } else {
    gtk_style_context_add_class(ctx, "arc-end");
  }

  gtk_render_frame(ctx, cr,
                     x1 - width/2 - 1,
                     y1 - width/2 - 1,
                     width + 2,
                     width + 2);

  gtk_style_context_restore(ctx);
}

/** 
 * <JA>
 * •»•Ï•Í•π√±∏Ï§Ú…¡≤Ë§π§Î•µ•÷¥ÿøÙ. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
 * @param tre [in] …¡≤Ë§π§Î•»•Ï•Í•π√±∏Ï
 * @param last_tre [in] @a tre §Œƒæ¡∞§Œ•»•Ï•Í•π√±∏Ï
 * @param sw [in] …¡≤Ë§Œ∂Ø§µ
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
draw_atom_sub(GtkWidget    *widget,
              cairo_t      *cr,
              TRELLIS_ATOM *tre,
              TRELLIS_ATOM *last_tre,
              int           sw)
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
    my_render_arc(widget, cr,
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
    my_render_arc(widget, cr,
		   scale_x(from_t),
		   scale_y(from_s, from_t),
		   scale_x(tre->endtime),
		   scale_y(tre->backscore, tre->endtime),
		   sw);
  }
}

/** 
 * <JA>
 * ¬Ë1•—•π§Œ§¢§Î•»•Ï•Í•π√±∏Ï§Ú…¡≤Ë§π§Î
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
 * @param tre [in] •»•Ï•Í•π√±∏Ï
 * @param sw [in] …¡≤Ë§Œ∂Ø§µ
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
draw_atom(GtkWidget    *widget,
          cairo_t      *cr,
          TRELLIS_ATOM *tre,
          int           sw)
{
  draw_atom_sub(widget, cr, tre, tre->last_tre, sw);
}

/* draw word output text */
/** 
 * <JA>
 * •»•Ï•Í•π√±∏Ï§Œ√±∏Ï∆…§ﬂ§Ú…¡≤Ë§π§Î. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
 * @param tre [in] •»•Ï•Í•π√±∏Ï
 * @param sw [in] …¡≤Ë§Œ∂Ø§µ
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
draw_atom_text(GtkWidget    *widget,
               cairo_t      *cr,
               TRELLIS_ATOM *tre,
               int           sw)
{
  GtkStyleContext *ctx;
  PangoLayout *layout;
  gint text_width;
  const gchar *css_class;
  int style;
  int dx, dy, x, y;
  WORD_INFO *winfo;

  winfo = re->process_list->lm->winfo;
  ctx = gtk_widget_get_style_context(widget);

  if (!winfo->woutput[tre->wid] || strlen(winfo->woutput[tre->wid]) == 0)
    return;

  switch(sw) {
  case 0:  style = -1; break;
  case 1:  css_class = "text";       style = 0; break;
  case 2:  css_class = "text-best";  style = 1; break;
  case 3:  css_class = "pass2-next"; style = 0; break;
  case 4:  css_class = "pass2";      style = 0; break;
  case 5:  css_class = "pass2";      style = 1; break;
  case 6:  css_class = "pass2-best"; style = 1; break;
  default: css_class = "text";       style = 0; break;
  }
  if (style == -1) return;	/* do not draw text */

  /* Retrieve the text width */
  gtk_style_context_save(ctx);
  gtk_style_context_add_class(ctx, css_class);

  layout = gtk_widget_create_pango_layout(widget, winfo->woutput[tre->wid]);
  pango_layout_get_pixel_size(layout, &text_width, NULL);

  x = scale_x(tre->endtime) - text_width;
  if (sw_wid_axis) {
    y = scale_y_wid(tre->wid);
  } else {
    y = scale_y(tre->backscore, tre->endtime);
  }

  gtk_render_layout(ctx, cr, x, y, layout);

  gtk_style_context_restore(ctx);
  g_clear_object(&layout);
}


/**********************************************************************/
/* wrapper for narrowing atoms to be drawn */
/**********************************************************************/
static WORD_ID *wordlist = NULL; ///< List of drawn words for search
static WORD_ID wordlistnum = 0;	///< Length of @a wordlist

/** 
 * <JA>
 * …¡≤Ë√±∏Ï•Í•π•»§Œ√Ê§À√±∏Ï§¨§¢§Î§´§…§¶§´∏°∫˜§π§Î. 
 * 
 * @param wid [in] √±∏ÏID
 * 
 * @return •Í•π•»§À∏´§ƒ§´§Ï§– TRUE°§∏´§ƒ§´§È§ §±§Ï§– FALSE §Ú ÷§π. 
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
 * √±∏Ï§Ú…¡≤Ë§π§Î° …¡≤Ë√±∏Ï•Í•π•»§À§¢§Î§‚§Œ§Œ§ﬂ°À. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
 * @param tre [in] …¡≤Ë§π§Î•»•Ï•Í•π√±∏Ï
 * @param sw [in] …¡≤Ë§Œ∂Ø§µ
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
draw_atom_top(GtkWidget    *widget,
              cairo_t      *cr,
              TRELLIS_ATOM *tre,
              int           sw)
{
  if (wordlistnum == 0 || wordlist_find(tre->wid)) {
    draw_atom(widget, cr, tre, sw);
  }
}

/** 
 * <JA>
 * √±∏Ï§Ú…¡≤Ë§π§Î° …¡≤Ë√±∏Ï•Í•π•»§À§¢§Î§‚§Œ§Œ§ﬂ°À. 
 * √±∏Ï§Œ•∆•≠•π•»§Ú…¡≤Ë§π§Î° •Í•π•»§À§ §±§Ï§–…¡≤Ë§∑§ §§°À. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
 * @param tre [in] …¡≤Ë§π§Î•»•Ï•Í•π√±∏Ï
 * @param sw [in] …¡≤Ë§Œ∂Ø§µ
 * </JA>
 * <EN>
 * √±∏Ï§Œ•∆•≠•π•»§Ú…¡≤Ë§π§Î° …¡≤Ë√±∏Ï•Í•π•»§À§¢§Î§‚§Œ§Œ§ﬂ°À. 
 * Draw text of a word on canvas (only words in wordlist).
 * 
 * @param widget [in] drawing widget
 * @param tre [in] trellis word to be drawn
 * @param sw [in] strength of drawing
 * </EN>
 */
static void
draw_atom_text_top(GtkWidget    *widget,
                   cairo_t      *cr,
                   TRELLIS_ATOM *tre,
                   int           sw)
{
  if (wordlistnum == 0 || wordlist_find(tre->wid)) {
    draw_atom_text(widget, cr, tre, sw);
  }
}


/**********************************************************************/
/* Draw a set of atom according to their properties */
/**********************************************************************/

/** 
 * <JA>
 * ¡¥§∆§Œ•»•Ï•Í•π√±∏Ï§Ú…¡≤Ë§π§Î. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
 * </JA>
 * <EN>
 * Draw all survived words in trellis.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
draw_all_atom(GtkWidget *widget,
              cairo_t   *cr)
{
  int t, i;
  TRELLIS_ATOM *tre;

  for (t=0;t<btlocal->framelen;t++) {
    for (i=0;i<btlocal->num[t];i++) {
      tre = btlocal->rw[t][i];
      draw_atom_top(widget, cr, tre, 0);
    }
  }
  if (sw_text) {
    for (t=0;t<btlocal->framelen;t++) {
      for (i=0;i<btlocal->num[t];i++) {
        tre = btlocal->rw[t][i];
        draw_atom_text_top(widget, cr, tre, 0);
      }
    }
  }
}

/** 
 * <JA>
 * ¬Ë1•—•π§À§™§§§∆§Ω§Œº°√±∏Ï§¨¿∏§≠ªƒ§√§ø•»•Ï•Í•π√±∏Ï§Œ§ﬂ…¡≤Ë§π§Î. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
 * </JA>
 * <EN>
 * Draw words whose next word was survived on the 1st pass.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
draw_context_valid_atom(GtkWidget *widget,
                        cairo_t   *cr)
{
  int t, i;
  TRELLIS_ATOM *tre;

  for (t=0;t<btlocal->framelen;t++) {
    for (i=0;i<btlocal->num[t];i++) {
      tre = btlocal->rw[t][i];
      if (tre->last_tre != NULL && tre->last_tre->wid != WORD_INVALID) {
	draw_atom_top(widget, cr, tre->last_tre, 1);
      }
    }
  }
  if (sw_text) {
    for (t=0;t<btlocal->framelen;t++) {
      for (i=0;i<btlocal->num[t];i++) {
	tre = btlocal->rw[t][i];
	if (tre->last_tre != NULL && tre->last_tre->wid != WORD_INVALID) {
	  draw_atom_text_top(widget, cr, tre->last_tre, 1);
	}
      }
    }
  }
}

#ifdef WORD_GRAPH
/** 
 * <JA>
 * √±∏Ï•∞•È•’§»§∑§∆…¡≤Ë§π§Î
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
 * </JA>
 * <EN>
 * Draw words as word graph.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
draw_word_graph(GtkWidget *widget,
                cairo_t   *cr)
{
  int t, i;
  TRELLIS_ATOM *tre;

  /* assume (1)word atoms in word graph are already marked in
     generate_lattice() in beam.c, and (2) backtrellis is wid-sorted */
  for (t=0;t<btlocal->framelen;t++) {
    for (i=0;i<btlocal->num[t];i++) {
      tre = btlocal->rw[t][i];
      if (tre->within_wordgraph) {
	draw_atom_top(widget, cr, tre, 1);
      }
    }
  }
  if (sw_text) {
    for (t=0;t<btlocal->framelen;t++) {
      for (i=0;i<btlocal->num[t];i++) {
	tre = btlocal->rw[t][i];
	if (tre->within_wordgraph) {
	  draw_atom_text_top(widget, cr, tre, 1);
	}
      }
    }
  }
  
}
#endif

/** 
 * <JA>
 * ¬Ë1•—•π§Œ∞Ï∞Ã ∏≤æ¿‚§Ú…¡≤Ë§π§Î. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
 * </JA>
 * <EN>
 * Draw the best path at the 1st pass.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
draw_best_path(GtkWidget *widget,
               cairo_t   *cr)
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
  draw_atom_top(widget, cr, last_tre, 2);
  tre = last_tre;
  while (tre->begintime > 0) {
    tre = tre->last_tre;
    draw_atom_top(widget, cr, tre, 2);
  }
  if (sw_text) {
    draw_atom_text_top(widget, cr, last_tre, 2);
    tre = last_tre;
    while (tre->begintime > 0) {
      tre = tre->last_tre;
      draw_atom_text_top(widget, cr, tre, 2);
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
 * ¬Ë2•—•π≤ƒªÎ≤Ω§Œ§ø§·§ŒΩÈ¥¸≤Ω§Úπ‘§¶. 
 * 
 * @param maxhypo [in] ¬Ë2•—•π§À§™§§§∆•›•√•◊§µ§Ï§ø≤æ¿‚§Œ∫«¬ÁøÙ
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
 * •π•ø•√•Ø§´§ÈºË§ÍΩ–§∑§ø≤æ¿‚§Ú•Ì°º•´•Î§À ›¬∏§π§Î. 
 * 
 * @param n [in] ≤æ¿‚
 * @param popctr [in] ∏Ω∫ﬂ§Œ•›•√•◊øÙ•´•¶•Û•»
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
 * ¿∏¿Æ§µ§Ï§ø≤æ¿‚§Œ•Œ°º•…§Ú ›¬∏§π§Î. 
 * 
 * @param next [in] ¿∏¿Æ§µ§Ï§øº°√±∏Ï≤æ¿‚
 * @param prev [in] ≈∏≥´∏µ§Œ≤æ¿‚
 * @param popctr [in] ∏Ω∫ﬂ§Œ¿∏¿Æ≤æ¿‚øÙ•´•¶•Û•ø
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
 * •›•√•◊§µ§Ï§ø≤æ¿‚∏ı ‰§Ú ›¬∏§π§Î. 
 * 
 * @param now [in]  ∏≤æ¿‚
 * @param winfo [in] √±∏Ïº≠ΩÒ
 * </JA>
 * <EN>
 * Store last popped hypothesis of best hypothesis.
 * 
 * @param now [in]  ∏≤æ¿‚
 * @param winfo [in] √±∏Ïº≠ΩÒ
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
 * √Ç√®2¬•√ë¬•¬π√É¬µ¬∫√∑√É√¶¬§√ã¬°¬§¬•¬π¬•¬ø¬•√É¬•¬Ø¬§¬´¬§√©¬º√®¬§√™¬Ω√ê¬§¬µ¬§√¨¬§¬ø¬≤¬æ√Ä√¢¬§√à¬§¬Ω¬§√é¬º¬°√É¬±¬∏√¨¬Ω¬∏¬π√ß¬§√≤√â√Å¬≤√®¬§¬π¬§√´.
 * 
 * @param widget √â√Å¬≤√®¬•¬¶¬•¬£¬•¬∏¬•¬ß¬•√É¬•√à
 * </JA>
 * <EN>
 * Draw popped hypotheses and their next candidates appeared while search.
 * 
 * @param widget √â√Å¬≤√®¬•¬¶¬•¬£¬•¬∏¬•¬ß¬•√É¬•√à
 * </EN>
 */
static void
draw_final_results(GtkWidget *widget,
                   cairo_t   *cr)
{
  POPNODE *firstp, *lastp, *p;

  for(firstp = lastpop; firstp; firstp = firstp->next) {
    if (firstp->tre != NULL) {
      draw_atom(widget, cr, firstp->tre, 6);
    }
    lastp = firstp;
    for(p = firstp->last; p; p = p->last) {
      if (p->tre != NULL) {
	draw_atom_sub(widget, cr, p->tre, lastp->tre, 6);
	draw_atom_text_top(widget, cr, p->tre, 6);
      }
      lastp = p;
    }
  }
}

static LOGPROB maxscore;	///< Maximum score of popped hypotheses while search
static LOGPROB minscore;	///< Minimum score of popped hypotheses while search
/** 
 * <JA>
 * ¬Ë2•—•π§«Ω–∏Ω§∑§ø≈∏≥´∏µ≤æ¿‚§Œ•π•≥•¢§Œ∫«¬Á√Õ§»∫«æÆ√Õ§Úµ·§·§Î. 
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
 * ≤æ¿‚…Ωº®ª˛°§≤æ¿‚•π•≥•¢§Ú Y ∫¬…∏√Õ§À —¥π§π§Î. 
 * 
 * @param s [in] ≤æ¿‚•π•≥•¢
 * 
 * @return ¬–±˛§π§ÎY∫¬…∏§Ú ÷§π
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

  yoffset = (re->speechlen != 0 ? (WAVE_MARGIN + WAVE_HEIGHT) : 0);
  height = canvas_height - yoffset;
  y = (maxscore - s) * height / (maxscore - minscore) + yoffset;
  return(y);
}

/* draw popped words */
/** 
 * <JA>
 * •π•ø•√•Ø§´§ÈºË§ÍΩ–§µ§Ï§ø≤æ¿‚§Ú…¡≤Ë§π§Î. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
 * @param p [in] •π•ø•√•Ø§´§ÈºË§ÍΩ–§µ§Ï§ø≤æ¿‚§Œæ Û
 * @param c [in] ¿˛§Œøß
 * @param width [in] ¿˛§Œ…˝
 * @param style [in] ¿˛§Œ•π•ø•§•Î
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
draw_popped(GtkWidget   *widget,
            cairo_t     *cr,
            const gchar *styleclass,
            int          width,
            int          style,
            POPNODE     *p)
{
  GtkStyleContext *ctx;
  int text_width;
  gint x, y;

  if (p->tre == NULL) return;

  ctx = gtk_widget_get_style_context(widget);

  if (p->last != NULL && p->last->tre != NULL) {
    gtk_style_context_save(ctx);
    gtk_style_context_add_class(ctx, styleclass);

    gtk_render_line(ctx, cr,
                    scale_x(p->last->tre->endtime),
                    scale_hypo_y(p->last->score),
                    scale_x(p->tre->endtime),
                    scale_hypo_y(p->score));

    gtk_style_context_restore(ctx);
  }

  if (p->tre != NULL) {
    x = scale_x(p->tre->endtime);
    y = scale_hypo_y(p->score);

    gtk_style_context_save(ctx);

    if (style == 1) {
      gtk_style_context_add_class(ctx, "shadow");
    } else if (style == 2) {
      gtk_style_context_add_class(ctx, styleclass);
    }

    gtk_render_frame(ctx, cr, x - 3, y - 3, 7, 7);
    gtk_style_context_restore(ctx);


    gtk_style_context_save(ctx);
    gtk_style_context_add_class(ctx, styleclass);
    gtk_render_frame(ctx, cr, x - 2, y - 2, 5, 5);

    if (p->tre->wid != WORD_INVALID) {
      PangoLayout *layout;
      WORD_INFO *winfo;

      winfo = re->process_list->lm->winfo;
      layout = gtk_widget_create_pango_layout(widget, winfo->woutput[p->tre->wid]);

      pango_layout_get_pixel_size(layout, &text_width, NULL);

      gtk_render_layout(ctx, cr, x - text_width - 1, y - 5, layout);

      g_clear_object(&layout);
    }
  }
    
  
}

/* draw popped words at one hypothesis expantion */

static int old_popctr;		///< popctr of previously popped hypo.

/** 
 * <JA>
 * ¬Ë2•—•π§Œ√±∏Ï≈∏≥´§ŒÕÕª“§Ú…¡≤Ë§π§Î. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
 * @param popctr [in] ≈∏≥´≤æ¿‚§Œ»÷πÊ
 * </JA>
 * <EN>
 * Draw a popped word and their expanded candidates for 2nd pass replay.
 * 
 * @param widget [in] drawing widget
 * @param popctr [in] counter of popped hypothesis to draw
 * </EN>
 */
static void
draw_popnodes(GtkWidget *widget, cairo_t *cr, int popctr)
{
  POPNODE *p, *porg;

  if (popctr < 0 || popctr >= pnum) {
    fprintf(stderr, "invalid popctr (%d > %d)!\n", popctr, pnum);
    return;
  }
  
  porg = &(popped[popctr]);

  /* draw expanded atoms */
  for(p = porg->next; p; p = p->next) {
    draw_popped(widget, cr, "line-best", 1, 0, p);
  }

  /* draw hypothesis context */
  for(p = porg->last; p; p = p->last) {
    draw_popped(widget, cr, "pass2-best", 2, 0, p);
  }
  draw_popped(widget, cr, "pass2-best", 3, 1, porg);

  old_popctr = popctr;
}
/** 
 * <JA>
 * ƒæ¡∞§À draw_popnodes() §«…¡≤Ë§∑§ø§Œ§ÚæÂΩÒ§≠§∑§∆æ√§π. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
 * </JA>
 * <EN>
 * Erase previous one drawn at draw_popnodes(), by overwriting.
 * 
 * @param widget [in] drawing widget
 * </EN>
 */
static void
draw_popnodes_old(GtkWidget *widget,
                  cairo_t   *cr)
{
  POPNODE *p, *porg;

  porg = &(popped[old_popctr]);

  /* draw expanded atoms */
  for(p = porg->next; p; p = p->next) {
    draw_popped(widget, cr, "line-faint", 1, 0, p);
  }

  /* draw hypothesis context */
  for(p = porg->last; p; p = p->last) {
    draw_popped(widget, cr, "pass2", 2, 0, p);
  }
  draw_popped(widget, cr, "pass2", 3, 2, porg);
}

/**********************************************************************/
/* GTK TopLevel draw/redraw functions */
/* will be called for each exposure/configure event */
/**********************************************************************/
static boolean fitscreen = TRUE; ///< •’•£•√•»•π•Ø•Í°º•ÛªÿƒÍª˛ TRUE

/** 
 * <JA>
 * …¡≤Ë•≠•„•Û•—•π∆‚§À…Ωº®§π§Ÿ§≠¡¥§∆§Œ√±∏Ïæ Û§Ú pixmap §À…¡≤Ë§π§Î. 
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
drawarea_draw(GtkWidget *widget,
              cairo_t   *cr,
              gpointer   user_data)
{

  if (re->speechlen != 0) {
    draw_waveform(widget, cr);
  }

  if (!sw_hypo) {
    if (btlocal != NULL) {
      /* draw objects */
      draw_all_atom(widget, cr);
#ifdef WORD_GRAPH
      draw_word_graph(widget, cr);
#else
      draw_context_valid_atom(widget, cr);
#endif
      draw_best_path(widget, cr);
    }
    if (popped != NULL) {
      /* draw 2nd pass objects */
      draw_final_results(widget, cr);
    }
  } else {
    if (draw_nodes) {
      double popctr = gtk_adjustment_get_value (adj);
      draw_popnodes_old(widget, cr);
      draw_popnodes(widget, cr, (int) popctr);
      draw_nodes = FALSE;
    }
  }
}

static gboolean
update_zoom_label(gpointer *data)
{
  gchar *dimentions;

  /* Update zoom label */
  dimentions = g_strdup_printf("x%3.1f", (float)canvas_width / (float)btlocal->framelen);
  gtk_label_set_label(GTK_LABEL(zoom_label), dimentions);
  g_free(dimentions);

  return(G_SOURCE_REMOVE);
}

/**
 * <JA>
 * Configure •§•Ÿ•Û•»ΩËÕ˝°ßresize §ﬁ§ø§œ map ª˛§À∫∆…¡≤Ë§π§Î. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
 * @param event [in] •§•Ÿ•Û•»æ Û
 * @param user_data [in] •Ê°º•∂•«°º•ø° Ã§ª»Õ—°À
 * 
 * @return gboolean §Ú ÷§π. 
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
drawarea_configure(GtkWidget      *widget,
                   GdkEventExpose *event,
                   gpointer        user_data)
{
  if (fitscreen) {		/* if in zoom mode, resizing window does not cause resize of the canvas */
    canvas_width = gtk_widget_get_allocated_width(widget); /* get canvas size */
  }
  /* canvas height will be always automatically changed by resizing */
  canvas_height = gtk_widget_get_allocated_height(widget);

  /* redraw objects to pixmap */
  gtk_widget_queue_draw(widget);

  g_idle_add((GSourceFunc) update_zoom_label, NULL);

  return(FALSE);
}


/**********************************************************************/
/* GTK callbacks for buttons */
/**********************************************************************/

/** 
 * <JA>
 * [show threshold] •‹•ø•Û•Ø•Í•√•Øª˛§Œ callback: •»•Í•¨§∑§≠§§√Õ¿˛§Œ…¡≤Ë
 * §Œ ON/ OFF. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
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
  gtk_widget_queue_draw(widget);
}

#ifdef PLAYCOMMAND
/** 
 * <JA>
 * [play] •‹•ø•Û•Ø•Í•√•Øª˛§Œ callback: ≤ª¿º§Ú∫∆¿∏§π§Î. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
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
  
  snprintf(command, 250, PLAYCOMMAND, re->jconf->amnow->analysis.para.smp_freq, buf);
  printf("play: [%s]\n", command);
  system(command);

  unlink(buf);
}
#endif

/** 
 * <JA>
 * [Word view] •‹•ø•Û•Ø•Í•√•Øª˛§Œ callback: Yº¥§Ú√±∏ÏID§À§π§Î. 
 * 
 * @param button [in] •‹•ø•Û•¶•£•∏•ß•√•»
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
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
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
    /* set switch */
    sw_wid_axis = TRUE;
    sw_hypo = FALSE;
    /* redraw objects to pixmap */
    gtk_widget_queue_draw(widget);
  } else {
    sw_wid_axis = FALSE;
  }
}

/** 
 * <JA>
 * [Score view] •‹•ø•Û•Ø•Í•√•Øª˛§Œ callback: Yº¥§Ú•π•≥•¢§À§π§Î. 
 * 
 * @param button [in] •‹•ø•Û•¶•£•∏•ß•√•»
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
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
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
    /* set switch */
    sw_score_beam = FALSE;
    sw_hypo = FALSE;

    gtk_label_set_label(GTK_LABEL(op_label), "Accumulated score (normalized by time)");

    /* redraw objects */
    gtk_widget_queue_draw(widget);
  }
}

/** 
 * <JA>
 * [Beam view] •‹•ø•Û•Ø•Í•√•Øª˛§Œ callback: Yº¥§Ú•’•Ï°º•‡§¥§»§Œ∫«¬Á•π•≥•¢
 * §´§È§Œ∫π ¨§À§π§Î. 
 * 
 * @param button [in] •‹•ø•Û•¶•£•∏•ß•√•»
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
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
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
    /* set switch */
    sw_score_beam = TRUE;
    sw_hypo = FALSE;

    gtk_label_set_label(GTK_LABEL(op_label), "Beam score");

    /* redraw objects */
    gtk_widget_queue_draw(widget);
  }
}

/** 
 * <JA>
 * [Arc on/off] •‹•ø•Û•Ø•Í•√•Øª˛§Œ callback: √±∏Ï¿Ë∆¨§»√±∏ÏΩ™√º§Œ¥÷§Œ¿˛§Œ
 * …¡≤Ë§Ú on/off §π§Î. 
 * 
 * @param button [in] •‹•ø•Û•¶•£•∏•ß•√•»
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
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
  boolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
  sw_text = active;
  sw_line = active;

  /* redraw objects */
  gtk_widget_queue_draw(widget);
}

/** 
 * <JA>
 * •∆•≠•π•»•¶•£•∏•ß•√•»§À√±∏ÏÃæ§¨∆˛Œœ§µ§Ï§ø§»§≠§Œcallback: …¡≤Ë√±∏Ï•Í•π•»§Ú
 * ππø∑§π§Î. 
 * 
 * @param widget [in] •∆•≠•π•»•¶•£•∏•ß•√•»
 * @param draw [in] …¡≤Ë•¶•£•∏•ß•√•»
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

  RecogProcess *r;
  Sentence *s;
  WORD_ID *seq;
  const gchar *entry_text;
  WORD_ID i;
  
  entry_text = gtk_entry_get_text(GTK_ENTRY(widget));
  r = re->process_list;
  s = &(r->result.sent[0]);

  /* allocate */
  if (wordlist == NULL) {
    wordlist = mymalloc(sizeof(WORD_ID) * s->word_num);
  }
  wordlistnum = 0;

  /* pickup words with the specified output text and regiter them to the lsit */
  if (strlen(entry_text) == 0) {
    wordlistnum = 0;
  } else {
    for (i=0;i<s->word_num;i++) {
      WORD_INFO *winfo = r->lm->winfo;
      if (strmatch(entry_text, winfo->woutput[i])) {
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
  gtk_widget_queue_draw(draw);
}

/** 
 * <JA>
 * [x2] •∫°º•‡•‹•ø•Û•Ø•Í•√•Øª˛§Œcallback: Xº¥§Ú2«‹§Àø≠ƒ•§π§Î. § §™FITSCREEN§œ
 * OFF§À§ §Î. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
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
    gtk_widget_set_size_request(widget, canvas_width, canvas_height);

  }
  gtk_widget_queue_draw(widget);
}

/** 
 * <JA>
 * [x4] •∫°º•‡•‹•ø•Û•Ø•Í•√•Øª˛§Œcallback: Xº¥§Ú4«‹§Àø≠ƒ•§π§Î. § §™FITSCREEN§œ
 * OFF§À§ §Î. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
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
    gtk_widget_set_size_request(widget, canvas_width, canvas_height);
  }
 
  gtk_widget_queue_draw(widget);
}

/** 
 * <JA>
 * [x8] •∫°º•‡•‹•ø•Û•Ø•Í•√•Øª˛§Œcallback: Xº¥§Ú8«‹§Àø≠ƒ•§π§Î. § §™FITSCREEN§œ
 * OFF§À§ §Î. 
 * 
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
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
    gtk_widget_set_size_request(widget, canvas_width, canvas_height);
  }
  
  gtk_widget_queue_draw(widget);
}

/** 
 * <JA>
 * [Fit] •‹•ø•Û•Ø•Í•√•Øª˛§Œ•≥°º•Î•–•√•Ø°ß•≠•„•Û•—•π•µ•§•∫§Ú•¶•£•Û•…•¶•µ•§•∫§À
 * º´∆∞≈™§ÀπÁ§Ô§ª§Î§Ë§¶§À§π§Î. ¬æ§Œ zoom ªÿƒÍª˛°§§Ω§Ï§È§Ú off §À§π§Î. 
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
  GtkWidget *parent = gtk_widget_get_parent(widget);

  fitscreen = TRUE;
  canvas_width = gtk_widget_get_allocated_width(parent);
  gtk_widget_set_size_request(widget, canvas_width, canvas_height);

  gtk_widget_queue_draw(widget);
}

/** 
 * <JA>
 * ¬Ë2•—•π∫∆∏ΩÕ—§Œ [start] •‹•ø•Û§Œcallback: ¬Ë2•—•π∫∆∏ΩÕ—§ÀΩ‡»˜§π§Î. 
 * 
 * @param button [in] •‹•ø•Û•¶•£•∏•ß•√•»
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
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
  sw_hypo = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

  gtk_widget_queue_draw(widget);
}

/** 
 * <JA>
 * ¬Ë2•—•π∫∆∏ΩÕ—§Œ•π•±°º•Î§Œcallback: √Õ§¨ N §À —ππ§µ§Ï§ø§»§≠§À°§
 * N »÷Ã‹§Œ√±∏Ï≈∏≥´§ŒÕÕª“§Ú…¡≤Ë§π§Î. 
 * 
 * @param adj [in] •¢•∏•„•π•ø
 * @param widget [in] …¡≤Ë•¶•£•∏•ß•√•»
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
  draw_nodes = TRUE;
  gtk_widget_queue_draw(widget);
}

/**
 * <JA>
 * GTK§ŒΩ™Œª•§•Ÿ•Û•»•œ•Û•…•È. •¢•◊•Í•±°º•∑•Á•Û§ÚΩ™Œª§π§Î. 
 * 
 * @param widget [in] •¶•£•∏•ß•√•»
 * @param data [in] •Ê°º•∂•«°º•ø
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
 * µØ∆∞ª˛°§≤ƒªÎ≤Ωµ°«Ω§ÚΩÈ¥¸≤Ω§π§Î. 
 * </JA>
 * <EN>
 * Initialize visualization functions at startup.
 * </EN>
 */
void
visual_init(Recog *recog)
{
  /* hold recognition instance to local */
  re = recog;

  /* reset values */
  btlocal = NULL;

  /* initialize Gtk/Gdk libraries */
  /* no argument passed as gtk options */
  /*gtk_init (&argc, &argv);*/
  gtk_init(NULL, NULL);

  fprintf(stderr, "GTK initialized\n");

}

static void
setup_css(void)
{
  GtkCssProvider *provider;
  GError *error;

  error = NULL;
  provider = gtk_css_provider_new();

  gtk_css_provider_load_from_data(provider, css_colors, -1, &error);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                            GTK_STYLE_PROVIDER (provider),
                                            GTK_STYLE_PROVIDER_PRIORITY_USER);

  g_clear_object (&provider);
}

/**
 * <JA>
 * «ßº±∑Î≤Ã§Ú∏µ§À°§√µ∫˜∂ı¥÷§Œ≤ƒªÎ≤Ω§Úº¬π‘§π§Î. 
 * 
 * @param bt [in] √±∏Ï•»•Ï•Í•π
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
  GtkWidget *window, *button, *draw, *entry, *scrolled_window, *scale, *headerbar;
  GtkWidget *box1, *box2, *label, *frame, *box3, *start_box;
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

  /* load css style classes */
  setup_css();

  /* create main window */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_resize(GTK_WINDOW(window), DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
  g_signal_connect(window, "destroy", G_CALLBACK(destroy), NULL);

  /* headerbar */
  headerbar = g_object_new(GTK_TYPE_HEADER_BAR,
                           "title", WINTITLE,
                           "show-close-button", TRUE,
                           NULL);
  gtk_window_set_titlebar(GTK_WINDOW(window), headerbar);

  /* create horizontal packing box */
  box1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_container_set_border_width(GTK_CONTAINER(box1), 18);
  gtk_container_add(GTK_CONTAINER(window), box1);

  /* box containing the drawing area and labels */
  start_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_hexpand(start_box, TRUE);
  gtk_widget_set_vexpand(start_box, TRUE);
  gtk_container_add(GTK_CONTAINER(box1), start_box);

  /* create scrolled window */
  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 18);

  /* create drawing area */
  draw = gtk_drawing_area_new();
  gtk_widget_set_hexpand(draw, TRUE);
  gtk_widget_set_vexpand(draw, TRUE);
  g_signal_connect(draw, "draw", G_CALLBACK(drawarea_draw), NULL);
  g_signal_connect(draw, "configure-event", G_CALLBACK(drawarea_configure), NULL);
  gtk_container_add(GTK_CONTAINER(scrolled_window), draw);
  gtk_box_pack_start(GTK_BOX(start_box), scrolled_window, TRUE, TRUE, 0);

  /* labels */
  zoom_label = gtk_label_new("");
  gtk_widget_set_halign(zoom_label, GTK_ALIGN_START);
  gtk_container_add(GTK_CONTAINER(start_box), zoom_label);

  op_label = gtk_label_new("Accumulated score (normalized by time)");
  gtk_widget_set_halign(op_label, GTK_ALIGN_START);
  gtk_container_add(GTK_CONTAINER(start_box), op_label);

  /* create packing box for buttons */
  box2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start(GTK_BOX(box1), box2, FALSE, TRUE, 0);

  if (re->speechlen != 0) {
    /* create waveform related frame */
    frame = gtk_frame_new("Waveform");
    gtk_box_pack_start(GTK_BOX(box2), frame, FALSE, FALSE, 0);
    box3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(box3), 12);
    gtk_container_add(GTK_CONTAINER(frame), box3);

    /* create play button if supported */
#ifdef PLAYCOMMAND
    button = gtk_button_new_with_label("Play");
    g_signal_connect_object(button, "clicked",
                            G_CALLBACK(action_play_waveform), draw,
                            G_CONNECT_AFTER);
    gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);
#endif

    /* create level thres toggle button */
    button = gtk_button_new_with_label("Threshold");
    g_signal_connect_object(button, "clicked",
                            G_CALLBACK(action_toggle_thres), draw,
                            G_CONNECT_AFTER);
    gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);
  }

  /* create scaling frame */
  frame = gtk_frame_new("Change view");
  gtk_box_pack_start(GTK_BOX(box2), frame, FALSE, FALSE, 0);
  box3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width(GTK_CONTAINER(box3), 12);
  gtk_container_add(GTK_CONTAINER(frame), box3);

  /* create word view button */
  button = gtk_radio_button_new_with_label(NULL, "Word");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  g_signal_connect(button, "toggled", G_CALLBACK(action_view_wid), draw);
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create score view button */
  group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
  button = gtk_radio_button_new_with_label(group, "Score");
  g_signal_connect(button, "toggled", G_CALLBACK(action_view_score), draw);
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create beam view button */
  group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
  button = gtk_radio_button_new_with_label(group, "Beam");
  g_signal_connect(button, "toggled", G_CALLBACK(action_view_beam), draw);
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create show/hide frame */
  frame = gtk_frame_new("Show/hide");
  gtk_box_pack_start(GTK_BOX(box2), frame, FALSE, FALSE, 0);
  box3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width(GTK_CONTAINER(box3), 12);
  gtk_container_add(GTK_CONTAINER(frame), box3);

  /* create text toggle button */
  button = gtk_toggle_button_new_with_label("Arcs");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  g_signal_connect(button, "toggled", G_CALLBACK(action_toggle_arc), draw);
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create word entry frame */
  frame = gtk_frame_new("View Words");
  gtk_box_pack_start(GTK_BOX(box2), frame, FALSE, FALSE, 0);
  box3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width(GTK_CONTAINER(box3), 12);
  gtk_container_add(GTK_CONTAINER(frame), box3);

  /* create word ID entry */
  entry = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(entry), 16);
  g_signal_connect(entry, "activate", G_CALLBACK(action_set_wid), draw);
  gtk_box_pack_start(GTK_BOX(box3), entry, FALSE, FALSE, 0);

  /* create zoom frame */
  frame = gtk_frame_new("Zoom");
  gtk_box_pack_start(GTK_BOX(box2), frame, FALSE, FALSE, 0);
  box3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width(GTK_CONTAINER(box3), 12);
  gtk_container_add(GTK_CONTAINER(frame), box3);

  /* create x zoom button */
  button = gtk_button_new_with_label("x2");
  g_signal_connect_swapped(button, "clicked", G_CALLBACK(action_zoom), draw);
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create x zoom button */
  button = gtk_button_new_with_label("x4");
  g_signal_connect_swapped(button, "clicked", G_CALLBACK(action_zoom_4), draw);
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create x more zoom button */
  button = gtk_button_new_with_label("x8");
  g_signal_connect_swapped(button, "clicked", G_CALLBACK(action_zoom_8), draw);
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create fit screen button */
  button = gtk_button_new_with_label("Fit");
  g_signal_connect_swapped(button, "clicked", G_CALLBACK(action_fit_screen), draw);
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create replay frame */
  frame = gtk_frame_new("Pass2 Replay");
  gtk_box_pack_start(GTK_BOX(box2), frame, FALSE, FALSE, 0);
  box3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width(GTK_CONTAINER(box3), 12);
  gtk_container_add(GTK_CONTAINER(frame), box3);

  /* create replay start button */
  button = gtk_toggle_button_new_with_label("Start");
  g_signal_connect(button, "toggled", G_CALLBACK (action_toggle_popctr), draw);
  gtk_box_pack_start(GTK_BOX(box3), button, FALSE, FALSE, 0);

  /* create replay scale widget */
  adj = gtk_adjustment_new(0.0, 0.0, (pnum-1) + 5.0, 1.0, 1.0, 5.0);
  g_signal_connect(adj, "value_changed", G_CALLBACK(action_change_popctr), draw);

  scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, adj);
  gtk_scale_set_digits(GTK_SCALE(scale), 0);
  gtk_box_pack_start(GTK_BOX(box3), scale, FALSE, FALSE, 0);

  /* show all the widget */
  gtk_widget_show_all(window);

  /* enter the gtk event routine */
  gtk_main();
}

#endif /* VISUALIZE */
