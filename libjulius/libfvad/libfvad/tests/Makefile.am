
check_LTLIBRARIES = libtest.la
libtest_la_SOURCES = test_common.h test_common.c
libtest_la_LIBADD = ../src/libfvad_int.la

noinst_HEADERS = vad_unittest.h

check_PROGRAMS = \
	spl_macro_unittest \
	spl_inline_unittest \
	spl_leading_zeros_unittest \
	spl_math_operations_unittest \
	spl_signal_processing_unittest \
	spl_resample_48_unittest \
	vad_core_init_unittest \
	vad_core_set_mode_unittest \
	vad_core_calc_vad_unittest \
	vad_filterbank_unittest \
	vad_gmm_unittest \
	vad_sp_unittest \
	vad_api_unittest


TESTS = $(check_PROGRAMS) wavtest.sh
CLEANFILES = wavtest.result
EXTRA_DIST = data wavtest.sh

LDADD = libtest.la

spl_macro_unittest_SOURCES = signal_processing_unittest.c
spl_macro_unittest_CPPFLAGS = -DTEST_SPL_MACRO

spl_inline_unittest_SOURCES = signal_processing_unittest.c
spl_inline_unittest_CPPFLAGS = -DTEST_SPL_INLINE

spl_math_operations_unittest_SOURCES = signal_processing_unittest.c
spl_math_operations_unittest_CPPFLAGS = -DTEST_SPL_MATH_OPERATIONS

spl_leading_zeros_unittest_SOURCES = signal_processing_unittest.c
spl_leading_zeros_unittest_CPPFLAGS = -DTEST_SPL_LEADING_ZEROS

spl_signal_processing_unittest_SOURCES = signal_processing_unittest.c
spl_signal_processing_unittest_CPPFLAGS = -DTEST_SPL_SIGNAL_PROCESSING

spl_resample_48_unittest_SOURCES = signal_processing_unittest.c
spl_resample_48_unittest_CPPFLAGS = -DTEST_SPL_RESAMPLE_48

vad_core_init_unittest_SOURCES = vad_core_unittest.c vad_unittest.c
vad_core_init_unittest_CPPFLAGS = -DTEST_VAD_CORE_INIT

vad_core_set_mode_unittest_SOURCES = vad_core_unittest.c vad_unittest.c
vad_core_set_mode_unittest_CPPFLAGS = -DTEST_VAD_CORE_SET_MODE

vad_core_calc_vad_unittest_SOURCES = vad_core_unittest.c vad_unittest.c
vad_core_calc_vad_unittest_CPPFLAGS = -DTEST_VAD_CORE_CALC_VAD

vad_filterbank_unittest_SOURCES = vad_filterbank_unittest.c vad_unittest.c

vad_api_unittest_SOURCES = vad_unittest.c
vad_api_unittest_CPPFLAGS = -DTEST_VAD_API
