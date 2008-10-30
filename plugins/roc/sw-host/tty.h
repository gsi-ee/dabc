#ifndef TTY_H
#define TTY_H

#undef linux //to use with gcc crosscompile

// TTY init/exit functions
int tty_init();
int tty_exit();

// Basic I/O functions
int tty_getchar();
void tty_print(char *buf);
void tty_putchar(int c);

// Advanced input functions
int tty_get_key();
char *tty_get_string(char *s, int size);

enum key_type {
  KEY_CTRL_A = 1, KEY_CTRL_B, KEY_CTRL_C, KEY_CTRL_D, KEY_CTRL_E, KEY_CTRL_F,
  KEY_CTRL_G, KEY_CTRL_H, KEY_CTRL_I, KEY_CTRL_J, KEY_CTRL_K, KEY_CTRL_L,
  KEY_CTRL_M, KEY_CTRL_N, KEY_CTRL_O, KEY_CTRL_P, KEY_CTRL_Q, KEY_CTRL_R,
  KEY_CTRL_S, KEY_CTRL_T, KEY_CTRL_U, KEY_CTRL_V, KEY_CTRL_W, KEY_CTRL_X,
  KEY_CTRL_Y, KEY_CTRL_Z,
  KEY_UP = (1 << 8) + 'A', KEY_DOWN, KEY_RIGHT, KEY_LEFT,
#ifdef linux
  KEY_HOME = (1 << 8) + '1', KEY_INS, KEY_DEL, KEY_END, KEY_PGUP, KEY_PGDOWN,
#else
  KEY_INS = (1 << 8) + '1', KEY_HOME, KEY_PGUP, KEY_DEL, KEY_END, KEY_PGDOWN,
#endif
  KEY_F1 = (1 << 16) + ('1' << 8) + '1', KEY_F2, KEY_F3, KEY_F4,
  KEY_F5, KEY_F5_, KEY_F6, KEY_F7, KEY_F8,
  KEY_F9 = (1 << 16) + ('2' << 8) + '0', KEY_F10, KEY_F10_, KEY_F11, KEY_F12
};

// Advanced input with command history
#define HISTSIZE 20
#define CMDLENGTH 80

char *tty_get_command();//char *prompt);

// Helper functions
int x_strlen(const char *s);
char *x_strcpy(char *dest, const char *src);

#endif
