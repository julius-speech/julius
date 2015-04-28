/* 
 *  libjcode.c -- 漢字変換ライブラリ    1.0 版
 *                (C) Kuramitsu Kimio, Tokyo Univ. 1996-97
 *
 *  このライブラリは、CGI Programming with C and Perl のために
 *  Ken Lunde 著 「日本語情報処理」 (O'llery) を参考にして、
 *  ストリーム用だったjconv.c を、ストリング対応にしてライブラリ化
 *  しました。 
 *  ただし、CGI (INTERNET)での利用を考えて、変更してあります。
 */

/* modified by ri to avoid may malloc */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "jlib.h"
#include "jlibconfig.h"

#include <sent/stddefs.h>
 
extern int detectKanjiCode(char *str);
static unsigned char *_to_jis(unsigned char *str);
static unsigned char *_to_ascii(unsigned char *str);
static void _jis_shift(int *p1, int *p2);
static void _sjis_shift(int *p1, int *p2);
static unsigned char *_sjis_han2zen(unsigned char *str, int *p1, int *p2);
static void _shift2seven(unsigned char *str, unsigned char *str2);
static void _shift2euc(unsigned char *str, unsigned char *str2);
static void _shift_self(unsigned char *str, unsigned char *str2);
static void _euc2seven(unsigned char *str, unsigned char *str2);
static void _euc2shift(unsigned char *str, unsigned char *str2);
static unsigned char *_skip_esc(unsigned char *str, int *esc_in);
static void _seven2shift(unsigned char *str, unsigned char *str2);
static void _seven2euc(unsigned char *str, unsigned char *str2);



#define CHAROUT(ch) *str2 = (unsigned char)(ch); str2++;

/* --------------------------------------- JIS(ISO-2022) コードへ切り替え -- */

static unsigned char *_to_jis(unsigned char *str) {
  *str = (unsigned char)ESC; str++;
  *str = (unsigned char)'$'; str++;
  *str = (unsigned char)'B'; str++;
  return str;
}

/* ----------------------------------------------- ASCII コードへ切り替え -- */

/* ESC ( B と ESC ( J の違い。
   本来は、 ESC ( J が正しいJIS-Roman 体系であるが、
   インターネットの上では、英数字はASCII の方が自然かと思われる。
   \ 記号と ~記号が違うだけである。 */

static unsigned char *_to_ascii(unsigned char *str) {
  *str = (unsigned char)ESC; str++;
  *str = (unsigned char)'('; str++;
  *str = (unsigned char)'B'; str++;
  return str;
}

/* -------------------------------------- JIS コード を SJISとしてシフト -- */

static void _jis_shift(int *p1, int *p2)
{
  unsigned char c1 = *p1;
  unsigned char c2 = *p2;
  int rowOffset = c1 < 95 ? 112 : 176;
  int cellOffset = c1 % 2 ? (c2 > 95 ? 32 : 31) : 126;

  *p1 = ((c1 + 1) >> 1) + rowOffset;
  *p2 += cellOffset;
}

/* --------------------------------- SJIS コードをJIS コードとしてシフト -- */

static void _sjis_shift(int *p1, int *p2)
{
  unsigned char c1 = *p1;
  unsigned char c2 = *p2;
  int adjust = c2 < 159;
  int rowOffset = c1 < 160 ? 112 : 176;
  int cellOffset = adjust ? (c2 > 127 ? 32 : 31) : 126;

  *p1 = ((c1 - rowOffset) << 1) - adjust;
  *p2 -= cellOffset;
}

/* ---------------------------------------------- SJIS 半角を全角に変換 -- */
#define HANKATA(a)  (a >= 161 && a <= 223)
#define ISMARU(a)   (a >= 202 && a <= 206)
#define ISNIGORI(a) ((a >= 182 && a <= 196) || (a >= 202 && a <= 206) || (a == 179))

