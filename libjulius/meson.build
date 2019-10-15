project('libjulius', 'c', version:'4.5')

libsent_proj = subproject('libsent')

add_project_arguments(['-fPIC'], language : 'c')

compiler = meson.get_compiler('c')

deps = []
sources = [
  'src/recogmain.c',
  'src/instance.c',
  'src/default.c',
  'src/jfunc.c',
  'src/callback.c',
  'src/useropt.c',
  'src/m_usage.c',
  'src/m_options.c',
  'src/m_jconf.c',
  'src/m_chkparam.c',
  'src/m_info.c',
  'src/m_fusion.c',
  'src/hmm_check.c',
  'src/multi-gram.c',
  'src/gramlist.c',
  'src/wchmm.c',
  'src/wchmm_check.c',
  'src/m_adin.c',
  'src/adin-cut.c',
  'src/wav2mfcc.c',
  'src/beam.c',
  'src/pass1.c',
  'src/spsegment.c',
  'src/realtime-1stpass.c',
  'src/factoring_sub.c',
  'src/outprob_style.c',
  'src/backtrellis.c',
  'src/search_bestfirst_main.c',
  'src/search_bestfirst_v1.c',
  'src/search_bestfirst_v2.c',
  'src/ngram_decode.c',
  'src/dfa_decode.c',
  'src/graphout.c',
  'src/confnet.c',
  'src/mbr.c',
  'src/gmm.c',
  'src/word_align.c',
  'src/plugin.c',
  'src/version.c'
]

conf = configuration_data()

if not get_option('disable-pthread')
	conf.set('HAVE_PTHREAD',1)
	deps += dependency('threads')
endif

# enable inter-word context dependency handling on 1st pass
want_pass1_iwcd=get_option('iwcd1')

# enable 1-gram factoring
want_unigram_factoring = get_option('factor1')

if get_option('factor2')
	want_unigram_factoring=false
else
	want_unigram_factoring=true
endif

# enable strict inter-word context handling on 2nd pass (slow)
want_pass2_strict_iwcd = get_option('strict-iwcd2')

# linear tree separation for hi-freq words
want_lowmem2 = get_option('lowmem2')

# Easy setup for default options
setup=get_option('setup')
if setup=='standard'
	want_unigram_factoring=true
	want_pass1_iwcd=true
	want_pass2_strict_iwcd=true
	want_gprune_default='safe'
elif setup=='fast'
	want_unigram_factoring=true
	want_lowmem2=true
	want_pass1_iwcd=true
	want_gprune_default='beam'
elif setup=='v2.1'
	want_gprune_default='safe'
endif


# no linear tree separation
if get_option('lowmem')
	conf.set('LOWMEM',1)
endif

# use monophone tree on 1st pass
if get_option('monotree')
	conf.set('MONOTREE',1)
endif

# enable score envelope beaming on 2nd pass forward scan
if not get_option('disable-score-beam')
	conf.set('SCAN_BEAM',1)
endif

if get_option('wpair')
	conf.set('WPAIR',1)
endif

if get_option('wpair-nlimit')
	conf.set('WPAIR',1)
	conf.set('WPAIR_KEEP_NLIMIT',1)
endif

if get_option('word-graph')
	conf.set('WORD_GRAPH',1)
endif

use_cm = not get_option('disable-cm')
use_cm_nbest = get_option('cm-nbest')
use_cm_multiple_alpha = get_option('cm-multiple-alpha')

if get_option('lmfix')
	conf.set('LM_FIX_DOUBLE_SCORING',1)
endif

use_graphout_nbest = get_option('graphout-nbest')
use_cmthres = get_option('cmthres')


if get_option('gmm-vad')
	conf.set('GMM_VAD',1)
	conf.set('BACKEND_VAD',1)
endif

if get_option('decoder-vad')
	conf.set('SPSEGMENT_NAIST',1)
	conf.set('BACKEND_VAD',1)
endif

if get_option('power-reject')
	conf.set('POWER_REJECT',1)
endif

