#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([gtbserver], [0.1], [matthew.finkel@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/gtbcommunication.cc])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX([clang++ g++ clang gcc])
AC_PROG_CC([clang gcc])

# Checks for libraries.
AC_SEARCH_LIBS([curl_easy_setopt], [curl], [], [AC_MSG_FAILURE([libcurl not found!])])
AC_SEARCH_LIBS([nettle_aes256], [nettle], [], [AC_MSG_FAILURE([libnettle not found!])])
AC_SEARCH_LIBS([pthread_exit], [pthread], [], [AC_MSG_FAILURE([libpthread not found!])])
AC_SEARCH_LIBS([gnutls_global_init], [gnutls], [], [AC_MSG_FAILURE([libgnutls not found!])])
LIBS="$LIBS `pkg-config gnutls --cflags --libs`"

# AC_SEARCH_LIBS_CXX(prologue, body, search-libs, [action-if-found], [action-if-not-found], [other-libs])
# -----------------------------------------------------------------------------------
AC_DEFUN([AC_SEARCH_LIBS_CXX],
  [
    AC_MSG_CHECKING([for library containing [$3]])
    TMP="$LIBS"
    NEWLIB=`pkg-config --silence-errors --cflags --libs $3`
    if @<:@ "${NEWLIBS}" == "" @:>@
    then
      LIBS="$LIBS -l$3"
    else
      LIBS="$LIBS $NEWLIBS"
    fi
    AC_LINK_IFELSE(
      [AC_LANG_PROGRAM(
        [
          [$1]
        ],
        [
          [$2;]
        ]
      )],
      [
	AC_MSG_RESULT([yes])
      ],
      [
	AC_MSG_RESULT([no])
	LIBS="$TMP"
        $5
      ]
    )
  ]
)

AC_LANG_PUSH([C++])
AC_SEARCH_LIBS_CXX([#include <cppconn/prepared_statement.h>], [sql::PreparedStatement *prepStmt], [mysqlcppconn], [], [AC_MSG_FAILURE([libmysqlcppcon not found!])])
AC_SEARCH_LIBS_CXX([#include <google/protobuf/message.h>], [google::protobuf::Message *msg], [protobuf], [], [AC_MSG_FAILURE([libprotobuf not found!])])
AC_LANG_POP([C++])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h inttypes.h locale.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/param.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([isascii memmove memset modf select setlocale socket strchr strerror strpbrk])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
