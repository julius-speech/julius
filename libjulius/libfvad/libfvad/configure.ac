AC_INIT([libfvad],[1.0],[dpirch@gmail.com],[],[https://github.com/dpirch/libfvad])
AC_PREREQ([2.60]) # for _AC_C_STD_TRY
AC_CONFIG_SRCDIR([src/fvad.c])
AC_CONFIG_AUX_DIR([ac-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([
    -Wall -Wno-extra-portability foreign subdir-objects
    tar-pax no-dist-gzip dist-xz
])

AC_PROG_CC

# Try to make the compiler support C11 (and C99). Necessary for older versions
# of gcc and clang (e.g. gcc 4.8) that default to C89.
# Temporary solution until autoconf 2.70, when we can use AC_PROG_CC for this.
_AC_C_STD_TRY(
    [c11],
    [#include <assert.h>],
    [static_assert(1, "foo"); for (int i = 0; i < 1; i++) ],
    [-std=gnu11 -std=c11],
    [],
    [AC_MSG_ERROR([compiler does not support C11])]
)

LT_INIT([disable-static pic-only])

# optional examples, requires libsndfile
AC_ARG_ENABLE([examples],
    [AS_HELP_STRING([--enable-examples],
        [build the fvadwav example program; requires libsndfile @<:@default=no@:>@])])

AS_IF([test "x$enable_examples" = xyes],
    [PKG_CHECK_MODULES([sndfile], [sndfile])],
    [enable_examples=no])

AM_CONDITIONAL([BUILD_EXAMPLES], [test "x$enable_examples" = xyes])

### output ###
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile examples/Makefile tests/Makefile libfvad.pc])
AC_OUTPUT

AC_MSG_RESULT([
    $PACKAGE $VERSION
    ===========

    source code location:   $srcdir
    installation prefix:    $prefix
    compiler:               $CC
    cflags:                 $CFLAGS

    build examples:         $enable_examples
])
