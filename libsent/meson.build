project('libsent','c', version: '4.5')

conf = configuration_data()

mictype = get_option('mictype')

deps = []
sources = []

if mictype =='alsa'
	deps += dependency('alsa')
	conf.set('HAS_ALSA', 1)
	sources += 'src/adin/adin_mic_linux.c'
	sources += 'src/adin/adin_mic_linux_alsa.c'
	aldesc='Advanced Linux Sound Architecture'
elif mictype =='pulseaudio'
	deps += dependency('libpulse')
	conf.set('HAS_PULSE',1)
	sources += 'src/adin/adin_mic_linux.c'
	sources += 'src/adin/adin_pulseaudio.c'
	aldesc='PulseAudio'
elif mictype == 'oss'
	deps += dependency('oss')
	conf.set('HAS_OSS')
	sources += 'src/adin/adin_mic_linux.c'
	sources += 'src/adin/adin_mic_linux_oss.c'
	aldesc='Open Sound System compatible'
elif mictype == 'esd'
	deps += dependency('esd')
	conf.set('HAS_ESD',1)
	sources += 'src/adin/adin_mic_linux.c'
	sources += 'endifsrc/adin/adin_esd.o'
	aldesc='EsoundD - The Enlightened Sound Daemon'
endif

conf.set('AUDIO_API_NAME','"'+mictype+'"')
conf.set('AUDIO_API_DESC','"'+aldesc+'"')


if not get_option('disable-addarray')
	conf.set('USE_ADDLOG_ARRAY',1)
endif

if get_option('words-int')
	conf.set('WORDS_INT',1)
endif

if get_option('class-ngram')
	conf.set('CLASS_NGRAM',1)
endif

if get_option('fork')
	conf.set('FORK_ADINNET',1)
endif

if not get_option('disable-mfcc-table')
	conf.set('MFCC_SINCOS_TABLE',1)
endif

if get_option('msd')
	conf.set('ENABLE_MSD',1)
endif

if not get_option('disable-mbr')
	conf.set('USE_MBR',1)
endif

if not get_option('without-sndfile')
	sndfile=dependency('sndfile', version:'>=1', required:false)
	if (sndfile.found())
		conf.set('HAVE_LIBSNDFILE',1)
		conf.set('HAVE_LIBSNDFILE_VER1',1)
		wavefile_support='various formats by libsndfile ver >= 1'
	else
		sndfile=dependency('sndfile', required:true)	
		conf.set('HAVE_LIBSNDFILE',1)
		wavefile_support='various formats by libsndfile ver.0'
	endif
	deps += sndfile
else
	wavefile_support='RAW and WAV only'
endif

conf.set('AUDIO_FORMAT_DESC', '"' + wavefile_support + '"')

if not get_option('disable-zlib')
	deps+=dependency('zlib')
	conf.set('HAVE_ZLIB',1)
	gzdesc='zlib library'
else
	gzip = find_program('gzip', required : false)
	if gzip.found()
		gzdesc = 'gzip command'
		conf.set('ZCAT', 'gzip -d -c')
	else
		gzdesc = 'none'
		message('gzip not found, compressed file input disabled')
	endif
endif

conf.set('GZIP_READING_DESC', '"'+gzdesc+'"')

arch = host_machine.cpu_family()

compiler = meson.get_compiler('c')

has_x86_simd  = false
if arch == 'arm' and not get_option('disable-neon')

	neon_args=['-mfpu=neon','-mfloat-abi=softp']

	code = '''
		#include <arm_neon.h>
		int test() {
			float32x4_t v1, v2;
			float32x4_t x = vmlaq_f32(x, v1, v2);
		}
	'''

	has_simdv2=false
	if compiler.compiles(code, name : 'check for NEON SIMDv2 instruction')
		has_simdv2=true
	elif compiler.compiles(code, 
			       args:neon_args,
			       name : 'check for NEON SIMDv2 instruction (with -mfpu=neon -mfloat-abi=softp)')
		has_simdv2=true
		add_project_arguments(neon_args, language : 'c')
	endif

	code = '''
		#include <arm_neon.h>
		int test {
			float32x4_t x = vdupq_n_f32(0);
		}
	'''

	has_simd=false
	if compiler.compiles(code, name : 'check for NEON SIMD instruction')
		has_simd=true
	elif compiler.compiles(code, 
			       args:neon_args,
			       name : 'check for NEON SIMD instruction (with -mfpu=neon -mfloat-abi=softp)')
		has_simd=true
		add_project_arguments(neon_args, language : 'c')
	endif

	if has_simdv2
		conf.set('HAS_SIMD_NEONV2',1)
	endif

	if has_simd
		conf.set('HAS_SIMD_NEON',1)
	endif

