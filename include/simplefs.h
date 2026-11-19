#ifndef SIMPLEFS_H
#define SIMPLEFS_H

#include <stdint.h>

#define SFS_MAGIC             0x56474653U /* "VGFS" */
#define SFS_MAX_FILES         64
#define SFS_MAX_NAME          32
#define SFS_FILE_MAX_SECTORS  8           /* 8 * 512 = 4096 bytes por arquivo */
#define SFS_FILE_MAX_SIZE     (SFS_FILE_MAX_SECTORS * 512)

#define SFS_SUPERBLOCK_LBA    1
#define SFS_DIR_START_LBA     2
#define SFS_DIR_SECTORS       8           /* 64 entradas * 64 bytes = 4096 bytes */
#define SFS_DATA_START_LBA    10

typedef struct {
  char     name[SFS_MAX_NAME];
  uint32_t used;
  uint32_t start_lba;
  uint32_t size;
  uint32_t reserved[5]; /* Completa exatos 64 bytes por entrada */
} __attribute__((packed)) sfs_entry_t;

/* Inicializa o SimpleFS (formata automaticamente se o disco estiver virgem). Retorna 0 em sucesso. */
int sfs_init(void);

/* Formata o disco gravando o superbloco e zerando a tabela de diretorios. */
int sfs_format(void);

/* Retorna 1 se o SimpleFS foi montado com sucesso. */
int sfs_is_ready(void);

/* Le o conteudo de um arquivo para out_buf. Retorna 0 em sucesso, <0 se nao encontrado ou erro. */
int sfs_read_file(const char *name, char *out_buf, uint32_t max_len, uint32_t *out_size);

/* Cria ou sobrescreve um arquivo no disco com 'data' de tamanho 'size'. Retorna 0 em sucesso. */
int sfs_write_file(const char *name, const char *data, uint32_t size);

/* Remove um arquivo da tabela de diretorios. Retorna 0 em sucesso. */
int sfs_delete_file(const char *name);

/* Copia a entrada de indice 'index' (0..SFS_MAX_FILES-1). Retorna 1 se usada, 0 se livre. */
int sfs_get_entry(int index, sfs_entry_t *out_entry);
void editor_open(const char *filename);
#endif /* SIMPLEFS_H */