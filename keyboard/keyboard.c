#include "keyboard.h"
#include <stdint.h>

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

// Layout US QWERTY + ABNT2 (0x27 = ç, 0x73 = /)
static const char kbd_US[128] = {
    0,   27,   '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
    '=', '\b', '\t', 'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']',  '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    (char)135, '\'', '`',  0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '/',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    '/',  0,   0,    0,   0,   0,   0,   0,   0};

static const char kbd_US_shift[128] = {
    0,   27,   '!',  '@', '#', '$', '%', '^', '&', '*', '(', ')', '_',
    '+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    '{', '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    (char)128, '"',  '~',  0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<',
    '>', '?',  0,    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    '?',  0,   0,   0,   0,   0,   0,   0,   0};

#define SC_SHIFT_L 0x2A
#define SC_SHIFT_R 0x36
#define SC_SHIFT_L_UP 0xAA
#define SC_SHIFT_R_UP 0xB6
#define SC_CAPSLOCK 0x3A
#define SC_CAPSLOCK_UP 0xBA
#define SC_CTRL_L 0x1D
#define SC_CTRL_L_UP 0x9D
#define SC_L 0x26
#define SC_C 0x2E
#define SC_V 0x2F

static int shift_pressionado = 0;
static int capslock_ativo = 0;
static int capslock_pressionado = 0;
static char ctrl_pressionado = 0;
static int e0_prefix = 0;

static char aplica_case(char c) {
  int maiuscula_pedida = shift_pressionado ^ capslock_ativo;
  if (c >= 'a' && c <= 'z')
    return maiuscula_pedida ? (c - 32) : c;
  if (c >= 'A' && c <= 'Z')
    return maiuscula_pedida ? c : (c + 32);
  if ((uint8_t)c == 135 || (uint8_t)c == 128)
    return maiuscula_pedida ? (char)128 : (char)135;
  return c;
}

int teclado_ler(void) {
  if (!(inb(0x64) & 1))
    return TECLA_NENHUMA;

  uint8_t scancode = inb(0x60);

  // Prefix E0 (teclas estendidas)
  if (scancode == 0xE0) {
    e0_prefix = 1;
    return TECLA_NENHUMA;
  }

  if (e0_prefix) {
    e0_prefix = 0;

    // Release de tecla estendida
    if (scancode & 0x80) {
      if (scancode == SC_CTRL_L_UP) { // 0x9D: Right Ctrl release
        ctrl_pressionado = 0;
      }
      return TECLA_NENHUMA;
    }

    // Press de tecla estendida
    switch (scancode) {
    case SC_CTRL_L: // 0x1D: Right Ctrl down
      ctrl_pressionado = 1;
      return TECLA_NENHUMA;
    case 0x48:
      return TECLA_SETA_CIMA;
    case 0x50:
      return TECLA_SETA_BAIXO;
    case 0x4B:
      return TECLA_SETA_ESQUERDA;
    case 0x4D:
      return TECLA_SETA_DIREITA;
    case 0x53:
      return TECLA_DELETE;
    case 0x47:
      return TECLA_HOME;
    case 0x4F:
      return TECLA_END;
    case 0x1C: // Keypad Enter
      return '\n';
    case 0x35: // Keypad /
      return '/';
    default:
      return TECLA_NENHUMA;
    }
  }

  // Teclas normais
  switch (scancode) {
  case SC_SHIFT_L:
  case SC_SHIFT_R:
    shift_pressionado = 1;
    return TECLA_NENHUMA;
  case SC_SHIFT_L_UP:
  case SC_SHIFT_R_UP:
    shift_pressionado = 0;
    return TECLA_NENHUMA;
  case SC_CAPSLOCK:
    if (!capslock_pressionado) {
      capslock_ativo = !capslock_ativo;
      capslock_pressionado = 1;
    }
    return TECLA_NENHUMA;
  case SC_CAPSLOCK_UP:
    capslock_pressionado = 0;
    return TECLA_NENHUMA;
  case SC_CTRL_L:
    ctrl_pressionado = 1;
    return TECLA_NENHUMA;
  case SC_CTRL_L_UP:
    ctrl_pressionado = 0;
    return TECLA_NENHUMA;
  }

  if (scancode & 0x80)
    return TECLA_NENHUMA; // solta de outra tecla, ignora

  if (ctrl_pressionado) {
    switch (scancode) {
    case SC_L:
      return TECLA_CTRL_L;
    case SC_C:
      return TECLA_CTRL_C;
    case SC_V:
      return TECLA_CTRL_V;
    }
    return TECLA_NENHUMA;
  }

  if (scancode >= 128)
    return TECLA_NENHUMA;

  char base = shift_pressionado ? kbd_US_shift[scancode] : kbd_US[scancode];
  return (int)(unsigned char)aplica_case(base);
}