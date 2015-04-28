#ifndef __CHARCONV_H__
#define __CHARCONV_H__

int charconv_setup(char *fromcode, char *tocode);
char *charconv(char *instr, char *outstr, int maxoutlen);

#endif /* __CHARCONV_H__ */
