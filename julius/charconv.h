/* charconv.c */
#include <stdarg.h>

void charconv_add_option();
boolean charconv_setup();

#ifdef CHARACTER_CONVERSION

char *charconv(char *instr, char *outstr, int maxoutlen);
#ifdef HAVE_ICONV
boolean charconv_iconv_setup(char *fromcode, char *tocode, boolean *enabled);
char *charconv_iconv(char *instr, char *outstr, int maxoutlen);
#endif
#ifdef USE_WIN32_MULTIBYTE
boolean charconv_win32_setup(char *fromcode, char *tocode, boolean *enabled);
char *charconv_win32(char *instr, char *outstr, int maxoutlen);
#endif
#ifdef USE_LIBJCODE
boolean charconv_libjcode_setup(char *fromcode, char *tocode, boolean *enabled);
char *charconv_libjcode(char *instr, char *outstr, int maxoutlen);
#endif

#endif /* CHARACTER_CONVERSION */