static int stable[][2] = {
    {129,66},{129,117},{129,118},{129,65},{129,69},{131,146},{131,64},
    {131,66},{131,68},{131,70},{131,72},{131,131},{131,133},{131,135},
    {131,98},{129,91},{131,65},{131,67},{131,69},{131,71},{131,73},
    {131,74},{131,76},{131,78},{131,80},{131,82},{131,84},{131,86},
    {131,88},{131,90},{131,92},{131,94},{131,96},{131,99},{131,101},
    {131,103},{131,105},{131,106},{131,107},{131,108},{131,109},
    {131,110},{131,113},{131,116},{131,119},{131,122},{131,125},
    {131,126},{131,128},{131,129},{131,130},{131,132},{131,134},
    {131,136},{131,137},{131,138},{131,139},{131,140},{131,141},
    {131,143},{131,147},{129,74},{129,75}};

static unsigned char *_sjis_han2zen(unsigned char *str, int *p1, int *p2)
{
  register int c1, c2;

  c1 = (int)*str; str++;
  *p1 = stable[c1 - 161][0];
  *p2 = stable[c1 - 161][1];

  /* 濁音、半濁音の処理 */
  c2 = (int)*str;
  if (c2 == 222 && ISNIGORI(c1)) {
    if ((*p2 >= 74 && *p2 <= 103) || (*p2 >= 110 && *p2 <= 122))
      (*p2)++;
    else if (*p1 == 131 && *p2 == 69)
      *p2 = 148;
    str++;
  }

  if (c2 == 223 && ISMARU(c1) && (*p2 >= 110 && *p2 <= 122) ) {
    *p2 += 2;
    str++;
  }
  return str++;
}

/* -------------------------------------------------- SJIS コードを変換 -- */

#define SJIS1(A)    ((A >= 129 && A <= 159) || (A >= 224 && A <= 239))
#define SJIS2(A)    (A >= 64 && A <= 252)

static void _shift2seven(unsigned char *str, unsigned char *str2)
{
  int p1,p2,esc_in = FALSE;

  while ((p1 = (int)*str) != '\0') {

    if (SJIS1(p1)) {
      if((p2 = (int)*(++str)) == '\0') break;
      if (SJIS2(p2)) {
        _sjis_shift(&p1,&p2);
        if (!esc_in) {
          esc_in = TRUE;
          str2 = _to_jis(str2);
        }
      }
      CHAROUT(p1);
      CHAROUT(p2);
      str++;
      continue;
    }

#ifdef NO_HANKAKU_SJIS
    /* 半角 SJIS は、強制的に全角に変える */
    if (HANKATA(p1)) {
      str = _sjis_han2zen(str, &p1, &p2);
      _sjis_shift(&p1,&p2);
      if (!esc_in) {
        esc_in = TRUE;
        str2 = _to_jis(str2);
      }
      CHAROUT(p1);
      CHAROUT(p2);
      continue;
    }
#endif

    if (esc_in) {
      /* LF / CR の場合は、正常にエスケープアウトされる */
      esc_in = FALSE;
      str2 = _to_ascii(str2);
    }
    CHAROUT(p1);
    str++;
  }

  if (esc_in)
    str2 = _to_ascii(str2);
  *str2='\0';
}

/* --------------------------------------------- SJIS を EUC に変換する -- */

static void _shift2euc(unsigned char *str, unsigned char *str2)
{
  int p1,p2;
  
  while ((p1 = (int)*str) != '\0') {
    if (SJIS1(p1)) {
      if((p2 = (int)*(++str)) == '\0') break;
      if (SJIS2(p2)) {
        _sjis_shift(&p1,&p2);
        p1 += 128;
        p2 += 128;
      }
      CHAROUT(p1);
      CHAROUT(p2);
      str++;
      continue;
    }

#ifdef NO_HANKAKU_SJIS
    /* 半角 SJIS は、強制的に全角に変える */
    if (HANKATA(p1)) {
      str = _sjis_han2zen(str,&p1,&p2);
      _sjis_shift(&p1,&p2);
      p1 += 128;
      p2 += 128;
      CHAROUT(p1);
      CHAROUT(p2);
      continue;
    }
#endif
    CHAROUT(p1);
    str++;
  }
  *str2='\0';
}

/* ------------------------------------------------- 半角 SJIS を取り除く -- */

