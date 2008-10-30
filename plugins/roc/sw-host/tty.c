#ifdef linux
#include <stdio.h>
#include <termio.h>
#else
#include "xbasic_types.h"
#include "xio.h"
//#include "console.h"
#endif

#include "tty.h"

#ifdef linux
static struct termio savemodes;
static int havemodes = 0;
#endif


// global history buffer
static char hist_buf[HISTSIZE+1][CMDLENGTH];
static int hist_entries = 0;


// TTY init/exit functions

int tty_init() {
#ifdef linux
  // ripped from comp.lang.c FAQ
  struct termio modmodes;
  if(ioctl(fileno(stdin), TCGETA, &savemodes) < 0)
    return -1;
  havemodes = 1;
  modmodes = savemodes;
  modmodes.c_lflag &= ~ICANON;
  modmodes.c_lflag &= ~ECHO;
  modmodes.c_cc[VMIN] = 1;
  modmodes.c_cc[VTIME] = 0;
  return ioctl(fileno(stdin), TCSETAW, &modmodes);
#else
  return 0;
#endif
}


int tty_exit() {
#ifdef linux
  if(!havemodes)
    return 0;
  return ioctl(fileno(stdin), TCSETAW, &savemodes);
#else
  return 0;
#endif
}


// Basic I/O functions

int tty_getchar() {
#ifdef linux
  return getchar();
#else
  return inbyte();
#endif
}


void tty_print(char *buf) {
#ifdef linux
  printf("%s", buf);
#else
  print(buf);
#endif
}


void tty_putchar(int c) {
#ifdef linux
  printf("%c", c);
#else
  outbyte(c);
#endif
}


// Advanced input functions

int tty_get_key() {
  int key, seq;

  // crude vt100-like character input
  key = tty_getchar();
  if (key != 27)
    return key;

  key = tty_getchar();
  if (key != '[')
    return key;

  key = tty_getchar();
  seq = (1 << 8) + key;
  if (key >= '0' && key <= '9') {
    while ((key = tty_getchar()) != '~') {
      seq = (seq << 8) + key;
    }
  }
  return seq;
}


enum gsinit_type {
  GSINIT_NONE, GSINIT_VISUAL, GSINIT_HIDDEN
};


int tty_get_string_with_flags(char *s, int size,
                              enum gsinit_type init, int allow_up_down) {
  int len, pos;
  int key, i;
  int done, result;

  pos = len = 0;
  if (init != GSINIT_NONE) {
    while(s[pos])
      pos++;
    len = pos;
    if (init == GSINIT_VISUAL) {
      tty_print(s);
    }
  } else {
    s[0] = '\0';
  }

  done = result = 0;
  while(!done) {

    key = tty_get_key();
    switch (key) {

    case '\n': // key: return
    case '\r':
      tty_putchar('\r');
      tty_putchar('\n');
      done = 1;
      break;

    case KEY_UP: // key: cursor up
      if (allow_up_down) {
        done = 1;
        result = -1;
      }
      break;

    case KEY_DOWN: // key: cursor down
      if (allow_up_down) {
        done = 1;
        result = 1;
      }
      break;

    case '\b': // key: backspace
      if (pos > 0) {
        for (i = pos; i <= len; i++)
          s[i-1] = s[i];
        len--;
        pos--;
        tty_putchar('\b');
        tty_print(s + pos);
        tty_putchar(' ');
        for (i = len - pos + 1; i > 0; i--)
          tty_putchar('\b');
      }
      break;

    case KEY_DEL: // key: delete
    case KEY_CTRL_D:
      if (pos < len) {
        for (i = pos; i < len; i++)
          s[i] = s[i+1];
        len--;
        tty_print(s + pos);
        tty_putchar(' ');
        for (i = len - pos + 1; i > 0; i--)
          tty_putchar('\b');
      }
      break;

    case KEY_CTRL_K: // key: ctrl-k (delete-to-end-of-line)
        for (i = len - pos; i > 0; i--)
          tty_putchar(' ');
        for (i = len - pos; i > 0; i--)
          tty_putchar('\b');
        s[pos] = '\0';
        len = pos;
      break;

    case KEY_LEFT: // key: cursor left
      if (pos > 0) {
        pos--;
        tty_putchar('\b');
      }
      break;

    case KEY_RIGHT: // key: cursor right
      if (pos < len) {
        tty_putchar(s[pos]);
        pos++;
      }
      break;

    case KEY_HOME: // key: cursor home
    case KEY_CTRL_A:
      for (i = pos; i > 0; i--)
        tty_putchar('\b');
      pos = 0;
      break;

    case KEY_END: // key: cursor end
    case KEY_CTRL_E:
      tty_print(s + pos);
      pos = len;
      break;

    default:
      if (key < 0x20 || key > 0xFF) {
        // silently ignore invalid chars
        break;
      }
      if (len < size - 1) {
        for (i = len; i >= pos; i--)
          s[i+1] = s[i];
        len++;
        s[pos] = key;
        tty_print(s + pos);
        pos++;
        for (i = len - pos; i > 0; i--)
          tty_putchar('\b');
      }
    }
  }

  return result;
}


char *tty_get_string(char *s, int size) {
  tty_get_string_with_flags(s, size, GSINIT_NONE, 0);

  return s;
}


// Advanced input with command history

char *tty_get_command()//char *prompt)
{
  static char cmd[CMDLENGTH];
  int hist_pos;
  int i, len_a, len_b;
  int res, done;

  done = 0;
  hist_pos = hist_entries;

  cmd[0] = '\0';
  //tty_print(prompt);
  while (!done) {
    res = tty_get_string_with_flags(cmd, CMDLENGTH, GSINIT_HIDDEN, 1);
    if (res == 0) {
      if (cmd[0] != '\0') {
        // add current command to history
        if (hist_entries < HISTSIZE) {
          x_strcpy(hist_buf[hist_entries], cmd);
          hist_entries++;
        } else {
          for (i = 0; i < HISTSIZE; i++) // TODO: check bounds!
            x_strcpy(hist_buf[i], hist_buf[i+1]);
          x_strcpy(hist_buf[HISTSIZE], cmd);
        }
      }
      done = 1;
    } else if ((res == -1 && hist_pos > 0)
               || (res == 1 && hist_pos < hist_entries)) {
      // replace current command with history entry
      len_a = x_strlen(cmd);
      for (i = len_a; i > 0; i--)
        tty_putchar('\b');
      x_strcpy(hist_buf[hist_pos], cmd);
      hist_pos += res;
      x_strcpy(cmd, hist_buf[hist_pos]);
      len_b = x_strlen(cmd);
      tty_print(cmd);
      for (i = len_a - len_b; i > 0; i--)
        tty_putchar(' ');
      for (i = len_a - len_b; i > 0; i--)
        tty_putchar('\b');
    }
  }

  return cmd;
}


// Helper functions

int x_strlen(const char *s) {
  int i = 0;

  while (*s++)
    i++;

  return i;
}


char *x_strcpy(char *dest, const char *src) {
  while ((*dest++ = *src++));

  return dest;
}
