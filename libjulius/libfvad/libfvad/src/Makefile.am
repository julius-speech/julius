
# internal library that exports all symbols, needed for unit tests
noinst_LTLIBRARIES = libfvad_int.la
libfvad_int_la_SOURCES = \
	signal_processing/energy.c \
	signal_processing/division_operations.c \
	signal_processing/get_scaling_square.c \
	signal_processing/resample_48khz.c \
	signal_processing/resample_by_2_internal.h \
	signal_processing/resample_by_2_internal.c \
	signal_processing/resample_fractional.c \
	signal_processing/signal_processing_library.h \
	signal_processing/spl_inl.h \
	signal_processing/spl_inl.c \
	vad/vad_core.h \
	vad/vad_core.c \
	vad/vad_filterbank.h \
	vad/vad_filterbank.c \
	vad/vad_gmm.h \
	vad/vad_gmm.c \
	vad/vad_sp.h \
	vad/vad_sp.c \
	fvad.c \
	common.h

# main public libary
lib_LTLIBRARIES = libfvad.la
libfvad_la_SOURCES =
libfvad_la_LDFLAGS = -version-info 0:0:0 -export-symbols-regex '^fvad_' -no-undefined
libfvad_la_LIBADD = libfvad_int.la






