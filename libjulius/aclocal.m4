# generated automatically by aclocal 1.15 -*- Autoconf -*-

# Copyright (C) 1996-2014 Free Software Foundation, Inc.

# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

m4_ifndef([AC_CONFIG_MACRO_DIRS], [m4_defun([_AM_CONFIG_MACRO_DIRS], [])m4_defun([AC_CONFIG_MACRO_DIRS], [_AM_CONFIG_MACRO_DIRS($@)])])
# Copyright (C) 2011-2014 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# AM_PROG_AR([ACT-IF-FAIL])
# -------------------------
# Try to determine the archiver interface, and trigger the ar-lib wrapper
# if it is needed.  If the detection of archiver interface fails, run
# ACT-IF-FAIL (default is to abort configure with a proper error message).
AC_DEFUN([AM_PROG_AR],
[AC_BEFORE([$0], [LT_INIT])dnl
AC_BEFORE([$0], [AC_PROG_LIBTOOL])dnl
AC_REQUIRE([AM_AUX_DIR_EXPAND])dnl
AC_REQUIRE_AUX_FILE([ar-lib])dnl
AC_CHECK_TOOLS([AR], [ar lib "link -lib"], [false])
: ${AR=ar}

AC_CACHE_CHECK([the archiver ($AR) interface], [am_cv_ar_interface],
  [AC_LANG_PUSH([C])
   am_cv_ar_interface=ar
   AC_COMPILE_IFELSE([AC_LANG_SOURCE([[int some_variable = 0;]])],
     [am_ar_try='$AR cru libconftest.a conftest.$ac_objext >&AS_MESSAGE_LOG_FD'
      AC_TRY_EVAL([am_ar_try])
      if test "$ac_status" -eq 0; then
        am_cv_ar_interface=ar
      else
        am_ar_try='$AR -NOLOGO -OUT:conftest.lib conftest.$ac_objext >&AS_MESSAGE_LOG_FD'
        AC_TRY_EVAL([am_ar_try])
        if test "$ac_status" -eq 0; then
          am_cv_ar_interface=lib
        else
          am_cv_ar_interface=unknown
        fi
      fi
      rm -f conftest.lib libconftest.a
     ])
   AC_LANG_POP([C])])

case $am_cv_ar_interface in
ar)
  ;;
lib)
  # Microsoft lib, so override with the ar-lib wrapper script.
  # FIXME: It is wrong to rewrite AR.
  # But if we don't then we get into trouble of one sort or another.
  # A longer-term fix would be to have automake use am__AR in this case,
  # and then we could set am__AR="$am_aux_dir/ar-lib \$(AR)" or something
  # similar.
  AR="$am_aux_dir/ar-lib $AR"
  ;;
unknown)
  m4_default([$1],
             [AC_MSG_ERROR([could not determine $AR interface])])
  ;;
esac
AC_SUBST([AR])dnl
])

# AM_AUX_DIR_EXPAND                                         -*- Autoconf -*-

# Copyright (C) 2001-2014 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# For projects using AC_CONFIG_AUX_DIR([foo]), Autoconf sets
# $ac_aux_dir to '$srcdir/foo'.  In other projects, it is set to
# '$srcdir', '$srcdir/..', or '$srcdir/../..'.
#
# Of course, Automake must honor this variable whenever it calls a
# tool from the auxiliary directory.  The problem is that $srcdir (and
# therefore $ac_aux_dir as well) can be either absolute or relative,
# depending on how configure is run.  This is pretty annoying, since
# it makes $ac_aux_dir quite unusable in subdirectories: in the top
# source directory, any form will work fine, but in subdirectories a
# relative path needs to be adjusted first.
#
# $ac_aux_dir/missing
#    fails when called from a subdirectory if $ac_aux_dir is relative
# $top_srcdir/$ac_aux_dir/missing
#    fails if $ac_aux_dir is absolute,
#    fails when called from a subdirectory in a VPATH build with
#          a relative $ac_aux_dir
#
# The reason of the latter failure is that $top_srcdir and $ac_aux_dir
# are both prefixed by $srcdir.  In an in-source build this is usually
# harmless because $srcdir is '.', but things will broke when you
# start a VPATH build or use an absolute $srcdir.
#
# So we could use something similar to $top_srcdir/$ac_aux_dir/missing,
# iff we strip the leading $srcdir from $ac_aux_dir.  That would be:
#   am_aux_dir='\$(top_srcdir)/'`expr "$ac_aux_dir" : "$srcdir//*\(.*\)"`
# and then we would define $MISSING as
#   MISSING="\${SHELL} $am_aux_dir/missing"
# This will work as long as MISSING is not called from configure, because
# unfortunately $(top_srcdir) has no meaning in configure.
# However there are other variables, like CC, which are often used in
# configure, and could therefore not use this "fixed" $ac_aux_dir.
#
# Another solution, used here, is to always expand $ac_aux_dir to an
# absolute PATH.  The drawback is that using absolute paths prevent a
# configured tree to be moved without reconfiguration.

AC_DEFUN([AM_AUX_DIR_EXPAND],
[AC_REQUIRE([AC_CONFIG_AUX_DIR_DEFAULT])dnl
# Expand $ac_aux_dir to an absolute path.
am_aux_dir=`cd "$ac_aux_dir" && pwd`
])

