AC_DEFUN([LIBITERM_CHECK],
[
oldlibs="$LIBS"
LIBS="-literm $LIBS"

AC_LANG_PUSH([C++])
AC_MSG_CHECKING([for an up-to-date libiterm])
AC_LINK_IFELSE(
[
extern "C" {
#include <iterm/core.h>
#include <iterm/unix/ttyio.h>
}

int main (void)
{
  VTCore_destroy(VTCore_new (NULL, 80, 24, 500));
  // Just a quick test to see if the simbols are there.
  return !(VTK_DELETE != VTK_INSERT && VTK_PAGE_DOWN != VTK_PAGE_UP && VTK_END != VTK_HOME);
}
],
[
LIBITERM_LIBS="-literm"
AC_SUBST(LIBITERM_LIBS)
have_libiterm=yes
AC_MSG_RESULT(yes)
],
[
have_libiterm=no
AC_MSG_RESULT(no)
])
AC_LANG_POP([C++])

LIBS="$oldlibs"

])