static void _shift_self(unsigned char *str, unsigned char *str2)
{
  int p1;
  
  while ((p1 = (int)*str) != '\0') {
#ifdef NO_HANKAKU_SJIS
    /* 半角 SJIS は、強制的に全角に変える */
    if (HANKATA(p1)) {
      str = _sjis_han2zen(str, &p1, &p2);
      CHAROUT(p1);
      CHAROUT(p2);
      continue;
    }
#endif
    CHAROUT(p1);
    str++;
  }
  *str2='\0';
}

/* ------------------------------------------------------EUC から JIS へ -- */

#define ISEUC(A)    (A >= 161 && A <= 254)

static void _euc2seven(unsigned char *str, unsigned char *str2)
{
  int p1, p2, esc_in = FALSE;

  while ((p1 = (int)*str) != '\0') {

    if (p1 == LF || p1 == CR) {
      if (esc_in) {
        esc_in = FALSE;
        str2 = _to_ascii(str2);
      }
      CHAROUT(p1);
      str++;
      continue;
    }

    if (ISEUC(p1)) {
      if((p2 = (int)*(++str)) == '\0') break;
      if (ISEUC(p2)) {

	if (!esc_in) {
	  esc_in = TRUE;
	  str2 =_to_jis(str2);
	}

	CHAROUT(p1-128);
	CHAROUT(p2-128);
	str++;
	continue;
      }
    }

    if (esc_in) {
      esc_in = FALSE;
      str2 = _to_ascii(str2);
    }
    CHAROUT(p1);
    str++;
  }
  *str2='\0';
}

/* ------------------------------------------------ EUC から SJIS に変換 -- */
 
static void _euc2shift(unsigned char *str, unsigned char *str2)
{
  int p1,p2;

  while ((p1 = (int)*str) != '\0') {
    if (ISEUC(p1)) {
      if((p2 = (int)*(++str)) == '\0') break;
      if (ISEUC(p2)) {
	p1 -= 128;
        p2 -= 128;
        _jis_shift(&p1,&p2);
      }
      CHAROUT(p1);
      CHAROUT(p2);
      str++;
      continue;
    }

    CHAROUT(p1);
    str++;
  }
  *str2='\0';
}

/* -------------------------------------- ESC シーケンスをスキップする ----- */

static unsigned char *_skip_esc(unsigned char *str, int *esc_in) {
  int c;
  
  c = (int)*(++str);
  if ((c == '$') || (c == '(')) str++;
  if ((c == 'K') || (c == '$')) *esc_in = TRUE;
  else *esc_in = FALSE;

  if(*str != '\0') str++;
  return str;
}


/* ----------------------------------------------- JIS を SJIS に変換する -- */

static void _seven2shift(unsigned char *str, unsigned char *str2)
{
  int p1, p2, esc_in = FALSE;

  while ((p1 = (int)*str) != '\0') {

    /* ESCシーケンスをスキップする */
    if (p1 == ESC) {
      str = _skip_esc(str, &esc_in);
      continue;
    }

    if (p1 == LF || p1 == CR) {
      if (esc_in) esc_in = FALSE;
    }

    if(esc_in) { /* ISO-2022-JP コード */
      if((p2 = (int)*(++str)) == '\0') break;

      _jis_shift(&p1, &p2);

      CHAROUT(p1);
      CHAROUT(p2);
    }else{       /* ASCII コード */
      CHAROUT(p1);
    }
    str++;
  }
  *str2 = '\0';
}

/* ------------------------------------------------ JIS を EUC に変換する -- */

static void _seven2euc(unsigned char *str, unsigned char *str2)
{
  int p1, esc_in = FALSE;

  while ((p1 = (int)*str) != '\0') {

    /* ESCシーケンスをスキップする */
    if (p1 == ESC) {
      str = _skip_esc(str, &esc_in);
      continue;
    }

    if (p1 == LF || p1 == CR) {
      if (esc_in) esc_in = FALSE;
    }

    if(esc_in) { /* ISO-2022-JP コード */
      CHAROUT(p1 + 128); 
      
      if((p1 = (int)*(++str)) == '\0') break;
      CHAROUT(p1 + 128);
    }else{       /* ASCII コード */
      CHAROUT(p1);
    }
    str++;
  }
  *str2 = '\0';
}

