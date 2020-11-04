#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(timer, 1.0, zhangming)
AM_INIT_AUTOMAKE(timer,1.0)
AC_CONFIG_SRCDIR([timer.c])
AC_CONFIG_HEADERS([config.h])
AC_PROG_RANLIB

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([limits.h locale.h stddef.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([localeconv memmove memset strchr])

#AC_CONFIG_FILES([Makefile])
AC_OUTPUT(Makefile)
