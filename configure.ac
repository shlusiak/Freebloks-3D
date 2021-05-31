AC_INIT
AC_CONFIG_SRCDIR([src/freebloks.cpp])
AM_INIT_AUTOMAKE(freebloks, 1.5)

AM_CONFIG_HEADER([config.h])

AC_PROG_CXX
AC_PROG_INSTALL

m4_warn([obsolete],
[The preprocessor macro `STDC_HEADERS' is obsolete.
  Except in unusual embedded environments, you can safely include all
  ISO C90 headers unconditionally.])dnl
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h sys/select.h])
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
m4_warn([obsolete],
[Update your code to rely only on HAVE_SYS_TIME_H,
then remove this warning and the obsolete code below it.
All current systems provide time.h; it need not be checked for.
Not all systems provide sys/time.h, but those that do, all allow
you to include it and time.h simultaneously.])dnl
AC_CHECK_HEADERS_ONCE([sys/time.h])
# Obsolete code to be removed.
if test $ac_cv_header_sys_time_h = yes; then
  AC_DEFINE([TIME_WITH_SYS_TIME],[1],[Define to 1 if you can safely include both <sys/time.h>
	     and <time.h>.  This macro is obsolete.])
fi
# End of obsolete code.


AC_ARG_WITH(gui,[  --without-gui 	don't compile the OpenGL user interface])
AC_ARG_WITH(dedicated,[  --without-dedicated 	don't compile dedicated server])
AC_ARG_WITH(client,[  --without-client 	don't compile dedicated client])
AC_ARG_WITH(benchmark,[  --without-benchmark 	don't compile benchmark])

AM_CONDITIONAL(WITH_DEDICATED, test "${with_dedicated}" != no)
AM_CONDITIONAL(WITH_GUI, test "${with_gui}" != no)
AM_CONDITIONAL(WITH_CLIENT, test "${with_client}" != no)
AM_CONDITIONAL(WITH_BENCHMARK, test "${with_benchmark}" != no)

# Checks for libraries.
AC_CHECK_LIB([nsl], [gethostbyname])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([socket], [recv])
AC_CHECK_LIB([m],[sinf])

if test "x${with_gui}" != xno
then
    AC_PATH_X
    AC_PATH_XTRA
    CXXFLAGS="${CXXFLAGS} ${X_CFLAGS}"
    CPPFLAGS="${CPPFLAGS} ${X_CFLAGS}"
    LIBS="${LIBS} ${X_LIBS}"
    AC_CHECK_HEADERS([GL/glx.h GL/glu.h X11/Xlib.h X11/xpm.h])
    AC_CHECK_LIB([GL],[glBegin])
    AC_CHECK_LIB([GLU], [gluLookAt])
    AC_CHECK_LIB([X11],[XCreateWindow])
    AC_CHECK_LIB([Xpm],[XpmCreatePixmapFromXpmImage])
fi

AC_FUNC_MALLOC
AC_CHECK_FUNCS([gethostbyname getaddrinfo gettimeofday isascii select socket sqrt strchr strdup strerror pthread_create listen recv])

if test "x${with_dedicated}" != xno
then
    AC_FUNC_FORK
fi

AC_CHECK_SIZEOF(char,1)
AC_CHECK_SIZEOF(short,2)

AC_MSG_CHECKING(if 1=2)
AC_MSG_RESULT(no)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
