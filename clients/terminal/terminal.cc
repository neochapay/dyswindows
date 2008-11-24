
#include "terminal.h"

#include "VTScreenView.h"

extern "C" {
#include <iterm/unix/ttyio.h>
#include <locale.h>
}

#include <iostream>

using std::cout;
using std::cerr;
using std::endl;
using sigc::bind;
//using sigc::mem_fun;

Terminal::Terminal (Y::Connection *y_) : cols(80), rows(24), y(y_)
{
  window = new Y::Window (y, "Terminal");
  window->setBackground(0);

  console = new Y::Console (y);
  window -> setChild (console);
  window -> setFocussed (console);

  window -> requestClose.connect (bind (sigc::ptr_fun (&exit), EXIT_SUCCESS));

  io = ioInit (cols, rows, 1);
  vtcore = VTCore_new (io, cols, rows, 500);

  iofd = TtyTerminalIO_get_associated_fd (io);
  y->registerFD (iofd, Y_LISTEN_READ, this, &Terminal::ioDispatch);

  view = VTScreenView_new (this);
  VTCore_set_screen_view (vtcore, view);
  VTCore_set_exit_callback (vtcore, &VTScreenView_exit);
  console -> ykbString.connect (mem_fun (*this, &Terminal::ykbString));
  console -> ykbEvent.connect (mem_fun (*this, &Terminal::ykbEvent));
  console -> resize.connect (mem_fun (*this, &Terminal::resize));

  window -> show ();
}

Terminal::~Terminal ()
{
  VTScreenView_destroy (view);
}

TerminalIO *
Terminal::ioInit (int cols, int rows, bool login)
{
  char defaultShell[] = "/bin/sh";
  char *shell;
  char *loginShell;
  char *program = defaultShell;
  char *argv[] = { program, NULL };
  char env[] = "TERM=xterm";
  putenv (env);
  shell = getenv ("SHELL");
  if (shell != NULL && shell[0] != '\0')
    {
      program = shell;
      argv[0] = shell;
    }

  if (login)
    {
      int length = strlen (program) + 1;
      loginShell = static_cast<char *> (malloc (length + 1));
      loginShell[0] = '-';
      memcpy (loginShell + 1, program, length);
      argv[0] = loginShell;
    }

  return TtyTerminalIO_new (cols, rows, program, argv);
}

void
Terminal::ioDispatch (int fd, int mask, void *data)
{
  Terminal *self = (Terminal *) data;
  self->y->changeFD (fd, 0);
  VTCore_dispatch (self -> vtcore);
  self->y->changeFD (fd, Y_LISTEN_READ);
}

void
Terminal::ykbString (std::string str, uint16_t modifiers)
{
  /* libiterm really sucks */
  size_t len = str.length();
  char buf[len];
  str.copy(buf, len);
  VTCore_write(vtcore, buf, len);
}

void
Terminal::ykbEvent (std::string event, uint16_t modifiers)
{
  if (event == "key:up")
    VTCore_send_key (vtcore, VTK_UP);
  else if (event == "key:down")
    VTCore_send_key (vtcore, VTK_DOWN);
  else if (event == "key:left")
    VTCore_send_key (vtcore, VTK_LEFT);
  else if (event == "key:right")
    VTCore_send_key (vtcore, VTK_RIGHT);
  else if (event == "key:return")
    VTCore_send_key (vtcore, VTK_CR);
  else if (event == "key:tab")
    {
      /* These next few are ascii characters, but YKB doesn't think of
       * them like that. This should really be handled by an
       * application keymap, emitting these as strings.
       */
      char c = 9;
      VTCore_write (vtcore, &c, 1);
    }
  else if (event == "key:backspace")
    {
      char c = 8;
      VTCore_write (vtcore, &c, 1);
    }
  else if (event == "key:escape")
    {
      char c = 27;
      VTCore_write (vtcore, &c, 1);
    }
  else if (event == "key:delete")
    VTCore_send_key (vtcore, VTK_DELETE);
  else if (event == "key:home")
    VTCore_send_key (vtcore, VTK_HOME);
  else if (event == "key:end")
    VTCore_send_key (vtcore, VTK_END);
  else if (event == "key:page up")
    {
      /* FIXME: This is the wrong way to do it. Should install an
       * application keymap instead. You're not allowed to make
       * assumptions like this about the meaning of modifier values
       */
      if (modifiers == 1)
        VTCore_scroll_up (vtcore, rows/2);
      else
        VTCore_send_key (vtcore, VTK_PAGE_UP);
    }
  else if (event == "key:page down")
    {
      if (modifiers == 1)
        VTCore_scroll_down (vtcore, rows/2);
      else
        VTCore_send_key (vtcore, VTK_PAGE_DOWN);
    }
}

void
Terminal::resize (uint32_t cols, uint32_t rows)
{
  this -> cols = cols;
  this -> rows = rows;
  VTCore_set_screen_size (vtcore, cols, rows);
}

void
Terminal::notifyOSC (int type, char *data, int length)
{
}

int
main (int argc, char **argv)
{
  setlocale(LC_CTYPE, "");
  setlocale(LC_COLLATE, "");
  setlocale(LC_MONETARY, "");
  setlocale(LC_NUMERIC, "");
  setlocale(LC_MESSAGES, "");
  setlocale(LC_TIME, "");

  Y::Connection y;
  Terminal *t = new Terminal (&y);
  y.run();
  delete t;
}

/* arch-tag: 451c0b5d-331b-40b8-8d0b-a907e35e4dce
 */