/* ------------------------------------------------------------------------ */
/* --------------------------------------------------------- Public 関数 -- */
char *toStringJIS(char *str, char *buf, int maxlen) {
  int detected;

  if(!str) return (NULL);
  detected = detectKanjiCode(str);
  if(detected == ASCII || detected == JIS)
    return strncpy(buf, str, maxlen);

  if (maxlen < strlen(str) * 2) return NULL;

  switch(detected) {
  case SJIS :
    _shift2seven((unsigned char *)str, (unsigned char *)buf);
    break;
  case EUC :
    _euc2seven((unsigned char *)str, (unsigned char *)buf);
    break;
  default:
    return strncpy(buf, str, maxlen);
    break;
  }
  return buf;
}

char *toStringEUC(char *str, char *buf, int maxlen) {
  int detected;

  if(!str) return (NULL);
  detected = detectKanjiCode(str);
  if(detected == ASCII || detected == EUC) 
    return strncpy(buf, str, maxlen);

  if (maxlen < strlen(str) * 2) return NULL;

  switch(detected) {
  case SJIS :
    _shift2euc((unsigned char *)str, (unsigned char *)buf);
    break;
  case JIS :
  case NEW : case OLD : case NEC :
     _seven2euc((unsigned char *)str, (unsigned char*)buf);
    break;
  default:
    return strncpy(buf, str, maxlen);
    break;
  }
  return buf;
}

char *toStringSJIS(char *str, char *buf, int maxlen) {
  int detected;

  if (!str) return NULL;
  detected = detectKanjiCode(str);
  if(detected == ASCII)
    return strncpy(buf, str, maxlen);
  
  if (maxlen < strlen(str) * 2) return NULL;

  switch(detected) {
  case NEW : case OLD : case NEC :
  case JIS :
    _seven2shift((unsigned char *)str, (unsigned char *)buf);
    break;
  case EUC :
    _euc2shift((unsigned char *)str, (unsigned char *)buf);
    break;
  case SJIS :  
  default:
    _shift_self((unsigned char *)str, (unsigned char *)buf);
  }
  return buf;
}

char *toStringAuto(char *str, char *buf, int maxlen) {
  static int  jpcode = -1;
  static char *sjis_locale_name[] = {SJIS_LOCALE_NAME, NULL};
  static char *jis_locale_name[]  = {JIS_LOCALE_NAME, NULL};
  static char *euc_locale_name[]  = {EUC_LOCALE_NAME, NULL};
  static struct LOCALETABLE {
    int code;
    char **name_list;
  } locale_table[] = { {SJIS, sjis_locale_name},
		     {EUC, euc_locale_name},
		     {JIS, jis_locale_name}};

  if(!str) return (NULL);

  if (jpcode == -1) {
    char *ctype = setlocale(LC_CTYPE, "");
    int i, j;
    for( j=0; jpcode == -1 && 
	      j < sizeof(locale_table)/sizeof(struct LOCALETABLE); j++ ) {
      char **name = locale_table[j].name_list;
      for( i=0; name[i]; i++ )
	if (strcasecmp(ctype, name[i]) == 0) {
	  jpcode = locale_table[j].code;
	  break;
	}
    }
    if(jpcode == -1)
        jpcode = ASCII;
  }

  switch (jpcode) {
    case SJIS:
      return (toStringSJIS(str, buf, maxlen));
    break;
    case JIS:
    case NEW : case OLD : case NEC :
      return (toStringJIS(str, buf, maxlen));
    break;
    case EUC:
      return (toStringEUC(str, buf, maxlen));
    break;
    default:
      return (strncpy(buf, str, maxlen));
    break;
  }
}

char *EUCtoSJIS(char *str, char *buf, int maxlen)
{
  if (!str) return NULL;
  _euc2shift((unsigned char *)str, (unsigned char *)buf);
  return buf;
}