elif arch == 'aarch64' and not get_option('disable-neon')
	# NEON support is implicit for AARCH64
	conf.set('HAS_SIMD_NEON',1)
	conf.set('HAS_SIMD_NEONV2',1)
elif arch == 'x86'
	has_x86_simd = true
elif arch == 'x86_64'
	has_x86_simd = true
endif

if has_x86_simd

	code = '''
		#include <immintrin.h>
		int test() {
	    		__m256 v1, v2;
			__m256 x = _mm256_fmadd_ps(v1, v2, x);
		}
	'''

	has_fma=false
	if compiler.compiles(code, name : 'check for FMA instruction')
		has_fma=true
	elif compiler.compiles(code, args:'-mfma', name : 'check for FMA instruction (with -mfma)')
		has_fma=true
		add_project_arguments('-mfma', language : 'c')
	endif

	code = '''
		#include <immintrin.h>
		int test() {
    			__m256 v = _mm256_setzero_ps();
		}
	'''

	has_avx=false
	if compiler.compiles(code, name : 'check for AVX instruction')
		has_avx=true
	elif compiler.compiles(code, args:'-mavx', name : 'check for AVX instruction (with -mavx)')
		has_avx=true
		add_project_arguments('-mavx', language : 'c')
	endif
	

	code = '''
		#include <immintrin.h>
		int test() {
			__m128 x = _mm_setzero_ps();
		}
	'''

	has_sse=false
	if compiler.compiles(code, name: 'check for SSE instruction')
		has_sse=true
	elif compiler.compiles(code, args:'-msse', name: 'check for SSE instruction (with -msse)')
		has_sse=true
		add_project_arguments('-msse', language : 'c')
	endif

	if (has_fma)
		conf.set('HAS_SIMD_FMA',1)
	endif
	if (has_avx)
		conf.set('HAS_SIMD_AVX',1)
	endif
	if (has_sse)
		conf.set('HAS_SIMD_SSE',1)
	endif
endif


conf.set('LIBSENT_VERSION', '"4.5"')
conf.set('USE_MIC', 1)

if compiler.has_function('strcasecmp', prefix : '#include <strings.h>')
	conf.set('HAVE_STRCASECMP',1)
endif

if compiler.has_header('alsa/asoundlib.h')
	conf.set('HAVE_ALSA_ASOUNDLIB_H', 1)
elif compiler.has_header('sys/asoundlib.h')
	conf.set('HAVE_SYS_ASOUNDLIB_H', 1)
endif

configure_file(
	output :'config.h', 
	configuration : conf,
	install_dir : join_paths(get_option('includedir'), 'sent')
)
	
builddir=meson.current_build_dir()
configure_file = '@0@/config.h'.format(builddir)
confdest=builddir + '/include/sent'
run_command('mkdir' ,'-p',  confdest)
run_command('cp', configure_file, confdest)

message('copied ' + configure_file + ' to ' + confdest)