if not get_option('disable-plugin')
	conf.set('ENABLE_PLUGIN',1)
	dl_dep = compiler.find_library('dl', required:false)
	deps += dl_dep
endif

if not get_option('disable-libfvad')
	sources += [ 
	'libfvad/libfvad/src/fvad.c',
	'libfvad/libfvad/src/signal_processing/division_operations.c',
	'libfvad/libfvad/src/signal_processing/energy.c',
	'libfvad/libfvad/src/signal_processing/get_scaling_square.c',
	'libfvad/libfvad/src/signal_processing/resample_48khz.c',
	'libfvad/libfvad/src/signal_processing/resample_by_2_internal.c',
	'libfvad/libfvad/src/signal_processing/resample_fractional.c',
	'libfvad/libfvad/src/signal_processing/spl_inl.c',
	'libfvad/libfvad/src/vad/vad_core.c',
	'libfvad/libfvad/src/vad/vad_filterbank.c',
	'libfvad/libfvad/src/vad/vad_gmm.c',
	'libfvad/libfvad/src/vad/vad_sp.c'
]
	conf.set('HAVE_LIBFVAD',1)
endif

if use_cm
	conf.set('CONFIDENCE_MEASURE',1)
endif
if use_cm_nbest
	if use_cm
		conf.set('CM_NBEST',1)
	else
		error('--disable-cm and --enable-cm-nbest conflicts')
	endif
endif

if use_cm_multiple_alpha 
	if use_cm
		conf.set('CM_MULTIPLE_ALPHA',1)
	else
		error('--disable-cm and --enable-cm-multiple-alpha conflicts')
	endif
endif

if use_cmthres
	if use_cm
		if use_cm_nbest
			error('--enable-cmthres cannot be used with --enable-cm-nbest')
		elif use_cm_multiple_alpha
			error('--enable-cmthres cannot be used with --enable-cm-multiple-alpha')
		else
			conf.set('CM_SEARCH_LIMIT',1)
		endif
	else
		error('--disable-cm and --enable-cmthres conflicts')
	endif

endif

if not use_graphout_nbest
	conf.set('GRAPHOUT_DYNAMIC',1)
	conf.set('GRAPHOUT_SEARCH',1)
endif

if want_unigram_factoring
	conf.set('UNIGRAM_FACTORING',1)
endif

if want_lowmem2
	conf.set('LOWMEM2',1)
endif

if want_pass1_iwcd
	conf.set('PASS1_IWCD',1)
endif

if want_pass2_strict_iwcd
	conf.set('PASS2_STRICT_IWCD', 1)
endif

if want_gprune_default == 'safe'
	conf.set('GPRUNE_DEFAULT_SAFE',1)
elif want_gprune_default == 'beam'
	conf.set('GPRUNE_DEFAULT_BEAM',1)
endif

julius_major_version='4'
julius_minor_version='5'

conf.set('JULIUS_PRODUCTNAME','"JuliusLib"')
conf.set('JULIUS_MAJOR_VERSION',julius_major_version)
conf.set('JULIUS_MINOR_VERSION',julius_minor_version)
conf.set('JULIUS_VERSION','"' + julius_major_version + '.' + julius_minor_version + '"')
conf.set('JULIUS_SETUP','"' + setup + '"')
conf.set('JULIUS_HOSTINFO', '"' + host_machine.system() + '"')
conf.set('JULIUS_BUILD_INFO', '""')

configure_file(output :'config.h', configuration : conf)

builddir=meson.current_build_dir()
configure_file = '@0@/config.h'.format(builddir)
confdest=builddir + '/include/julius'
run_command('mkdir' ,'-p',  confdest)
run_command('cp', configure_file, confdest)


incdir = include_directories('include')

libsent_dep = libsent_proj.get_variable('libsent_dep')

libjulius=static_library('julius', sources, dependencies:[deps, libsent_dep],  include_directories:incdir, install:true)
install_headers('include/julius/julius.h',subdir:'julius')

libjulius_dep = declare_dependency(link_with:libjulius, dependencies:[deps,libsent_dep],  include_directories:incdir)

