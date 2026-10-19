#include "panic.h"
#include "render.h"
#include <stddef.h>

#define PANIC_FUNDO 0x0035121D
#define PANIC_TEXTO 0x00FFFFFF
#define PANIC_DESTAQUE 0x00FFD166
#define MARGEM 16U
#define CHAR_LARGURA 16U
#define CHAR_ALTURA 24U
#define LINHA_ALTURA 28U

static struct {
  uint32_t *fb;
  uint32_t pitch, largura, altura;
} video;
static volatile int panico_em_andamento;
static uint32_t linha_y;

static void panic_parar(void) __attribute__((noreturn));
static void panic_parar(void) {
  // HLT suspende a CPU; o loop impede continuar caso ela seja despertada.
  for (;;)
    asm volatile("cli; hlt" ::: "memory");
}

int panic_configurar_video(uint32_t *fb, uint32_t pitch,
                          uint32_t largura, uint32_t altura) {
  // Uma falha de validacao nunca conserva o framebuffer anterior.
  video.fb = NULL;
  if (!fb || (uintptr_t)fb % 4 != 0 || largura < 48 || altura < 56 ||
      pitch == 0 || pitch % 4 != 0 ||
      pitch / 4 < largura || altura > UINT32_MAX / pitch)
    return 0;
  uint32_t tamanho = pitch * altura;
  if ((uintptr_t)fb > UINT32_MAX - tamanho)
    return 0;

  video.pitch = pitch;
  video.largura = largura;
  video.altura = altura;
  video.fb = fb;
  return 1;
}

// Escrita limitada a tela e ao numero de linhas solicitado. Nao usa printf.
static void panic_texto(const char *texto, uint32_t cor, unsigned max_linhas) {
  uint32_t x = MARGEM;
  unsigned linhas = 1;
  for (unsigned i = 0; i < 512 && texto[i]; i++) {
    if (texto[i] == '\n' || x + CHAR_LARGURA > video.largura - MARGEM) {
      if (++linhas > max_linhas)
        break;
      linha_y += LINHA_ALTURA;
      x = MARGEM;
      if (texto[i] == '\n')
        continue;
    }
    if (linha_y + CHAR_ALTURA > video.altura - MARGEM)
      break;
    desenhar_char_com_fundo(texto[i], x, linha_y, cor, PANIC_FUNDO,
                            video.fb, video.pitch);
    x += CHAR_LARGURA;
  }
  linha_y += LINHA_ALTURA;
}

// Separadores usam os espacos ja existentes, sem consumir linhas de texto.
static void panic_separador(uint32_t y) {
  if (y >= video.altura - MARGEM)
    return;
  uint32_t fim_x = video.largura - MARGEM;
  if (fim_x > MARGEM + 32U * CHAR_LARGURA)
    fim_x = MARGEM + 32U * CHAR_LARGURA;
  for (uint32_t x = MARGEM; x < fim_x; x++)
    video.fb[y * (video.pitch / 4) + x] = PANIC_DESTAQUE;
}

static char *panic_valor(char *destino, const char *nome, uint32_t valor) {
  const char *hex = "0123456789ABCDEF";
  unsigned letras = 0;
  while (*nome) {
    *destino++ = *nome++;
    letras++;
  }
  // Alinhe nomes e valores quando as duas colunas completas couberem.
  // Em telas estreitas, conserve o formato compacto original.
  if (video.largura - 2U * MARGEM >= 32U * CHAR_LARGURA)
    while (letras++ < 6)
      *destino++ = ' ';
  *destino++ = '=';
  for (int shift = 28; shift >= 0; shift -= 4)
    *destino++ = hex[(valor >> shift) & 15];
  *destino = '\0';
  return destino;
}

static void panic_par(const char *nome1, uint32_t valor1,
                      const char *nome2, uint32_t valor2) {
  // Nomes internos de no maximo 6 caracteres; duas colunas cabem neste buffer.
  char linha[40];
  char *fim = panic_valor(linha, nome1, valor1);
  *fim++ = ' ';
  *fim++ = ' ';
  panic_valor(fim, nome2, valor2);
  panic_texto(linha, PANIC_TEXTO, 2);
}

void kernel_panic(const char *message, registers_t *regs) {
  asm volatile("cli; cld" ::: "memory", "cc");
  // Uma segunda falha nao deve tentar redesenhar a tela recursivamente.
  if (panico_em_andamento)
    panic_parar();
  panico_em_andamento = 1;
  if (!video.fb)
    panic_parar();

  for (uint32_t y = 0; y < video.altura; y++)
    for (uint32_t x = 0; x < video.largura; x++)
      video.fb[y * (video.pitch / 4) + x] = PANIC_FUNDO;

  linha_y = MARGEM;
  panic_texto("VGOS - KERNEL PANIC", PANIC_DESTAQUE, 1);
  panic_separador(linha_y - 3U);
  panic_texto(message ? message : "Erro fatal sem mensagem.", PANIC_TEXTO, 3);
  linha_y += 8;

  if (regs) {
    uint16_t ss_atual;
    asm volatile("mov %%ss, %0" : "=r"(ss_atual));
    uint32_t ss = ss_atual;
    // PUSHA salvou ESP depois de int_no, err_code, EIP, CS e EFLAGS: 5*4 bytes.
    uint32_t esp = regs->esp_dummy + 20;
    if (regs->cs & 3) {
      // Numa futura entrada a partir de ring 3, a CPU tambem empilha ESP e SS.
      const uint32_t *pilha_usuario = (const uint32_t *)(uintptr_t)esp;
      esp = pilha_usuario[0];
      ss = pilha_usuario[1] & 0xFFFF;
    }
    panic_texto("Estado da CPU (hexadecimal):", PANIC_DESTAQUE, 1);
    panic_par("EAX", regs->eax, "EBX", regs->ebx);
    panic_par("ECX", regs->ecx, "EDX", regs->edx);
    panic_par("ESI", regs->esi, "EDI", regs->edi);
    panic_par("EBP", regs->ebp, "ESP", esp);
    panic_par("EIP", regs->eip, "EFLAGS", regs->eflags);
    panic_par("CS", regs->cs & 0xFFFF, "SS", ss);
    panic_par("DS", regs->ds & 0xFFFF, "VETOR", regs->int_no);
    char erro[24];
    panic_valor(erro, "ERRO", regs->err_code);
    panic_texto(erro, PANIC_TEXTO, 1);
    if (regs->int_no == 14) {
      uint32_t cr2;
      asm volatile("mov %%cr2, %0" : "=r"(cr2));
      panic_valor(erro, "CR2", cr2);
      panic_texto(erro, PANIC_TEXTO, 1);
    }
  } else {
    panic_texto("Registradores indisponiveis.", PANIC_TEXTO, 1);
  }

  panic_separador(linha_y + 3U);
  linha_y += 8;
  panic_texto("Sistema parado. Reinicie a maquina virtual.", PANIC_DESTAQUE, 2);
  panic_parar();
}