sources += [
 'src/adin/adin_file.c',
 'src/adin/adin_sndfile.c',
 'src/adin/adin_tcpip.c',
 'src/adin/zc-e.c',
 'src/adin/zmean.c',
 'src/adin/ds48to16.c',
 'src/anlz/param_malloc.c',
 'src/anlz/rdparam.c',
 'src/anlz/paramselect.c',
 'src/anlz/paramtypes.c',
 'src/anlz/strip.c',
 'src/anlz/strip_mfcc.c',
 'src/anlz/wrsamp.c',
 'src/anlz/wrwav.c',
 'src/anlz/vecin_net.c',
 'src/dfa/init_dfa.c',
 'src/dfa/rddfa.c',
 'src/dfa/dfa_lookup.c',
 'src/dfa/cpair.c',
 'src/dfa/mkcpair.c',
 'src/dfa/mkterminfo.c',
 'src/dfa/dfa_util.c',
 'src/dfa/dfa_malloc.c',
 'src/hmminfo/rdhmmdef.c',
 'src/hmminfo/rdhmmdef_data.c',
 'src/hmminfo/rdhmmdef_mpdf.c',
 'src/hmminfo/rdhmmdef_dens.c',
 'src/hmminfo/rdhmmdef_options.c',
 'src/hmminfo/rdhmmdef_state.c',
 'src/hmminfo/rdhmmdef_trans.c',
 'src/hmminfo/rdhmmdef_var.c',
 'src/hmminfo/rdhmmdef_streamweight.c',
 'src/hmminfo/rdhmmdef_tiedmix.c',
 'src/hmminfo/rdhmmdef_regtree.c',
 'src/hmminfo/hmm_lookup.c',
 'src/hmminfo/check_hmm_restriction.c',
 'src/hmminfo/check_hmmtype.c',
 'src/hmminfo/put_htkdata_info.c',
 'src/hmminfo/rdhmmlist.c',
 'src/hmminfo/cdhmm.c',
 'src/hmminfo/guess_cdHMM.c',
 'src/hmminfo/cdset.c',
 'src/hmminfo/init_phmm.c',
 'src/hmminfo/chkhmmlist.c',
 'src/hmminfo/write_binhmm.c',
 'src/hmminfo/read_binhmm.c',
 'src/hmminfo/write_binhmmlist.c',
 'src/hmminfo/read_binhmmlist.c',
 'src/net/rdwt.c',
 'src/net/server-client.c',
 'src/ngram/init_ngram.c',
 'src/ngram/ngram_read_arpa.c',
 'src/ngram/ngram_read_bin.c',
 'src/ngram/ngram_write_arpa.c',
 'src/ngram/ngram_write_bin.c',
 'src/ngram/ngram_compact_context.c',
 'src/ngram/ngram_access.c',
 'src/ngram/ngram_lookup.c',
 'src/ngram/ngram_util.c',
 'src/ngram/ngram_malloc.c',
 'src/phmm/outprob_init.c',
 'src/phmm/outprob.c',
 'src/phmm/gms.c',
 'src/phmm/gms_gprune.c',
 'src/phmm/calc_mix.c',
 'src/phmm/calc_tied_mix.c',
 'src/phmm/gprune_common.c',
 'src/phmm/gprune_none.c',
 'src/phmm/gprune_safe.c',
 'src/phmm/gprune_heu.c',
 'src/phmm/gprune_beam.c',
 'src/phmm/addlog.c',
 'src/phmm/mkwhmm.c',
 'src/phmm/vsegment.c',
 'src/phmm/calc_dnn.c',
 'src/phmm/calc_dnn_fma.c',
 'src/phmm/calc_dnn_avx.c',
 'src/phmm/calc_dnn_sse.c',
 'src/phmm/calc_dnn_neonv2.c',
 'src/phmm/calc_dnn_neon.c',
 'src/util/endian.c',
 'src/util/jlog.c',
 'src/util/mymalloc.c',
 'src/util/mybmalloc.c',
 'src/util/ptree.c',
 'src/util/aptree.c',
 'src/util/readfile.c',
 'src/util/gzfile.c',
 'src/util/mystrtok.c',
 'src/util/strcasecmp.c',
 'src/util/confout.c',
 'src/util/qsort.c',
 'src/voca/init_voca.c',
 'src/voca/voca_load_htkdict.c',
 'src/voca/voca_load_wordlist.c',
 'src/voca/voca_lookup.c',
 'src/voca/voca_malloc.c',
 'src/voca/voca_util.c',
 'src/wav2mfcc/ss.c',
 'src/wav2mfcc/wav2mfcc-buffer.c',
 'src/wav2mfcc/wav2mfcc-pipe.c',
 'src/wav2mfcc/mfcc-core.c',
 'src/wav2mfcc/para.c'
]

# Set it as not required for portability
m_dep = compiler.find_library('m', required : false)
deps += m_dep

incdir = include_directories('include')

libsent = static_library('sent', sources, dependencies: deps, include_directories:incdir, install:true)

libsent_dep = declare_dependency(link_with:libsent, include_directories:incdir)

