#include "../../include/ata.h"
#include <stddef.h>

static int g_ata_present = 0;

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
  asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
  uint16_t ret;
  asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
  asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

/* Atraso padrao de ~400ns lendo a porta de status alternativo 4 vezes */
static void ata_io_wait(void) {
  (void)inb(ATA_PRIMARY_CTRL);
  (void)inb(ATA_PRIMARY_CTRL);
  (void)inb(ATA_PRIMARY_CTRL);
  (void)inb(ATA_PRIMARY_CTRL);
}

/* Aguarda o bit BSY limpar e verifica se houve erro ou falha no disco */
static int ata_poll(int check_drq) {
  ata_io_wait();

  uint32_t timeout = 1000000U;
  uint8_t status = 0;

  while (timeout--) {
    status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    if (!(status & ATA_SR_BSY)) {
      break;
    }
  }

  if (timeout == 0) {
    return -1; /* Timeout */
  }

  if (status & (ATA_SR_ERR | ATA_SR_DF)) {
    return -2; /* Erro de hardware/setor */
  }

  if (check_drq) {
    timeout = 1000000U;
    while (timeout--) {
      status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
      if (status & (ATA_SR_ERR | ATA_SR_DF)) {
        return -2;
      }
      if (status & ATA_SR_DRQ) {
        return 0; /* Pronto para transferir os 512 bytes */
      }
    }
    return -3; /* Timeout aguardando DRQ */
  }

  return 0;
}

int ata_init(void) {
  /* Desabilita interrupcoes do controlador IDE (nIEN = 1) para operar em modo PIO polling limpo */
  outb(ATA_PRIMARY_CTRL, 0x02);

  /* Seleciona Primary Master (0xA0) */
  outb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, 0xA0);
  ata_io_wait();

  /* Zera registradores de contagem e LBA antes do IDENTIFY */
  outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, 0);
  outb(ATA_PRIMARY_IO + ATA_REG_LBA0, 0);
  outb(ATA_PRIMARY_IO + ATA_REG_LBA1, 0);
  outb(ATA_PRIMARY_IO + ATA_REG_LBA2, 0);

  /* Envia comando IDENTIFY (0xEC) */
  outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
  ata_io_wait();

  uint8_t status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
  if (status == 0 || status == 0xFF) {
    /* Barramento flutuante: nenhum disco conectado no Primary Master */
    g_ata_present = 0;
    return 0;
  }

  /* Aguarda BSY limpar */
  while (inb(ATA_PRIMARY_IO + ATA_REG_STATUS) & ATA_SR_BSY) {
    /* aguarda */
  }

  /* Verifica se nao e dispositivo ATAPI/SATA (LBA1 e LBA2 devem ser 0 para ATA padrao) */
  if (inb(ATA_PRIMARY_IO + ATA_REG_LBA1) != 0 || inb(ATA_PRIMARY_IO + ATA_REG_LBA2) != 0) {
    g_ata_present = 0;
    return 0;
  }

  if (ata_poll(1) != 0) {
    g_ata_present = 0;
    return 0;
  }

  /* Drena os 256 words (512 bytes) de identificacao do dispositivo */
  for (int i = 0; i < 256; i++) {
    (void)inw(ATA_PRIMARY_IO + ATA_REG_DATA);
  }

  g_ata_present = 1;
  return 1;
}

int ata_is_present(void) {
  return g_ata_present;
}

static void ata_select_lba28(uint32_t lba, uint8_t sector_count) {
  /* 0xE0 = Modo LBA (bit 6 = 1) + Primary Master (bit 4 = 0) + bits superiores LBA[27:24] */
  outb(ATA_PRIMARY_IO + ATA_REG_HDDEVSEL, (uint8_t)(0xE0 | ((lba >> 24) & 0x0F)));
  ata_io_wait();

  outb(ATA_PRIMARY_IO + ATA_REG_FEATURES, 0x00);
  outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT0, sector_count);
  outb(ATA_PRIMARY_IO + ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
  outb(ATA_PRIMARY_IO + ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
  outb(ATA_PRIMARY_IO + ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
}

int ata_read_sector(uint32_t lba, uint8_t *buffer) {
  if (buffer == NULL || !g_ata_present) {
    return -1;
  }

  /* Limite do enderecamento LBA 28-bit: 2^28 - 1 (0x0FFFFFFF) */
  if (lba > 0x0FFFFFFFU) {
    return -4;
  }

  ata_select_lba28(lba, 1);
  outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

  int err = ata_poll(1);
  if (err != 0) {
    return err;
  }

  /* Le 256 words (512 bytes) da porta de dados 0x1F0 */
  for (int i = 0; i < 256; i++) {
    uint16_t word = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
    buffer[i * 2]     = (uint8_t)(word & 0xFF);
    buffer[i * 2 + 1] = (uint8_t)((word >> 8) & 0xFF);
  }

  ata_io_wait();
  return 0;
}

int ata_write_sector(uint32_t lba, const uint8_t *buffer) {
  if (buffer == NULL || !g_ata_present) {
    return -1;
  }

  if (lba > 0x0FFFFFFFU) {
    return -4;
  }

  ata_select_lba28(lba, 1);
  outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

  int err = ata_poll(1);
  if (err != 0) {
    return err;
  }

  /* Escreve 256 words (512 bytes) na porta de dados 0x1F0 */
  for (int i = 0; i < 256; i++) {
    uint16_t word = (uint16_t)buffer[i * 2] | ((uint16_t)buffer[i * 2 + 1] << 8);
    outw(ATA_PRIMARY_IO + ATA_REG_DATA, word);
  }

  /* Garante que o cache interno do controlador seja gravado no disco fisico/imagem */
  outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
  return ata_poll(0);
}