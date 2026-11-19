#ifndef ATA_H
#define ATA_H

#include <stdint.h>

/* Portas I/O do Barramento IDE Primario */
#define ATA_PRIMARY_IO       0x1F0
#define ATA_PRIMARY_CTRL     0x3F6

/* Offsets a partir de ATA_PRIMARY_IO (0x1F0 - 0x1F7) */
#define ATA_REG_DATA         0x00  /* 0x1F0 - Leitura/Escrita de dados (16-bit) */
#define ATA_REG_ERROR        0x01  /* 0x1F1 - Leitura: Erro */
#define ATA_REG_FEATURES     0x01  /* 0x1F1 - Escrita: Features */
#define ATA_REG_SECCOUNT0    0x02  /* 0x1F2 - Numero de setores */
#define ATA_REG_LBA0         0x03  /* 0x1F3 - LBA bits 0..7 */
#define ATA_REG_LBA1         0x04  /* 0x1F4 - LBA bits 8..15 */
#define ATA_REG_LBA2         0x05  /* 0x1F5 - LBA bits 16..23 */
#define ATA_REG_HDDEVSEL     0x06  /* 0x1F6 - Drive (Master/Slave) + LBA bits 24..27 */
#define ATA_REG_COMMAND      0x07  /* 0x1F7 - Escrita: Comando */
#define ATA_REG_STATUS       0x07  /* 0x1F7 - Leitura: Status */

/* Comandos ATA PIO */
#define ATA_CMD_READ_PIO     0x20
#define ATA_CMD_WRITE_PIO    0x30
#define ATA_CMD_CACHE_FLUSH  0xE7
#define ATA_CMD_IDENTIFY     0xEC

/* Bits do Registrador de Status (0x1F7) */
#define ATA_SR_BSY           0x80  /* Busy: controlador ocupado */
#define ATA_SR_DRDY          0x40  /* Drive Ready */
#define ATA_SR_DF            0x20  /* Drive Write Fault */
#define ATA_SR_DRQ           0x08  /* Data Request: pronto para transferir words */
#define ATA_SR_ERR           0x01  /* Erro na operacao */

#define ATA_SECTOR_SIZE      512

/* Inicializa e detecta o disco Primary Master. Retorna 1 se presente, 0 caso contrario. */
int ata_init(void);

/* Retorna 1 se um disco ATA foi detectado com sucesso no boot. */
int ata_is_present(void);

/* Le 1 setor de 512 bytes no endereco LBA 28-bit para 'buffer'. Retorna 0 em sucesso, <0 em erro. */
int ata_read_sector(uint32_t lba, uint8_t *buffer);

/* Escreve 1 setor de 512 bytes de 'buffer' no endereco LBA 28-bit. Retorna 0 em sucesso, <0 em erro. */
int ata_write_sector(uint32_t lba, const uint8_t *buffer);

#endif /* ATA_H */