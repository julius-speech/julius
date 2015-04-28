SHELL=/bin/sh

.SUFFIXES:
.SUFFIXES: .c .jpi
.c.jpi:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

CC=gcc
CFLAGS=-shared
LDFLAGS=
J_CFLAGS=-I../libsent/include -I../libjulius/include `../libsent/libsent-config --cflags` `../libjulius/libjulius-config --cflags`
J_LDFLAGS=-L../libjulius `../libjulius/libjulius-config --libs` -L../libsent `../libsent/libsent-config --libs`

all: adin_oss.jpi audio_postprocess.jpi fvin.jpi feature_postprocess.jpi calcmix.jpi generic_callback.jpi result.jpi 

calcmix.jpi: calcmix.c
	$(CC) $(CFLAGS) $(J_CFLAGS) -o calcmix.jpi $(LDFLAGS) $(J_LDFLAGS) calcmix.c

generic_callback.jpi: generic_callback.c
	$(CC) $(CFLAGS) $(J_CFLAGS) -o generic_callback.jpi generic_callback.c $(LDFLAGS) $(J_LDFLAGS)

clean:
	rm *.jpi
