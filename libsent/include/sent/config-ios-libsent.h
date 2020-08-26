#define LIBSENT_VERSION "4.6"
#define AUDIO_API_NAME ""
#define AUDIO_API_DESC ""
#define AUDIO_FORMAT_DESC ""
#define GZIP_READING_DESC ""
#define USE_MIC 1
#define USE_ADDLOG_ARRAY 1
#define HAVE_SOCKLEN_T 1
#define HAVE_UNISTD_H 1
#define HAVE_ZLIB 1
#define HAVE_STRCASECMP 1
#define HAVE_SLEEP 1
#define CLASS_NGRAM 1
#define MFCC_SINCOS_TABLE 1
#if defined(__FMA__)
#define HAS_SIMD_FMA
#elif defined(__AVX__)
#define HAS_SIMD_AVX
#elif defined(__SSE__)
#define HAS_SIMD_SSE
#endif
#ifdef __ARM_NEON__
#define HAS_SIMD_NEONV2
#endif
