/* Version string */
#define LIBSENT_VERSION "4.2.3"

/* Audio API name */
#define AUDIO_API_NAME "pa-dsound"

/* Audio API description */
#define AUDIO_API_DESC "PortAudio library (DirectSound)"

/* Description of supported audio file formats */
#define AUDIO_FORMAT_DESC "WAV and RAW"

/* Description of gzip file reading method */
#define GZIP_READING_DESC "zlib"

/* Define to empty if the keyword does not work on your compiler. */
//#undef const

/* Define if you have the ANSI C header files.   */
#define STDC_HEADERS

/* Define if your processor stores words with the most significant byte first (like Motorola and SPARC, unlike Intel and VAX).   */
#undef WORDS_BIGENDIAN

/* Define if use microphone input  */
#define USE_MIC

/* Define if use Datlink/Netaudio input  */
#undef USE_NETAUDIO

/* Define if libsndfile support is available */
#undef HAVE_LIBSNDFILE

/* Define for libsndfile support (ver.1)  */
#undef HAVE_LIBSNDFILE_VER1

/* Define if you have spaudio library  */
#undef USE_SPLIB

/* Define if you use integer word WORD_ID (for vocaburary of over 60k)  */
#undef WORDS_INT

/* Define if you prefer addlog array function */
#define USE_ADDLOG_ARRAY

/* Define if you use zcat command for compressed file input */
#undef ZCAT

/* Define if you have socklen definition */
#undef HAVE_SOCKLEN_T

/* Define if you have the <sndfile.h> header file.   */
#undef HAVE_SNDFILE_H

/* Define if you have the <unistd.h> header file.   */
#undef HAVE_UNISTD_H

/* Define if you have the nsl library (-lnsl).   */
#undef HAVE_LIBNSL

/* Define if you have the socket library (-lsocket).   */
#undef HAVE_LIBSOCKET

/* Define if you have zlib library (-lz).  */
#define HAVE_ZLIB

/* Define if you have strcasecmp function  */
#undef HAVE_STRCASECMP

/* Define if you have sleep function  */
#undef HAVE_SLEEP

/* Define if you have iconv function */
#undef HAVE_ICONV

/* Define if you use libjcode */
#undef USE_LIBJCODE

/* Define if you allow class N-gram  */
#define CLASS_NGRAM

/* Define if you want to enable process forking for each adinnet connection */
#undef FORK_ADINNET

/* Define if use sin/cos table for MFCC calculation  */
#define MFCC_SINCOS_TABLE

/* Define if <sys/soundcard.h> found */
#undef HAVE_SYS_SOUNDCARD_H

/* Define if <machine/soundcard.h> found */
#undef HAVE_MACHINE_SOUNDCARD_H

/* Define if <alsa/asoundlib.h> found */
#undef HAVE_ALSA_ASOUNDLIB_H

/* Define if <sys/asoundlib.h> exist and <alsa/asoundlib.h> not exist */
#undef HAVE_SYS_ASOUNDLIB_H

/* Define if <esd.h> exist  */
#undef HAVE_ESD_H

/* Define if <pulse/simple.h> exist  */
#undef HAVE_PULSE_SIMPLE_H

/* Define if enable alsa */
#undef HAS_ALSA

/* Define if enable oss */
#undef HAS_OSS

/* Define if enable pulseaudio */
#undef HAS_PULSEAUDIO

/* Define if enable esd */
#undef HAS_ESD

/* Define if MSD-HMM support is enabled */
#undef ENABLE_MSD

/* Define if MBR support is enabled */
#undef USE_MBR
