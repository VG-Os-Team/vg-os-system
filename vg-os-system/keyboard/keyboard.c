#include "keyboard.h"
#include <stdint.h>

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

// Layout ABNT2 Brasileiro (Dead keys: 0x1A = ´/`, 0x28 = ~/^)
static const char kbd_ABNT2[128] = {
    0,    27,   '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
    '=',  '\b', '\t', 'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    0,    '[',  '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    (char)135, 0, '\'', 0, ']',  'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.',  ';',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
    0,    0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,    0,    0,   0,    0,   0,   0,   '\\', 0,  0,   0,   0,
    0,    0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,    '/',  0,   0,    0,   0,   0,   0,   0,   0};

static const char kbd_ABNT2_shift[128] = {
    0,    27,   '!',  '@', '#',  '$', '%', '^', '&', '*', '(', ')', '_',
    '+',  '\b', '\t', 'Q', 'W',  'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    0,    '{',  '\n', 0,   'A',  'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    (char)128, 0, '"',  0, '}',  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<',
    '>',  ':',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
    0,    0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,    0,    0,   0,    0,   0,   0,   '|',  0,  0,   0,   0,
    0,    0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,    '?',  0,   0,    0,   0,   0,   0,   0,   0};

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

#define SC_ACENTO_AGUDO_CRASE 0x1A
#define SC_TIL_CIRCUNFLEXO    0x28
#define SC_SPACE              0x39
#define SC_BACKSPACE          0x0E

enum {
  ACENTO_NENHUM = 0,
  ACENTO_AGUDO,       // ´
  ACENTO_GRAVE,       // `
  ACENTO_TIL,         // ~
  ACENTO_CIRCUNFLEXO  // ^
};

static int shift_pressionado = 0;
static int capslock_ativo = 0;
static int capslock_pressionado = 0;
static char ctrl_pressionado = 0;
static int e0_prefix = 0;
static int acento_pendente = ACENTO_NENHUM;

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

static char combina_acento(char c, int acento) {
  switch (acento) {
  case ACENTO_AGUDO:
    switch (c) {
    case 'a': return (char)160; // á
    case 'A': return (char)181; // Á
    case 'e': return (char)130; // é
    case 'E': return (char)144; // É
    case 'i': return (char)161; // í
    case 'I': return (char)214; // Í
    case 'o': return (char)162; // ó
    case 'O': return (char)224; // Ó
    case 'u': return (char)163; // ú
    case 'U': return (char)233; // Ú
    case 'c': return (char)135; // ç
    case 'C': return (char)128; // Ç
    }
    break;

  case ACENTO_GRAVE:
    switch (c) {
    case 'a': return (char)133; // à
    case 'A': return (char)183; // À
    }
    break;

  case ACENTO_TIL:
    switch (c) {
    case 'a': return (char)198; // ã
    case 'A': return (char)199; // Ã
    case 'o': return (char)228; // õ
    case 'O': return (char)229; // Õ
    }
    break;

  case ACENTO_CIRCUNFLEXO:
    switch (c) {
    case 'a': return (char)131; // â
    case 'A': return (char)182; // Â
    case 'e': return (char)136; // ê
    case 'E': return (char)210; // Ê
    case 'o': return (char)147; // ô
    case 'O': return (char)226; // Ô
    }
    break;
  }
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

  // Teclas normais de controle
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

  // Tratamento de Dead Keys (Acentos)
  if (scancode == SC_ACENTO_AGUDO_CRASE) {
    int desejado = shift_pressionado ? ACENTO_GRAVE : ACENTO_AGUDO;
    if (acento_pendente == desejado) {
      // Pressionou o mesmo acento duas vezes: emite o acento isolado
      acento_pendente = ACENTO_NENHUM;
      return (desejado == ACENTO_GRAVE) ? '`' : (int)(unsigned char)239; // ´
    }
    acento_pendente = desejado;
    return TECLA_NENHUMA;
  }

  if (scancode == SC_TIL_CIRCUNFLEXO) {
    int desejado = shift_pressionado ? ACENTO_CIRCUNFLEXO : ACENTO_TIL;
    if (acento_pendente == desejado) {
      // Pressionou o mesmo acento duas vezes: emite o acento isolado
      acento_pendente = ACENTO_NENHUM;
      return (desejado == ACENTO_CIRCUNFLEXO) ? '^' : '~';
    }
    acento_pendente = desejado;
    return TECLA_NENHUMA;
  }

  // Se houver acento pendente e apertar espaço: emite o acento isolado
  if (acento_pendente != ACENTO_NENHUM && scancode == SC_SPACE) {
    int ac = acento_pendente;
    acento_pendente = ACENTO_NENHUM;
    if (ac == ACENTO_AGUDO) return (int)(unsigned char)239; // ´
    if (ac == ACENTO_GRAVE) return '`';
    if (ac == ACENTO_TIL) return '~';
    if (ac == ACENTO_CIRCUNFLEXO) return '^';
  }

  // Se houver acento pendente e apertar Backspace: apenas cancela o acento
  if (acento_pendente != ACENTO_NENHUM && scancode == SC_BACKSPACE) {
    acento_pendente = ACENTO_NENHUM;
    return TECLA_NENHUMA;
  }

  char base = shift_pressionado ? kbd_ABNT2_shift[scancode] : kbd_ABNT2[scancode];
  char c = aplica_case(base);

  if (acento_pendente != ACENTO_NENHUM) {
    int ac = acento_pendente;
    acento_pendente = ACENTO_NENHUM;
    c = combina_acento(c, ac);
  }

  return (int)(unsigned char)c;
}